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
#include <string>
#include "wayland_display.h"
#include "ErrorCode.h"
#include "Logger.h"
#include "wayland_plugin.h"
#include "wayland_videoformat.h"
#include "wayland_shm.h"
#include "wayland_dma.h"
#include "wayland_buffer.h"

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

#define UNUSED_PARAM(x) ((void)(x))

#define TAG "rlib:wayland_display"

void WaylandDisplay::dmabuf_modifiers(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
         uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    Tls::Mutex::Autolock _l(self->mMutex);
    if (wl_dmabuf_format_to_video_format (format) != VIDEO_FORMAT_UNKNOWN) {
        TRACE(self->mLogCategory,"regist dmabuffer format:%d (%s) hi:%x,lo:%x",format,print_dmabuf_format_name(format),modifier_hi,modifier_lo);
        uint64_t modifier = ((uint64_t)modifier_hi << 32) | modifier_lo;
        auto item = self->mDmaBufferFormats.find(format);
        if (item == self->mDmaBufferFormats.end()) {
            std::pair<uint32_t ,uint64_t> item(format, modifier);
            self->mDmaBufferFormats.insert(item);
        } else { //found format
            item->second = modifier;
        }
    }
}

void
WaylandDisplay::dmaBufferFormat (void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
    uint32_t format)
{
#if 0
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);

    if (wl_dmabuf_format_to_video_format (format) != VIDEO_FORMAT_UNKNOWN) {
        TRACE(mLogCategory,"regist dmabuffer format:%d : %s",format);
        //self->mDmaBufferFormats.push_back(format);
    }
#endif
   /* XXX: deprecated */
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
    WaylandDisplay::dmaBufferFormat,
    WaylandDisplay::dmabuf_modifiers
};

static void
handle_xdg_wm_base_ping (void *user_data, struct xdg_wm_base *xdg_wm_base,
    uint32_t serial)
{
  xdg_wm_base_pong (xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
  handle_xdg_wm_base_ping
};

void WaylandDisplay::shmFormat(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    self->mShmFormats.push_back(format);
}

static const struct wl_shm_listener shm_listener = {
  WaylandDisplay::shmFormat
};

void WaylandDisplay::outputHandleGeometry( void *data,
                                  struct wl_output *output,
                                  int x,
                                  int y,
                                  int physicalWidth,
                                  int physicalHeight,
                                  int subPixel,
                                  const char *make,
                                  const char *model,
                                  int transform )
{
    UNUSED_PARAM(make);
    UNUSED_PARAM(model);

    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    DEBUG(self->mLogCategory,"wl_output %p x:%d,y:%d,physicalWidth:%d,physicalHeight:%d,subPixel:%d,trans:%d",
            output,x, y,physicalWidth, physicalHeight,subPixel,transform);
    Tls::Mutex::Autolock _l(self->mMutex);
    for (int i = 0; i < DEFAULT_DISPLAY_OUTPUT_NUM; i++) {
        if (output == self->mOutput[i].wlOutput) {
            self->mOutput[i].offsetX = x;
            self->mOutput[i].offsetY = y;
        }
    }
}

void WaylandDisplay::outputHandleMode( void *data,
                              struct wl_output *output,
                              uint32_t flags,
                              int width,
                              int height,
                              int refreshRate )
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);

    if ( flags & WL_OUTPUT_MODE_CURRENT ) {
        Tls::Mutex::Autolock _l(self->mMutex);
        for (int i = 0; i < DEFAULT_DISPLAY_OUTPUT_NUM; i++) {
            if (output == self->mOutput[i].wlOutput) {
                self->mOutput[i].width = width;
                self->mOutput[i].height = height;
                self->mOutput[i].refreshRate = refreshRate;
            }
        }
        //if a displayoutput had been selected,set this rectangle to wayland
        int selectOutput = self->mActiveOutput;
        DEBUG(self->mLogCategory,"wl_output: %p (%dx%d) refreshrate:%d,active output index %d\n",output, width, height,refreshRate,selectOutput);
        if (selectOutput >= 0 && selectOutput < DEFAULT_DISPLAY_OUTPUT_NUM) {
            if (self->mOutput[selectOutput].width > 0 && self->mOutput[selectOutput].height > 0) {
                self->setRenderRectangle(self->mOutput[selectOutput].offsetX,
                        self->mOutput[selectOutput].offsetY,
                        self->mOutput[selectOutput].width,
                        self->mOutput[selectOutput].height);
            }
        }
    }
}

void WaylandDisplay::outputHandleDone( void *data,
                              struct wl_output *output )
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(output);
}

void WaylandDisplay::outputHandleScale( void *data,
                               struct wl_output *output,
                               int32_t scale )
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(output);
    UNUSED_PARAM(scale);
}

static const struct wl_output_listener outputListener = {
    WaylandDisplay::outputHandleGeometry,
    WaylandDisplay::outputHandleMode,
    WaylandDisplay::outputHandleDone,
    WaylandDisplay::outputHandleScale
};

void WaylandDisplay::pointerHandleEnter(void *data, struct wl_pointer *pointer,
                                uint32_t serial, struct wl_surface *surface,
                                wl_fixed_t sx, wl_fixed_t sy)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);

    if (self->mFullScreen) {
        wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
    }
}

void WaylandDisplay::pointerHandleLeave(void *data, struct wl_pointer *pointer,
                                uint32_t serial, struct wl_surface *surface)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(pointer);
    UNUSED_PARAM(serial);
    UNUSED_PARAM(surface);
}

void WaylandDisplay::pointerHandleMotion(void *data, struct wl_pointer *pointer,
                                uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    int x = wl_fixed_to_int(sx);
    int y = wl_fixed_to_int(sy);
    DEBUG(self->mLogCategory,"pointer motion fixed[%d, %d] to-int: x[%d] y[%d]\n", sx, sy, x, y);
}

void WaylandDisplay::pointerHandleButton(void *data, struct wl_pointer *wl_pointer,
                                    uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(wl_pointer);
    UNUSED_PARAM(time);
    UNUSED_PARAM(serial);
    UNUSED_PARAM(button);
    UNUSED_PARAM(state);
}

void WaylandDisplay::pointerHandleAxis(void *data, struct wl_pointer *wl_pointer,
                                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(wl_pointer);
    UNUSED_PARAM(time);
    UNUSED_PARAM(axis);
    UNUSED_PARAM(value);
}

