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
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <linux/videodev2.h>
#include <meson_drm.h>
#include "drm_display.h"
#include "Logger.h"
#include "drm_framepost.h"
#include "drm_framerecycle.h"
#include "ErrorCode.h"

using namespace Tls;

#define TAG "rlib:drm_display"

#define BLACK_FRAME_WIDTH 64
#define BLACK_FRAME_HEIGHT 64

DrmDisplay::DrmDisplay(DrmPlugin *plugin, int logcategory)
    : mPlugin(plugin),
    mLogCategory(logcategory)
{
    mDrmHandle = NULL;
    mBlackFrame = 0;
    mIsPip = false;
    mDrmFramePost = NULL;
    mDrmFrameRecycle = NULL;
    mVideoFormat = VIDEO_FORMAT_UNKNOWN;
    mWinRect.x = 0;
    mWinRect.y = 0;
    mWinRect.w = 0;
    mWinRect.h = 0;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mBlackFrame = NULL;
    mBlackFrameAddr = NULL;
    mHideVideo = false;
    mKeepLastFrame = false;
    mDrmMesonLib = drmMesonLoadLib(logcategory);
}

DrmDisplay::~DrmDisplay()
{
    if (mDrmHandle) {
        if (mDrmMesonLib) {
            mDrmMesonLib->libDrmDisplayDestroy(mDrmHandle);
            mDrmHandle = NULL;
        }
    }
    //free dlopen resources
    if (mDrmMesonLib) {
        drmMesonUnloadLib(mLogCategory, mDrmMesonLib);
        mDrmMesonLib = NULL;
    }
}

bool DrmDisplay::start(bool pip)
{
    mIsPip = pip;

    DEBUG(mLogCategory, "start pip:%d",pip);
    if (mDrmMesonLib) {
        mDrmHandle = mDrmMesonLib->libDrmDisplayInit();
    }

    if (!mDrmHandle) {
        ERROR(mLogCategory, "drm display init failed");
        return false;
    }
#ifdef SUPPORT_DRM_FREEZEN
    //set keep last frame if needed;
    if (mKeepLastFrame) {
        mDrmHandle->freeze = 1;
    } else {
        mDrmHandle->freeze = 0;
    }
    INFO(mLogCategory, "set keep last frame %d", mDrmHandle->freeze);
#endif

    if (!mDrmFramePost) {
        mDrmFramePost = new DrmFramePost(this, mLogCategory);
        mDrmFramePost->start();
        /*if window size set, we set to frame post,this needed
         when yts resize case,window size must effect when post
         buffer to drm
        */
        if (mWinRect.w > 0 && mWinRect.h > 0) {
            mDrmFramePost->setWindowSize(mWinRect.x, mWinRect.y, mWinRect.w, mWinRect.h);
        }
    }

    if (!mDrmFrameRecycle) {
        mDrmFrameRecycle = new DrmFrameRecycle(this,mLogCategory);
        mDrmFrameRecycle->start();
    }
    return true;
}

bool DrmDisplay::stop()
{
    if (mDrmFramePost) {
        DEBUG(mLogCategory, "stop frame post thread");
        mDrmFramePost->stop();
        delete mDrmFramePost;
        mDrmFramePost = NULL;
    }
    if (mDrmFrameRecycle) {
        DEBUG(mLogCategory, "stop frame recycle thread");
        mDrmFrameRecycle->stop();
        delete mDrmFrameRecycle;
        mDrmFrameRecycle = NULL;
    }
    if (mBlackFrameAddr) {
        munmap (mBlackFrameAddr, mBlackFrame->width * mBlackFrame->height * 2);
        mBlackFrameAddr = NULL;
    }
    if (mBlackFrame) {
        if (mDrmMesonLib) {
            mDrmMesonLib->libDrmFreeBuf(mBlackFrame);
        }
        mBlackFrame = NULL;
    }
    //if video plane hided,we should unhide here,otherwise next playback will no video showed
    if (mHideVideo) {
        setHideVideo(false);
    }
    if (mDrmHandle) {
        if (mDrmMesonLib) {
            mDrmMesonLib->libDrmDisplayDestroy(mDrmHandle);
            mDrmHandle = NULL;
        }
    }
    return true;
}

