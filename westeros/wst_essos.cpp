#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include "wst_essos.h"
#include "Logger.h"

#define TAG "rlib:wst_essos"

#define DEFAULT_WINDOW_WIDTH (1280)
#define DEFAULT_WINDOW_HEIGHT (720)
#define DEFAULT_USAGE (EssRMgrVidUse_fullResolution|EssRMgrVidUse_fullQuality|EssRMgrVidUse_fullPerformance)

void WstEssRMgrOps::resMgrNotify( EssRMgr *rm, int event, int type, int id, void* userData )
{
   WstEssRMgrOps *rMgrOps = static_cast<WstEssRMgrOps *>(userData);

   DEBUG(rMgrOps->mLogCategory,"resMgrNotify: enter");
   switch ( type )
   {
        case EssRMgrResType_videoDecoder:
            switch ( event )
            {
                case EssRMgrEvent_granted: {
                    rMgrOps->mResAssignedId = id;
                    memset( &rMgrOps->mResCurrCaps, 0, sizeof(EssRMgrCaps) );
                    if ( !EssRMgrResourceGetCaps( rMgrOps->mEssRmgr, EssRMgrResType_videoDecoder, rMgrOps->mResAssignedId, &rMgrOps->mResCurrCaps ) )
                    {
                        ERROR(rMgrOps->mLogCategory,"resMgrNotify: failed to get caps of assigned decoder");
                    }
                    DEBUG(rMgrOps->mLogCategory,"essos notify granted,async assigned id %d caps %X (%dx%d)",
                            rMgrOps->mResAssignedId,
                            rMgrOps->mResCurrCaps.capabilities,
                            rMgrOps->mResCurrCaps.info.video.maxWidth,
                            rMgrOps->mResCurrCaps.info.video.maxHeight);
                    if (rMgrOps->mUserCallback) {
                        rMgrOps->mUserCallback(EssRMgrEvent_granted, rMgrOps->mUserData);
                    }
                } break;
                case EssRMgrEvent_revoked: {
                    memset( &rMgrOps->mResCurrCaps, 0, sizeof(EssRMgrCaps) );
                    WARNING(rMgrOps->mLogCategory,"essos notify revoked , releasing video decoder %d", id);
                    if (rMgrOps->mUserCallback) {
                        rMgrOps->mUserCallback(EssRMgrEvent_revoked, rMgrOps->mUserData);
                    }
                    rMgrOps->mResAssignedId = -1;
                    WARNING(rMgrOps->mLogCategory,"done releasing video decoder %d", id);
                } break;
            default:
                break;
        }
        break;
        default:
            break;
   }
   DEBUG(rMgrOps->mLogCategory,"resMgrNotify: exit");
}

WstEssRMgrOps::WstEssRMgrOps(int logCategory) {
    mUserData = NULL;
    mResPriority = 0;
    mResAssignedId = -1;
    mResUsage = DEFAULT_USAGE;
    mEssRmgr = NULL;
    mLogCategory = logCategory;
    mUserCallback = NULL;
};

WstEssRMgrOps::~WstEssRMgrOps() {
};

void WstEssRMgrOps::setCallback(essMgrCallback essCallback, void *userData)
{
    mUserData = userData;
    mUserCallback = essCallback;
}

void WstEssRMgrOps::resMgrInit()
{
    mEssRmgr = EssRMgrCreate();
    if (!mEssRmgr) {
        ERROR(mLogCategory,"EssRMgrCreate failed");
        return;
    }
    memset( &mResCurrCaps, 0, sizeof(EssRMgrCaps) );
    INFO(mLogCategory,"resMgrInit,ok");
    return;
}

void WstEssRMgrOps::resMgrTerm()
{
    if (mEssRmgr) {
        EssRMgrDestroy(mEssRmgr);
        mEssRmgr = NULL;
        memset( &mResCurrCaps, 0, sizeof(EssRMgrCaps) );
    }
    INFO(mLogCategory,"resMgrTerm, ok");
}

void WstEssRMgrOps::resMgrRequestDecoder(bool pip)
{
    bool result;

    if (pip) {
        mResUsage = 0;
    }

    mResReq.type = EssRMgrResType_videoDecoder;
    mResReq.priority = mResPriority;
    mResReq.usage = mResUsage;
    mResReq.info.video.maxWidth= DEFAULT_WINDOW_WIDTH;
    mResReq.info.video.maxHeight= DEFAULT_WINDOW_HEIGHT;
    mResReq.asyncEnable = true;
    mResReq.notifyCB = WstEssRMgrOps::resMgrNotify;
    mResReq.notifyUserData = this;

    if (!mEssRmgr)
    {
        WARNING(mLogCategory,"EssRMgrCreate failed");
        return;
    }

    DEBUG(mLogCategory,"request resource resUsage %d priority %X", mResUsage, mResPriority);
    result = EssRMgrRequestResource(mEssRmgr, EssRMgrResType_videoDecoder, &mResReq);
    if ( result )
    {
        if ( mResReq.assignedId >= 0 )
        {
            DEBUG(mLogCategory,"assigned id %d caps %X", mResReq.assignedId, mResReq.assignedCaps);
            mResAssignedId = mResReq.assignedId;
            memset( &mResCurrCaps, 0, sizeof(EssRMgrCaps) );
            if ( !EssRMgrResourceGetCaps( mEssRmgr, EssRMgrResType_videoDecoder, mResAssignedId, &mResCurrCaps ) )
            {
               ERROR(mLogCategory,"resMgrRequestDecoder: failed to get caps of assigned decoder");
            }
            DEBUG(mLogCategory,"done assigned id %d caps %X (%dx%d)",
                    mResAssignedId,
                    mResCurrCaps.capabilities,
                    mResCurrCaps.info.video.maxWidth,
                    mResCurrCaps.info.video.maxHeight);
        }
        else
        {
            DEBUG(mLogCategory,"async grant pending" );
        }
    }
    else
    {
        ERROR(mLogCategory,"resMgrRequestDecoder: request failed");
    }
}

void WstEssRMgrOps::resMgrReleaseDecoder()
{
    if (!mEssRmgr)
    {
        WARNING(mLogCategory,"EssRMgrCreate failed");
        return;
    }
    DEBUG(mLogCategory,"EssRMgrReleaseResource assigned id %d",mResAssignedId);
    if (mResAssignedId >= 0) {
        EssRMgrReleaseResource( mEssRmgr, EssRMgrResType_videoDecoder, mResAssignedId );
        DEBUG(mLogCategory,"EssRMgrReleaseResource done");
        mResReq.assignedId = -1;
        mResAssignedId = -1;
    }
}

void WstEssRMgrOps::resMgrUpdateState(int state)
{
    if (!mEssRmgr)
    {
        WARNING(mLogCategory,"EssRMgrCreate failed");
        return;
    }
    if ( mResAssignedId >= 0 )
    {
        EssRMgrResourceSetState( mEssRmgr, EssRMgrResType_videoDecoder, mResAssignedId, state );
    }
}
