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
#ifndef __RENDER_COMMON_H__
#define __RENDER_COMMON_H__
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define RENDER_MAX_PLANES 3

/*allocate render buffer flag */
typedef enum _BufferFlag {
    BUFFER_FLAG_NONE       = 0,
    BUFFER_FLAG_DMA_BUFFER = 1 << 1,
    BUFFER_FLAG_RAW_BUFFER = 1 << 2,
} RenderBufferFlag;

typedef struct _RenderRawBuffer {
    void *dataPtr; //video render buffer pointer
    int size; //video render buffer size
} RenderRawBuffer;

typedef struct _RenderDmaBuffer {
    int width;
    int height;
    int planeCnt;
    uint32_t handle[RENDER_MAX_PLANES];
    uint32_t stride[RENDER_MAX_PLANES];
    uint32_t offset[RENDER_MAX_PLANES];
    uint32_t size[RENDER_MAX_PLANES];
    int fd[RENDER_MAX_PLANES];
} RenderDmaBuffer;

typedef struct _RenderBuffer {
    int id; //buffer id,set by video render
    int flag; /*render buffer flag, refer to RenderBufferFlag*/
    RenderDmaBuffer dma;
    RenderRawBuffer raw;
    int64_t pts; //time is nano second
    int64_t time; //frame display time
    void *priv; //user data passed to render lib
    int reserved[4]; //reserved for extend
} RenderBuffer;

/*video render rectangle*/
typedef struct _RenderRect{
    int x;
    int y;
    int w;
    int h;
} RenderRect;

/*video frame size*/
typedef struct _RenderFrameSize {
    int width;
    int height;
} RenderFrameSize;

typedef struct {
   int num; //numerator
   int denom; //denominator
} RenderFraction;

typedef enum _RenderMsgType {
    //notify a msg
    MSG_NOTIFY                    = 0,
    //frame buffer is released
    MSG_RELEASE_BUFFER            = 100, //the msg value type is RenderBuffer
    //frame buffer is displayed
    MSG_DISPLAYED_BUFFER          = 101, //the msg value type is RenderBuffer
    //the frame buffer is droped
    MSG_DROPED_BUFFER             = 102,//the msg value type is RenderBuffer
    //first frame displayed msg
    MSG_FIRST_FRAME               = 103, //the msg value type is frame pts
    //under flow msg
    MSG_UNDER_FLOW                = 104, //the msg value type is null
    //pause with special pts
    MSG_PAUSED_PTS                = 105, //the msg value type is RenderBuffer
} RenderMsgType;

/**video format*/
typedef enum {
    VIDEO_FORMAT_UNKNOWN,
    VIDEO_FORMAT_ENCODED,
    VIDEO_FORMAT_I420,
    VIDEO_FORMAT_YV12,
    VIDEO_FORMAT_YUY2,
    VIDEO_FORMAT_UYVY,
    VIDEO_FORMAT_AYUV,
    VIDEO_FORMAT_RGBx,
    VIDEO_FORMAT_BGRx,
    VIDEO_FORMAT_xRGB,
    VIDEO_FORMAT_xBGR,
    VIDEO_FORMAT_RGBA,
    VIDEO_FORMAT_BGRA,
    VIDEO_FORMAT_ARGB,
    VIDEO_FORMAT_ABGR,
    VIDEO_FORMAT_RGB,
    VIDEO_FORMAT_BGR,
    VIDEO_FORMAT_Y41B,
    VIDEO_FORMAT_Y42B,
    VIDEO_FORMAT_YVYU,
    VIDEO_FORMAT_Y444,
    VIDEO_FORMAT_v210,
    VIDEO_FORMAT_v216,
    VIDEO_FORMAT_NV12,
    VIDEO_FORMAT_NV21,
    VIDEO_FORMAT_GRAY8,
    VIDEO_FORMAT_GRAY16_BE,
    VIDEO_FORMAT_GRAY16_LE,
    VIDEO_FORMAT_v308,
    VIDEO_FORMAT_RGB16,
    VIDEO_FORMAT_BGR16,
    VIDEO_FORMAT_RGB15,
    VIDEO_FORMAT_BGR15,
    VIDEO_FORMAT_UYVP,
    VIDEO_FORMAT_A420,
    VIDEO_FORMAT_RGB8P,
    VIDEO_FORMAT_YUV9,
    VIDEO_FORMAT_YVU9,
    VIDEO_FORMAT_IYU1,
    VIDEO_FORMAT_ARGB64,
    VIDEO_FORMAT_AYUV64,
    VIDEO_FORMAT_r210,
    VIDEO_FORMAT_I420_10BE,
    VIDEO_FORMAT_I420_10LE,
    VIDEO_FORMAT_I422_10BE,
    VIDEO_FORMAT_I422_10LE,
    VIDEO_FORMAT_Y444_10BE,
    VIDEO_FORMAT_Y444_10LE,
    VIDEO_FORMAT_GBR,
    VIDEO_FORMAT_GBR_10BE,
    VIDEO_FORMAT_GBR_10LE,
    VIDEO_FORMAT_NV16,
    VIDEO_FORMAT_NV24,
    VIDEO_FORMAT_NV12_64Z32,
    VIDEO_FORMAT_A420_10BE,
    VIDEO_FORMAT_A420_10LE,
    VIDEO_FORMAT_A422_10BE,
    VIDEO_FORMAT_A422_10LE,
    VIDEO_FORMAT_A444_10BE,
    VIDEO_FORMAT_A444_10LE,
    VIDEO_FORMAT_NV61,
    VIDEO_FORMAT_P010_10BE,
    VIDEO_FORMAT_P010_10LE,
    VIDEO_FORMAT_IYU2,
    VIDEO_FORMAT_VYUY,
    VIDEO_FORMAT_GBRA,
    VIDEO_FORMAT_GBRA_10BE,
    VIDEO_FORMAT_GBRA_10LE,
    VIDEO_FORMAT_GBR_12BE,
    VIDEO_FORMAT_GBR_12LE,
    VIDEO_FORMAT_GBRA_12BE,
    VIDEO_FORMAT_GBRA_12LE,
    VIDEO_FORMAT_I420_12BE,
    VIDEO_FORMAT_I420_12LE,
    VIDEO_FORMAT_I422_12BE,
    VIDEO_FORMAT_I422_12LE,
    VIDEO_FORMAT_Y444_12BE,
    VIDEO_FORMAT_Y444_12LE,
    VIDEO_FORMAT_GRAY10_LE32,
    VIDEO_FORMAT_NV12_10LE32,
    VIDEO_FORMAT_NV16_10LE32,
    VIDEO_FORMAT_NV12_10LE40,
    VIDEO_FORMAT_Y210,
    VIDEO_FORMAT_Y410,
    VIDEO_FORMAT_VUYA,
    VIDEO_FORMAT_BGR10A2_LE,
} RenderVideoFormat;

#ifdef  __cplusplus
}
#endif
#endif /*__RENDER_COMMON_H__*/