bool DrmDisplay::displayFrame(RenderBuffer *buf, int64_t displayTime)
{
    int ret;

    FrameEntity *frameEntity = createFrameEntity(buf, displayTime);
    if (frameEntity) {
        if (mDrmFramePost) {
            mDrmFramePost->readyPostFrame(frameEntity);
        } else {
            WARNING(mLogCategory,"no frame post service");
            handleDropedFrameEntity(frameEntity);
            handleReleaseFrameEntity(frameEntity);
        }
    } else { //drop and release render buffer
        if (mPlugin) {
            mPlugin->handleFrameDropped(buf);
            mPlugin->handleBufferRelease(buf);
        }
    }

    return true;
}

void DrmDisplay::flush()
{
    DEBUG(mLogCategory,"flush");
    Tls::Mutex::Autolock _l(mMutex);
    if (mDrmFramePost) {
        mDrmFramePost->flush();
    }
}

void DrmDisplay::pause()
{
    mDrmFramePost->pause();
}

void DrmDisplay::resume()
{
    mDrmFramePost->resume();
}

void DrmDisplay::setVideoFormat(RenderVideoFormat videoFormat)
{
    mVideoFormat = videoFormat;
    DEBUG(mLogCategory,"video format:%d",mVideoFormat);
}

void DrmDisplay::setWindowSize(int x, int y, int w, int h)
{
    mWinRect.x = x;
    mWinRect.y = y;
    mWinRect.w = w;
    mWinRect.h = h;
    DEBUG(mLogCategory,"window size:(%dx%dx%dx%d)",x, y,w,h);
    if (mDrmFramePost) {
        mDrmFramePost->setWindowSize(x, y, w, h);
    }
}

void DrmDisplay::setFrameSize(int width, int height)
{
    mFrameWidth = width;
    mFrameHeight = height;
    DEBUG(mLogCategory,"frame size:%dx%d",width, height);
}

void DrmDisplay::showBlackFrame()
{
    int rc;
    struct drm_buf_metadata info;

    memset(&info, 0 , sizeof(struct drm_buf_metadata));

    /* use single planar for black frame */
    info.width = BLACK_FRAME_WIDTH;
    info.height = BLACK_FRAME_HEIGHT;
    info.flags = 0;
    info.fourcc = DRM_FORMAT_YUYV;

    if (!mIsPip)
        info.flags |= MESON_USE_VD1;
    else
        info.flags |= MESON_USE_VD2;

    if (mDrmHandle && mDrmMesonLib && !mBlackFrame) {
        mBlackFrame = mDrmMesonLib->libDrmAllocBuf(mDrmHandle, &info);
    }

    if (!mBlackFrame) {
        ERROR(mLogCategory,"Unable to alloc drm buf");
        goto tag_error;
    }

    if (!mBlackFrameAddr) {
        mBlackFrameAddr = mmap (NULL, info.width * info.height * 2, PROT_WRITE,
            MAP_SHARED, mBlackFrame->fd[0], mBlackFrame->offsets[0]);
        if (mBlackFrameAddr == MAP_FAILED) {
            ERROR(mLogCategory,"mmap fail %d", errno);
            mBlackFrameAddr = NULL;
            goto tag_error;
        }
    }

    /* full screen black frame */
    memset (mBlackFrameAddr, 0, info.width * info.height * 2);
    mBlackFrame->crtc_x = 0;
    mBlackFrame->crtc_y = 0;
    mBlackFrame->crtc_w = -1;
    mBlackFrame->crtc_h = -1;

    //disable plane,only this flag can take effect for black screen
    mBlackFrame->disable_plane = 1;

    //post black frame buf to drm
    if (mDrmHandle && mDrmMesonLib) {
        rc = mDrmMesonLib->libDrmPostBuf(mDrmHandle, mBlackFrame);
    }
    if (rc) {
        ERROR(mLogCategory, "post black frame to drm failed");
        goto tag_error;
    }

    return;
tag_error:
    if (mBlackFrameAddr) {
        munmap (mBlackFrameAddr, mBlackFrame->width * mBlackFrame->height * 2);
        mBlackFrameAddr = NULL;
    }
    if (mBlackFrame) {
        if (mDrmMesonLib) {
            mDrmMesonLib->libDrmFreeBuf(mBlackFrame);
        }
        mBlackFrame = NULL;
    }
    return;
}

