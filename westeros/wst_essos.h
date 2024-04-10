#ifndef _WST_ESSOS_H_
#define _WST_ESSOS_H_

#include <mutex>
#include "essos-resmgr.h"

class WstClientPlugin;

typedef void (*essMgrCallback)(int event, void *userData);

class WstEssRMgrOps {
public:
   WstEssRMgrOps(int logCategory);
   virtual ~WstEssRMgrOps();

   void setCallback(essMgrCallback essCallback, void *userData);
   void resMgrInit();
   void resMgrTerm();
   void resMgrRequestDecoder(bool pip);
   void resMgrReleaseDecoder();
   void resMgrUpdateState(int state);

   static void resMgrNotify( EssRMgr *rm, int event, int type, int id, void* userData );
private:
   int mLogCategory;
   void *mUserData;
   EssRMgr *mEssRmgr;
   uint mResPriority;
   uint mResUsage;
   int mResAssignedId;
   EssRMgrCaps mResCurrCaps;
   EssRMgrRequest mResReq;

   essMgrCallback mUserCallback;
   std::mutex mLock;
};
#endif //_WST_ESSOS_H_
