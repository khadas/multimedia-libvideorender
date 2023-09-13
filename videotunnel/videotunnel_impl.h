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
#ifndef __VIDEO_TUNNEL_IMPLEMENT_H__
#define __VIDEO_TUNNEL_IMPLEMENT_H__
#include <unordered_map>
#include "Mutex.h"
#include "Thread.h"
#include "Poll.h"
#include "videotunnel_lib_wrap.h"

class VideoTunnelPlugin;

class VideoTunnelImpl : public Tls::Thread
{
  public:
    VideoTunnelImpl(VideoTunnelPlugin *plugin, int logCategory);
    virtual ~VideoTunnelImpl();
    bool init();
    bool release();
    bool connect();
    bool disconnect();
    bool displayFrame(RenderBuffer *buf, int64_t displayTime);
    void flush();
    void setFrameSize(int width, int height);
    void setVideotunnelId(int id);
    void getVideotunnelId(int *id) {
        if (id) {
            *id = mInstanceId;
        }
    };
    //thread func
    virtual void readyToRun();
    virtual bool threadLoop();
  private:
    void waitFence(int fence);
    VideoTunnelPlugin *mPlugin;
    mutable Tls::Mutex mMutex;
    VideotunnelLib *mVideotunnelLib;

    int mLogCategory;

    int mFd;
    int mInstanceId;
    bool mIsVideoTunnelConnected;
    bool mStarted;
    bool mRequestStop;

    //fd as key index,if has two fd,the key is fd0 << 32 | fd1
    std::unordered_map<int, RenderBuffer *> mQueueRenderBufferMap;
    int mQueueFrameCnt;

    int mFrameWidth;
    int mFrameHeight;
    Tls::Poll *mPoll;
    int64_t mLastDisplayTime;
    bool mSignalFirstFrameDiplayed;
    bool mUnderFlowDetect;
};

#endif /*__VIDEO_TUNNEL_IMPLEMENT_H__*/