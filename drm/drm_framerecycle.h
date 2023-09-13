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
#ifndef __DRM_FRAME_RECYCLE_H__
#define __DRM_FRAME_RECYCLE_H__
#include "Mutex.h"
#include "Thread.h"
#include "Queue.h"

class DrmDisplay;
struct FrameEntity;

class DrmFrameRecycle : public Tls::Thread
{
  public:
    DrmFrameRecycle(DrmDisplay *drmDisplay, int logCategory);
    virtual ~DrmFrameRecycle();
    bool start();
    bool stop();
    bool recycleFrame(FrameEntity * frameEntity);
    //thread func
    void readyToRun();
    virtual bool threadLoop();
    static void queueFlushCallback(void *userdata,void *data);
  private:
    DrmDisplay *mDrmDisplay;
    int mLogCategory;

    bool mStop;
    bool mWaitVideoFence;
    mutable Tls::Mutex mMutex;
    Tls::Queue *mQueue;
};

#endif /*__DRM_FRAME_RECYCLE_H__*/