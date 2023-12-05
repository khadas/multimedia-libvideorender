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
#include <linux/videodev2.h>
#include "wstclient_wayland.h"
#include "wstclient_plugin.h"
#include "Logger.h"
#include "ErrorCode.h"

#define TAG  "rlib:wstClient_plugin"
#define DEFAULT_VIDEO_SERVER "video"

WstClientPlugin::WstClientPlugin(int logCategory)
    : mFullscreen(true),
    mLogCategory(logCategory)
{
    mIsVideoPip = false;
    mBufferFormat = VIDEO_FORMAT_UNKNOWN;
    mNumDroppedFrames = 0;
    mCommitFrameCnt = 0;
    mReadyDisplayFrameCnt = 0;
    mWayland = new WstClientWayland(this, logCategory);
    mWstClientSocket = NULL;
    mKeepLastFrame.isSet = false;
    mKeepLastFrame.value = 0;
    mHideVideo.isSet = false;
    mHideVideo.value = 0;
    mFirstFramePts = -1;
    mImmediatelyOutput = false;
    mSetCropFrameRect = false;
    mFrameRateFractionNum = 0;
    mFrameRateFractionDenom = 0;
    mFrameRateChanged = false;
    mWstEssRMgrOps = new WstEssRMgrOps(this,logCategory);
}

WstClientPlugin::~WstClientPlugin()
{
    if (mWayland) {
        delete mWayland;
        mWayland = NULL;
    }
    if (mWstEssRMgrOps) {
        delete mWstEssRMgrOps;
        mWstEssRMgrOps = NULL;
    }

    TRACE(mLogCategory,"deconstruct");
}

void WstClientPlugin::init()
{
    INFO(mLogCategory,"\n--------------------------------\n"
            "plugin      : westeros\n"
            "ARCH        : %s\n"
            "branch name : %s\n"
            "git version : %s\n"
            "change id   : %s \n"
            "ID          : %s \n"
            "last changed: %s\n"
            "build-time  : %s\n"
            "build-name  : %s\n"
            "--------------------------------\n",
#if defined(__aarch64__)
            "arm64",
#else
            "arm",
#endif
            BRANCH_NAME,
            GIT_VERSION,
            COMMIT_CHANGEID,
            COMMIT_PD,
            LAST_CHANGED,
            BUILD_TIME,
            BUILD_NAME
    );
}

void WstClientPlugin::release()
{
    DEBUG(mLogCategory,"release");
}

void WstClientPlugin::setCallback(void *userData, PluginCallback *callback)
{
    mUserData = userData;
    mCallback = callback;
}

int WstClientPlugin::openDisplay()
{
    int ret = 0;

    DEBUG(mLogCategory,"openDisplay");

    mWstEssRMgrOps->resMgrInit();
    mWstEssRMgrOps->resMgrRequestDecoder(mIsVideoPip);

    //connect video server first
    if (!mWstClientSocket) {
        bool rc;
        mWstClientSocket = new WstClientSocket(this, mLogCategory);
        rc = mWstClientSocket->connectToSocket(DEFAULT_VIDEO_SERVER);
        if (!rc) {
            ERROR(mLogCategory,"Error connect to video server fail");
            delete mWstClientSocket;
            mWstClientSocket = NULL;
            return ERROR_OPEN_FAIL;
        }
    }

    if (mWstClientSocket) {
        mWstClientSocket->sendLayerVideoClientConnection(mIsVideoPip);
        mWstClientSocket->sendResourceVideoClientConnection(mIsVideoPip);
    }

    ret =  mWayland->connectToWayland();
    if (ret != NO_ERROR) {
        ERROR(mLogCategory,"Error open display");
    } else {
        //run wl display queue dispatch
        DEBUG(mLogCategory,"To run wl display dispatch queue");
        mWayland->run("display queue");
    }

    DEBUG(mLogCategory,"openDisplay end");
    return NO_ERROR;
}

