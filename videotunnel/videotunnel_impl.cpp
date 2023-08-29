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
#include <poll.h>
#include "videotunnel_plugin.h"
#include "videotunnel_impl.h"
#include "Logger.h"
#include "Times.h"
#include "video_tunnel.h"
#include "videotunnel.h"

using namespace Tls;

#define TAG "rlib:videotunnel_impl"

#define UNDER_FLOW_EXPIRED_TIME_MS 83


VideoTunnelImpl::VideoTunnelImpl(VideoTunnelPlugin *plugin)
    : mPlugin(plugin)
{
    mFd = -1;
    mInstanceId = 0;
    mIsVideoTunnelConnected = false;
    mQueueFrameCnt = 0;
    mStarted = false;
    mRequestStop = false;
    mVideotunnelLib = NULL;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mUnderFlowDetect = false;
}

VideoTunnelImpl::~VideoTunnelImpl()
{
}

bool VideoTunnelImpl::init()
{
    mVideotunnelLib = videotunnelLoadLib();
    if (!mVideotunnelLib) {
        ERROR("videotunnelLoadLib load symbol fail");
        return false;
    }
    return true;
}

bool VideoTunnelImpl::release()
{
    if (mVideotunnelLib) {
        videotunnelUnloadLib(mVideotunnelLib);
        mVideotunnelLib = NULL;
    }
    return true;
}

bool VideoTunnelImpl::connect()
{
    int ret;
    DEBUG("in");
    mRequestStop = false;
    mSignalFirstFrameDiplayed = false;
    if (mVideotunnelLib && mVideotunnelLib->vtOpen) {
        mFd = mVideotunnelLib->vtOpen();
    }
    if (mFd > 0) {
        //ret = meson_vt_alloc_id(mFd, &mInstanceId);
    }
    if (mFd > 0 && mInstanceId >= 0) {
        if (mVideotunnelLib && mVideotunnelLib->vtConnect) {
             ret = mVideotunnelLib->vtConnect(mFd, mInstanceId, VT_ROLE_PRODUCER);
        }
        mIsVideoTunnelConnected = true;
    } else {
        ERROR("open videotunnel fail or alloc id fail");
        return false;
    }
    INFO("vt fd:%d, instance id:%d",mFd,mInstanceId);
    DEBUG("out");
    return true;
}

bool VideoTunnelImpl::disconnect()
{
    mRequestStop = true;
    DEBUG("in");
    if (isRunning()) {
        requestExitAndWait();
        mStarted = false;
    }

    if (mFd > 0) {
        if (mIsVideoTunnelConnected) {
            INFO("instance id:%d",mInstanceId);
            if (mVideotunnelLib && mVideotunnelLib->vtDisconnect) {
                mVideotunnelLib->vtDisconnect(mFd, mInstanceId, VT_ROLE_PRODUCER);
            }
            mIsVideoTunnelConnected = false;
        }
        if (mInstanceId >= 0) {
            INFO("free instance id:%d",mInstanceId);
            //meson_vt_free_id(mFd, mInstanceId);
        }
        INFO("close vt fd:%d",mFd);
        if (mVideotunnelLib && mVideotunnelLib->vtClose) {
            mVideotunnelLib->vtClose(mFd);
        }
        mFd = -1;
    }

    //flush all buffer those do not displayed
    DEBUG("release all posted to videotunnel buffers");
    std::lock_guard<std::mutex> lck(mMutex);
    for (auto item = mQueueRenderBufferMap.begin(); item != mQueueRenderBufferMap.end(); ) {
        RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
        mQueueRenderBufferMap.erase(item++);
        mPlugin->handleFrameDropped(renderbuffer);
        mPlugin->handleBufferRelease(renderbuffer);
    }
    DEBUG("out");
    return true;
}

bool VideoTunnelImpl::displayFrame(RenderBuffer *buf, int64_t displayTime)
{
    int ret;
    if (mStarted == false) {
        DEBUG("to run VideoTunnelImpl");
        run("VideoTunnelImpl");
        mStarted = true;
        mSignalFirstFrameDiplayed = true;
    }

    int fd0 = buf->dma.fd[0];
    //leng.fang suggest fence id set -1
    if (mVideotunnelLib && mVideotunnelLib->vtQueueBuffer) {
        ret = mVideotunnelLib->vtQueueBuffer(mFd, mInstanceId, fd0, -1 /*fence_fd*/, displayTime);
        if (mUnderFlowDetect) {
            mUnderFlowDetect = false;
        }
    }

    std::lock_guard<std::mutex> lck(mMutex);
    std::pair<int, RenderBuffer *> item(fd0, buf);
    mQueueRenderBufferMap.insert(item);
    ++mQueueFrameCnt;
    TRACE("***fd:%d,w:%d,h:%d,displaytime:%lld,commitCnt:%d",buf->dma.fd[0],buf->dma.width,buf->dma.height,displayTime,mQueueFrameCnt);
    mPlugin->handleFrameDisplayed(buf);
    mLastDisplayTime = Tls::Times::getSystemTimeMs();
    return true;
}

