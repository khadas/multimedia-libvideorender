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
#ifndef __WAYLAND_SHM_H__
#define __WAYLAND_SHM_H__
#include <sys/types.h>
#include "render_plugin.h"
#include "wayland_wlwrap.h"

class WaylandDisplay;

class WaylandShmBuffer : public WaylandWLWrap
{
  public:
    WaylandShmBuffer(WaylandDisplay *display, int logCategory);
    virtual ~WaylandShmBuffer();
    virtual struct wl_buffer *getWlBuffer() {
        return mWlBuffer;
    };
    virtual void *getDataPtr() {
        return mData;
    };
    virtual int getSize() {
        return mSize;
    };
    struct wl_buffer *constructWlBuffer(int width, int height, RenderVideoFormat format);
    int getWidth() {
        return mWidth;
    };
    int getHeight() {
        return mHeight;
    };
    RenderVideoFormat getFormat() {
        return mFormat;
    }
  private:
    int createAnonymousFile(off_t size);
    WaylandDisplay *mDisplay;
    struct wl_buffer *mWlBuffer;
    void *mData;
    int mStride;
    int mSize;
    int mWidth;
    int mHeight;
    RenderVideoFormat mFormat;

    int mLogCategory;
};

#endif /*__WAYLAND_SHM_H__*/