int WstClientPlugin::openWindow()
{
    int ret;

    DEBUG(mLogCategory,"openWindow");
    mCommitFrameCnt = 0;
    mNumDroppedFrames = 0;
    mReadyDisplayFrameCnt = 0;
    /*send session info to server
    we use mediasync to sync a/v,so select AV_SYNC_MODE_VIDEO_MONO as av clock*/
    if (mWstClientSocket) {
        if (mImmediatelyOutput) {
            mWstClientSocket->sendSessionInfoVideoClientConnection(INVALID_SESSION_ID, SYNC_IMMEDIATE);
        } else {
            mWstClientSocket->sendSessionInfoVideoClientConnection(AV_SYNC_SESSION_V_MONO, AV_SYNC_MODE_VIDEO_MONO);
        }
    }

    //send hide video
    if (mWstClientSocket && mHideVideo.isSet) {
        mWstClientSocket->sendHideVideoClientConnection(mHideVideo.value);
    }

    //send keep last video frame
    if (mWstClientSocket && mKeepLastFrame.isSet) {
        mWstClientSocket->sendKeepLastFrameVideoClientConnection(mKeepLastFrame.value);
    }

    //send crop frame size if set before window opened
    if (mSetCropFrameRect) {
        setCropFrameRect();
    }

    DEBUG(mLogCategory,"openWindow,end");
    return ret;
}

int WstClientPlugin::prepareFrame(RenderBuffer *buffer)
{
    return NO_ERROR;
}

int WstClientPlugin::displayFrame(RenderBuffer *buffer, int64_t displayTime)
{
    bool ret;
    WstBufferInfo wstBufferInfo;
    WstRect wstRect;
    int x,y,w,h;

    if (mWstClientSocket && mFrameRateChanged) {
        mFrameRateChanged = false;
        mWstClientSocket->sendRateVideoClientConnection(mFrameRateFractionNum, mFrameRateFractionDenom);
    }

    mWayland->getVideoBounds(&x, &y, &w, &h);

    //init wstBufferInfo,must set fd to -1 value
    memset(&wstBufferInfo, 0, sizeof(WstBufferInfo));
    for (int i = 0; i < WST_MAX_PLANES; i++) {
        wstBufferInfo.planeInfo[0].fd = -1;
        wstBufferInfo.planeInfo[1].fd = -1;
        wstBufferInfo.planeInfo[2].fd = -1;
    }

    wstBufferInfo.bufferId = buffer->id;
    wstBufferInfo.planeCount = buffer->dma.planeCnt;
    TRACE(mLogCategory,"buffer width:%d,height:%d",buffer->dma.width,buffer->dma.height);
    for (int i = 0; i < buffer->dma.planeCnt; i++) {
        wstBufferInfo.planeInfo[i].fd = buffer->dma.fd[i];
        wstBufferInfo.planeInfo[i].stride = buffer->dma.stride[i];
        wstBufferInfo.planeInfo[i].offset = buffer->dma.offset[i];
        DEBUG(mLogCategory,"buffer id:%d,plane[%d],fd:%d,stride:%d,offset:%d",buffer->id,i,buffer->dma.fd[i],buffer->dma.stride[i],buffer->dma.offset[i]);
    }

    wstBufferInfo.frameWidth = buffer->dma.width;
    wstBufferInfo.frameHeight = buffer->dma.height;
    wstBufferInfo.frameTime = displayTime;

    //change render lib video format to v4l2 support format
    if (mBufferFormat == VIDEO_FORMAT_NV12) {
        wstBufferInfo.pixelFormat = V4L2_PIX_FMT_NV12;
    } else if (mBufferFormat == VIDEO_FORMAT_NV21) {
        wstBufferInfo.pixelFormat = V4L2_PIX_FMT_NV21;
    } else {
        ERROR(mLogCategory,"unknown video buffer format:%d",mBufferFormat);
    }

    wstRect.x = x;
    wstRect.y = y;
    wstRect.w = w;
    wstRect.h = h;

    if (mWstClientSocket) {
        ret = mWstClientSocket->sendFrameVideoClientConnection(&wstBufferInfo, &wstRect);
        if (!ret) {
            ERROR(mLogCategory,"send video frame to server fail");
            handleFrameDropped(buffer);
            handleBufferRelease(buffer);
            return ERROR_FAILED_TRANSACTION;
        }
    }

    //storage render buffer to manager
    std::lock_guard<std::mutex> lck(mRenderLock);
    std::pair<int, RenderBuffer *> item(buffer->id, buffer);
    mRenderBuffersMap.insert(item);
    ++mCommitFrameCnt;
    ++mReadyDisplayFrameCnt;
    TRACE(mLogCategory,"committed to westeros cnt:%d,readyDisplayFramesCnt:%d",mCommitFrameCnt,mReadyDisplayFrameCnt);

    //storage displayed render buffer
    std::pair<int, int64_t> displayitem(buffer->id, displayTime);
    mDisplayedFrameMap.insert(displayitem);
    if (mFirstFramePts == -1) {
        mFirstFramePts = buffer->pts;
    }

    return NO_ERROR;
}

