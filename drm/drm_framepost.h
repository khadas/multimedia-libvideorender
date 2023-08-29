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
#ifndef __DRM_FRAME_POST_H__
#define __DRM_FRAME_POST_H__
#include <mutex>
#include "Thread.h"
#include "Queue.h"

class DrmDisplay;
struct FrameEntity;

class DrmFramePost : public Tls::Thread
{
  public:
    DrmFramePost(DrmDisplay *drmDisplay);
    virtual ~DrmFramePost();
    bool start();
    bool stop();
    bool readyPostFrame(FrameEntity * frameEntity);
    void flush();
    void pause();
    void resume();
    void setImmediatelyOutout(bool on);
    void setWindowSize(int x, int y, int w, int h);
    //thread func
    void readyToRun();
    virtual bool threadLoop();
  private:
    typedef struct {
      int x;
      int y;
      int w;
      int h;
    } WinRect;
    DrmDisplay *mDrmDisplay;

    bool mPaused;
    bool mStop;
    mutable std::mutex mMutex;
    Tls::Queue *mQueue;

    /*immediately output video frame to display*/
    bool mImmediatelyOutput;
    WinRect mWinRect;
};

#endif /*__DRM_FRAME_POST_H__*/