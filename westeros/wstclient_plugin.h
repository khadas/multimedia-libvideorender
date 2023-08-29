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
#ifndef __WST_CLIENT_PLUGIN_H__
#define __WST_CLIENT_PLUGIN_H__
#include <unordered_map>
#include "render_plugin.h"
#include "wstclient_wayland.h"
#include "wstclient_socket.h"
#include "wst_essos.h"
#include <mutex>

class WstClientPlugin : public RenderPlugin
{
  public:
    WstClientPlugin();
    virtual ~WstClientPlugin();
    virtual void init();
    virtual void release();
    void setCallback(void *userData, PluginCallback *callback);
    virtual int openDisplay();
    virtual int openWindow();
    virtual int prepareFrame(RenderBuffer *buffer);
    virtual int displayFrame(RenderBuffer *buffer, int64_t displayTime);
    virtual int flush();
    virtual int pause();
    virtual int resume();
    virtual int closeDisplay();
    virtual int closeWindow();
    virtual int getValue(PluginKey key, void *value);
    virtual int setValue(PluginKey key, void *value);
    //buffer release callback
    void handleBufferRelease(RenderBuffer *buffer);
    //buffer had displayed ,but not release
    void handleFrameDisplayed(RenderBuffer *buffer);
    //buffer droped callback
    void handleFrameDropped(RenderBuffer *buffer);
    //plugin msg callback
    void handleMsgNotify(int type, void *detail);

    void onWstSocketEvent(WstEvent *event);

    void setVideoRect(int videoX, int videoY, int videoWidth, int videoHeight);
  private:
    typedef struct {
        bool isSet;
        bool value;
    } ConfigValue;
    /**
     * @brief Get the Display Frame Buffer Id object
     *
     * @param displayTime
     * @return int > 0 if success, -1 if not found
     */
    int getDisplayFrameBufferId(int64_t displayTime);
    /**
     * @brief send crop frame rect to server if needed
     * @return true send ok, false if failed
    */
    bool setCropFrameRect();
    PluginCallback *mCallback;
    WstClientWayland *mWayland;
    WstClientSocket *mWstClientSocket;
    RenderRect mWinRect;
    bool mSetCropFrameRect;
    RenderRect mCropFrameRect; //set frame crop size

    ConfigValue mKeepLastFrame;
    ConfigValue mHideVideo;

    std::mutex mRenderLock;
    bool mFullscreen; //default true value to full screen show video

    int mNumDroppedFrames; //the count of dropped frames
    int mCommitFrameCnt; //the count frames of committing to server
    int mReadyDisplayFrameCnt; //the count frames of ready to display
    int64_t mFirstFramePts; //the first display frame pts

    RenderVideoFormat mBufferFormat;
    std::unordered_map<int, RenderBuffer *> mRenderBuffersMap;
    /*key is buffer id, value is display time*/
    std::unordered_map<int, int64_t> mDisplayedFrameMap;

    bool mIsVideoPip;
    mutable std::mutex mMutex;
    void *mUserData;

    /*immediately output video frame to display*/
    bool mImmediatelyOutput;

    //essos resource manager
    WstEssRMgrOps *mWstEssRMgrOps;
};


#endif /*__WST_CLIENT_PLUGIN_H__*/