int WstClientPlugin::flush()
{
    int ret;
    INFO(mLogCategory,"flush");
    if (mWstClientSocket) {
        mWstClientSocket->sendFlushVideoClientConnection();
    }
    //drop frames those had committed to westeros
    std::lock_guard<std::mutex> lck(mRenderLock);
    for (auto item = mDisplayedFrameMap.begin(); item != mDisplayedFrameMap.end(); ) {
        int bufferid = (int)item->first;
        auto bufItem = mRenderBuffersMap.find(bufferid);
        if (bufItem == mRenderBuffersMap.end()) {
            continue;
        }
        mDisplayedFrameMap.erase(item++);
        RenderBuffer *renderbuffer = (RenderBuffer*) bufItem->second;
        if (renderbuffer) {
            handleFrameDropped(renderbuffer);
        }
    }

    return NO_ERROR;
}

int WstClientPlugin::pause()
{
    int ret;
    INFO(mLogCategory,"pause");
    if (mWstClientSocket) {
        int waitCnt = 20; //we wait about 10 vsync duration
        while (mReadyDisplayFrameCnt != 0 && waitCnt > 0) {
            usleep(8000);
            --waitCnt;
        }
        std::lock_guard<std::mutex> lck(mRenderLock);
        if (mReadyDisplayFrameCnt == 0) { //send pause cmd immediatialy if no frame displays
            mWstClientSocket->sendPauseVideoClientConnection(true);
        }
    }
    mWstEssRMgrOps->resMgrUpdateState(EssRMgrRes_paused);
    return NO_ERROR;
}

int WstClientPlugin::resume()
{
    int ret;
    INFO(mLogCategory,"resume");
    std::lock_guard<std::mutex> lck(mRenderLock);
    if (mWstClientSocket) {
        mWstClientSocket->sendPauseVideoClientConnection(false);
    }
    mWstEssRMgrOps->resMgrUpdateState(EssRMgrRes_active);
    return NO_ERROR;
}

int WstClientPlugin::closeDisplay()
{
    INFO(mLogCategory,"closeDisplay, in");
    mWayland->disconnectFromWayland();
    mWstEssRMgrOps->resMgrReleaseDecoder();
    mWstEssRMgrOps->resMgrTerm();
    INFO(mLogCategory,"closeDisplay, out");
    return NO_ERROR;
}

