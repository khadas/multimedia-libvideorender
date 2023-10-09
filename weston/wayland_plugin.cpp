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
#include "wayland_plugin.h"
#include "wayland_display.h"
#include "Logger.h"
#include "Times.h"
#include "ErrorCode.h"

#define TAG "rlib:wayland_plugin"

WaylandPlugin::WaylandPlugin(int logCatgory)
    : mRenderLock("renderlock"),
    mLogCategory(logCatgory)
{
    mDisplay = new WaylandDisplay(this, logCatgory);
    mQueue = new Tls::Queue();
    mPaused = false;
    mImmediatelyOutput = false;
}

WaylandPlugin::~WaylandPlugin()
{
    if (mDisplay) {
        delete mDisplay;
    }
    if (mQueue) {
        mQueue->flush();
        delete mQueue;
        mQueue = NULL;
    }
    TRACE(mLogCategory,"desconstruct");
}

void WaylandPlugin::init()
{
    INFO(mLogCategory,"\n--------------------------------\n"
            "plugin      : weston\n"
            "ARCH        : %s\n"
            "branch name : %s\n"
            "git version : %s\n"
            "change id   : %s \n"
            "ID          : %s \n"
            "last changed: %s\n"
            "build-time  : %s\n"
            "build-name  : %s\n"
            "--------------------------------\n",
#if defined(__aarch64__)
            "arm64",
#else
            "arm",
#endif
            BRANCH_NAME,
            GIT_VERSION,
            COMMIT_CHANGEID,
            COMMIT_PD,
            LAST_CHANGED,
            BUILD_TIME,
            BUILD_NAME
    );
}

void WaylandPlugin::release()
{
}

void WaylandPlugin::setCallback(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int WaylandPlugin::openDisplay()
{
    int ret;

    Tls::Mutex::Autolock _l(mRenderLock);
    DEBUG(mLogCategory,"openDisplay");
    ret =  mDisplay->openDisplay();
    if (ret != NO_ERROR) {
        ERROR(mLogCategory,"Error open display");
        return ret;
    }
    DEBUG(mLogCategory,"openDisplay end");
    return ret;
}

int WaylandPlugin::openWindow()
{
    Tls::Mutex::Autolock _l(mRenderLock);
    /* if weston can't support pts feature,
     * we should create a post buffer thread to
     * send buffer by mono time
     */
    if (!mDisplay->isSentPtsToWeston()) {
            DEBUG(mLogCategory,"run frame post thread");
        setThreadPriority(50);
        run("waylandPostThread");
    }
    return NO_ERROR;
}

int WaylandPlugin::prepareFrame(RenderBuffer *buffer)
{
    mDisplay->prepareFrameBuffer(buffer);
    return NO_ERROR;
}

int WaylandPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    /* if weston can't support pts feature,
     * push buffer to queue, the buffer will send to
     * weston in post thread
     */
    if (!mDisplay->isSentPtsToWeston()) {
        buffer->time = displayTime;
        mQueue->push(buffer);
            DEBUG(mLogCategory,"queue size:%d",mQueue->getCnt());
    } else {
        mDisplay->displayFrameBuffer(buffer, displayTime);
    }
    return NO_ERROR;
}

void WaylandPlugin::queueFlushCallback(void *userdata,void *data)
{
    WaylandPlugin* plugin = static_cast<WaylandPlugin *>(userdata);
    plugin->handleFrameDropped((RenderBuffer *)data);
    plugin->handleBufferRelease((RenderBuffer *)data);
}

int WaylandPlugin::flush()
{
    RenderBuffer *entity;
    mQueue->flushAndCallback(this, WaylandPlugin::queueFlushCallback);
    mDisplay->flushBuffers();
    return NO_ERROR;
}

int WaylandPlugin::pause()
{
    mPaused = true;
    return NO_ERROR;
}
int WaylandPlugin::resume()
{
    mPaused = false;
    return NO_ERROR;
}

int WaylandPlugin::closeDisplay()
{
    RenderBuffer *entity;
    Tls::Mutex::Autolock _l(mRenderLock);
    mDisplay->closeDisplay();
    while (mQueue->pop((void **)&entity) == Q_OK)
    {
        handleBufferRelease(entity);
    }

    return NO_ERROR;
}

