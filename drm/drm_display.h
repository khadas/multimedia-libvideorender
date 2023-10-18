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
#ifndef __DRM_DISPLAY_WRAPPER_H__
#define __DRM_DISPLAY_WRAPPER_H__
#include <list>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <linux/videodev2.h>
#include "Mutex.h"
#include "Thread.h"
#include "Queue.h"
#include "drm_plugin.h"
#include "drm_lib_wrap.h"

typedef struct FrameEntity
{
    RenderBuffer *renderBuf;
    int64_t displayTime;
    struct drm_buf *drmBuf;
} FrameEntity;

class DrmPlugin;
class DrmFramePost;
class DrmFrameRecycle;

class DrmDisplay
{
  public:
    DrmDisplay(DrmPlugin *plugin, int logCategory);
    virtual ~DrmDisplay();
    bool start(bool pip);
    bool stop();
    bool displayFrame(RenderBuffer *buf, int64_t displayTime);
    void flush();
    void pause();
    void resume();
    void setVideoFormat(RenderVideoFormat videoFormat);
    void setWindowSize(int x, int y, int w, int h);
    void setFrameSize(int width, int height);
    void showBlackFrame();
    void setImmediatelyOutout(bool on);
    struct drm_display *getDrmHandle() {
        return mDrmHandle;
    };
    DrmMesonLib *getDrmMesonLib() {
      return mDrmMesonLib;
    }
    void setHideVideo(bool hide);
    bool isHideVideo() {
      return mHideVideo;
    };

    /*set keeping last frame yes or not when stop playing*/
    void setKeepLastFrame(bool keep);

    /**
     * @brief handle frame that had posted to drm display,must commit this posted frame
     * to frame recycle service to wait frame display
     *
     * @param frameEntity frame resource info
     */
    void handlePostedFrameEntity(FrameEntity * frameEntity);
    /**
     * @brief handle droped frame when two frames display time is in
     * one vsync duration
     *
     * @param frameEntity frame resource info
     */
    void handleDropedFrameEntity(FrameEntity * frameEntity);
    /**
     * @brief handle displayed frame
     *
     * @param frameEntity
     */
    void handleDisplayedFrameEntity(FrameEntity * frameEntity);
    /**
     * @brief handle displayed frame and ready to release
     *
     * @param frameEntity
     */
    void handleReleaseFrameEntity(FrameEntity * frameEntity);
    //handle frame post or recycle msg
    void handleMsg(int type, void *detail);
  private:
    FrameEntity *createFrameEntity(RenderBuffer *buf, int64_t displayTime);
    void destroyFrameEntity(FrameEntity * frameEntity);

    DrmPlugin *mPlugin;
    int mLogCategory;

    DrmMesonLib *mDrmMesonLib; //libdrm meson dlopen handle

    mutable Tls::Mutex mMutex;

    bool mIsPip;
    RenderVideoFormat mVideoFormat;
    RenderRect mWinRect;
    int mFrameWidth;
    int mFrameHeight;

    struct drm_display *mDrmHandle;
    struct drm_buf *mBlackFrame;
    void *mBlackFrameAddr;

    DrmFramePost *mDrmFramePost;
    DrmFrameRecycle *mDrmFrameRecycle;

    bool mHideVideo;
    bool mIsDropFrames;

    bool mKeepLastFrame;
    bool mPaused;
};

#endif /*__DRM_DISPLAY_WRAPPER_H__*/