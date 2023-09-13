/*
 * Copyright (C) 2021 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <sys/mman.h>
#include "Logger.h"
#include "drm_display.h"
#include "drm_framerecycle.h"

using namespace Tls;

#define TAG "rlib:drm_framerecycle"

DrmFrameRecycle::DrmFrameRecycle(DrmDisplay *drmDisplay, int logCategory)
{
    mDrmDisplay = drmDisplay;
    mLogCategory = logCategory;
    mStop = false;
    mWaitVideoFence = false;
    mQueue = new Tls::Queue();
}

DrmFrameRecycle::~DrmFrameRecycle()
{
    if (isRunning()) {
        mStop = true;
        DEBUG(mLogCategory,"stop frame recycle thread");
        requestExitAndWait();
    }

    if (mQueue) {
        mQueue->flushAndCallback(this, DrmFrameRecycle::queueFlushCallback);
        delete mQueue;
        mQueue = NULL;
    }
}

bool DrmFrameRecycle::start()
{
    DEBUG(mLogCategory,"start frame recycle thread");
    mStop = false;
    run("frame recycle thread");
    return true;
}

bool DrmFrameRecycle::stop()
{
    if (isRunning()) {
        mStop = true;
        DEBUG(mLogCategory,"stop frame recycle thread");
        requestExitAndWait();
    }
    mWaitVideoFence = false;
    Tls::Mutex::Autolock _l(mMutex);
    mQueue->flushAndCallback(this, DrmFrameRecycle::queueFlushCallback);
    return true;
}

bool DrmFrameRecycle::recycleFrame(FrameEntity * frameEntity)
{
    Tls::Mutex::Autolock _l(mMutex);
    if (mStop) {
        mDrmDisplay->handleReleaseFrameEntity(frameEntity);
        return true;
    }
    mQueue->push(frameEntity);
    TRACE(mLogCategory,"queue cnt:%d",mQueue->getCnt());
    /* when two frame are posted, fence can be retrieved.
     * So waiting video fence starts
     */
    if (!mWaitVideoFence && mQueue->getCnt() > 2) {
        mWaitVideoFence = true;
    }
    return true;
}

void DrmFrameRecycle::queueFlushCallback(void *userdata,void *data)
{
    DrmFrameRecycle* self = static_cast<DrmFrameRecycle *>(userdata);
    self->mDrmDisplay->handleReleaseFrameEntity((FrameEntity *)data);
}

void DrmFrameRecycle::readyToRun()
{

}
bool DrmFrameRecycle::threadLoop()
{
    int rc;
    struct drm_buf* gem_buf;
    FrameEntity *frameEntity;
    DrmMesonLib *drmMesonLib = mDrmDisplay->getDrmMesonLib();

    if (mStop) {
        return false;
    }
    if (!mWaitVideoFence || mQueue->getCnt() < 2) {
        usleep(8000);
        return true;
    }

    rc = mQueue->popAndWait((void**)&frameEntity);
    if (rc != Q_OK) {
        return true;
    }

    TRACE(mLogCategory,"wait video fence for frame:%lld(pts:%lld)",frameEntity->displayTime,frameEntity->renderBuf->pts);
    if (drmMesonLib) {
        rc = drmMesonLib->libDrmWaitVideoFence(frameEntity->drmBuf->fd[0]);
    }

    if (rc <= 0) {
        WARNING(mLogCategory, "wait fence error %d", rc);
    }

    mDrmDisplay->handleReleaseFrameEntity(frameEntity);
    return true;
}