static const struct wl_pointer_listener pointer_listener = {
    WaylandDisplay::pointerHandleEnter,
    WaylandDisplay::pointerHandleLeave,
    WaylandDisplay::pointerHandleMotion,
    WaylandDisplay::pointerHandleButton,
    WaylandDisplay::pointerHandleAxis,
};

void WaylandDisplay::touchHandleDown(void *data, struct wl_touch *wl_touch,
                                    uint32_t serial, uint32_t time, struct wl_surface *surface,
                                    int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
}

void WaylandDisplay::touchHandleUp(void *data, struct wl_touch *wl_touch,
                                    uint32_t serial, uint32_t time, int32_t id)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(wl_touch);
    UNUSED_PARAM(serial);
    UNUSED_PARAM(time);
    UNUSED_PARAM(id);
}

void WaylandDisplay::touchHandleMotion(void *data, struct wl_touch *wl_touch,
                                    uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(wl_touch);
    UNUSED_PARAM(time);
    UNUSED_PARAM(id);
    UNUSED_PARAM(x_w);
    UNUSED_PARAM(y_w);
}

void WaylandDisplay::touchHandleFrame(void *data, struct wl_touch *wl_touch)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(wl_touch);
}

void WaylandDisplay::touchHandleCancel(void *data, struct wl_touch *wl_touch)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(wl_touch);
}

static const struct wl_touch_listener touch_listener = {
    WaylandDisplay::touchHandleDown,
    WaylandDisplay::touchHandleUp,
    WaylandDisplay::touchHandleMotion,
    WaylandDisplay::touchHandleFrame,
    WaylandDisplay::touchHandleCancel,
};

void WaylandDisplay::keyboardHandleKeymap(void *data, struct wl_keyboard *keyboard,
                                    uint32_t format, int fd, uint32_t size)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(keyboard);
    UNUSED_PARAM(format);
    UNUSED_PARAM(fd);
    UNUSED_PARAM(size);
}

void WaylandDisplay::keyboardHandleEnter(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(keyboard);
    UNUSED_PARAM(serial);
    UNUSED_PARAM(surface);
    UNUSED_PARAM(keys);
}

void WaylandDisplay::keyboardHandleLeave(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, struct wl_surface *surface)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(keyboard);
    UNUSED_PARAM(serial);
    UNUSED_PARAM(surface);
}

void WaylandDisplay::keyboardHandleKey(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(keyboard);
    UNUSED_PARAM(serial);
    UNUSED_PARAM(time);
    UNUSED_PARAM(key);
    UNUSED_PARAM(state);
}

void WaylandDisplay::keyboardHandleModifiers(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, uint32_t mods_depressed,
                                    uint32_t mods_latched, uint32_t mods_locked,
                                    uint32_t group)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(keyboard);
    UNUSED_PARAM(serial);
    UNUSED_PARAM(mods_depressed);
    UNUSED_PARAM(mods_latched);
    UNUSED_PARAM(mods_locked);
    UNUSED_PARAM(group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    WaylandDisplay::keyboardHandleKeymap,
    WaylandDisplay::keyboardHandleEnter,
    WaylandDisplay::keyboardHandleLeave,
    WaylandDisplay::keyboardHandleKey,
    WaylandDisplay::keyboardHandleModifiers,
};

void WaylandDisplay::seatHandleCapabilities(void *data, struct wl_seat *seat,
                               uint32_t caps)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !self->mPointer) {
        self->mPointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(self->mPointer, &pointer_listener, data);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && self->mPointer) {
        wl_pointer_destroy(self->mPointer);
        self->mPointer = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !self->mKeyboard) {
        self->mKeyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(self->mKeyboard, &keyboard_listener, data);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && self->mKeyboard) {
        wl_keyboard_destroy(self->mKeyboard);
        self->mKeyboard = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !self->mTouch) {
        self->mTouch = wl_seat_get_touch(seat);
        wl_touch_set_user_data(self->mTouch, data);
        wl_touch_add_listener(self->mTouch, &touch_listener, data);
    } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && self->mTouch) {
        wl_touch_destroy(self->mTouch);
        self->mTouch = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    WaylandDisplay::seatHandleCapabilities,
};


void WaylandDisplay::handleXdgToplevelClose (void *data, struct xdg_toplevel *xdg_toplevel)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);

    INFO(self->mLogCategory,"XDG toplevel got a close event.");
}

void WaylandDisplay::handleXdgToplevelConfigure (void *data, struct xdg_toplevel *xdg_toplevel,
            int32_t width, int32_t height, struct wl_array *states)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    uint32_t *state;

    INFO(self->mLogCategory, "XDG toplevel got a configure event, width:height [ %d, %d ].", width, height);
    /*
    wl_array_for_each (state, states) {
        switch (*state) {
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
        case XDG_TOPLEVEL_STATE_RESIZING:
        case XDG_TOPLEVEL_STATE_ACTIVATED:
            break;
        }
    }
    */

    if (width <= 0 || height <= 0)
        return;

    int selectOutput = self->mActiveOutput;
    if (width == self->mOutput[selectOutput].width && height == self->mOutput[selectOutput].height) {
        self->setRenderRectangle(self->mOutput[selectOutput].offsetX,
                            self->mOutput[selectOutput].offsetY,
                            self->mOutput[selectOutput].width,
                            self->mOutput[selectOutput].height);
    } else{
        self->setRenderRectangle(self->mRenderRect.x, self->mRenderRect.y, width, height);
    }
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    WaylandDisplay::handleXdgToplevelConfigure,
    WaylandDisplay::handleXdgToplevelClose,
};

void WaylandDisplay::handleXdgSurfaceConfigure (void *data, struct xdg_surface *xdg_surface,
    uint32_t serial)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    xdg_surface_ack_configure (xdg_surface, serial);

    TRACE(self->mLogCategory,"handleXdgSurfaceConfigure");
    Tls::Mutex::Autolock _l(self->mConfigureMutex);
    self->mXdgSurfaceConfigured = true;
    if (self->mRenderRect.w > 0 && self->mRenderRect.h) {
        DEBUG(self->mLogCategory,"set xdg surface geometry(%d,%d,%d,%d)",self->mRenderRect.x,self->mRenderRect.y,self->mRenderRect.w,self->mRenderRect.h);
        xdg_surface_set_window_geometry(self->mXdgSurface,self->mRenderRect.x,self->mRenderRect.y,self->mRenderRect.w,self->mRenderRect.h);
    }
}

static const struct xdg_surface_listener xdg_surface_listener = {
    WaylandDisplay::handleXdgSurfaceConfigure,
};

