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
#ifndef __WAYLAND_VIDEO_FORMAT_H__
#define __WAYLAND_VIDEO_FORMAT_H__
#include <string.h>
#include <wayland-client-protocol.h>
#include "render_plugin.h"

#ifdef  __cplusplus
extern "C" {
#endif

uint32_t video_format_to_wl_dmabuf_format (RenderVideoFormat format);
RenderVideoFormat wl_dmabuf_format_to_video_format (uint32_t wl_format);

int32_t video_format_to_wl_shm_format (RenderVideoFormat format);
RenderVideoFormat wl_shm_format_to_video_format (enum wl_shm_format wl_format);

const char* print_dmabuf_format_name(uint32_t dmabufferformat);
const char* print_render_video_format_name(RenderVideoFormat format);

#ifdef  __cplusplus
}
#endif
#endif