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
#ifndef __WAYLAND_DMA_H__
#define __WAYLAND_DMA_H__
#include "Mutex.h"
#include "Condition.h"
#include "wayland_wlwrap.h"
#include "render_plugin.h"

class WaylandDisplay;

class WaylandDmaBuffer : public WaylandWLWrap
{
  public:
    WaylandDmaBuffer(WaylandDisplay *display, int logCategory);
    virtual ~WaylandDmaBuffer();
    virtual struct wl_buffer *getWlBuffer() {
        return mWlBuffer;
    };
    virtual void *getDataPtr() {
        return mData;
    };
    virtual int getSize() {
        return mSize;
    };
    struct wl_buffer *constructWlBuffer(RenderDmaBuffer *dmabuf, RenderVideoFormat format);
    static void dmabufCreateSuccess(void *data,
            struct zwp_linux_buffer_params_v1 *params,
            struct wl_buffer *new_buffer);
    static void dmabufCreateFail(void *data,
            struct zwp_linux_buffer_params_v1 *params);
  private:
    WaylandDisplay *mDisplay;
    RenderDmaBuffer mRenderDmaBuffer;
    struct wl_buffer *mWlBuffer;
    mutable Tls::Mutex mMutex;
    Tls::Condition mCondition;
    void *mData;
    int mSize;

    int mLogCategory;
};
#endif /*__WAYLAND_DMA_H__*/