void DrmDisplay::setImmediatelyOutout(bool on)
{
    if (mDrmFramePost) {
        mDrmFramePost->setImmediatelyOutout(on);
    }
}

void DrmDisplay::setHideVideo(bool hide) {
    INFO(mLogCategory,"hide video:%d",hide);
    mHideVideo = hide;
    if (mDrmHandle && mDrmMesonLib && mDrmMesonLib->libDrmMutePlane) {
        unsigned int planeType = 1; //0:osd plane,1:video plane
        unsigned int planeMute; //0: unmute,1:mute
        if (hide) {
            planeMute = 1; //1:mute
            INFO(mLogCategory,"mute video");
        } else {
            planeMute = 0; //0: unmute
            INFO(mLogCategory,"unmute video");
        }
        mDrmMesonLib->libDrmMutePlane(mDrmHandle->drm_fd, planeType, planeMute);
    } else {
        ERROR(mLogCategory, "Error dlsym drm_mute_plane");
    }
}

void DrmDisplay::setKeepLastFrame(bool keep)
{
    int kp = keep? 1: 0;
    mKeepLastFrame = keep;

#ifdef SUPPORT_DRM_FREEZEN
    if (mDrmHandle) {
        mDrmHandle->freeze = kp;
    }
    INFO(mLogCategory, "set keep last frame %d(%d)", keep,mDrmHandle->freeze);
    if (!kp) {
        INFO(mLogCategory, "set keep last frame %d, show black frame to drm", kp);
        showBlackFrame();
    }
#endif
}