int WstClientPlugin::closeWindow()
{
    DEBUG(mLogCategory,"closeWindow, in");
    if (mWstClientSocket) {
        mWstClientSocket->disconnectFromSocket();
        delete mWstClientSocket;
        mWstClientSocket = NULL;
    }

    std::lock_guard<std::mutex> lck(mRenderLock);
    //drop all frames those don't displayed
    for (auto item = mDisplayedFrameMap.begin(); item != mDisplayedFrameMap.end(); ) {
        int bufferid = (int)item->first;
        auto bufItem = mRenderBuffersMap.find(bufferid);
        if (bufItem == mRenderBuffersMap.end()) {
            continue;
        }
        mDisplayedFrameMap.erase(item++);
        RenderBuffer *renderbuffer = (RenderBuffer*) bufItem->second;
        if (renderbuffer) {
            handleFrameDropped(renderbuffer);
        }
    }
    //release all frames those had committed to westeros server
    for (auto item = mRenderBuffersMap.begin(); item != mRenderBuffersMap.end();) {
        RenderBuffer *renderbuffer = (RenderBuffer*) item->second;
        mRenderBuffersMap.erase(item++);
        if (renderbuffer) {
            handleBufferRelease(renderbuffer);
        }
    }
    mWstEssRMgrOps->resMgrUpdateState(EssRMgrRes_idle);
    mRenderBuffersMap.clear();
    mDisplayedFrameMap.clear();
    mCommitFrameCnt = 0;
    mNumDroppedFrames = 0;
    mReadyDisplayFrameCnt = 0;
    mImmediatelyOutput = false;
    DEBUG(mLogCategory,"out");
    return NO_ERROR;
}

int WstClientPlugin::getValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_KEEP_LAST_FRAME: {
            *(int *)value = mKeepLastFrame.value;
            TRACE(mLogCategory,"get keep last frame:%d",*(int *)value);
        } break;
        case PLUGIN_KEY_HIDE_VIDEO: {
            *(int *)value = mHideVideo.value;
            TRACE(mLogCategory,"get hide video:%d",*(int *)value);
        } break;
        case PLUGIN_KEY_CROP_FRAME_SIZE: {
            RenderRect* rect = static_cast<RenderRect*>(value);
            rect->x = mCropFrameRect.x;
            rect->y = mCropFrameRect.y;
            rect->w = mCropFrameRect.w;
            rect->h = mCropFrameRect.h;
        } break;
    }
    return NO_ERROR;
}

