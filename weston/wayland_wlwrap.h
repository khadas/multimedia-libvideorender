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
#ifndef __WAYLAND_WLWRAP_H__
#define __WAYLAND_WLWRAP_H__

class WaylandWLWrap {
  public:
    WaylandWLWrap() {}
    virtual ~WaylandWLWrap() {};
    virtual struct wl_buffer *getWlBuffer() = 0;
    virtual void *getDataPtr() = 0;
    virtual int getSize() = 0;
};

#endif /*__WAYLAND_WLWRAP_H__*/