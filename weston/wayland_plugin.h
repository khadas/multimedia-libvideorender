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
#ifndef __WAYLAND_PLUGIN_H__
#define __WAYLAND_PLUGIN_H__
#include "render_plugin.h"
#include "wayland_display.h"
#include "Thread.h"
#include "Queue.h"

class WaylandPlugin : public RenderPlugin, public Tls::Thread
{
  public:
    WaylandPlugin(int logCatgory);
    virtual ~WaylandPlugin();
    virtual void init();
    virtual void release();
    void setCallback(void *userData, PluginCallback *callback);
    virtual int openDisplay();
    virtual int openWindow();
    virtual int prepareFrame(RenderBuffer *buffer);
    virtual int displayFrame(RenderBuffer *buffer, int64_t displayTime);
    virtual int flush();
    virtual int pause();
    virtual int resume();
    virtual int closeDisplay();
    virtual int closeWindow();
    virtual int getValue(PluginKey key, void *value);
    virtual int setValue(PluginKey key, void *value);
    int getLogCategory() {
        return mLogCategory;
    };
    //thread func
    void readyToRun();
    virtual bool threadLoop();
    //buffer release callback
    virtual void handleBufferRelease(RenderBuffer *buffer);
    //buffer had displayed ,but not release
    virtual void handleFrameDisplayed(RenderBuffer *buffer);
    //buffer droped callback
    virtual void handleFrameDropped(RenderBuffer *buffer);
    static void queueFlushCallback(void *userdata,void *data);
  private:
    PluginCallback *mCallback;
    WaylandDisplay *mDisplay;

    int mLogCategory;

    mutable Tls::Mutex mRenderLock;
    int mFrameWidth;
    int mFrameHeight;

    void *mUserData;
    Tls::Queue *mQueue;
    bool mPaused;
    /*immediately output video frame to display*/
    bool mImmediatelyOutput;
};


#endif /*__WAYLAND_PLUGIN_H__*/