FrameEntity *DrmDisplay::createFrameEntity(RenderBuffer *buf, int64_t displayTime)
{
    struct drm_buf_import info;
    FrameEntity* frame = NULL;
    struct drm_buf * drmBuf = NULL;
    int displayWidth = 0, displayHeight = 0;

    frame = (FrameEntity*)calloc(1, sizeof(FrameEntity));
    if (!frame) {
        ERROR(mLogCategory,"oom calloc FrameEntity mem failed");
        goto tag_error;
    }

    frame->displayTime = displayTime;
    frame->renderBuf = buf;

    memset(&info, 0 , sizeof(struct drm_buf_import));

    info.width = buf->dma.width;
    info.height = buf->dma.height;
    info.flags = 0;
    switch (mVideoFormat)
    {
        case VIDEO_FORMAT_NV21: {
            info.fourcc = DRM_FORMAT_NV21;
        } break;
        case VIDEO_FORMAT_NV12: {
            info.fourcc = DRM_FORMAT_NV12;
        } break;

        default: {
            info.fourcc = DRM_FORMAT_YUYV;
            WARNING(mLogCategory,"unknown video format, set to default YUYV format");
        }
    }

    if (!mIsPip) {
        info.flags |= MESON_USE_VD1;
    } else {
        info.flags |= MESON_USE_VD2;
    }

    /*warning: must dup fd, because drm_free_buf will close buf fd
    if not dup buf fd, fd will be double free*/
    for (int i = 0; i < buf->dma.planeCnt; i++) {
        info.fd[i] = dup(buf->dma.fd[i]);
        TRACE(mLogCategory,"dup fd[%d]:%d->%d",i,buf->dma.fd[i],info.fd[i]);
    }

    if (mDrmHandle && mDrmMesonLib) {
        drmBuf = mDrmMesonLib->libDrmImportBuf(mDrmHandle, &info);
    }

    if (!drmBuf) {
        ERROR(mLogCategory, "unable drm_import_buf");
        goto tag_error;
    }

    //set source content's size
    drmBuf->src_x = 0;
    drmBuf->src_y = 0;
    drmBuf->src_w = buf->dma.width;
    drmBuf->src_h = buf->dma.height;

    /*default set window size to full screen
     *if window size set,the crtc size will be reset
     *before post drm buffer
     */
    //get current display mode
    if (mDrmHandle && mDrmMesonLib) {
        DisplayMode displayMode;
        mDrmMesonLib->libDrmGetModeInfo(mDrmHandle->drm_fd, MESON_CONNECTOR_RESERVED, &displayMode);
        displayWidth = displayMode.w;
        displayHeight = displayMode.h;
        ERROR(mLogCategory, "displayWidth:%d,displayHeight:%d",displayWidth,displayHeight);
    }
    drmBuf->crtc_x = 0;
    drmBuf->crtc_y = 0;
    if (displayWidth > 0 && displayHeight > 0) {
        drmBuf->crtc_w = displayWidth;
        drmBuf->crtc_h = displayHeight;
    } else {
        drmBuf->crtc_w = mDrmHandle->width;
        drmBuf->crtc_h = mDrmHandle->height;
    }

    frame->drmBuf = drmBuf;
    TRACE(mLogCategory,"crtc(%d,%d,%d,%d),src(%d,%d,%d,%d)",drmBuf->crtc_x,drmBuf->crtc_y,drmBuf->crtc_w,drmBuf->crtc_h,
        drmBuf->src_x,drmBuf->src_y,drmBuf->src_w,drmBuf->src_h);

    return frame;
tag_error:
    for (int i = 0; i < buf->dma.planeCnt; i++) {
        if (info.fd[i] > 0) {
            close(info.fd[i]);
            info.fd[i] = -1;
        }
    }
    if (frame) {
        free(frame);
    }
    return NULL;
}

void DrmDisplay::destroyFrameEntity(FrameEntity * frameEntity)
{
    int rc;

    if (!frameEntity) {
        return;
    }
    if (frameEntity->drmBuf) {
        if (mDrmMesonLib) {
            rc = mDrmMesonLib->libDrmFreeBuf(frameEntity->drmBuf);
        }

        if (rc) {
            WARNING(mLogCategory, "drm_free_buf free %p failed",frameEntity->drmBuf);
        }
        TRACE(mLogCategory,"drm_free_buf displaytime:%lld(pts:%lld ms)",frameEntity->displayTime,frameEntity->renderBuf->pts/1000000);
    }
    free(frameEntity);
}

void DrmDisplay::handlePostedFrameEntity(FrameEntity * frameEntity)
{
    if (mDrmFrameRecycle) {
        mDrmFrameRecycle->recycleFrame(frameEntity);
    }
}

void DrmDisplay::handleDropedFrameEntity(FrameEntity * frameEntity)
{
    if (mPlugin) {
        mPlugin->handleFrameDropped(frameEntity->renderBuf);
    }
}

void DrmDisplay::handleDisplayedFrameEntity(FrameEntity * frameEntity)
{
    if (mPlugin) {
        mPlugin->handleFrameDisplayed(frameEntity->renderBuf);
    }
}

void DrmDisplay::handleReleaseFrameEntity(FrameEntity * frameEntity)
{
    if (mPlugin) {
        mPlugin->handleBufferRelease(frameEntity->renderBuf);
    }
    destroyFrameEntity(frameEntity);
}

void DrmDisplay::handleMsg(int type, void *detail)
{
    if (mPlugin) {
        mPlugin->handleMsgNotify(type, detail);
    }
}
