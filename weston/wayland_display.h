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
#ifndef __WAYLAND_DISPLAY_H__
#define __WAYLAND_DISPLAY_H__
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>
#include <list>
#include <unordered_map>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "linux-explicit-synchronization-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "wayland-cursor.h"
#include "Thread.h"
#include "Poll.h"
#include "render_plugin.h"

using namespace std;

#define DEFAULT_DISPLAY_OUTPUT_NUM 2

class WaylandPlugin;
class WaylandShmBuffer;
class WaylandBuffer;

class WaylandDisplay : public Tls::Thread{
  public:
    WaylandDisplay(WaylandPlugin *plugin, int logCategory);
    virtual ~WaylandDisplay();
    /**
     * @brief connet client to compositor server
     * and acquire a display from compositor
     *
     * @return int 0 success,other fail
     */
    int  openDisplay();
    /**
     * @brief release display that acquired from compositor
     *
     */
    void closeDisplay();
    /**
     * @brief change RenderVideoFormat to wayland protocol dma buffer format
     * and get the matched dmabuffer modifiers
     *
     */
    int toDmaBufferFormat(RenderVideoFormat format, uint32_t *outDmaformat /*out param*/, uint64_t *outDmaformatModifiers /*out param*/);
    /**
     * @brief change RenderVideoFormat to wayland protocol shm buffer format
     *
     * @param format RenderVideoFormat
     * @param outformat wayland protocol shm buffer format
     * @return int 0 success,other fail
     */
    int toShmBufferFormat(RenderVideoFormat format, uint32_t *outformat);
    /**
     * @brief Set the Video Buffer Format object
     *
     * @param format RenderVideoFormat it is defined in render_lib.h
     */
    void setVideoBufferFormat(RenderVideoFormat format);
    RenderVideoFormat getVideoBufferFormat() {
        return mBufferFormat;
    };
    struct wl_display *getWlDisplay() {
        return mWlDisplay;
    };
    struct zwp_linux_dmabuf_v1 * getDmaBuf()
    {
        return mDmabuf;
    };
    struct wl_shm *getShm()
    {
        return mShm;
    };
    struct wl_output *getWlOutput()
    {
        return mOutput[mActiveOutput].wlOutput;
    };
    /**
     * @brief Set the Select Display Output index
     *
     * @param output selected display output index
     */
    void setDisplayOutput(int output);
    /**
     * @brief Get the Select Display Output index
     *
     * @return int the index of selected output
     */
    int getDisplayOutput();

    /**
     * @brief set pip video
     * @param pip if set to 1, the video data will display in pip video plane
     * otherwise video data will display in main video plane
    */
    void setPip(int pip);

    bool isSentPtsToWeston() {
        return mIsSendPtsToWeston;
    }

    void setRedrawingPending(bool val) {
        mRedrawingPending = val;
    };

    bool isRedrawingPending() {
        return mRedrawingPending;
    };

    void setRenderRectangle(int x, int y, int w, int h);
    void setFrameSize(int w, int h);
    void setWindowSize(int x, int y, int w, int h);
    int  prepareFrameBuffer(RenderBuffer * buf);
    void displayFrameBuffer(RenderBuffer * buf, int64_t realDisplayTime);
    void setOpaque();
    void flushBuffers();
    void ensureFullscreen(bool fullscreen);
    void handleBufferReleaseCallback(WaylandBuffer *buf);
    void handleFrameDisplayedCallback(WaylandBuffer *buf);
    void handleFrameDropedCallback(WaylandBuffer *buf);

    //thread func
    void readyToRun();
    virtual bool threadLoop();

