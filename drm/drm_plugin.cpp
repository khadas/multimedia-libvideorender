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
#include "drm_plugin.h"
#include "Logger.h"
#include "drm_display.h"

#define TAG "rlib:drm_plugin"

DrmPlugin::DrmPlugin()
{
    mVideoFormat = VIDEO_FORMAT_UNKNOWN;
    mIsPip = false;
    mDrmDisplay = new DrmDisplay(this);
}

DrmPlugin::~DrmPlugin()
{
    if (mDrmDisplay) {
        delete mDrmDisplay;
        mDrmDisplay = NULL;
    }
}

void DrmPlugin::init()
{
    INFO("\n--------------------------------\n"
            "plugin      : drmmeson\n"
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

void DrmPlugin::release()
{
}

void DrmPlugin::setCallback(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int DrmPlugin::openDisplay()
{
    int ret;

    DEBUG("openDisplay");

    return ret;
}

int DrmPlugin::openWindow()
{
    int ret = 0;

    DEBUG("openWindow");

    bool rc = mDrmDisplay->start(mIsPip);
    if (!rc) {
        ERROR("drm window open failed");
        return -1;
    }

    DEBUG("openWindow,end");
    return ret;
}

int DrmPlugin::prepareFrame(RenderBuffer *buffer)
{

}

int DrmPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    mDrmDisplay->displayFrame(buffer, displayTime);
    return 0;
}

int DrmPlugin::flush()
{
    mDrmDisplay->flush();
    return 0;
}

int DrmPlugin::pause()
{
    mDrmDisplay->pause();
    return 0;
}
int DrmPlugin::resume()
{
    mDrmDisplay->resume();
    return 0;
}

int DrmPlugin::closeDisplay()
{
    return 0;
}

int DrmPlugin::closeWindow()
{
    int ret;
    mDrmDisplay->stop();
    return 0;
}


int DrmPlugin::getValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_VIDEO_FORMAT: {
            *(int *)value = mVideoFormat;
            TRACE("get video format:%d",*(int *)value);
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            *(int *)value = (mIsPip == true) ? 1 : 0;
        };
        case PLUGIN_KEY_HIDE_VIDEO: {
            bool isHide = false;
            if (mDrmDisplay) {
                isHide = mDrmDisplay->isHideVideo();
            }
            *(int *)value = isHide == true? 1: 0;
            TRACE("get hide video:%d",*(int *)value);
        } break;
    }

    return 0;
}

int DrmPlugin::setValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            RenderRect* rect = static_cast<RenderRect*>(value);
            if (mDrmDisplay) {
                mDrmDisplay->setWindowSize(rect->x, rect->y, rect->w, rect->h);
            }
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            RenderFrameSize * frameSize = static_cast<RenderFrameSize * >(value);
            if (mDrmDisplay) {
                mDrmDisplay->setFrameSize(frameSize->width, frameSize->height);
            }
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int format = *(int *)(value);
            mVideoFormat = (RenderVideoFormat) format;
            DEBUG("Set video format :%d",mVideoFormat);
            if (mDrmDisplay) {
                mDrmDisplay->setVideoFormat(mVideoFormat);
            }
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            int pip = *(int *) (value);
            mIsPip = pip > 0? true:false;
        };
        case PLUGIN_KEY_IMMEDIATELY_OUTPUT: {
            bool immediately = (*(int *)(value)) > 0? true: false;
            DEBUG( "Set immediately output:%d",immediately);
            if (mDrmDisplay) {
                mDrmDisplay->setImmediatelyOutout(immediately);
            }
        } break;
        case PLUGIN_KEY_HIDE_VIDEO: {
            int hide = *(int *)(value);
            if (mDrmDisplay) {
                mDrmDisplay->setHideVideo(hide > 0? true:false);
            }
        } break;
        case PLUGIN_KEY_KEEP_LAST_FRAME: {
            int keep = *(int *) (value);
            bool keepLastFrame = keep > 0? true:false;
            DEBUG("Set keep last frame :%d",keepLastFrame);
            if (mDrmDisplay) {
                mDrmDisplay->setKeepLastFrame(keepLastFrame);
            }
        } break;
    }
    return 0;
}

void DrmPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void DrmPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void DrmPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}

void DrmPlugin::handleMsgNotify(int type, void *detail)
{
    if (mCallback) {
        mCallback->doMsgCallback(mUserData, type, detail);
    }
}

void *makePluginInstance(int id)
{
    char *env = getenv("VIDEO_RENDER_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO("VIDEO_RENDER_LOG_LEVEL=%d",level);
    }
    DrmPlugin *drmPlugin = new DrmPlugin();
    return static_cast<void *>(drmPlugin);
}

void destroyPluginInstance(void * plugin)
{
    DrmPlugin *pluginInstance = static_cast<DrmPlugin *>(plugin);
    delete pluginInstance;
}
