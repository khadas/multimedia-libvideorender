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
#include "drm_framepost.h"

using namespace Tls;

#define TAG "rlib:drm_framepost"

DrmFramePost::DrmFramePost(DrmDisplay *drmDisplay)
{
    mDrmDisplay = drmDisplay;
    mPaused = false;
    mStop = false;
    mImmediatelyOutput = false;
    mQueue = new Tls::Queue();
    mWinRect.x = 0;
    mWinRect.y = 0;
    mWinRect.w = 0;
    mWinRect.h = 0;
}

DrmFramePost::~DrmFramePost()
{
    if (isRunning()) {
        mStop = true;
        DEBUG("stop frame post thread");
        requestExitAndWait();
    }

    if (mQueue) {
        mQueue->flush();
        delete mQueue;
        mQueue = NULL;
    }
}

bool DrmFramePost::start()
{
    DEBUG("start frame post thread");
    run("frame post thread");
    return true;
}

bool DrmFramePost::stop()
{
    DEBUG("stop frame post thread");
    if (isRunning()) {
        mStop = true;
        DEBUG("stop frame post thread");
        requestExitAndWait();
    }
    flush();
    return true;
}

bool DrmFramePost::readyPostFrame(FrameEntity * frameEntity)
{
    std::lock_guard<std::mutex> lck(mMutex);
    mQueue->push(frameEntity);
    TRACE("queue cnt:%d",mQueue->getCnt());
    return true;
}

void DrmFramePost::flush()
{
    FrameEntity *entity;
    std::lock_guard<std::mutex> lck(mMutex);
    while (mQueue->pop((void **)&entity) == Q_OK)
    {
        mDrmDisplay->handleDropedFrameEntity(entity);
        mDrmDisplay->handleReleaseFrameEntity(entity);
    }
}

void DrmFramePost::pause()
{
    mPaused = true;
}

void DrmFramePost::resume()
{
    mPaused = false;
}

void DrmFramePost::setImmediatelyOutout(bool on)
{
    mImmediatelyOutput = on;
    DEBUG("set immediately output:%d",on);
}

void DrmFramePost::setWindowSize(int x, int y, int w, int h)
{
    mWinRect.x = x;
    mWinRect.y = y;
    mWinRect.w = w;
    mWinRect.h = h;
}

void DrmFramePost::readyToRun()
{
}

bool DrmFramePost::threadLoop()
{
    int rc;
    drmVBlank vbl;
    int64_t vBlankTime;
    FrameEntity *curFrameEntity = NULL;
    FrameEntity *expiredFrameEntity = NULL;
    long long refreshInterval= 0LL;
    struct drm_display *drmHandle = mDrmDisplay->getDrmHandle();
    DrmMesonLib *drmMesonLib = mDrmDisplay->getDrmMesonLib();

    //stop thread
    if (mStop) {
        return false;
    }

    if (!drmHandle) {
        ERROR("drm handle is NULL");
        return false;
    }

    memset(&vbl, 0, sizeof(drmVBlank));

    vbl.request.type = DRM_VBLANK_RELATIVE;
    vbl.request.sequence = 1;
    vbl.request.signal = 0;

    //drmWaitBland need link libdrm.so
    rc = drmWaitVBlank(drmHandle->drm_fd, &vbl);
    if (rc) {
        ERROR("drmWaitVBlank error %d", rc);
        usleep(4000);
        return true;
    }

    if (drmHandle->vrefresh)
    {
        refreshInterval= (1000000LL+(drmHandle->vrefresh/2))/drmHandle->vrefresh;
    }

    vBlankTime = vbl.reply.tval_sec*1000000LL + vbl.reply.tval_usec;
    vBlankTime += refreshInterval; //check next blank time

    std::lock_guard<std::mutex> lck(mMutex);
    //if queue is empty or paused, loop next
    if (mQueue->isEmpty() || mPaused) {
        //TRACE(mLogCategory,"empty or paused");
        goto tag_next;
    }

    //we output video frame asap
    if (mImmediatelyOutput) {
        //pop the peeked frame
        mQueue->pop((void **)&expiredFrameEntity);
        goto tag_post;
    }

    while (mQueue->peek((void **)&curFrameEntity, 0) == Q_OK)
    {
        TRACE("vBlankTime:%lld,frame time:%lld(pts:%lld ms),refreshInterval:%lld",vBlankTime,curFrameEntity->displayTime,curFrameEntity->renderBuf->pts/1000000,refreshInterval);
        //no frame expired,loop next
        if (vBlankTime < curFrameEntity->displayTime) {
            break;
        }

        //pop the peeked frame
        mQueue->pop((void **)&curFrameEntity);

        //drop last expired frame,got a new expired frame
        if (expiredFrameEntity) {
            DEBUG("drop frame,vBlankTime:%lld,frame time:%lld(pts:%lld ms)",vBlankTime,expiredFrameEntity->displayTime,expiredFrameEntity->renderBuf->pts/1000000);
            mDrmDisplay->handleDropedFrameEntity(expiredFrameEntity);
            mDrmDisplay->handleReleaseFrameEntity(expiredFrameEntity);
            expiredFrameEntity = NULL;
        }

        expiredFrameEntity = curFrameEntity;
        TRACE("expire,frame time:%lld(pts:%lld ms)",expiredFrameEntity->displayTime,expiredFrameEntity->renderBuf->pts/1000000);
    }

tag_post:
    //no frame will be posted to drm display
    if (!expiredFrameEntity) {
        TRACE("no frame expire");
        goto tag_next;
    }

    //double check window size before post drm buffer
    if (mWinRect.w > 0 && mWinRect.h > 0) {
        struct drm_buf * drmBuf = expiredFrameEntity->drmBuf;
        drmBuf->crtc_x = mWinRect.x;
        drmBuf->crtc_y = mWinRect.y;
        drmBuf->crtc_w = mWinRect.w;
        drmBuf->crtc_h = mWinRect.h;
        TRACE("postBuf crtc(%d,%d,%d,%d),src(%d,%d,%d,%d)",drmBuf->crtc_x,drmBuf->crtc_y,drmBuf->crtc_w,drmBuf->crtc_h,
        drmBuf->src_x,drmBuf->src_y,drmBuf->src_w,drmBuf->src_h);
    }

    if (drmHandle && drmMesonLib) {
        rc = drmMesonLib->libDrmPostBuf(drmHandle, expiredFrameEntity->drmBuf);
    }

    if (rc) {
        ERROR("drm_post_buf error %d", rc);
        mDrmDisplay->handleDropedFrameEntity(expiredFrameEntity);
        mDrmDisplay->handleReleaseFrameEntity(expiredFrameEntity);
        goto tag_next;
    }

    TRACE("drm_post_buf,frame time:%lld(pts:%lld ms)",expiredFrameEntity->displayTime,expiredFrameEntity->renderBuf->pts/1000000);
    mDrmDisplay->handlePostedFrameEntity(expiredFrameEntity);
    mDrmDisplay->handleDisplayedFrameEntity(expiredFrameEntity);
tag_next:
    return true;
}
