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
#ifndef _WST_SOCKET_CLIENT_H_
#define _WST_SOCKET_CLIENT_H_

#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "Thread.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define WST_MAX_PLANES (3)
#define AV_SYNC_SESSION_V_MONO 64 //when set it, AV_SYNC_MODE_VIDEO_MONO of sync mode must be selected

#define SYNC_IMMEDIATE (255) //msync video sync type

#define INVALID_SESSION_ID (16)

enum av_sync_mode {
    AV_SYNC_MODE_VMASTER = 0,
    AV_SYNC_MODE_AMASTER = 1,
    AV_SYNC_MODE_PCR_MASTER = 2,
    AV_SYNC_MODE_IPTV = 3,
    AV_SYNC_MODE_FREE_RUN = 4,
    AV_SYNC_MODE_VIDEO_MONO = 5, /* video render by system mono time */
    AV_SYNC_MODE_MAX
};

typedef struct _WstPlaneInfo
{
    int fd;
    int stride;
    int offset;
    void *start;
    int capacity;
} WstPlaneInfo;

typedef struct _WstBufferInfo
{
    WstPlaneInfo planeInfo[WST_MAX_PLANES];
    int planeCount;
    int bufferId;
    uint32_t pixelFormat;

    int frameWidth;
    int frameHeight;
    int64_t frameTime;
} WstBufferInfo;

typedef struct _WstRect
{
   int x;
   int y;
   int w;
   int h;
} WstRect;

typedef enum _WstEventType
{
    WST_REFRESH_RATE = 0,
    WST_BUFFER_RELEASE,
    WST_STATUS,
    WST_UNDERFLOW,
    WST_ZOOM_MODE,
    WST_DEBUG_LEVEL,
} WstEventType;

typedef struct _WstEvent
{
    WstEventType event;
    int param;
    int param1;
    int param2;
    int64_t lparam;
} WstEvent;

#ifdef  __cplusplus
}
#endif

class WstClientPlugin;

class WstClientSocket : public Tls::Thread{
  public:
    WstClientSocket(WstClientPlugin *plugin);
    virtual ~WstClientSocket();
    bool connectToSocket(const char *name);
    bool disconnectFromSocket();
    /**
     * @brief send video plane resource id to westeros server
     *
     * @param resourceId if 0 value,westeros server will select main video plane
     *          if other value,westeros server will select an other video plane
     */
    void sendLayerVideoClientConnection(bool pip);
    void sendResourceVideoClientConnection(bool pip);
    void sendFlushVideoClientConnection();
    void sendPauseVideoClientConnection(bool pause);
    void sendHideVideoClientConnection(bool hide);
    void sendSessionInfoVideoClientConnection(int sessionId, int syncType );
    void sendFrameAdvanceVideoClientConnection();
    void sendRectVideoClientConnection(int videoX, int videoY, int videoWidth, int videoHeight );
    void sendRateVideoClientConnection(int fpsNum, int fpsDenom );
    bool sendFrameVideoClientConnection(WstBufferInfo *wstBufferInfo, WstRect *wstRect);
    void processMessagesVideoClientConnection();
    void sendKeepLastFrameVideoClientConnection(bool keep);
    void sendGetDefaultWindowSizeClientConnection();
    void sendCropFrameSizeClientConnection(int x, int y, int w, int h);
        //thread func
    void readyToRun();
    virtual bool threadLoop();
  private:
    const char *mName;
    struct sockaddr_un mAddr;
    int mSocketFd;
    int mServerRefreshRate;
    int64_t mServerRefreshPeriod;
    int mZoomMode;
    WstClientPlugin *mPlugin;
};

#endif /*_WST_SOCKET_CLIENT_H_*/
