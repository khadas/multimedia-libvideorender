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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "wayland_display.h"
#include "wayland_buffer.h"
#include "Logger.h"
#include "wayland_videoformat.h"
#include "wayland_dma.h"
#include "wayland_shm.h"
#include "Utils.h"
#include "ErrorCode.h"

#define TAG "rlib:wayland_buffer"

WaylandBuffer::WaylandBuffer(WaylandDisplay *display, int logCategory)
    : mDisplay(display),
    mLogCategory(logCategory)
{
    mRenderBuffer = NULL;
    mWaylandWlWrap = NULL;
    mUsedByCompositor = false;
    mRedrawingPending = false;
    mRealTime = -1;
    mBufferFormat = VIDEO_FORMAT_UNKNOWN;
    mFrameWidth = 0;
    mFrameHeight = 0;
}

WaylandBuffer::~WaylandBuffer()
{
    /*if weston obtains the wl_buffer,we need
     * notify user to release renderBuffer*/
    if (mRenderBuffer) {
        mDisplay->handleBufferReleaseCallback(this);
        mRenderBuffer = NULL;
    }
    if (mWaylandWlWrap) {
        delete mWaylandWlWrap;
        mWaylandWlWrap = NULL;
    }
}

void WaylandBuffer::bufferRelease (void *data, struct wl_buffer *wl_buffer)
{
    WaylandBuffer* waylandBuffer = static_cast<WaylandBuffer*>(data);
    TRACE(waylandBuffer->mLogCategory,"--wl_buffer:%p,renderBuffer:%p",wl_buffer,waylandBuffer->mRenderBuffer);
    waylandBuffer->mUsedByCompositor = false;
    //sometimes this callback be called twice
    //this cause double free,so check mRenderBuffer
    if (waylandBuffer->mRenderBuffer) {
        waylandBuffer->mDisplay->handleBufferReleaseCallback(waylandBuffer);
        waylandBuffer->mRenderBuffer = NULL;
    }
}

void WaylandBuffer::bufferdroped (void *data, struct wl_buffer *wl_buffer)
{
    WaylandBuffer* waylandBuffer = static_cast<WaylandBuffer*>(data);
    WARNING(waylandBuffer->mLogCategory,"--droped wl_buffer:%p,renderBuffer:%p",wl_buffer,waylandBuffer->mRenderBuffer);
    waylandBuffer->mUsedByCompositor = false;

    if (waylandBuffer->mRenderBuffer) {
        waylandBuffer->mDisplay->handleFrameDropedCallback(waylandBuffer);
        waylandBuffer->mDisplay->handleBufferReleaseCallback(waylandBuffer);
        waylandBuffer->mRenderBuffer = NULL;
    }
}

//static const struct wl_buffer_listener buffer_listener = {
//    WaylandBuffer::bufferRelease,
//    WaylandBuffer::bufferdroped
//};
 static const struct wl_buffer_listener buffer_listener = {
     WaylandBuffer::bufferRelease
 };

/*if we commit buffers to weston too fast,it causes weston can't invoke this callback*/
void WaylandBuffer::frameDisplayedCallback(void *data, struct wl_callback *callback, uint32_t time)
{
    WaylandBuffer* waylandBuffer = static_cast<WaylandBuffer*>(data);
    waylandBuffer->mDisplay->setRedrawingPending(false);
    waylandBuffer->mLock.lock();
    bool redrawing = waylandBuffer->mRedrawingPending;
    waylandBuffer->mRedrawingPending = false;
    waylandBuffer->mLock.unlock();
    if (waylandBuffer->mRenderBuffer && redrawing) {
        waylandBuffer->mDisplay->handleFrameDisplayedCallback(waylandBuffer);
    }
    wl_callback_destroy (callback);
}

static const struct wl_callback_listener frame_callback_listener = {
  WaylandBuffer::frameDisplayedCallback
};

int WaylandBuffer::constructWlBuffer(RenderBuffer *buf)
{
    struct wl_buffer * wlbuffer = NULL;

    mRenderBuffer = buf;
    if (mWaylandWlWrap) {
        return NO_ERROR;
    }

    if (buf->flag & BUFFER_FLAG_DMA_BUFFER) {
        WaylandDmaBuffer *waylanddma = new WaylandDmaBuffer(mDisplay, mLogCategory);
        wlbuffer = waylanddma->constructWlBuffer(&buf->dma, mBufferFormat);
        if (!wlbuffer) {
            delete waylanddma;
            ERROR(mLogCategory,"create wl_buffer fail");
            return ERROR_INVALID_OPERATION;
        }
        mWaylandWlWrap = waylanddma;
        mFrameWidth = buf->dma.width;
        mFrameHeight = buf->dma.height;
    }

    /*register buffer release listen*/
    wl_buffer_add_listener (wlbuffer, &buffer_listener, this);

    return NO_ERROR;
}

struct wl_buffer *WaylandBuffer::getWlBuffer()
{
    if (mWaylandWlWrap) {
        return mWaylandWlWrap->getWlBuffer();
    } else {
        return NULL;
    }
}

void WaylandBuffer::attach(struct wl_surface *surface)
{
    struct wl_callback *callback;
    struct wl_buffer *wlbuffer = NULL;
    if (mUsedByCompositor) {
        DEBUG(mLogCategory,"buffer used by compositor");
        return;
    }

    //callback when this frame displayed
    callback = wl_surface_frame (surface);
    wl_callback_add_listener (callback, &frame_callback_listener, this);
    mDisplay->setRedrawingPending(true);

    wlbuffer = getWlBuffer();
    if (wlbuffer) {
        wl_surface_attach (surface, wlbuffer, 0, 0);
    }

    mUsedByCompositor = true;
    mRedrawingPending = true;
}