    /**wayland callback functions**/
    static void dmabuf_modifiers(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
		 uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo);
    static void dmaBufferFormat (void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf, uint32_t format);
    static void registryHandleGlobal (void *data, struct wl_registry *registry,
            uint32_t id, const char *interface, uint32_t version);
    static void registryHandleGlobalRemove (void *data, struct wl_registry *registry, uint32_t name);
    static void shmFormat (void *data, struct wl_shm *wl_shm, uint32_t format);
    static void outputHandleGeometry( void *data,
                                  struct wl_output *output,
                                  int x,
                                  int y,
                                  int mmWidth,
                                  int mmHeight,
                                  int subPixel,
                                  const char *make,
                                  const char *model,
                                  int transform );
    static void outputHandleMode( void *data,
                              struct wl_output *output,
                              uint32_t flags,
                              int width,
                              int height,
                              int refreshRate );
    static void outputHandleDone( void *data,
                              struct wl_output *output );
    static void outputHandleScale( void *data,
                               struct wl_output *output,
                               int32_t scale );
    static void pointerHandleEnter(void *data, struct wl_pointer *pointer,
                                uint32_t serial, struct wl_surface *surface,
                                wl_fixed_t sx, wl_fixed_t sy);
    static void pointerHandleLeave(void *data, struct wl_pointer *pointer,
                                uint32_t serial, struct wl_surface *surface);
    static void pointerHandleMotion(void *data, struct wl_pointer *pointer,
                                uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
    static void pointerHandleButton(void *data, struct wl_pointer *wl_pointer,
                                uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    static void pointerHandleAxis(void *data, struct wl_pointer *wl_pointer,
                                    uint32_t time, uint32_t axis, wl_fixed_t value);
    static void touchHandleDown(void *data, struct wl_touch *wl_touch,
                                    uint32_t serial, uint32_t time, struct wl_surface *surface,
                                    int32_t id, wl_fixed_t x_w, wl_fixed_t y_w);
    static void touchHandleUp(void *data, struct wl_touch *wl_touch,
                                    uint32_t serial, uint32_t time, int32_t id);
    static void touchHandleMotion(void *data, struct wl_touch *wl_touch,
                                    uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w);
    static void touchHandleFrame(void *data, struct wl_touch *wl_touch);
    static void touchHandleCancel(void *data, struct wl_touch *wl_touch);
    static void keyboardHandleKeymap(void *data, struct wl_keyboard *keyboard,
                                    uint32_t format, int fd, uint32_t size);
    static void keyboardHandleEnter(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
    static void keyboardHandleLeave(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, struct wl_surface *surface);
    static void keyboardHandleKey(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    static void keyboardHandleModifiers(void *data, struct wl_keyboard *keyboard,
                                    uint32_t serial, uint32_t mods_depressed,
                                    uint32_t mods_latched, uint32_t mods_locked,
                                    uint32_t group);
    static void seatHandleCapabilities(void *data, struct wl_seat *seat,
                                    uint32_t caps);
    static void handleXdgToplevelClose (void *data, struct xdg_toplevel *xdg_toplevel);
    static void handleXdgToplevelConfigure (void *data, struct xdg_toplevel *xdg_toplevel,
                                    int32_t width, int32_t height, struct wl_array *states);
    static void handleXdgSurfaceConfigure (void *data, struct xdg_surface *xdg_surface, uint32_t serial);
  private:
    typedef struct DisplayOutput {
        struct wl_output *wlOutput;
        int offsetX;
        int offsetY;
        int width;
        int height;
        int refreshRate;
        bool isPrimary;
        uint32_t name;
    } DisplayOutput;
    struct Rectangle {
        int x;
        int y;
        int w;
        int h;
    };
    char *require_xdg_runtime_dir();
    void createCommonWindowSurface();
    void createXdgShellWindowSurface();
    void destroyWindowSurfaces();
    void resizeVideoSurface(bool commit);
    void videoCenterRect(Rectangle src, Rectangle dst, Rectangle *result, bool scaling);
    void updateBorders();
    std::size_t calculateDmaBufferHash(RenderDmaBuffer &dmabuf);
    void cleanSurface();
    void addWaylandBuffer(RenderBuffer * buf, WaylandBuffer *waylandbuf);
    WaylandBuffer* findWaylandBuffer(RenderBuffer * buf);
    void cleanAllWaylandBuffer();

    WaylandPlugin *mWaylandPlugin;
    struct wl_display *mWlDisplay;
    struct wl_display *mWlDisplayWrapper;
    struct wl_event_queue *mWlQueue;

    struct wl_registry *mRegistry;
    struct wl_compositor *mCompositor;
    struct wl_subcompositor *mSubCompositor;
    struct xdg_wm_base *mXdgWmBase;
    struct wp_viewporter *mViewporter;
    struct zwp_linux_dmabuf_v1 *mDmabuf;
    struct wl_shm *mShm;
    struct wl_seat *mSeat;
    struct wl_pointer *mPointer;
    struct wl_touch *mTouch;
    struct wl_keyboard *mKeyboard;

    /*primary output will signal first,so 0 index is primary wl_output, 1 index is extend wl_output*/
    DisplayOutput mOutput[DEFAULT_DISPLAY_OUTPUT_NUM]; //info about wl_output
    int mActiveOutput; //default is primary output

    int mLogCategory;

    std::list<uint32_t> mShmFormats;
    std::unordered_map<uint32_t, uint64_t> mDmaBufferFormats;
    RenderVideoFormat mBufferFormat;

    mutable Tls::Mutex mBufferMutex;
    mutable Tls::Mutex mMutex;
    int mFd;
    Tls::Poll *mPoll;

    /*the followed is windows variable*/
    mutable Tls::Mutex mRenderMutex;
    struct wl_surface *mAreaSurface;
    struct wl_surface *mAreaSurfaceWrapper;
    struct wl_surface *mVideoSurface;
    struct wl_surface *mVideoSurfaceWrapper;
    struct wl_subsurface *mVideoSubSurface;
    struct xdg_surface *mXdgSurface;
    struct xdg_toplevel *mXdgToplevel;
    struct wp_viewport *mAreaViewport;
    struct wp_viewport *mVideoViewport;
    WaylandShmBuffer *mAreaShmBuffer;
    bool mXdgSurfaceConfigured;
    Tls::Condition mConfigureCond;
    Tls::Mutex mConfigureMutex;
    bool mFullScreen; //default full screen

    bool mIsSendPtsToWeston;

    bool mReCommitAreaSurface;

    /* the size and position of the area_(sub)surface
    it is full screen size now*/
    struct Rectangle mRenderRect;

    /*the size and position of window */
    struct Rectangle mWindowRect;

    /* the size and position of the video_subsurface */
    struct Rectangle mVideoRect;
    /* the size of the video in the buffers */
    int mVideoWidth;
    int mVideoHeight;

    //the count display buffer of committed to weston
    int mCommitCnt;

    /*store waylandbuffers when set reusing waylandbuffer flag*/
    std::unordered_map<std::size_t, WaylandBuffer *> mWaylandBuffersMap;
    bool mNoBorderUpdate;

    /*store committed to weston waylandbuffer,key is pts*/
    std::unordered_map<int64_t, WaylandBuffer *> mCommittedBufferMap;

    int mPip; //pip video, 1->pip, 0: main video(default)
    bool mIsSendVideoPlaneId; //default true,otherwise false if set
    bool mRedrawingPending;//it will be true when weston obtains a buffer rendering,otherwise false when rendered
};

#endif /*__WAYLAND_DISPLAY_H__*/