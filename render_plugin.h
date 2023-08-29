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
#ifndef __RENDER_PLUGIN_H__
#define __RENDER_PLUGIN_H__
#include "render_common.h"

typedef void (*pluginMsgCallback)(void *handle, int msg, void *detail);
typedef void (*pluginBufferReleaseCallback)(void *handle,void *data);
typedef void (*pluginBufferDisplayedCallback)(void *handle,void *data);
typedef void (*pluginBufferDropedCallback)(void *handle,void *data);

typedef struct _PluginCallback {
    pluginMsgCallback doMsgCallback;
    pluginBufferReleaseCallback doBufferReleaseCallback;
    pluginBufferDisplayedCallback doBufferDisplayedCallback;
    pluginBufferDropedCallback doBufferDropedCallback;
} PluginCallback;

/**
 * @brief plugin key type
 * it is used by set/get function
 *
 */
typedef enum _PluginKey {
    PLUGIN_KEY_WINDOW_SIZE, //value type is PluginRect point
    PLUGIN_KEY_FRAME_SIZE, //value type is PluginFrameSize point
    PLUGIN_KEY_VIDEO_FORMAT, //value type is uint32_t,detail see RenderVideoFormat that is in render_lib.h
    PLUGIN_KEY_VIDEO_PIP, //is pip window, int type of value
    PLUGIN_KEY_VIDEOTUNNEL_ID,//set/get videotunnel instance id when videotunnel plugin is selected
    PLUGIN_KEY_KEEP_LAST_FRAME, //set/get keep last frame when play end ,value type is int, 0 not keep, 1 keep
    PLUGIN_KEY_HIDE_VIDEO, //set/get hide video,it effect immediatialy,value type is int, 0 not hide, 1 hide
    PLUGIN_KEY_FORCE_ASPECT_RATIO, //set/get force pixel aspect ratio,value type is int, 1 is force,0 is not force
    PLUGIN_KEY_SELECT_DISPLAY_OUTPUT,//set/get selected displayt output,value type is int,0 is primary output, 1 is extend output
    PLUGIN_KEY_IMMEDIATELY_OUTPUT, //set/get immediately output video frame to display, 0 is default value off, 1 is on
    PLUGIN_KEY_CROP_FRAME_SIZE, //set/get crop frame size,value type is PluginRect
    PLUGIN_KEY_PIXEL_ASPECT_RATIO, //set/get pixel aspect ratio,valut type is PixelAspectRatio
} PluginKey;

/**
 * render plugin interface
 * api sequence:
 * 1.new RenderPlugin
 * 2.plugin->init
 * 3.plugin->setuserData
 * 4.plugin->openDisplay
 * 5.plugin->openWindow
 * 6.plugin->set
 * 7.plugin->displayFrame
 * ......
 * after running ,stop plugin
 * 8.plugin->closeWindow
 * 9.plugin->closeDisplay
 * 10.plugin->release
 * 11.delete RenderPlugin
 *
 */
class RenderPlugin {
  public:
    RenderPlugin() {};
    virtual ~RenderPlugin() {};
    /**
     * @brief init render plugin
     *
     */
    virtual void init() = 0;
    /**
     * @brief release render plugin
     *
     */
    virtual void release() = 0;
    /**
     * @brief register callback to plugin
     *
     * @param userData user data
     * @param callback user callback function
     */
	virtual void setCallback(void *userData, PluginCallback *callback) = 0;
    /**
     * @brief open display from compositor
     *
     * @return 0 success,other value if failure
     */
    virtual int openDisplay() = 0;
    /**
     * @brief open window from compositor
     *
     * @return 0 success,other value if failure
     */
    virtual int openWindow() = 0;
    /**
     * @brief prepare RenderBuffer before displaying frame
     *
     * @param buffer video frame buffer
     * @return 0 success,other value if failure
     */
    virtual int prepareFrame(RenderBuffer *buffer) = 0;
    /**
     * @brief sending a video frame to compositor
     *
     * @param buffer video frame buffer
     * @param displayTime the frame render realtime
     * @return 0 success,other value if failure
     */
    virtual int displayFrame(RenderBuffer *buffer, int64_t displayTime) = 0;
    /**
     * @brief flush buffers those obtained by plugin
     *
     * @return 0 success,other value if failure
     */
    virtual int flush() = 0;
    /**
     * @brief pause plugin
     *
     * @return 0 success,other value if failure
     */
    virtual int pause() = 0;
    /**
     * @brief resume plugin
     *
     * @return 0 success,other value if failure
     */
    virtual int resume() = 0;
    /**
     * @brief close display
     *
     * @return 0 success,other value if failure
     */
    virtual int closeDisplay() = 0;
    /**
     * @brief close window
     *
     * @return 0 success,other value if failure
     */
    virtual int closeWindow() = 0;
    /**
     * @brief get property from plugin
     * the value must map the key type,the detail please see
     * enum _PluginKey
     *
     * @param key property key
     * @param value property value
     * @return 0 success,other value if failure
     */
    virtual int getValue(PluginKey key, void *value) = 0;
    /**
     * @brief set property to plugin
     * the value must map the key type,refer to PluginKey
     *
     * @param key property key
     * @param value property value
     * @return 0 success,other value if failure
     */
    virtual int setValue(PluginKey key, void *value) = 0;
};
#ifdef  __cplusplus
extern "C" {
#endif

/**
 * make a render lib plugin instance
 * @param id instance id
 * @return the pointer to render lib plugin instance
 */
void *makePluginInstance(int id);

/**
 * destroy render lib plugin instance that created by makePluginInstance
 * @param the handle point of video render plugin instance
 */
void destroyPluginInstance(void *);

#ifdef  __cplusplus
}
#endif

#endif /*__RENDER_PLUGIN_H__*/