int WaylandPlugin::closeWindow()
{
    Tls::Mutex::Autolock _l(mRenderLock);
    if (isRunning()) {
        DEBUG(mLogCategory,"stop frame post thread");
        requestExitAndWait();
    }
    return NO_ERROR;
}


int WaylandPlugin::getValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_SELECT_DISPLAY_OUTPUT: {
            *(int *)(value) = mDisplay->getDisplayOutput();
            TRACE(mLogCategory,"get select display output:%d",*(int *)value);
        } break;
    }
    return NO_ERROR;
}

int WaylandPlugin::setValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            RenderRect* rect = static_cast<RenderRect*>(value);
            if (mDisplay) {
                mDisplay->setWindowSize(rect->x, rect->y, rect->w, rect->h);
            }
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            RenderFrameSize * frameSize = static_cast<RenderFrameSize * >(value);
            if (mDisplay) {
                mDisplay->setFrameSize(frameSize->width, frameSize->height);
            }
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int videoFormat = *(int *)(value);
            DEBUG(mLogCategory,"Set video format :%d",videoFormat);
            mDisplay->setVideoBufferFormat((RenderVideoFormat)videoFormat);
        } break;
        case PLUGIN_KEY_SELECT_DISPLAY_OUTPUT: {
            int outputIndex = *(int *)(value);
            DEBUG(mLogCategory,"Set select display output :%d",outputIndex);
            mDisplay->setDisplayOutput(outputIndex);
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            int pip = *(int *) (value);
            pip = pip > 0? 1: 0;
            mDisplay->setPip(pip);
        } break;
        case PLUGIN_KEY_IMMEDIATELY_OUTPUT: {
            bool mImmediatelyOutput = (*(int *)(value)) > 0? true: false;
            DEBUG(mLogCategory, "Set immediately output:%d",mImmediatelyOutput);
        } break;
    }
    return 0;
}

void WaylandPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void WaylandPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void WaylandPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}

void WaylandPlugin::readyToRun()
{
}

bool WaylandPlugin::threadLoop()
{
    RenderBuffer *curFrameEntity = NULL;
    RenderBuffer *expiredFrameEntity = NULL;
    int64_t nowMonotime = Tls::Times::getSystemTimeUs();

    //if queue is empty or paused, loop next
    if (mQueue->isEmpty() || mPaused) {
        goto tag_next;
    }

    //if weston obtains a buffer rendering,we can't send buffer to weston
    if (mDisplay->isRedrawingPending()) {
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
        //no frame expired,loop next
        if (nowMonotime < curFrameEntity->time) {
            break;
        }

        //pop the peeked frame
        mQueue->pop((void **)&curFrameEntity);

        //drop last expired frame,got a new expired frame
        if (expiredFrameEntity) {
            WARNING(mLogCategory,"drop,now:%lld,display:%lld(pts:%lld ms),n-d:%lld ms",
                nowMonotime,expiredFrameEntity->time,expiredFrameEntity->pts/1000000,
                (nowMonotime - expiredFrameEntity->time)/1000);
            handleFrameDropped(expiredFrameEntity);
            handleBufferRelease(expiredFrameEntity);
            expiredFrameEntity = NULL;
        }

        expiredFrameEntity = curFrameEntity;
    }

tag_post:
    if (!expiredFrameEntity) {
        //TRACE(mLogCategory,"no frame expire");
        goto tag_next;
    }

    if (mDisplay) {
        TRACE(mLogCategory,"post,now:%lld,display:%lld(pts:%lld ms),n-d::%lld ms",
            nowMonotime,expiredFrameEntity->time,expiredFrameEntity->pts/1000000,
            (nowMonotime - expiredFrameEntity->time)/1000);
        mDisplay->displayFrameBuffer(expiredFrameEntity, expiredFrameEntity->time);
    }

tag_next:
    usleep(4*1000);
    return true;
}

void *makePluginInstance(int id)
{
    int category =Logger_init(id);
    char *env = getenv("VIDEO_RENDER_PLUGIN_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO(category,"VIDEO_RENDER_PLUGIN_LOG_LEVEL=%d",level);
    }
    WaylandPlugin *pluginInstance = new WaylandPlugin(category);
    return static_cast<void *>(pluginInstance);
}

void destroyPluginInstance(void * plugin)
{
    int category;

    WaylandPlugin *pluginInstance = static_cast<WaylandPlugin *>(plugin);
    category = pluginInstance->getLogCategory();
    delete pluginInstance;
    Logger_exit(category);
}