int WstClientPlugin::setValue(PluginKey key, void *value)
{
    switch (key) {
        case PLUGIN_KEY_WINDOW_SIZE: {
            RenderRect* rect = static_cast<RenderRect*>(value);
            if (mWayland) {
                mWayland->setWindowSize(rect->x, rect->y, rect->w, rect->h);
            }
        } break;
        case PLUGIN_KEY_FRAME_SIZE: {
            RenderFrameSize * frameSize = static_cast<RenderFrameSize * >(value);
            if (mWayland) {
                mWayland->setFrameSize(frameSize->width, frameSize->height);
            }
            //if set crop property before set frame size,we set crop params now, because
            //crop property dependent frame size
            if (mSetCropFrameRect) {
                setCropFrameRect();
            }
        } break;
        case PLUGIN_KEY_VIDEO_FORMAT: {
            int format = *(int *)(value);
            mBufferFormat = (RenderVideoFormat) format;
            DEBUG(mLogCategory,"Set video format :%d",mBufferFormat);
        } break;
        case PLUGIN_KEY_VIDEO_PIP: {
            int pip = *(int *) (value);
            mIsVideoPip = pip > 0? true:false;
        } break;
        case PLUGIN_KEY_KEEP_LAST_FRAME: {
            int keep = *(int *) (value);
            mKeepLastFrame.value = keep > 0? true:false;
            mKeepLastFrame.isSet = true;
            DEBUG(mLogCategory, "Set keep last frame :%d",mKeepLastFrame.value);
            if (mWstClientSocket) {
                mWstClientSocket->sendKeepLastFrameVideoClientConnection(mKeepLastFrame.value);
            }
        } break;
        case PLUGIN_KEY_HIDE_VIDEO: {
            int hide = *(int *)(value);
            mHideVideo.value = hide > 0? true:false;
            mHideVideo.isSet = true;
            DEBUG(mLogCategory, "Set hide video:%d",mHideVideo.value);
            if (mWstClientSocket) {
                mWstClientSocket->sendHideVideoClientConnection(mHideVideo.value);
            }
        } break;
        case PLUGIN_KEY_FORCE_ASPECT_RATIO: {
            int forceAspectRatio = *(int *)(value);
            if (mWayland) {
                mWayland->setForceAspectRatio(forceAspectRatio > 0? true:false);
            }
        } break;
        case PLUGIN_KEY_IMMEDIATELY_OUTPUT: {
            int immediately = *(int *)(value);
            mImmediatelyOutput = immediately > 0? true: false;
            DEBUG(mLogCategory,"Set immediately output:%d",mImmediatelyOutput);
            if (mImmediatelyOutput && mWstClientSocket) {
                mWstClientSocket->sendSessionInfoVideoClientConnection(INVALID_SESSION_ID, SYNC_IMMEDIATE);
            }
        } break;
        case PLUGIN_KEY_CROP_FRAME_SIZE: {
            RenderRect* rect = static_cast<RenderRect*>(value);
            mCropFrameRect.x = rect->x;
            mCropFrameRect.y = rect->y;
            mCropFrameRect.w = rect->w;
            mCropFrameRect.h = rect->h;
            mSetCropFrameRect = true;
            INFO(mLogCategory,"crop params (%d,%d,%d,%d)",rect->x,rect->y,rect->w,rect->h);
            setCropFrameRect();
        } break;
        case PLUGIN_KEY_PIXEL_ASPECT_RATIO: {
            double ratio = *(double *)(value);
            INFO(mLogCategory,"pixel aspect ratio :%f",ratio);
            if (mWayland) {
                mWayland->setPixelAspectRatio((double)ratio);
            }
        } break;
        case PLUGIN_KEY_VIDEO_FRAME_RATE: {
            RenderFraction * fraction = static_cast<RenderFraction*>(value);
            INFO(mLogCategory,"frame rate,num:%d,denom:%d",fraction->num,fraction->denom);
            if (fraction->num != mFrameRateFractionNum ||
                fraction->denom != mFrameRateFractionDenom) {
                mFrameRateFractionNum = fraction->num;
                mFrameRateFractionDenom = fraction->denom;
                if (mFrameRateFractionDenom == 0) {
                    mFrameRateFractionDenom = 1;
                }
                mFrameRateChanged = true;
            }
            if (mWstClientSocket && mFrameRateChanged) {
                mFrameRateChanged = false;
                mWstClientSocket->sendRateVideoClientConnection(mFrameRateFractionNum, mFrameRateFractionDenom);
            }
        } break;
    }
    return NO_ERROR;
}

void WstClientPlugin::handleBufferRelease(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferReleaseCallback(mUserData, (void *)buffer);
    }
}

void WstClientPlugin::handleFrameDisplayed(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDisplayedCallback(mUserData, (void *)buffer);
    }
}

void WstClientPlugin::handleFrameDropped(RenderBuffer *buffer)
{
    if (mCallback) {
        mCallback->doBufferDropedCallback(mUserData, (void *)buffer);
    }
}

void WstClientPlugin::handleMsgNotify(int type, void *detail)
{
    if (mCallback) {
        mCallback->doMsgCallback(mUserData, type, detail);
    }
}

void WstClientPlugin::setVideoRect(int videoX, int videoY, int videoWidth, int videoHeight)
{
    if (mWstClientSocket) {
        mWstClientSocket->sendRectVideoClientConnection(videoX, videoY, videoWidth, videoHeight);
    }
}

