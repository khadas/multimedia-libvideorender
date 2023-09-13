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


VideoTunnelImpl::VideoTunnelImpl(VideoTunnelPlugin *plugin, int logcategory)
    : mPlugin(plugin),
    mLogCategory(logcategory)
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
    mPoll = new Tls::Poll(true);
}

VideoTunnelImpl::~VideoTunnelImpl()
{
    if (mPoll) {
        delete mPoll;
        mPoll = NULL;
    }
}

bool VideoTunnelImpl::init()
{
    mVideotunnelLib = videotunnelLoadLib(mLogCategory);
    if (!mVideotunnelLib) {
        ERROR(mLogCategory,"videotunnelLoadLib load symbol fail");
        return false;
    }
    return true;
}

bool VideoTunnelImpl::release()
{
    if (mVideotunnelLib) {
        videotunnelUnloadLib(mLogCategory, mVideotunnelLib);
        mVideotunnelLib = NULL;
    }
    return true;
}

bool VideoTunnelImpl::connect()
{
    int ret;
    DEBUG(mLogCategory,"in");
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
        ERROR(mLogCategory,"open videotunnel fail or alloc id fail");
        return false;
    }
    INFO(mLogCategory,"vt fd:%d, instance id:%d",mFd,mInstanceId);
    DEBUG(mLogCategory,"out");
    return true;
}

bool VideoTunnelImpl::disconnect()
{
    mRequestStop = true;
    DEBUG(mLogCategory,"in");
    if (isRunning()) {
        if (mPoll) {
            mPoll->setFlushing(true);
        }
        requestExitAndWait();
        mStarted = false;
    }

    if (mFd > 0) {
        if (mIsVideoTunnelConnected) {
            INFO(mLogCategory,"instance id:%d",mInstanceId);
            if (mVideotunnelLib && mVideotunnelLib->vtDisconnect) {
                mVideotunnelLib->vtDisconnect(mFd, mInstanceId, VT_ROLE_PRODUCER);
            }
            mIsVideoTunnelConnected = false;
        }
        if (mInstanceId >= 0) {
            INFO(mLogCategory,"free instance id:%d",mInstanceId);
            //meson_vt_free_id(mFd, mInstanceId);
        }
        INFO(mLogCategory,"close vt fd:%d",mFd);
        if (mVideotunnelLib && mVideotunnelLib->vtClose) {
            mVideotunnelLib->vtClose(mFd);
        }
        mFd = -1;
    }

    //flush all buffer those do not displayed
    DEBUG(mLogCategory,"release all posted to videotunnel buffers");
    Tls::Mutex::Autolock _l(mMutex);
    for (auto item = mQueueRenderBufferMap.begin(); item != mQueueRenderBufferMap.end(); ) {
        RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
        mQueueRenderBufferMap.erase(item++);
        mPlugin->handleFrameDropped(renderbuffer);
        mPlugin->handleBufferRelease(renderbuffer);
    }
    DEBUG(mLogCategory,"out");
    return true;
}

bool VideoTunnelImpl::displayFrame(RenderBuffer *buf, int64_t displayTime)
{
    int ret;
    if (mStarted == false) {
        DEBUG(mLogCategory,"to run VideoTunnelImpl");
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

    Tls::Mutex::Autolock _l(mMutex);
    std::pair<int, RenderBuffer *> item(fd0, buf);
    mQueueRenderBufferMap.insert(item);
    ++mQueueFrameCnt;
    TRACE(mLogCategory,"***fd:%d,w:%d,h:%d,displaytime:%lld,commitCnt:%d",buf->dma.fd[0],buf->dma.width,buf->dma.height,displayTime,mQueueFrameCnt);
    mPlugin->handleFrameDisplayed(buf);
    mLastDisplayTime = Tls::Times::getSystemTimeMs();
    return true;
}

void VideoTunnelImpl::flush()
{
    DEBUG(mLogCategory,"flush");
    Tls::Mutex::Autolock _l(mMutex);
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
    DEBUG(mLogCategory,"after flush,commitCnt:%d",mQueueFrameCnt);
}

void VideoTunnelImpl::setFrameSize(int width, int height)
{
    mFrameWidth = width;
    mFrameHeight = height;
    DEBUG(mLogCategory, "set frame size, width:%d, height:%d",mFrameWidth,mFrameHeight);
}

void VideoTunnelImpl::setVideotunnelId(int id)
{
    mInstanceId = id;
}

void VideoTunnelImpl::waitFence(int fence) {
    if (fence > 0) {
        mPoll->addFd(fence);
        mPoll->setFdReadable(fence, true);

        for ( ; ; ) {
            int rc = mPoll->wait(3000); //3 sec
            if ((rc == -1) && ((errno == EINTR) || (errno == EAGAIN))) {
                continue;
            } else if (rc <= 0) {
                if (rc == 0) errno = ETIME;
            }
            break;
        }
        mPoll->removeFd(fence);
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
        DEBUG(mLogCategory,"request stop");
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
            DEBUG(mLogCategory,"request stop");
            return false;
        }
        return true;
    }
    if (mRequestStop) {
        DEBUG(mLogCategory,"request stop");
        return false;
    }

    //send uvm fd to driver after getted fence
    waitFence(fenceId);

    {
        Tls::Mutex::Autolock _l(mMutex);
        auto item = mQueueRenderBufferMap.find(bufferId);
        if (item == mQueueRenderBufferMap.end()) {
            ERROR(mLogCategory,"Not found in mQueueRenderBufferMap bufferId:%d",bufferId);
            return true;
        }
        // try locking when removing item from mQueueRenderBufferMap
        buffer = (RenderBuffer*) item->second;
        mQueueRenderBufferMap.erase(bufferId);
        --mQueueFrameCnt;
        TRACE(mLogCategory,"***dq buffer fd:%d,commitCnt:%d",bufferId,mQueueFrameCnt);
    }
    //send first frame displayed msg
    if (mSignalFirstFrameDiplayed) {
        mSignalFirstFrameDiplayed = false;
        INFO(mLogCategory,"send first frame displayed msg");
        mPlugin->handleMsgNotify(MSG_FIRST_FRAME,(void*)&buffer->pts);
    }
    mPlugin->handleBufferRelease(buffer);
    return true;
}