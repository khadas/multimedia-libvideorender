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
#include "ErrorCode.h"

#define TAG "rlib:drm_plugin"

DrmPlugin::DrmPlugin(int logCategory)
    : mLogCategory(logCategory)
{
    mVideoFormat = VIDEO_FORMAT_UNKNOWN;
    mIsPip = false;
    mDrmDisplay = new DrmDisplay(this,logCategory);
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
    INFO(mLogCategory,"\n--------------------------------\n"
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

    DEBUG(mLogCategory,"openDisplay");

    DEBUG(mLogCategory,"openDisplay end");
    return ret;
}

int DrmPlugin::openWindow()
{
    int ret = NO_ERROR;

    DEBUG(mLogCategory,"openWindow");

    bool rc = mDrmDisplay->start(mIsPip);
    if (!rc) {
        ret = ERROR_OPEN_FAIL;
        ERROR(mLogCategory,"drm window open failed");
        return ret;
    }

    DEBUG(mLogCategory,"openWindow,end");
    return ret;
}

int DrmPlugin::prepareFrame(RenderBuffer *buffer)
{
    return NO_ERROR;
}

int DrmPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    mDrmDisplay->displayFrame(buffer, displayTime);
    return NO_ERROR;
}

int DrmPlugin::flush()
{
    mDrmDisplay->flush();
    return NO_ERROR;
}

int DrmPlugin::pause()
{
    mDrmDisplay->pause();
    return NO_ERROR;
}
int DrmPlugin::resume()
{
    mDrmDisplay->resume();
    return NO_ERROR;
}

int DrmPlugin::closeDisplay()
{
    return NO_ERROR;
}

int DrmPlugin::closeWindow()
{
    int ret;
    mDrmDisplay->stop();
    return NO_ERROR;
}


int DrmPlugin::getValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_VIDEO_FORMAT: {
            *(int *)value = mVideoFormat;
            TRACE(mLogCategory,"get video format:%d",*(int *)value);
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
            TRACE(mLogCategory,"get hide video:%d",*(int *)value);
        } break;
    }

    return NO_ERROR;
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
            DEBUG(mLogCategory,"Set video format :%d",mVideoFormat);
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
            DEBUG(mLogCategory, "Set immediately output:%d",immediately);
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
            DEBUG(mLogCategory, "Set keep last frame :%d",keepLastFrame);
            if (mDrmDisplay) {
                mDrmDisplay->setKeepLastFrame(keepLastFrame);
            }
        } break;
    }
    return NO_ERROR;
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
    int category =Logger_init(id);
    char *env = getenv("VIDEO_RENDER_PLUGIN_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO(category,"VIDEO_RENDER_PLUGIN_LOG_LEVEL=%d",level);
    }
    DrmPlugin *drmPlugin = new DrmPlugin(category);
    return static_cast<void *>(drmPlugin);
}

void destroyPluginInstance(void * plugin)
{
    int category;

    DrmPlugin *pluginInstance = static_cast<DrmPlugin *>(plugin);
    category = pluginInstance->getLogCategory();
    delete pluginInstance;
    Logger_exit(category);
}
