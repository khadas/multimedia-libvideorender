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
#ifndef __VIDEO_TUNNEL_LIB_WRAP_H__
#define __VIDEO_TUNNEL_LIB_WRAP_H__
#include "video_tunnel.h"

typedef int (*vt_open)();
typedef int (*vt_close)(int fd);
typedef int (*vt_alloc_id)(int fd, int *tunnel_id);
typedef int (*vt_free_id)(int fd, int tunnel_id);
typedef int (*vt_connect)(int fd, int tunnel_id, int role);
typedef int (*vt_disconnect)(int fd, int tunnel_id, int role);

/* for producer */
typedef int (*vt_queue_buffer)(int fd, int tunnel_id, int buffer_fd, int fence_fd, int64_t expected_present_time);
typedef int (*vt_dequeue_buffer)(int fd, int tunnel_id, int *buffer_fd, int *fence_fd);
typedef int (*vt_cancel_buffer)(int fd, int tunnel_id);
typedef int (*vt_set_sourceCrop)(int fd, int tunnel_id, struct vt_rect rect);
typedef int (*vt_getDisplayVsyncAndPeriod)(int fd, int tunnel_id, uint64_t *timestamp, uint32_t *period);

/* for video cmd */
typedef int (*vt_set_mode)(int fd, int block_mode);
typedef int (*vt_send_cmd)(int fd, int tunnel_id, enum vt_cmd cmd, int cmd_data);
typedef int (*vt_recv_cmd)(int fd, int tunnel_id, enum vt_cmd *cmd, struct vt_cmd_data *cmd_data);

typedef struct {
    void *libHandle; //videotunnel lib handle of dlopen
    vt_open vtOpen;
    vt_close vtClose;
    vt_alloc_id vtAllocId;
    vt_free_id vtFreeId;
    vt_connect vtConnect;
    vt_disconnect vtDisconnect;
    vt_queue_buffer vtQueueBuffer;
    vt_dequeue_buffer vtDequeueBuffer;
    vt_cancel_buffer vtCancelBuffer;
    vt_set_sourceCrop vtSetSourceCrop;
    vt_getDisplayVsyncAndPeriod vtGetDisplayVsyncAndPeriod;
    vt_set_mode vtSetMode;
    vt_send_cmd vtSendCmd;
    vt_recv_cmd vtRecvCmd;
} VideotunnelLib;


VideotunnelLib * videotunnelLoadLib(int logCategory);
int videotunnelUnloadLib(int logCategory,VideotunnelLib *vt);

#endif /*__VIDEO_TUNNEL_LIB_WRAP_H__*/