void
WaylandDisplay::registryHandleGlobal (void *data, struct wl_registry *registry,
    uint32_t name, const char *interface, uint32_t version)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    TRACE(self->mLogCategory,"registryHandleGlobal,name:%u,interface:%s,version:%d",name,interface,version);

    if (strcmp (interface, "wl_compositor") == 0) {
        self->mCompositor = (struct wl_compositor *)wl_registry_bind (registry, name, &wl_compositor_interface, 1/*MIN (version, 3)*/);
    } else if (strcmp (interface, "wl_subcompositor") == 0) {
        self->mSubCompositor = (struct wl_subcompositor *)wl_registry_bind (registry, name, &wl_subcompositor_interface, 1);
    } else if (strcmp (interface, "xdg_wm_base") == 0) {
        self->mXdgWmBase = (struct xdg_wm_base *)wl_registry_bind (registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener (self->mXdgWmBase, &xdg_wm_base_listener, (void *)self);
    } else if (strcmp (interface, "wl_shm") == 0) {
        self->mShm = (struct wl_shm *)wl_registry_bind (registry, name, &wl_shm_interface, 1);
        wl_shm_add_listener (self->mShm, &shm_listener, self);
    } else if (strcmp (interface, "zwp_fullscreen_shell_v1") == 0) {
        //self->mFullscreenShell = (struct zwp_fullscreen_shell_v1 *)wl_registry_bind (registry, name,
        //    &zwp_fullscreen_shell_v1_interface, 1);
    } else if (strcmp (interface, "wp_viewporter") == 0) {
        self->mViewporter = (struct wp_viewporter *)wl_registry_bind (registry, name, &wp_viewporter_interface, 1);
    } else if (strcmp (interface, "zwp_linux_dmabuf_v1") == 0) {
        if (version < 3)
            return;
        self->mDmabuf = (struct zwp_linux_dmabuf_v1 *)wl_registry_bind (registry, name, &zwp_linux_dmabuf_v1_interface, 3);
        zwp_linux_dmabuf_v1_add_listener (self->mDmabuf, &dmabuf_listener, (void *)self);
    }  else if (strcmp (interface, "wl_output") == 0) {
        for (int i = 0; i < DEFAULT_DISPLAY_OUTPUT_NUM; i++) {
            if (self->mOutput[i].wlOutput ==  NULL) {
                self->mOutput[i].name = name;
                self->mOutput[i].wlOutput = (struct wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version);
                wl_output_add_listener(self->mOutput[i].wlOutput, &outputListener, (void *)self);
                if (i == 0) { //primary wl_output
                    self->mOutput[i].isPrimary = true;
                }
                return;
            }
        }
        WARNING(self->mLogCategory,"Not enough free output");
    } else if (strcmp(interface, "wl_seat") == 0) {
        //self->mSeat = (struct wl_seat *)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        //wl_seat_add_listener(self->mSeat, &seat_listener, (void *)self);
    }
}

void
WaylandDisplay::registryHandleGlobalRemove (void *data, struct wl_registry *registry, uint32_t name)
{
    WaylandDisplay *self = static_cast<WaylandDisplay *>(data);
    /* check wl_output changed */
    DEBUG(self->mLogCategory,"wayland display remove registry handle global,name:%u",name);
    for (int i = 0; i < DEFAULT_DISPLAY_OUTPUT_NUM; i++) {
        if (self->mOutput[i].name == name) {
            self->mOutput[i].name = 0;
            self->mOutput[i].wlOutput = NULL;
            DEBUG(self->mLogCategory,"remove wl_output name:%u",name);
        }
    }
}

static const struct wl_registry_listener registry_listener = {
  WaylandDisplay::registryHandleGlobal,
  WaylandDisplay::registryHandleGlobalRemove
};

WaylandDisplay::WaylandDisplay(WaylandPlugin *plugin, int logCategory)
    :mBufferMutex("bufferMutex"),
    mWaylandPlugin(plugin),
    mLogCategory(logCategory)
{
    TRACE(mLogCategory,"construct WaylandDisplay");
    mWlDisplay = NULL;
    mWlDisplayWrapper = NULL;
    mWlQueue = NULL;
    mRegistry = NULL;
    mCompositor = NULL;
    mXdgWmBase = NULL;
    mViewporter = NULL;
    mDmabuf = NULL;
    mShm = NULL;
    mSeat = NULL;
    mPointer = NULL;
    mTouch = NULL;
    mKeyboard = NULL;
    mActiveOutput = 0; //default is primary output
    mPoll = new Tls::Poll(true);
    //window
    mVideoWidth = 0;
    mVideoHeight = 0;
    mVideoSurface = NULL;
    mXdgSurface = NULL;
    mXdgToplevel = NULL;
    mAreaViewport = NULL;
    mVideoViewport = NULL;
    mNoBorderUpdate = false;
    mAreaShmBuffer = NULL;
    mCommitCnt = 0;
    mIsSendPtsToWeston = false;
    mReCommitAreaSurface = false;
    mAreaSurface = NULL;
    mAreaSurfaceWrapper = NULL;
    mVideoSurfaceWrapper = NULL;
    mVideoSubSurface = NULL;
    mXdgSurfaceConfigured = false;
    mPip = 0;
    mIsSendVideoPlaneId = true;
    memset(&mRenderRect, 0, sizeof(struct Rectangle));
    memset(&mVideoRect, 0, sizeof(struct Rectangle));
    memset(&mWindowRect, 0, sizeof(struct Rectangle));
    mFullScreen = true; //default is full screen
    char *env = getenv("VIDEO_RENDER_SEND_PTS_TO_WESTON");
    if (env) {
        int isSet = atoi(env);
        if (isSet > 0) {
            mIsSendPtsToWeston = true;
            INFO(mLogCategory,"set send pts to weston");
        } else {
            mIsSendPtsToWeston = false;
            INFO(mLogCategory,"do not send pts to weston");
        }
    }
    env = getenv("VIDEO_RENDER_SEND_VIDEO_PLANE_ID_TO_WESTON");
    if (env) {
        int isSet = atoi(env);
        if (isSet == 0) {
            mIsSendVideoPlaneId = false;
            INFO(mLogCategory,"do not send video plane id to weston");
        } else {
            mIsSendVideoPlaneId = true;
            INFO(mLogCategory,"send video plane id to weston");
        }
    }
    for (int i = 0; i < DEFAULT_DISPLAY_OUTPUT_NUM; i++) {
        mOutput[i].wlOutput = NULL;
        mOutput[i].offsetX = 0;
        mOutput[i].offsetY = 0;
        mOutput[i].width = 0;
        mOutput[i].height = 0;
        mOutput[i].refreshRate = 0;
        mOutput[i].isPrimary = false;
        mOutput[i].name = 0;
    }
}

