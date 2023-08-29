#ifndef _WST_ESSOS_H_
#define _WST_ESSOS_H_

typedef void EssRMgr;

typedef enum _EssRMgrAudioUsage
{
   EssRMgrAudUse_none=                (0),
} EssRMgrAudioUsage;

typedef enum _EssRMgrResType
{
   EssRMgrResType_videoDecoder,
   EssRMgrResType_audioDecoder,
   EssRMgrResType_frontEnd,
   EssRMgrResType_svpAllocator
} EssRMgrResType;

typedef enum _EssRMgrResState
{
   EssRMgrRes_idle= 0,
   EssRMgrRes_paused= 1,
   EssRMgrRes_active= 2,
   EssRMgrRes_max= 3
} EssRMgrResState;

typedef enum _EssRMgrEvent
{
   /*
    * Received when a resource is granted asynchronusly.
    */
   EssRMgrEvent_granted,

   /*
    * Received when a resource is being revoked.  When received,
    * the application should release the resource and then call
    * EssRMgrReleaseResource.
    */
   EssRMgrEvent_revoked
} EssRMgrEvent;

typedef enum _EssRMgrVideoUsage
{
   EssRMgrVidUse_none=                (0),
   EssRMgrVidUse_fullResolution=      (1<<0),
   EssRMgrVidUse_fullQuality=         (1<<1),
   EssRMgrVidUse_fullPerformance=     (1<<2),

} EssRMgrVideoUsage;


typedef void (*EssRMgrNotifyCB)(EssRMgr *rm, int event, int type, int id, void* userData);

typedef struct _EssRMgrVideoInfo
{
   int maxWidth;
   int maxHeight;
} EssRMgrVideoInfo;

typedef struct _EssRMgrAudioInfo
{
} EssRMgrAudioInfo;

typedef struct _EssRMgrFEInfo
{
} EssRMgrFEInfo;

typedef struct _EssRMgrSVPAInfo
{
} EssRMgrSVPAInfo;

typedef union EssRMgrUsageInfo
{
   EssRMgrVideoInfo video;
   EssRMgrAudioInfo audio;
   EssRMgrFEInfo frontEnd;
   EssRMgrSVPAInfo svpAllocator;
} EssRMgrUsageInfo;

typedef struct _EssRMgrRequest
{
   int type;
   int usage;
   int priority;
   bool asyncEnable;
   EssRMgrNotifyCB notifyCB;
   void *notifyUserData;
   EssRMgrUsageInfo info;
   int requestId;
   int assignedId;
   int assignedCaps;
} EssRMgrRequest;

typedef struct _EssRMgrCaps
{
   int capabilities;
   EssRMgrUsageInfo info;
} EssRMgrCaps;

typedef struct _EssRMgrUsage
{
   int usage;
   EssRMgrUsageInfo info;
} EssRMgrUsage;

typedef EssRMgr* (*essRMgrCreate)(void);
typedef void (*essRMgrDestroy)(EssRMgr *rm);
typedef bool (*essRMgrRequestResource)(EssRMgr *rm, int type, EssRMgrRequest *req);
typedef void (*essRMgrReleaseResource)(EssRMgr *rm, int type, int id);
typedef void (*essRMgrDumpStates)(EssRMgr *rm);
typedef bool (*essRMgrResourceGetCaps)( EssRMgr *rm, int type, int id, EssRMgrCaps *caps);
typedef bool (*essRMgrGetPolicyPriorityTie)( EssRMgr *rm );
typedef bool (*essRMgrResourceSetState)( EssRMgr *rm, int type, int id, int state );

class WstClientPlugin;

class WstEssRMgrOps {
public:
   WstEssRMgrOps(WstClientPlugin *plugin);
   virtual ~WstEssRMgrOps();

   void resMgrInit();
   void resMgrTerm();
   void resMgrRequestDecoder(bool pip);
   void resMgrReleaseDecoder();
   void resMgrUpdateState(int state);

   static void resMgrNotify( EssRMgr *rm, int event, int type, int id, void* userData );
private:
   WstClientPlugin *mPlugin;
   void *mLibHandle;
   EssRMgr *mEssRmgr;
   uint mResPriority;
   uint mResUsage;
   int mResAssignedId;
   EssRMgrCaps mResCurrCaps;
   EssRMgrRequest mResReq;

   essRMgrCreate EssRMgrCreate;
   essRMgrDestroy EssRMgrDestroy;
   essRMgrRequestResource EssRMgrRequestResource;
   essRMgrReleaseResource EssRMgrReleaseResource;
   essRMgrDumpStates EssRMgrDumpState;
   essRMgrResourceGetCaps EssRMgrResourceGetCaps;
   essRMgrGetPolicyPriorityTie EssRMgrGetPolicyPriorityTie;
   essRMgrResourceSetState EssRMgrResourceSetState;
};
#endif //_WST_ESSOS_H_