void VideoTunnelImpl::flush()
{
    DEBUG("flush");
    std::lock_guard<std::mutex> lck(mMutex);
    if (mVideotunnelLib && mVideotunnelLib->vtCancelBuffer) {
        mVideotunnelLib->vtCancelBuffer(mFd, mInstanceId);
    }
    for (auto item = mQueueRenderBufferMap.begin(); item != mQueueRenderBufferMap.end(); ) {
        RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
        mQueueRenderBufferMap.erase(item++);
        mPlugin->handleFrameDropped(renderbuffer);
        mPlugin->handleBufferRelease(renderbuffer);
    }
    mQueueFrameCnt = 0;
    DEBUG("after flush,commitCnt:%d",mQueueFrameCnt);
}

void VideoTunnelImpl::setFrameSize(int width, int height)
{
    mFrameWidth = width;
    mFrameHeight = height;
    DEBUG("set frame size, width:%d, height:%d",mFrameWidth,mFrameHeight);
}

void VideoTunnelImpl::setVideotunnelId(int id)
{
    mInstanceId = id;
}

void VideoTunnelImpl::waitFence(int fence) {
    if (fence > 0) {
        struct pollfd fds;

        fds.fd = fence;
        fds.events = POLLERR | POLLNVAL | POLLHUP |POLLIN | POLLPRI | POLLRDNORM;
        fds.revents = 0;

        for ( ; ; ) {
            int rc = poll(&fds, 1, 3000);
            if ((rc == -1) && ((errno == EINTR) || (errno == EAGAIN))) {
                continue;
            } else if (rc <= 0) {
                if (rc == 0) errno = ETIME;
            }
            break;
        }
        close(fence);
        fence = -1;
    }
}

void VideoTunnelImpl::readyToRun()
{
    struct vt_rect rect;

    if (mFrameWidth > 0 && mFrameHeight > 0) {
        rect.left = 0;
        rect.top = 0;
        rect.right = mFrameWidth;
        rect.bottom = mFrameHeight;
        if (mVideotunnelLib && mVideotunnelLib->vtSetSourceCrop) {
            mVideotunnelLib->vtSetSourceCrop(mFd, mInstanceId, rect);
        }
    }
}

bool VideoTunnelImpl::threadLoop()
{
    int ret;
    int bufferId = 0;
    int fenceId;
    RenderBuffer *buffer = NULL;

    if (mRequestStop) {
        DEBUG("request stop");
        return false;
    }
    //if last frame expired,and not send frame to compositor for under flow expired time
    //we need to send under flow msg
    if (mQueueFrameCnt < 1 &&
         Tls::Times::getSystemTimeMs() - mLastDisplayTime > UNDER_FLOW_EXPIRED_TIME_MS &&
         !mUnderFlowDetect) {
        mUnderFlowDetect = true;
        mPlugin->handleMsgNotify(MSG_UNDER_FLOW, NULL);
    }

    if (mVideotunnelLib && mVideotunnelLib->vtDequeueBuffer) {
        ret = mVideotunnelLib->vtDequeueBuffer(mFd, mInstanceId, &bufferId, &fenceId);
    }

    if (ret != 0) {
        if (mRequestStop) {
            DEBUG("request stop");
            return false;
        }
        return true;
    }
    if (mRequestStop) {
        DEBUG("request stop");
        return false;
    }

    //send uvm fd to driver after getted fence
    waitFence(fenceId);

    {
        std::lock_guard<std::mutex> lck(mMutex);
        auto item = mQueueRenderBufferMap.find(bufferId);
        if (item == mQueueRenderBufferMap.end()) {
            ERROR("Not found in mQueueRenderBufferMap bufferId:%d",bufferId);
            return true;
        }
        // try locking when removing item from mQueueRenderBufferMap
        buffer = (RenderBuffer*) item->second;
        mQueueRenderBufferMap.erase(bufferId);
        --mQueueFrameCnt;
        TRACE("***dq buffer fd:%d,commitCnt:%d",bufferId,mQueueFrameCnt);
    }
    //send first frame displayed msg
    if (mSignalFirstFrameDiplayed) {
        mSignalFirstFrameDiplayed = false;
        INFO("send first frame displayed msg");
        mPlugin->handleMsgNotify(MSG_FIRST_FRAME,(void*)&buffer->pts);
    }
    mPlugin->handleBufferRelease(buffer);
    return true;
}