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
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "drm_lib_wrap.h"
#include "Logger.h"
#include "ErrorCode.h"

#define TAG "rlib:drm_lib_wrap"

#define DRM_MESON_LIB_NAME "libdrm_meson.so"

DrmMesonLib * drmMesonLoadLib(int logCategory)
{
    INFO(logCategory, "load libdrm meson so symbol");

    DrmMesonLib *handle = (DrmMesonLib*)calloc( 1, sizeof(DrmMesonLib));
    if (!handle) {
        ERROR(logCategory, "calloc DrmMesonLib struct fail");
        goto err_labal;
    }

    handle->libHandle = dlopen(DRM_MESON_LIB_NAME, RTLD_NOW);
    if (handle->libHandle == NULL) {
        ERROR(logCategory, "unable to dlopen %s : %s",DRM_MESON_LIB_NAME, dlerror());
        goto err_labal;
    }

    handle->libDrmDisplayInit = (lib_drm_display_init)dlsym(handle->libHandle, "drm_display_init");
    if (handle->libDrmDisplayInit == NULL) {
        ERROR(logCategory,"dlsym drm_display_init failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmDisplayDestroy = (lib_drm_destroy_display)dlsym(handle->libHandle, "drm_destroy_display");
    if (handle->libDrmDisplayDestroy == NULL) {
        ERROR(logCategory,"dlsym drm_destroy_display failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmDisplayRegisterDonCb = (lib_drm_display_register_done_cb)dlsym(handle->libHandle, "drm_display_register_done_cb");
    if (handle->libDrmDisplayRegisterDonCb == NULL) {
        ERROR(logCategory, "dlsym drm_display_register_done_cb failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmDisplayRegisterResCb = (lib_drm_display_register_res_cb)dlsym(handle->libHandle, "drm_display_register_res_cb");
    if (handle->libDrmDisplayRegisterResCb == NULL) {
        ERROR(logCategory,"dlsym drm_display_register_res_cb failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmSetAllocOnlyFlag = (lib_drm_set_alloc_only_flag)dlsym(handle->libHandle, "drm_set_alloc_only_flag");
    if (handle->libDrmSetAllocOnlyFlag == NULL) {
        ERROR(logCategory,"dlsym drm_set_alloc_only_flag failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmAllocBufs = (lib_drm_alloc_bufs)dlsym(handle->libHandle, "drm_alloc_bufs");
    if (handle->libDrmAllocBufs == NULL) {
        ERROR(logCategory,"dlsym drm_alloc_bufs failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmFreeBufs = (lib_drm_free_bufs)dlsym(handle->libHandle, "drm_free_bufs");
    if (handle->libDrmFreeBufs == NULL) {
        ERROR(logCategory,"dlsym drm_free_bufs failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmAllocBuf = (lib_drm_alloc_buf)dlsym(handle->libHandle, "drm_alloc_buf");
    if (handle->libDrmAllocBuf == NULL) {
        ERROR(logCategory,"dlsym drm_alloc_buf failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmImportBuf = (lib_drm_import_buf)dlsym(handle->libHandle, "drm_import_buf");
    if (handle->libDrmImportBuf == NULL) {
        ERROR(logCategory,"dlsym drm_import_buf failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmFreeBuf = (lib_drm_free_buf)dlsym(handle->libHandle, "drm_free_buf");
    if (handle->libDrmFreeBuf == NULL) {
        ERROR(logCategory,"dlsym drm_free_buf failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmPostBuf = (lib_drm_post_buf)dlsym(handle->libHandle, "drm_post_buf");
    if (handle->libDrmPostBuf == NULL) {
        ERROR(logCategory,"dlsym drm_post_buf failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmModeAsyncAtomicCommit = (lib_drmModeAsyncAtomicCommit)dlsym(handle->libHandle, "drmModeAsyncAtomicCommit");
    if (handle->libDrmModeAsyncAtomicCommit == NULL) {
        ERROR(logCategory,"dlsym drmModeAsyncAtomicCommit failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmWaitVideoFence = (lib_drm_waitvideoFence)dlsym(handle->libHandle, "drm_waitvideoFence");
    if (handle->libDrmWaitVideoFence == NULL) {
        ERROR(logCategory,"dlsym drm_waitvideoFence failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmGetModeInfo = (lib_drm_getModeInfo)dlsym(handle->libHandle, "meson_drm_getModeInfo");
    if (handle->libDrmGetModeInfo == NULL) {
        ERROR(logCategory,"dlsym meson_drm_getModeInfo failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->libDrmMutePlane = (lib_drm_setPlaneMute)dlsym(handle->libHandle, "meson_drm_setPlaneMute");
    if (handle->libDrmMutePlane == NULL) {
        ERROR(logCategory,"dlsym meson_drm_setPlaneMute failed, err=%s \n", dlerror());
        goto err_labal;
    }

    return handle;
err_labal:
    if (handle) {
        if (handle->libHandle) {
            dlclose(handle->libHandle);
        }
        free(handle);
    }
    return NULL;
}

int drmMesonUnloadLib(int logCategory,DrmMesonLib *drmMesonLib)
{
    if (drmMesonLib) {
        if (drmMesonLib->libHandle) {
            INFO(logCategory, "dlclose libdrm meson so symbol");
            dlclose(drmMesonLib->libHandle);
        }
        free(drmMesonLib);
    }
    return NO_ERROR;
}
