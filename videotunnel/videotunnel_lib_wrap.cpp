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
#include "videotunnel_lib_wrap.h"
#include "Logger.h"

#define TAG "rlib:videotunnel_lib_wrap"

#define VIDEOTUNNEL_LIB_NAME "libvideotunnel.so"

VideotunnelLib * videotunnelLoadLib()
{
    INFO("load videotunel so symbol");

    VideotunnelLib *handle = (VideotunnelLib*)calloc( 1, sizeof(VideotunnelLib));
    if (!handle) {
        ERROR("calloc VideotunnelLib struct fail");
        goto err_labal;
    }
    handle->libHandle = dlopen(VIDEOTUNNEL_LIB_NAME, RTLD_NOW);
    if (handle->libHandle == NULL) {
        ERROR("unable to dlopen %s : %s",VIDEOTUNNEL_LIB_NAME, dlerror());
        goto err_labal;
    }

    handle->vtOpen = (vt_open)dlsym(handle->libHandle, "meson_vt_open");
    if (handle->vtOpen == NULL) {
        ERROR("dlsym meson_vt_open failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtClose = (vt_close)dlsym(handle->libHandle, "meson_vt_close");
    if (handle->vtClose == NULL) {
        ERROR("dlsym meson_vt_close failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtAllocId = (vt_alloc_id)dlsym(handle->libHandle, "meson_vt_alloc_id");
    if (handle->vtAllocId == NULL) {
        ERROR("dlsym meson_vt_alloc_id failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtFreeId = (vt_free_id)dlsym(handle->libHandle, "meson_vt_free_id");
    if (handle->vtFreeId == NULL) {
        ERROR("dlsym meson_vt_free_id failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtConnect = (vt_connect)dlsym(handle->libHandle, "meson_vt_connect");
    if (handle->vtConnect == NULL) {
        ERROR("dlsym meson_vt_connect failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtDisconnect = (vt_disconnect)dlsym(handle->libHandle, "meson_vt_disconnect");
    if (handle->vtDisconnect == NULL) {
        ERROR("dlsym meson_vt_disconnect failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtQueueBuffer = (vt_queue_buffer)dlsym(handle->libHandle, "meson_vt_queue_buffer");
    if (handle->vtQueueBuffer == NULL) {
        ERROR("dlsym meson_vt_queue_buffer failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtDequeueBuffer = (vt_dequeue_buffer)dlsym(handle->libHandle, "meson_vt_dequeue_buffer");
    if (handle->vtDequeueBuffer == NULL) {
        ERROR("dlsym meson_vt_dequeue_buffer failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtCancelBuffer = (vt_cancel_buffer)dlsym(handle->libHandle, "meson_vt_cancel_buffer");
    if (handle->vtCancelBuffer == NULL) {
        ERROR("dlsym meson_vt_cancel_buffer failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtSetSourceCrop = (vt_set_sourceCrop)dlsym(handle->libHandle, "meson_vt_set_sourceCrop");
    if (handle->vtSetSourceCrop == NULL) {
        ERROR("dlsym meson_vt_set_sourceCrop failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtGetDisplayVsyncAndPeriod = (vt_getDisplayVsyncAndPeriod)dlsym(handle->libHandle, "meson_vt_getDisplayVsyncAndPeriod");
    if (handle->vtGetDisplayVsyncAndPeriod == NULL) {
        ERROR("dlsym meson_vt_getDisplayVsyncAndPeriod failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtSetMode = (vt_set_mode)dlsym(handle->libHandle, "meson_vt_set_mode");
    if (handle->vtSetMode == NULL) {
        ERROR("dlsym meson_vt_set_mode failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtSendCmd = (vt_send_cmd)dlsym(handle->libHandle, "meson_vt_send_cmd");
    if (handle->vtSendCmd == NULL) {
        ERROR("dlsym meson_vt_send_cmd failed, err=%s \n", dlerror());
        goto err_labal;
    }

    handle->vtRecvCmd = (vt_recv_cmd)dlsym(handle->libHandle, "meson_vt_recv_cmd");
    if (handle->vtRecvCmd == NULL) {
        ERROR("dlsym meson_vt_recv_cmd failed, err=%s \n", dlerror());
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
int videotunnelUnloadLib(VideotunnelLib *vt)
{
    INFO("unload videotunel so symbol");
    if (vt) {
        if (vt->libHandle) {
            dlclose(vt->libHandle);
        }
        free(vt);
    }
    return 0;
}