void WstClientPlugin::onWstSocketEvent(WstEvent *event)
{
    Tls::Mutex::Autolock _l(mMutex);
    switch (event->event)
    {
        case WST_REFRESH_RATE: {
            int rate = event->param;
            INFO(mLogCategory,"refresh rate:%d",rate);
        } break;
        case WST_BUFFER_RELEASE: {
            int bufferid = event->param;
            TRACE(mLogCategory,"Buffer release id:%d",bufferid);
            RenderBuffer *renderbuffer = NULL;
            bool isDropped = false;
            int64_t displaytime = 0;
            {
                std::lock_guard<std::mutex> lck(mRenderLock);
                auto renderBufferItem = mRenderBuffersMap.find(bufferid);
                if (renderBufferItem == mRenderBuffersMap.end()) {
                    WARNING(mLogCategory,"can't find map Renderbuffer");
                    return ;
                }
                --mCommitFrameCnt;
                renderbuffer = (RenderBuffer*) renderBufferItem->second;
                //remove had release render buffer
                mRenderBuffersMap.erase(bufferid);
                auto displayFrameItem = mDisplayedFrameMap.find(bufferid);
                if (displayFrameItem != mDisplayedFrameMap.end()) {
                    mDisplayedFrameMap.erase(bufferid);
                    --mReadyDisplayFrameCnt;
                    isDropped = true;
                    displaytime = (int64_t)displayFrameItem->second;
                }
            }

            if (renderbuffer) {
                /*if we can find item in mDisplayedFrameMap,
                this buffer is dropped by westeros server,so we
                must call displayed callback*/
                if (isDropped) {
                    WARNING(mLogCategory,"Frame droped,pts:%lld us,displaytime:%lld,readyDisplayFramesCnt:%d",\
                            renderbuffer->pts/1000,displaytime,mReadyDisplayFrameCnt);
                    handleFrameDropped(renderbuffer);
                }
                handleBufferRelease(renderbuffer);
            }
            TRACE(mLogCategory,"remain on westeros cnt:%d",mCommitFrameCnt);
        } break;
        case WST_STATUS: {
            int dropframes = event->param;
            int64_t frameTime = event->lparam;
            TRACE(mLogCategory,"WST_STATUS,dropframes:%d,frameTime:%lld",dropframes,frameTime);
            RenderBuffer *renderbuffer = NULL;
            if (mNumDroppedFrames != event->param) {
                mNumDroppedFrames = event->param;
                WARNING(mLogCategory,"frame dropped cnt:%d",mNumDroppedFrames);
            }
            //update status,if frameTime isn't equal -1LL
            //this buffer had displayed
            if (frameTime != -1LL) {
                std::lock_guard<std::mutex> lck(mRenderLock);
                --mReadyDisplayFrameCnt;
                TRACE(mLogCategory,"displayed frame time:%lld,readyDisplayFramesCnt:%d",frameTime,mReadyDisplayFrameCnt);
                int bufferId = getDisplayFrameBufferId(frameTime);
                if (bufferId < 0) {
                    WARNING(mLogCategory,"can't find map displayed frame:%lld",frameTime);
                    return ;
                }
                auto displayItem = mDisplayedFrameMap.find(bufferId);
                if (displayItem != mDisplayedFrameMap.end()) {
                    mDisplayedFrameMap.erase(bufferId);
                }
                auto item = mRenderBuffersMap.find(bufferId);
                if (item != mRenderBuffersMap.end()) {
                    renderbuffer = (RenderBuffer*) item->second;
                }
            }
            if (renderbuffer) {
                //first frame signal
                if (renderbuffer->pts == mFirstFramePts) {
                    handleMsgNotify(MSG_FIRST_FRAME,(void*)&renderbuffer->pts);
                }
                handleFrameDisplayed(renderbuffer);
            }
        } break;
        case WST_UNDERFLOW: {
            uint64_t frameTime = event->lparam;
            TRACE(mLogCategory,"under flow frametime:%lld",frameTime);
            //under flow event sended must be after sending first frame to westeros
            if (mFirstFramePts != -1) {
                handleMsgNotify(MSG_UNDER_FLOW, NULL);
            }
        } break;
        case WST_ZOOM_MODE: {
            int mode = event->param;
            bool globalZoomActive = event->param1 > 0? true:false;
            bool allow4kZoom = event->param2 > 0? true:false;;
            mWayland->setZoomMode(mode, globalZoomActive, allow4kZoom);
        } break;
        case WST_DEBUG_LEVEL: {
            /* code */
        } break;
        default:
            break;
    }
}

