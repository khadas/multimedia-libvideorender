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
#ifndef __DRM_LIB_WRAP_H__
#define __DRM_LIB_WRAP_H__

extern "C" {
#include "meson_drm_util.h"
#include "meson_drm_settings.h"
}

typedef struct drm_display *(*lib_drm_display_init)(void);
typedef void (*lib_drm_destroy_display)(struct drm_display *disp);
typedef void (*lib_drm_display_register_done_cb)(struct drm_display *disp, void *func, void *priv);
typedef void (*lib_drm_display_register_res_cb)(struct drm_display *disp, void *func, void *priv);

typedef int (*lib_drm_set_alloc_only_flag)(struct drm_display *disp, int flag);

typedef int (*lib_drm_alloc_bufs)(struct drm_display *disp, int num, struct drm_buf_metadata *info);
typedef int (*lib_drm_free_bufs)(struct drm_display *disp);

typedef struct drm_buf *(*lib_drm_alloc_buf)(struct drm_display *disp, struct drm_buf_metadata *info);
typedef struct drm_buf *(*lib_drm_import_buf)(struct drm_display *disp, struct drm_buf_import *info);
typedef int (*lib_drm_free_buf)(struct drm_buf *buf);
typedef int (*lib_drm_post_buf)(struct drm_display *disp, struct drm_buf *buf);

typedef int (*lib_drmModeAsyncAtomicCommit)(int fd, drmModeAtomicReqPtr req,
                                   uint32_t flags, void *user_data);

typedef int (*lib_drm_waitvideoFence)( int dmabuffd);
typedef int (*lib_drm_getModeInfo)(int drmFd, MESON_CONNECTOR_TYPE connType, DisplayMode* modeInfo);
typedef int (*lib_drm_setPlaneMute)(int drmFd, unsigned int plane_type, unsigned int plane_mute);

typedef struct {
    void *libHandle; //lib handle of dlopen
    lib_drm_display_init libDrmDisplayInit;
    lib_drm_destroy_display libDrmDisplayDestroy;
    lib_drm_display_register_done_cb libDrmDisplayRegisterDonCb;
    lib_drm_display_register_res_cb libDrmDisplayRegisterResCb;
    lib_drm_set_alloc_only_flag libDrmSetAllocOnlyFlag;
    lib_drm_alloc_bufs libDrmAllocBufs;
    lib_drm_free_bufs libDrmFreeBufs;
    lib_drm_alloc_buf libDrmAllocBuf;
    lib_drm_import_buf libDrmImportBuf;
    lib_drm_free_buf libDrmFreeBuf;
    lib_drm_post_buf libDrmPostBuf;
    lib_drmModeAsyncAtomicCommit libDrmModeAsyncAtomicCommit;
    lib_drm_waitvideoFence libDrmWaitVideoFence;
    lib_drm_getModeInfo libDrmGetModeInfo;
    lib_drm_setPlaneMute libDrmMutePlane;
} DrmMesonLib;


DrmMesonLib * drmMesonLoadLib(int logCategory);
int drmMesonUnloadLib(int logCategory,DrmMesonLib *drmMesonLib);

#endif /*__DRM_LIB_WRAP_H__*/