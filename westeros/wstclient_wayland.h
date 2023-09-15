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
#ifndef __WST_CLIENT_WAYLAND_H__
#define __WST_CLIENT_WAYLAND_H__
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>
#include <list>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "linux-explicit-synchronization-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "simpleshell-client-protocol.h"
#include "vpc-client-protocol.h"
#include "simplebuffer-client-protocol.h"
#include "Thread.h"
#include "Poll.h"
#include "render_common.h"
#include "wstclient_socket.h"

using namespace std;

class WstClientPlugin;

class WstClientWayland : public Tls::Thread{
  public:
    WstClientWayland(WstClientPlugin *plugin, int logCategory);
    virtual ~WstClientWayland();
    /**
     * @brief connet client to compositor server
     * and acquire a display from compositor
     *
     * @return int 0 success,other fail
     */
    int connectToWayland();
    /**
     * @brief release display that acquired from compositor
     *
     */
    void disconnectFromWayland();

    //thread func
    void readyToRun();
    virtual bool threadLoop();

    void getVideoBounds(int *x, int *y, int *w, int *h);

    void setTextureCrop(int vx, int vy, int vw, int vh);

    void setForceAspectRatio(bool force);

    void setPixelAspectRatio(double ratio);

    struct wl_display *getWlDisplay() {
        return mWlDisplay;
    };
    struct wl_compositor * getWlCompositor()
    {
        return mWlCompositor;
    };
    struct wl_event_queue *getWlEventQueue() {
        return mWlQueue;
    };

    void setWindowSize(int x, int y, int w, int h);

    void getWindowSize(int *x, int *y, int *w, int *h) {
        *x = mWindowX;
        *y = mWindowY;
        *w = mWindowWidth;
        *h = mWindowHeight;
    };
    void setFrameSize(int frameWidth, int frameHeight);

    void getFrameSize(int *w, int *h) {
        *w = mFrameWidth;
        *h = mFrameHeight;
    };

    void setZoomMode(int zoomMode, bool globalZoomActive, bool allow4kZoom);

    /**callback functions**/
    static void shellSurfaceId(void *data,
                           struct wl_simple_shell *wl_simple_shell,
                           struct wl_surface *surface,
                           uint32_t surfaceId);
    static void shellSurfaceCreated(void *data,
                                struct wl_simple_shell *wl_simple_shell,
                                uint32_t surfaceId,
                                const char *name);
    static void shellSurfaceDestroyed(void *data,
                                  struct wl_simple_shell *wl_simple_shell,
                                  uint32_t surfaceId,
                                  const char *name);
    static void shellSurfaceStatus(void *data,
                               struct wl_simple_shell *wl_simple_shell,
                               uint32_t surfaceId,
                               const char *name,
                               uint32_t visible,
                               int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height,
                               wl_fixed_t opacity,
                               wl_fixed_t zorder);
    static void shellGetSurfacesDone(void *data, struct wl_simple_shell *wl_simple_shell);
    static void vpcVideoPathChange(void *data,
                               struct wl_vpc_surface *wl_vpc_surface,
                               uint32_t new_pathway );
    static void vpcVideoXformChange(void *data,
                                struct wl_vpc_surface *wl_vpc_surface,
                                int32_t x_translation,
                                int32_t y_translation,
                                uint32_t x_scale_num,
                                uint32_t x_scale_denom,
                                uint32_t y_scale_num,
                                uint32_t y_scale_denom,
                                uint32_t output_width,
                                uint32_t output_height);
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
    static void sbFormat(void *data, struct wl_sb *wl_sb, uint32_t format);
    static void registryHandleGlobal (void *data, struct wl_registry *registry,
            uint32_t id, const char *interface, uint32_t version);
    static void registryHandleGlobalRemove (void *data, struct wl_registry *registry, uint32_t name);
  private:
    void updateVideoPosition();
    void setVideoPath(bool useGfxPath);
    bool approxEqual( double v1, double v2);
    void executeCmd(const char *cmd);
    WstClientPlugin *mPlugin;
    struct wl_display *mWlDisplay;
    struct wl_event_queue *mWlQueue;
    struct wl_surface *mWlSurface;
    struct wl_vpc *mWlVpc;
    struct wl_vpc_surface *mWlVpcSurface;
    struct wl_output *mWlOutput;
    struct wl_simple_shell *mWlShell;
    uint32_t mWlShellSurfaceId;
    struct wl_sb *mWlSb;
    struct wl_registry *mWlRegistry;
    struct wl_compositor *mWlCompositor;

    int mLogCategory;

    int mTransX;
    int mTransY;
    int mScaleXNum;
    int mScaleXDenom;
    int mScaleYNum;
    int mScaleYDenom;
    int mOutputWidth;
    int mOutputHeight;

    int mDisplayWidth; //the terminal display width
    int mDisplayHeight;  //the terminal display width
    int mWindowX;
    int mWindowY;
    int mWindowWidth; //the window width set
    int mWindowHeight; //the window height set

    int mVideoX;
    int mVideoY;
    int mVideoWidth;
    int mVideoHeight;

    bool mVideoPaused;

    bool mWindowSet; //the window size set flag
    bool mWindowSizeOverride; //the window size override
    bool mWindowChange;

    int mFrameWidth;
    int mFrameHeight;

    bool mForceAspectRatio;
    double mPixelAspectRatio;
    bool mPixelAspectRatioChanged;
    float mOpacity;
    float mZorder;

    int mZoomMode;
    bool mZoomModeGlobal;
    bool mAllow4kZoom;

    mutable Tls::Mutex mMutex;
    int mFd;
    Tls::Poll *mPoll;

    bool mForceFullScreen;
};

#endif /*__WST_CLIENT_WAYLAND_H__*/