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
#include "videotunnel_plugin.h"
#include "Logger.h"
#include "video_tunnel.h"
#include "ErrorCode.h"

#define TAG "rlib:videotunnel_plugin"

VideoTunnelPlugin::VideoTunnelPlugin(int logCategory)
    : mDisplayLock("displaylock"),
    mLogCategory(logCategory),
    mRenderLock("renderlock")
{
    mWinRect.x = 0;
    mWinRect.y = 0;
    mWinRect.w = 0;
    mWinRect.h = 0;
    mVideoTunnel = new VideoTunnelImpl(this,logCategory);
}

VideoTunnelPlugin::~VideoTunnelPlugin()
{
    if (mVideoTunnel) {
        delete mVideoTunnel;
        mVideoTunnel = NULL;
    }
}

void VideoTunnelPlugin::init()
{
    INFO(mLogCategory,"\n--------------------------------\n"
            "plugin      : videotunnel\n"
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
    mVideoTunnel->init();
}

void VideoTunnelPlugin::release()
{
    mVideoTunnel->release();
}

void VideoTunnelPlugin::setCallback(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int VideoTunnelPlugin::openDisplay()
{
    DEBUG(mLogCategory,"openDisplay");
    return NO_ERROR;
}

int VideoTunnelPlugin::openWindow()
{
    DEBUG(mLogCategory,"openWindow");
    mVideoTunnel->connect();
    DEBUG(mLogCategory,"openWindow,end");
    return NO_ERROR;
}

int VideoTunnelPlugin::prepareFrame(RenderBuffer *buffer)
{
    return NO_ERROR;
}

int VideoTunnelPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    mVideoTunnel->displayFrame(buffer, displayTime);
    return NO_ERROR;
}

int VideoTunnelPlugin::flush()
{
    mVideoTunnel->flush();
    return NO_ERROR;
}

int VideoTunnelPlugin::pause()
{
    return NO_ERROR;
}
int VideoTunnelPlugin::resume()
{
    return NO_ERROR;
}

int VideoTunnelPlugin::closeDisplay()
{
    return NO_ERROR;
}

int VideoTunnelPlugin::closeWindow()
{
    mVideoTunnel->disconnect();
    return NO_ERROR;
}


int VideoTunnelPlugin::getValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_VIDEOTUNNEL_ID: {
            mVideoTunnel->getVideotunnelId((int *)value);
        } break;
    }

    return NO_ERROR;
}

int VideoTunnelPlugin::setValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            RenderRect* rect = static_cast<RenderRect*>(value);
            mWinRect.x = rect->x;
            mWinRect.y = rect->y;
            mWinRect.w = rect->w;
            mWinRect.h = rect->h;
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            RenderFrameSize * frameSize = static_cast<RenderFrameSize * >(value);
            mVideoTunnel->setFrameSize(frameSize->width, frameSize->height);
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int videoFormat = *(int *)(value);
            DEBUG(mLogCategory,"Set video format :%d",videoFormat);
        } break;
        case PLUGIN_KEY_VIDEOTUNNEL_ID: {
            int videotunnelId = *(int *)(value);
            DEBUG(mLogCategory,"Set videotunnel id :%d",videotunnelId);
            mVideoTunnel->setVideotunnelId(videotunnelId);
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            int pip = *(int *) (value);
            if (pip > 0) {
                int videotunnelId = 1; //videotunnel id 1 be used to pip
                DEBUG(mLogCategory,"Set pip ,videotunnel id :%d",videotunnelId);
                mVideoTunnel->setVideotunnelId(videotunnelId);
            }
        } break;
    }
    return NO_ERROR;
}

void VideoTunnelPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void VideoTunnelPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void VideoTunnelPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}

void VideoTunnelPlugin::handleMsgNotify(int type, void *detail)
{
    if (mCallback) {
        mCallback->doMsgCallback(mUserData, type, detail);
    }
}

void *makePluginInstance(int id)
{
    int category =Logger_init(id);
    char *env = getenv("VIDEO_RENDER_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO(category,"VIDEO_RENDER_LOG_LEVEL=%d",level);
    }
    VideoTunnelPlugin *plugin = new VideoTunnelPlugin(category);
    return static_cast<void *>(plugin);
}

void destroyPluginInstance(void * plugin)
{
    int category;

    VideoTunnelPlugin *pluginInstance = static_cast<VideoTunnelPlugin *>(plugin);
    category = pluginInstance->getLogCategory();
    delete pluginInstance;
    Logger_exit(category);
}