int WstClientPlugin::getDisplayFrameBufferId(int64_t displayTime)
{
    int bufId = -1;
    for (auto item = mDisplayedFrameMap.begin(); item != mDisplayedFrameMap.end(); item++) {
        int64_t time = (int64_t)item->second;
        if (time == displayTime) {
            bufId = (int)item->first;
            break;
        }
    }
    return bufId;
}

bool WstClientPlugin::setCropFrameRect()
{
    if (mWayland) {
        int frameWidth,frameHeight;
        mWayland->getFrameSize(&frameWidth, &frameHeight);
        if (frameWidth <= 0 || frameHeight <= 0) {
            WARNING(mLogCategory, "no set video frame size,set crop params later");
            goto bad_param;
        }

        //correct crop rect if needed
        if (frameWidth > 0 && (mCropFrameRect.x + mCropFrameRect.w) > frameWidth) {
            if (mCropFrameRect.x < frameWidth) {
                int oriX = mCropFrameRect.x;
                int oriW = mCropFrameRect.w;
                mCropFrameRect.w = frameWidth - mCropFrameRect.x;
                WARNING(mLogCategory, "correct crop x:%d, w:%d to x:%d, w:%d", \
                    oriX,oriW,mCropFrameRect.x,mCropFrameRect.w);
            } else {
                ERROR(mLogCategory, "Error crop params (%d,%d,%d,%d) frame(%d,%d)", \
                    mCropFrameRect.x,mCropFrameRect.y,mCropFrameRect.w,mCropFrameRect.h,frameWidth,frameHeight);
                goto bad_param;
            }
        }
        if (frameHeight > 0 && (mCropFrameRect.y + mCropFrameRect.h) > frameHeight) {
            if (mCropFrameRect.y < frameHeight) {
                int oriY = mCropFrameRect.x;
                int oriH = mCropFrameRect.w;
                mCropFrameRect.h = frameHeight - mCropFrameRect.y;
                WARNING(mLogCategory, "correct crop y:%d, h:%d to y:%d, h:%d", \
                    oriY,oriH,mCropFrameRect.y,mCropFrameRect.h);
            } else {
                ERROR(mLogCategory, "Error crop params (%d,%d,%d,%d) frame(%d,%d)", \
                    mCropFrameRect.x,mCropFrameRect.y,mCropFrameRect.w,mCropFrameRect.h,frameWidth,frameHeight);
                goto bad_param;
            }
        }
        INFO(mLogCategory, "crop (%d,%d,%d,%d)", \
            mCropFrameRect.x,mCropFrameRect.y,mCropFrameRect.w,mCropFrameRect.h);
    }
    if (mWstClientSocket && mSetCropFrameRect) {
        mWstClientSocket->sendCropFrameSizeClientConnection(mCropFrameRect.x, mCropFrameRect.y, mCropFrameRect.w, mCropFrameRect.h);
        mSetCropFrameRect = false;
    }
    return true;
bad_param:
    return false;
}

void *makePluginInstance(int id)
{
    int category =Logger_init(id);
    char *env = getenv("VIDEO_RENDER_PLUGIN_LOG_LEVEL");
    if (env) {
        int level = atoi(env);
        Logger_set_level(level);
        INFO(category,"VIDEO_RENDER_PLUGIN_LOG_LEVEL=%d",level);
    }
    WstClientPlugin *pluginInstance = new WstClientPlugin(category);
    return static_cast<void *>(pluginInstance);
}

void destroyPluginInstance(void * plugin)
{
    int category;

    WstClientPlugin *pluginInstance = static_cast<WstClientPlugin *>(plugin);
    category = pluginInstance->getLogCategory();
    delete pluginInstance;
    Logger_exit(category);
}