WaylandDisplay::~WaylandDisplay()
{
    TRACE(mLogCategory,"desconstruct WaylandDisplay");
    if (mPoll) {
        delete mPoll;
        mPoll = NULL;
    }
}

char *WaylandDisplay::require_xdg_runtime_dir()
{
    char *val = getenv("XDG_RUNTIME_DIR");
    INFO(mLogCategory,"XDG_RUNTIME_DIR=%s",val);

    return val;
}

int WaylandDisplay::openDisplay()
{
    char *name = require_xdg_runtime_dir();
    //DEBUG(mLogCategory,"name:%s",name);
    DEBUG(mLogCategory,"openDisplay in");
    mWlDisplay = wl_display_connect(NULL);
    if (!mWlDisplay) {
        ERROR(mLogCategory,"Failed to connect to the wayland display, XDG_RUNTIME_DIR='%s'",
        name ? name : "NULL");
        return ERROR_OPEN_FAIL;
    }

    mWlDisplayWrapper = (struct wl_display *)wl_proxy_create_wrapper ((void *)mWlDisplay);
    mWlQueue = wl_display_create_queue (mWlDisplay);
    wl_proxy_set_queue ((struct wl_proxy *)mWlDisplayWrapper, mWlQueue);

    mRegistry = wl_display_get_registry (mWlDisplayWrapper);
    wl_registry_add_listener (mRegistry, &registry_listener, (void *)this);

    /* we need exactly 2 roundtrips to discover global objects and their state */
    for (int i = 0; i < 2; i++) {
        if (wl_display_roundtrip_queue (mWlDisplay, mWlQueue) < 0) {
            ERROR(mLogCategory,"Error communicating with the wayland display");
            return ERROR_OPEN_FAIL;
        }
    }

    if (!mCompositor) {
        ERROR(mLogCategory,"Could not bind to wl_compositor. Either it is not implemented in " \
        "the compositor, or the implemented version doesn't match");
        return ERROR_OPEN_FAIL;
    }

    if (!mDmabuf) {
        ERROR(mLogCategory,"Could not bind to zwp_linux_dmabuf_v1");
        return ERROR_OPEN_FAIL;
    }

    if (!mXdgWmBase) {
        /* If wl_surface and wl_display are passed via GstContext
        * wl_shell, xdg_shell and zwp_fullscreen_shell are not used.
        * In this case is correct to continue.
        */
        ERROR(mLogCategory,"Could not bind to either wl_shell, xdg_wm_base or "
            "zwp_fullscreen_shell, video display may not work properly.");
        return ERROR_OPEN_FAIL;
    }

    //create window surface
    createCommonWindowSurface();
    createXdgShellWindowSurface();

    //config weston video plane
    if (mIsSendVideoPlaneId) {
        INFO(mLogCategory,"set weston video plane:%d",mPip);
        wl_surface_set_video_plane(mVideoSurfaceWrapper, mPip);
    }

    //run wl display queue dispatch
    DEBUG(mLogCategory,"To run wl display dispatch queue");
    run("display queue");
    mRedrawingPending = false;

    DEBUG(mLogCategory,"openDisplay out");
    return NO_ERROR;
}

void WaylandDisplay::closeDisplay()
{
    DEBUG(mLogCategory,"closeDisplay in");

   if (isRunning()) {
        TRACE(mLogCategory,"try stop dispatch thread");
        if (mPoll) {
            mPoll->setFlushing(true);
        }
        requestExitAndWait();
    }

    //first destroy window surface
    destroyWindowSurfaces();

    if (mViewporter) {
        wp_viewporter_destroy (mViewporter);
        mViewporter = NULL;
    }

    if (mDmabuf) {
        zwp_linux_dmabuf_v1_destroy (mDmabuf);
        mDmabuf = NULL;
    }

    if (mXdgWmBase) {
        xdg_wm_base_destroy (mXdgWmBase);
        mXdgWmBase = NULL;
    }

    if (mCompositor) {
        wl_compositor_destroy (mCompositor);
        mCompositor = NULL;
    }

    if (mSubCompositor) {
        wl_subcompositor_destroy (mSubCompositor);
        mSubCompositor = NULL;
    }

    if (mRegistry) {
        wl_registry_destroy (mRegistry);
        mRegistry= NULL;
    }

    if (mWlDisplayWrapper) {
        wl_proxy_wrapper_destroy (mWlDisplayWrapper);
        mWlDisplayWrapper = NULL;
    }

    if (mWlQueue) {
        wl_event_queue_destroy (mWlQueue);
        mWlQueue = NULL;
    }

    if (mWlDisplay) {
        wl_display_flush (mWlDisplay);
        wl_display_disconnect (mWlDisplay);
        mWlDisplay = NULL;
    }

    DEBUG(mLogCategory,"closeDisplay out");
}

int WaylandDisplay::toDmaBufferFormat(RenderVideoFormat format, uint32_t *outDmaformat /*out param*/, uint64_t *outDmaformatModifiers /*out param*/)
{
    if (!outDmaformat || !outDmaformatModifiers) {
        WARNING(mLogCategory,"NULL params");
        return ERROR_PARAM_NULL;
    }

    *outDmaformat = 0;
    *outDmaformatModifiers = 0;

    uint32_t dmaformat = video_format_to_wl_dmabuf_format (format);
    if (dmaformat == -1) {
        ERROR(mLogCategory,"Error not found render video format:%d to wl dmabuf format",format);
        return ERROR_NOT_FOUND;
    }

    TRACE(mLogCategory,"render video format:%d -> dmabuf format:%d",format,dmaformat);
    *outDmaformat = (uint32_t)dmaformat;

    /*get dmaformat and modifiers*/
    auto item = mDmaBufferFormats.find(dmaformat);
    if (item == mDmaBufferFormats.end()) { //not found
        WARNING(mLogCategory,"Not found dmabuf for render video format :%d",format);
        *outDmaformatModifiers = 0;
        return NO_ERROR;
    }

    *outDmaformatModifiers = (uint64_t)item->second;

    return NO_ERROR;
}

