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
#ifndef __WAYLAND_BUFFER_H__
#define __WAYLAND_BUFFER_H__
#include "Mutex.h"
#include "Condition.h"
#include "wayland_wlwrap.h"

class WaylandDisplay;
class WaylandWindow;

/**
 * @brief create waylandbuffer include wl_buffer to use
 * if create waylandbuffer with dma buffer, the sequence is
 * 1.waylandbuffer = new WaylandBuffer
 * 2.waylandbuffer->setRenderRealTime
 * 3.setRenderRealTime->setBufferFormat
 * 4.dmabufConstructWlBuffer
 *
 * if create waylandbuffer with shm pool
 *
 */
class WaylandBuffer {
  public:
    WaylandBuffer(WaylandDisplay *display, int logCategory);
    virtual ~WaylandBuffer();
    int constructWlBuffer(RenderBuffer *buf);
    void forceRedrawing() {
        mLock.lock();
        mRedrawingPending = false;
        mLock.unlock();
    };
    void setRenderRealTime(int64_t realTime) {
        mRealTime = realTime;
    };
    int64_t getRenderRealTime() {
        return mRealTime;
    };
    void attach(struct wl_surface *surface);
    void setBufferFormat(RenderVideoFormat format)
    {
        mBufferFormat = format;
    };
    RenderBuffer *getRenderBuffer()
    {
        return mRenderBuffer;
    };
    WaylandDisplay *getWaylandDisplay()
    {
        return mDisplay;
    };
    bool isFree()
    {
        return !mUsedByCompositor;
    };
    int getFrameWidth() {
        return mFrameWidth;
    };
    int getFrameHeight() {
        return mFrameHeight;
    };
    struct wl_buffer *getWlBuffer();
    static void bufferRelease (void *data, struct wl_buffer *wl_buffer);
    static void bufferdroped (void *data, struct wl_buffer *wl_buffer);
    static void frameDisplayedCallback(void *data, struct wl_callback *callback, uint32_t time);
  private:
    int mLogCategory;
    WaylandDisplay *mDisplay;
    RenderBuffer *mRenderBuffer;
    WaylandWLWrap *mWaylandWlWrap; //wl_buffer wrapper
    int64_t mRealTime;
    bool mUsedByCompositor;
    RenderVideoFormat mBufferFormat;
    int mFrameWidth;
    int mFrameHeight;
    mutable Tls::Mutex mLock;
    bool mRedrawingPending;
};

#endif /*__WAYLAND_BUFFER_H__*/