int WaylandDisplay::toShmBufferFormat(RenderVideoFormat format, uint32_t *outformat)
{
    if (!outformat) {
        WARNING(mLogCategory,"NULL params");
        return ERROR_PARAM_NULL;
    }

    *outformat = 0;

    int shmformat = (int)video_format_to_wl_shm_format(format);
    if (shmformat < 0) {
        ERROR(mLogCategory,"Error not found render video format:%d to wl shmbuf format",format);
        return ERROR_NOT_FOUND;
    }

    for (auto item = mShmFormats.begin(); item != mShmFormats.end(); ++item) {
        uint32_t  registFormat = (uint32_t)*item;
        if (registFormat == (uint32_t)shmformat) {
            *outformat = registFormat;
            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}

void WaylandDisplay::setVideoBufferFormat(RenderVideoFormat format)
{
    TRACE(mLogCategory,"set video buffer format: %d",format);
    mBufferFormat = format;
};

void WaylandDisplay::setDisplayOutput(int output)
{
    TRACE(mLogCategory,"select display output: %d",output);
    if (output < 0 || output >= DEFAULT_DISPLAY_OUTPUT_NUM) {
        ERROR(mLogCategory, "display output index error,please set 0:primary or 1:extend,now:%d",output);
        return;
    }
    if (!mOutput[output].wlOutput) {
        ERROR(mLogCategory, "Error output index,wl_output is null,now:%d",output);
        return;
    }
    if (mActiveOutput != output) {
        setRenderRectangle(mOutput[output].offsetX, mOutput[output].offsetY,
                        mOutput[output].width, mOutput[output].height);
        mActiveOutput = output;
    }
}

int WaylandDisplay::getDisplayOutput()
{
    return mActiveOutput;
}

void WaylandDisplay::setPip(int pip)
{
    INFO(mLogCategory,"set pip:%d",pip);
    mPip = pip;
}

void WaylandDisplay::createCommonWindowSurface()
{
    struct wl_region *region;

    mAreaSurface = wl_compositor_create_surface (mCompositor);
    mVideoSurface = wl_compositor_create_surface (mCompositor);
    mAreaSurfaceWrapper = (struct wl_surface *)wl_proxy_create_wrapper (mAreaSurface);
    mVideoSurfaceWrapper = (struct wl_surface *)wl_proxy_create_wrapper (mVideoSurface);

    wl_proxy_set_queue ((struct wl_proxy *) mAreaSurfaceWrapper, mWlQueue);
    wl_proxy_set_queue ((struct wl_proxy *) mVideoSurfaceWrapper, mWlQueue);

    /* embed video_surface in area_surface */
    mVideoSubSurface = wl_subcompositor_get_subsurface (mSubCompositor, mVideoSurface, mAreaSurface);
    wl_subsurface_set_desync (mVideoSubSurface);

    if (mViewporter) {
        mAreaViewport = wp_viewporter_get_viewport (mViewporter, mAreaSurface);
        mVideoViewport = wp_viewporter_get_viewport (mViewporter, mVideoSurface);
    }

    /* do not accept input */
    region = wl_compositor_create_region (mCompositor);
    wl_surface_set_input_region (mAreaSurface, region);
    wl_region_destroy (region);

    region = wl_compositor_create_region (mCompositor);
    wl_surface_set_input_region (mVideoSurface, region);
    wl_region_destroy (region);
}

void WaylandDisplay::createXdgShellWindowSurface()
{
    /* Check which protocol we will use (in order of preference) */
    if (mXdgWmBase) {
        /* First create the XDG surface */
        mXdgSurface= xdg_wm_base_get_xdg_surface (mXdgWmBase, mAreaSurface);
        if (!mXdgSurface) {
            ERROR(mLogCategory,"Unable to get xdg_surface");
            return;
        }
        xdg_surface_add_listener (mXdgSurface, &xdg_surface_listener,(void *)this);

        /* Then the toplevel */
        mXdgToplevel= xdg_surface_get_toplevel (mXdgSurface);
        if (!mXdgSurface) {
            ERROR(mLogCategory,"Unable to get xdg_toplevel");
            return;
        }
        xdg_toplevel_add_listener (mXdgToplevel, &xdg_toplevel_listener, this);

        /* Finally, commit the xdg_surface state as toplevel */
        mXdgSurfaceConfigured = false;
        wl_surface_commit (mAreaSurface);
        wl_display_flush (mWlDisplay);
        /* we need exactly 3 roundtrips to discover global objects and their state */
        for (int i = 0; i < 3; i++) {
            if (wl_display_roundtrip_queue(mWlDisplay, mWlQueue) < 0) {
                ERROR(mLogCategory,"Error communicating with the wayland display");
            }
        }

        if (mXdgSurfaceConfigured) {
            INFO(mLogCategory,"xdg surface had configured");
        } else {
            WARNING(mLogCategory,"xdg surface not configured");
        }

        //full screen show
        if (mFullScreen && mOutput[mActiveOutput].wlOutput) {
            ensureFullscreen(mFullScreen);
        }

        //if wl_output had detected, the width and height of mRenderRect will be set
        //we need invoking setRenderRectangle
        // if (mRenderRect.w > 0 && mRenderRect.h > 0) {
        //     setRenderRectangle(mRenderRect.x, mRenderRect.y, mRenderRect.w, mRenderRect.h);
        // }
    } else {
        ERROR(mLogCategory,"Unable to use xdg_wm_base ");
        return;
    }
}

void WaylandDisplay::destroyWindowSurfaces()
{
    if (mAreaShmBuffer) {
        delete mAreaShmBuffer;
        mAreaShmBuffer = NULL;
    }

    //clean all wayland buffers
    cleanAllWaylandBuffer();

    if (mXdgToplevel) {
        xdg_toplevel_destroy (mXdgToplevel);
        mXdgToplevel = NULL;
    }

    if (mXdgSurface) {
        xdg_surface_destroy (mXdgSurface);
        mXdgSurface = NULL;
    }

    if (mVideoSurfaceWrapper) {
        wl_proxy_wrapper_destroy (mVideoSurfaceWrapper);
        mVideoSurfaceWrapper = NULL;
    }

    if (mVideoSubSurface) {
        wl_subsurface_destroy (mVideoSubSurface);
        mVideoSubSurface = NULL;
    }

    if (mVideoSurface) {
        wl_surface_destroy (mVideoSurface);
        mVideoSurface = NULL;
    }

    if (mAreaSurfaceWrapper) {
        wl_proxy_wrapper_destroy (mAreaSurfaceWrapper);
        mAreaSurfaceWrapper = NULL;
    }

    if (mAreaSurface) {
        wl_surface_destroy (mAreaSurface);
        mAreaSurface = NULL;
        mReCommitAreaSurface = false;
    }
}

void WaylandDisplay::ensureFullscreen(bool fullscreen)
{
    if (mXdgWmBase) {
        DEBUG(mLogCategory,"full screen : %d",fullscreen);
        if (fullscreen) {
            xdg_toplevel_set_fullscreen (mXdgToplevel, mOutput[mActiveOutput].wlOutput);
        } else {
            xdg_toplevel_unset_fullscreen (mXdgToplevel);
        }
    }
}

void WaylandDisplay::setRenderRectangle(int x, int y, int w, int h)
{
    DEBUG(mLogCategory,"set render rect:x:%d,y:%d,w:%d,h:%d",x,y,w,h);

    if (w <= 0 || h <= 0) {
        WARNING(mLogCategory, "wrong render width or height %dx%d",w,h);
        return;
    }

    mRenderRect.x = x;
    mRenderRect.y = y;
    mRenderRect.w = w;
    mRenderRect.h = h;

    if (!mXdgSurfaceConfigured) {
        WARNING(mLogCategory,"Not configured xdg");
        return;
    }

    if (mAreaViewport) {
        wp_viewport_set_destination (mAreaViewport, w, h);
    }

    updateBorders();

    if (mVideoWidth != 0 && mVideoSurface) {
        wl_subsurface_set_sync (mVideoSubSurface);
        resizeVideoSurface(true);
    }

    wl_surface_damage (mAreaSurfaceWrapper, 0, 0, w, h);
    wl_surface_commit (mAreaSurfaceWrapper);

    if (mVideoWidth != 0) {
        wl_subsurface_set_desync (mVideoSubSurface);
    }
}


void WaylandDisplay::setFrameSize(int w, int h)
{
    mVideoWidth = w;
    mVideoHeight = h;
    TRACE(mLogCategory,"frame w:%d,h:%d",mVideoWidth,mVideoHeight);
    if (mRenderRect.w > 0 && mVideoSurface) {
        resizeVideoSurface(true);
    }
}

void WaylandDisplay::setWindowSize(int x, int y, int w, int h)
{
    mWindowRect.x = x;
    mWindowRect.y = y;
    mWindowRect.w = w;
    mWindowRect.h = h;
    TRACE(mLogCategory,"window size:x:%d,y:%d,w:%d,h:%d",mWindowRect.x,mWindowRect.y,mWindowRect.w,mWindowRect.h);
    if (mWindowRect.w > 0 && mVideoWidth > 0 && mVideoSurface) {
        //if had full screen, unset it and set window size
        if (mFullScreen) {
            mFullScreen = false;
            ensureFullscreen(mFullScreen);
        }
        resizeVideoSurface(true);
    }
}


void WaylandDisplay::resizeVideoSurface(bool commit)
{
    Rectangle src = {0,};
    Rectangle dst = {0,};
    Rectangle res;

    /* center the video_subsurface inside area_subsurface */
    src.w = mVideoWidth;
    src.h = mVideoHeight;
    /*if had set the window size, we will scall
    video surface to this window size*/
    if (mWindowRect.w > 0 && mWindowRect.h > 0) {
        dst.x = mWindowRect.x;
        dst.y = mWindowRect.y;
        dst.w = mWindowRect.w;
        dst.h = mWindowRect.h;
        if (mWindowRect.w > mRenderRect.w && mWindowRect.h > mRenderRect.h) {
            WARNING(mLogCategory,"Error window size:%dx%d, but render size:%dx%d,reset to render size",
                mWindowRect.w,mWindowRect.h,mRenderRect.w,mRenderRect.h);
            dst.x = mRenderRect.x;
            dst.y = mRenderRect.y;
            dst.w = mRenderRect.w;
            dst.h = mRenderRect.h;
        }
        //to do,we need set geometry?
        //if (mXdgSurface) {
        //    xdg_surface_set_window_geometry(mXdgSurface, mWindowRect.x, mWindowRect.y, mWindowRect.w, mWindowRect.h);
        //}
    } else { //scal video to full screen
        dst.w = mRenderRect.w;
        dst.h = mRenderRect.h;
    }

    if (mViewporter) {
        videoCenterRect(src, dst, &res, true);
    } else {
        videoCenterRect(src, dst, &res, false);
    }

    wl_subsurface_set_position (mVideoSubSurface, res.x, res.y);

    if (commit) {
        wl_surface_damage (mVideoSurfaceWrapper, 0, 0, res.w, res.h);
        wl_surface_commit (mVideoSurfaceWrapper);
    }

    //top level setting
    if (mXdgToplevel) {
        struct wl_region *region;

        region = wl_compositor_create_region (mCompositor);
        wl_region_add (region, 0, 0, mRenderRect.w, mRenderRect.h);
        wl_surface_set_input_region (mAreaSurface, region);
        wl_region_destroy (region);
    }

    /* this is saved for use in wl_surface_damage */
    mVideoRect.x = res.x;
    mVideoRect.y = res.y;
    mVideoRect.w = res.w;
    mVideoRect.h = res.h;

    //to scale video surface to full screen
    wp_viewport_set_destination(mVideoViewport, res.w, res.h);
    TRACE(mLogCategory,"video rectangle,x:%d,y:%d,w:%d,h:%d",mVideoRect.x, mVideoRect.y, mVideoRect.w, mVideoRect.h);
}

void WaylandDisplay::setOpaque()
{
    struct wl_region *region;

    /* Set area opaque */
    region = wl_compositor_create_region (mCompositor);
    wl_region_add (region, 0, 0, mRenderRect.w, mRenderRect.h);
    wl_surface_set_opaque_region (mAreaSurface, region);
    wl_region_destroy (region);
}

int  WaylandDisplay::prepareFrameBuffer(RenderBuffer * buf)
{
    WaylandBuffer *waylandBuf = NULL;
    int ret;

    waylandBuf = findWaylandBuffer(buf);
    if (waylandBuf == NULL) {
        waylandBuf = new WaylandBuffer(this, mLogCategory);
        waylandBuf->setBufferFormat(mBufferFormat);
        ret = waylandBuf->constructWlBuffer(buf);
        if (ret != NO_ERROR) {
            WARNING(mLogCategory,"dmabufConstructWlBuffer fail,release waylandbuf");
            //delete waylanBuf,WaylandBuffer object destruct will call release callback
            goto waylandbuf_fail;
        } else {
            addWaylandBuffer(buf, waylandBuf);
        }
    }
    return NO_ERROR;
waylandbuf_fail:
    //delete waylandbuf
    delete waylandBuf;
    waylandBuf = NULL;
    return ERROR_UNKNOWN;
}

void WaylandDisplay::displayFrameBuffer(RenderBuffer * buf, int64_t realDisplayTime)
{
    WaylandBuffer *waylandBuf = NULL;
    struct wl_buffer * wlbuffer = NULL;
    int ret;

    if (!buf) {
        ERROR(mLogCategory,"Error input params, waylandbuffer is null");
        return;
    }

    //must commit areasurface first, because weston xdg surface maybe timeout
    //this cause video is not display,commit can resume xdg surface
    if (!mReCommitAreaSurface) {
        mReCommitAreaSurface = true;
        wl_surface_commit (mAreaSurface);
    }

    TRACE(mLogCategory,"display renderBuffer:%p,PTS:%lld,realtime:%lld",buf, buf->pts, realDisplayTime);

    if (buf->flag & BUFFER_FLAG_DMA_BUFFER) {
        if (buf->dma.width <=0 || buf->dma.height <=0) {
            buf->dma.width = mVideoWidth;
            buf->dma.height = mVideoHeight;
        }
        waylandBuf = findWaylandBuffer(buf);
        if (waylandBuf) {
            waylandBuf->setRenderRealTime(realDisplayTime);
            ret = waylandBuf->constructWlBuffer(buf);
            if (ret != NO_ERROR) {
                WARNING(mLogCategory,"dmabufConstructWlBuffer fail,release waylandbuf");
                //delete waylanBuf,WaylandBuffer object destruct will call release callback
                goto waylandbuf_fail;
            }
        } else {
            ERROR(mLogCategory,"NOT found wayland buffer,please prepare buffer first");
            goto waylandbuf_fail;
        }
    }

    if (waylandBuf) {
        wlbuffer = waylandBuf->getWlBuffer();
    }
    if (wlbuffer) {
        Tls::Mutex::Autolock _l(mRenderMutex);
        ++mCommitCnt;
        uint32_t hiPts = realDisplayTime >> 32;
        uint32_t lowPts = realDisplayTime & 0xFFFFFFFF;
        //attach this wl_buffer to weston
        TRACE(mLogCategory,"++attach,renderbuf:%p,wl_buffer:%p(0,0,%d,%d),pts:%lld,commitCnt:%d",buf,wlbuffer,mVideoRect.w,mVideoRect.h,buf->pts,mCommitCnt);
        waylandBuf->attach(mVideoSurfaceWrapper);

        if (mIsSendPtsToWeston) {
            TRACE(mLogCategory,"display time:%lld,hiPts:%u,lowPts:%u",realDisplayTime, hiPts, lowPts);
            wl_surface_set_pts(mVideoSurfaceWrapper, hiPts, lowPts);
        }

        wl_surface_damage (mVideoSurfaceWrapper, 0, 0, mVideoRect.w, mVideoRect.h);
        wl_surface_commit (mVideoSurfaceWrapper);
        //insert this buffer to committed weston buffer manager
        std::pair<int64_t, WaylandBuffer *> item(buf->pts, waylandBuf);
        mCommittedBufferMap.insert(item);
    } else {
        WARNING(mLogCategory,"wlbuffer is NULL");
        /* clear both video and parent surfaces */
        cleanSurface();
    }

    wl_display_flush (mWlDisplay);

    return;
waylandbuf_fail:
    //notify dropped
    mWaylandPlugin->handleFrameDropped(buf);
    //notify app release this buf
    mWaylandPlugin->handleBufferRelease(buf);
    //delete waylandbuf
    delete waylandBuf;
    waylandBuf = NULL;
    return;
}

void WaylandDisplay::handleBufferReleaseCallback(WaylandBuffer *buf)
{
    {
        Tls::Mutex::Autolock _l(mRenderMutex);
        --mCommitCnt;
        //remove buffer if this buffer is ready to release
        RenderBuffer *renderBuffer = buf->getRenderBuffer();
        auto item = mCommittedBufferMap.find(renderBuffer->pts);
        if (item != mCommittedBufferMap.end()) {
            mCommittedBufferMap.erase(item);
        } else {
            WARNING(mLogCategory,"Can't find WaylandBuffer in buffer map");
            return;
        }
    }
    RenderBuffer *renderBuffer = buf->getRenderBuffer();
    TRACE(mLogCategory,"handle release renderBuffer :%p,priv:%p,PTS:%lld,realtime:%lld us,commitCnt:%d",renderBuffer,renderBuffer->priv,renderBuffer->pts/1000,buf->getRenderRealTime(),mCommitCnt);
    mWaylandPlugin->handleBufferRelease(renderBuffer);
}

void WaylandDisplay::handleFrameDisplayedCallback(WaylandBuffer *buf)
{
    RenderBuffer *renderBuffer = buf->getRenderBuffer();
    TRACE(mLogCategory,"handle displayed renderBuffer :%p,PTS:%lld us,realtime:%lld us",renderBuffer,renderBuffer->pts/1000,buf->getRenderRealTime());
    mWaylandPlugin->handleFrameDisplayed(renderBuffer);
}

void WaylandDisplay::handleFrameDropedCallback(WaylandBuffer *buf)
{
    RenderBuffer *renderBuffer = buf->getRenderBuffer();
    TRACE(mLogCategory,"handle droped renderBuffer :%p,PTS:%lld us,realtime:%lld us",renderBuffer,renderBuffer->pts/1000,buf->getRenderRealTime());
    mWaylandPlugin->handleFrameDropped(renderBuffer);
}


void WaylandDisplay::readyToRun()
{
    mFd = wl_display_get_fd (mWlDisplay);
    if (mPoll) {
        mPoll->addFd(mFd);
        mPoll->setFdReadable(mFd, true);
    }
}

bool WaylandDisplay::threadLoop()
{
    int ret;

    while (wl_display_prepare_read_queue (mWlDisplay, mWlQueue) != 0) {
      wl_display_dispatch_queue_pending (mWlDisplay, mWlQueue);
    }

    wl_display_flush (mWlDisplay);

    /*poll timeout value must > 300 ms,otherwise zwp_linux_dmabuf will create failed,
     so do use -1 to wait for ever*/
    ret = mPoll->wait(-1); //wait for ever
    if (ret < 0) { //poll error
        WARNING(mLogCategory,"poll error");
        wl_display_cancel_read(mWlDisplay);
        return false;
    } else if (ret == 0) { //poll time out
        return true; //run loop
    }

    if (wl_display_read_events (mWlDisplay) == -1) {
        goto tag_error;
    }

    wl_display_dispatch_queue_pending (mWlDisplay, mWlQueue);
    return true;
tag_error:
    ERROR(mLogCategory,"Error communicating with the wayland server");
    return false;
}

void WaylandDisplay::videoCenterRect(Rectangle src, Rectangle dst, Rectangle *result, bool scaling)
{
    //if dst is a small window, we scale video to map window size,don't doing center
    if (mRenderRect.w != dst.w && mRenderRect.h != dst.h) {
        result->x = dst.x;
        result->y = dst.y;
        result->w = dst.w;
        result->h = dst.h;
        TRACE(mLogCategory,"small window source is %dx%d dest is %dx%d, result is %dx%d with x,y %dx%d",
        src.w, src.h, dst.w, dst.h, result->w, result->h, result->x, result->y);
        return;
    }
    if (!scaling) {
        result->w = MIN (src.w, dst.w);
        result->h = MIN (src.h, dst.h);
        result->x = dst.x + (dst.w - result->w) / 2;
        result->y = dst.y + (dst.h - result->h) / 2;
    } else {
        double src_ratio, dst_ratio;

        src_ratio = (double) src.w / src.h;
        dst_ratio = (double) dst.w / dst.h;

        if (src_ratio > dst_ratio) {
            result->w = dst.w;
            result->h = dst.w / src_ratio;
            result->x = dst.x;
            result->y = dst.y + (dst.h - result->h) / 2;
        } else if (src_ratio < dst_ratio) {
            result->w = dst.h * src_ratio;
            result->h = dst.h;
            result->x = dst.x + (dst.w - result->w) / 2;
            result->y = dst.y;
        } else {
            result->x = dst.x;
            result->y = dst.y;
            result->w = dst.w;
            result->h = dst.h;
        }
    }

    TRACE(mLogCategory,"source is %dx%d dest is %dx%d, result is %dx%d with x,y %dx%d",
        src.w, src.h, dst.w, dst.h, result->w, result->h, result->x, result->y);
}

void WaylandDisplay::updateBorders()
{
    int width,height;

    if (mNoBorderUpdate)
        return;

    if (mViewporter) {
        width = height = 1;
        mNoBorderUpdate = true;
    } else {
        width = mRenderRect.w;
        height = mRenderRect.h;
    }

    RenderVideoFormat format = VIDEO_FORMAT_BGRA;
    mAreaShmBuffer = new WaylandShmBuffer(this, mLogCategory);
    struct wl_buffer *wlbuf = mAreaShmBuffer->constructWlBuffer(width, height, format);
    if (wlbuf == NULL) {
        delete mAreaShmBuffer;
        mAreaShmBuffer = NULL;
    }

    wl_surface_attach(mAreaSurfaceWrapper, wlbuf, 0, 0);
}

std::size_t WaylandDisplay::calculateDmaBufferHash(RenderDmaBuffer &dmabuf)
{
    std::string hashString("");
    for (int i = 0; i < dmabuf.planeCnt; i++) {
        char hashtmp[1024];
        snprintf (hashtmp, 1024, "%d%d%d%d%d%d", dmabuf.width,dmabuf.height,dmabuf.planeCnt,
                dmabuf.stride[i],dmabuf.offset[i],dmabuf.fd[i]);
        std::string tmp(hashtmp);
        hashString += tmp;
    }

    std::size_t hashval = std::hash<std::string>()(hashString);
    //TRACE(mLogCategory,"hashstr:%s,val:%zu",hashString.c_str(),hashval);
    return hashval;
}

void WaylandDisplay::addWaylandBuffer(RenderBuffer * buf, WaylandBuffer *waylandbuf)
{
    if (buf->flag & BUFFER_FLAG_DMA_BUFFER) {
        std::size_t hashval = calculateDmaBufferHash(buf->dma);
        std::pair<std::size_t, WaylandBuffer *> item(hashval, waylandbuf);
        //TRACE(mLogCategory,"fd:%d,w:%d,h:%d,%p,hash:%zu",buf->dma.fd[0],buf->dma.width,buf->dma.height,waylandbuf,hashval);
        mWaylandBuffersMap.insert(item);
    }

    //clean invalid wayland buffer,if video resolution changed
    for (auto item = mWaylandBuffersMap.begin(); item != mWaylandBuffersMap.end(); ) {
        WaylandBuffer *waylandbuf = (WaylandBuffer*)item->second;
        if ((waylandbuf->getFrameWidth() != buf->dma.width ||
            waylandbuf->getFrameHeight() != buf->dma.height) &&
            waylandbuf->isFree()) {
            TRACE(mLogCategory,"delete wayland buffer,width:%d(%d),height:%d(%d)",
                waylandbuf->getFrameWidth(),buf->dma.width,
                waylandbuf->getFrameHeight(),buf->dma.height);
            mWaylandBuffersMap.erase(item++);
            delete waylandbuf;
        } else {
            item++;
        }
    }
    TRACE(mLogCategory,"mWaylandBuffersMap size:%d",mWaylandBuffersMap.size());
}

WaylandBuffer* WaylandDisplay::findWaylandBuffer(RenderBuffer * buf)
{
    std::size_t hashval = calculateDmaBufferHash(buf->dma);
    auto item = mWaylandBuffersMap.find(hashval);
    if (item == mWaylandBuffersMap.end()) {
        return NULL;
    }

    return (WaylandBuffer*) item->second;
}

void WaylandDisplay::cleanAllWaylandBuffer()
{
    //free all obtain buff
    for (auto item = mWaylandBuffersMap.begin(); item != mWaylandBuffersMap.end(); ) {
        WaylandBuffer *waylandbuf = (WaylandBuffer*)item->second;
        mWaylandBuffersMap.erase(item++);
        delete waylandbuf;
    }
}

void WaylandDisplay::flushBuffers()
{
    INFO(mLogCategory,"flushBuffers");
    Tls::Mutex::Autolock _l(mRenderMutex);
    for (auto item = mCommittedBufferMap.begin(); item != mCommittedBufferMap.end(); item++) {
        WaylandBuffer *waylandbuf = (WaylandBuffer*)item->second;
        waylandbuf->forceRedrawing();
        handleFrameDisplayedCallback(waylandbuf);
    }
}

void WaylandDisplay::cleanSurface()
{
    /* clear both video and parent surfaces */
    wl_surface_attach (mVideoSurfaceWrapper, NULL, 0, 0);
    wl_surface_commit (mVideoSurfaceWrapper);
    wl_surface_attach (mAreaSurfaceWrapper, NULL, 0, 0);
    wl_surface_commit (mAreaSurfaceWrapper);
}