/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
//btrCore.c
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef UNIT_TEST
#define STATIC static
#else 
#define STATIC 
#define STREAM_IN_SUPPORTED
#endif
/* System Headers */
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>     //for strtoll
#include <unistd.h>     //for getpid
#include <sched.h>      //for StopDiscovery test
#include <string.h>     //for strcnp
#include <errno.h>      //for error numbers
#include <stdint.h>

/* Ext lib Headers */
#include <glib.h>

/* Interface lib Headers */
#include "btrCore_logger.h"
#include "safec_lib.h"

/* Local Headers */
#include "btrCore.h"
#include "btrCore_service.h"

#include "btrCore_avMedia.h"
#include "btrCore_le.h"

#include "btrCore_bt_ifce.h"


#ifdef RDK_LOGGER_ENABLED
int b_rdk_logger_enabled = 0;
#endif

/*Local defines*/
#define BTRCORE_LOW_BATTERY_THRESHOLD 10
#define BTRCORE_LOW_BATTERY_REFRESH_INTERVAL 60
#define BTRCORE_BATTERY_REFRESH_INTERVAL 300
#define BATTERY_LEVEL_RETRY_ATTEMPTS 6
#define BATTERY_LEVEL_NOT_FOUND_REFRESH_INTERVAL 5

#define BTRCORE_REMOTE_CONTROL_APPEARANCE 0x0180
#define BTRCORE_LE_HID_DEVICE_APPEARANCE 0x03c4
#define BTRCORE_REMOTE_OUI_LENGTH 8
#define BTRCORE_AMAZON_OUI_LENGTH 8
#define BTRCORE_GOOGLE_OUI_LENGTH 8

static char * BTRCORE_REMOTE_OUI_VALUES[] = {
    "20:44:41", //LC103
    "E8:0F:C8", //EC302
    "98:06:3A", //EC201
    "18:46:44", //EC201
    "20:E7:B6", //Platco
    "E4:A6:34", //Platco
    "B8:F2:55", //Platco
    "1C:41:90", //Platco
    "B4:CB:B8",  //P-Xumo XR100
    NULL
};

static char * BTRCORE_AMAZON_OUI_VALUES[] = {
    "20:A1:71", //Luna Gamepad
    "EC:FD:86", //AmazonController-0B0
    NULL
};

static char * BTRCORE_GOOGLE_OUI_VALUES[] = {
    "61:47:AA", //Stadia8ZSF-5e22
    "EF:6F:4D", //StadiaJVRN-58e0
    "CA:7B:25", //Stadia2TFX-0fa6
    NULL
};
/* Local types */
//TODO: Move to a private header
typedef enum _enBTRCoreTaskOp {
    enBTRCoreTaskOpStart,
    enBTRCoreTaskOpStop,
    enBTRCoreTaskOpIdle,
    enBTRCoreTaskOpProcess,
    enBTRCoreTaskOpExit,
    enBTRCoreTaskOpUnknown
} enBTRCoreTaskOp;

typedef enum _enBTRCoreTaskProcessType {
    enBTRCoreTaskPTcBAdapterStatus,
    enBTRCoreTaskPTcBDeviceDisc,
    enBTRCoreTaskPTcBDeviceRemoved,
    enBTRCoreTaskPTcBDeviceLost,
    enBTRCoreTaskPTcBDeviceStatus,
    enBTRCoreTaskPTcBMediaStatus,
    enBTRCoreTaskPTcBDevOpInfoStatus,
    enBTRCoreTaskPTcBConnIntim,
    enBTRCoreTaskPTcBConnAuth,
    enBTRCoreTaskPTcBModaliasUpdate,
    enBTRCoreTaskPTUnknown
} enBTRCoreTaskProcessType;


typedef struct _stBTRCoreTaskGAqData {
    enBTRCoreTaskOp             enBTRCoreTskOp;
    enBTRCoreTaskProcessType    enBTRCoreTskPT;
    void*                       pvBTRCoreTskInData;
} stBTRCoreTaskGAqData;


typedef struct _stBTRCoreOTskInData {
    tBTRCoreDevId       bTRCoreDevId;
    enBTRCoreDeviceType enBTRCoreDevType;
    stBTDeviceInfo*     pstBTDevInfo;
} stBTRCoreOTskInData;


typedef struct _stBTRCoreDevStateInfo {
    enBTRCoreDeviceState    eDevicePrevState;
    enBTRCoreDeviceState    eDeviceCurrState;
} stBTRCoreDevStateInfo;


typedef struct _stBTRCoreHdl {

    tBTRCoreAVMediaHdl              avMediaHdl;
    tBTRCoreLeHdl                   leHdl;

    void*                           connHdl;
    char*                           agentPath;

    unsigned int                    numOfAdapters;
    char*                           adapterPath[BTRCORE_MAX_NUM_BT_ADAPTERS];
    char*                           adapterAddr[BTRCORE_MAX_NUM_BT_ADAPTERS];

    char*                           curAdapterPath;
    char*                           curAdapterAddr;

    unsigned int                    numOfScannedDevices;
    stBTRCoreBTDevice               stScannedDevicesArr[BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES];
    stBTRCoreDevStateInfo           stScannedDevStInfoArr[BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES];

    unsigned int                    numOfPairedDevices;
    stBTRCoreBTDevice               stKnownDevicesArr[BTRCORE_MAX_NUM_BT_DEVICES];
    stBTRCoreDevStateInfo           stKnownDevStInfoArr[BTRCORE_MAX_NUM_BT_DEVICES];


    stBTRCoreDiscoveryCBInfo        stDiscoveryCbInfo;
    stBTRCoreDevStatusCBInfo        stDevStatusCbInfo;
    stBTRCoreMediaStatusCBInfo      stMediaStatusCbInfo;
    stBTRCoreConnCBInfo             stConnCbInfo;

    fPtr_BTRCore_DeviceDiscCb       fpcBBTRCoreDeviceDisc;
    fPtr_BTRCore_StatusCb           fpcBBTRCoreStatus;
    fPtr_BTRCore_MediaStatusCb      fpcBBTRCoreMediaStatus;
    fPtr_BTRCore_ConnIntimCb        fpcBBTRCoreConnIntim; 
    fPtr_BTRCore_ConnAuthCb         fpcBBTRCoreConnAuth;

    void*                           pvcBDevDiscUserData;
    void*                           pvcBStatusUserData;
    void*                           pvcBMediaStatusUserData;
    void*                           pvcBConnIntimUserData;
    void*                           pvcBConnAuthUserData;


    GThread*                        pThreadRunTask;
    GAsyncQueue*                    pGAQueueRunTask;

    GThread*                        pThreadOutTask;
    GAsyncQueue*                    pGAQueueOutTask;
    enBTRCoreDeviceType             aenDeviceDiscoveryType;
    unsigned long long int          skipDeviceDiscUpdate;

    GMutex                          batteryLevelMutex;
    GThread *                       batteryLevelThread;
    unsigned short                  batteryLevelRefreshInterval;
    GCond                           batteryLevelCond;
    BOOLEAN                         batteryLevelThreadExit;
} stBTRCoreHdl;


/* Static Function Prototypes */
static void btrCore_InitDataSt (stBTRCoreHdl* apsthBTRCore);
static tBTRCoreDevId btrCore_GenerateUniqueDeviceID (const char* apcDeviceMac);
static BOOLEAN btrCore_IsDevNameSameAsAddress(const stBTRCoreBTDevice *dev);
static BOOLEAN btrCore_CheckLeHidConnectionStability(stBTRCoreBTDevice *DeviceInfo);
static void btrCore_RemoveUnstableDeviceFromActionList(stBTRCoreBTDevice *DeviceInfo);
static enBTRCoreDeviceClass btrCore_MapClassIDtoAVDevClass(unsigned int aui32ClassId);
static enBTRCoreDeviceClass btrCore_MapServiceClasstoDevType(unsigned int aui32ClassId);
static enBTRCoreDeviceClass btrCore_MapClassIDtoDevClass(unsigned int aui32ClassId);
static enBTRCoreDeviceType btrCore_MapClassIDToDevType(unsigned int aui32ClassId, enBTDeviceType aeBtDeviceType);
static enBTRCoreDeviceType btrCore_MapDevClassToDevType(enBTRCoreDeviceClass aenBTRCoreDevCl);
static void btrCore_ClearScannedDevicesList (stBTRCoreHdl* apsthBTRCore);
static int btrCore_AddDeviceToScannedDevicesArr (stBTRCoreHdl* apsthBTRCore, stBTDeviceInfo* apstBTDeviceInfo, stBTRCoreBTDevice* apstFoundDevice); 
static enBTRCoreRet btrCore_RemoveDeviceFromScannedDevicesArr (stBTRCoreHdl* apsthBTRCore, tBTRCoreDevId aBTRCoreDevId, stBTRCoreBTDevice* astRemovedDevice);
static int btrCore_AddDeviceToKnownDevicesArr (stBTRCoreHdl* apsthBTRCore, stBTDeviceInfo* apstBTDeviceInfo);
static enBTRCoreRet btrCore_RemoveDeviceFromKnownDevicesArr (stBTRCoreHdl* apstlhBTRCore, tBTRCoreDevId aBTRCoreDevId);
static enBTRCoreRet btrCore_PopulateModaliasValues (char * pcModalias, unsigned int * ui32ModVendor, unsigned int * ui32ModProduct, unsigned int * ui32ModDevice);
static enBTRCoreRet btrCore_PopulateListOfPairedDevices(stBTRCoreHdl* apsthBTRCore, const char* pAdapterPath);
static void btrCore_MapKnownDeviceListFromPairedDeviceInfo (stBTRCoreBTDevice* knownDevicesArr, stBTPairedDeviceInfo* pairedDeviceInfo);
static const char* btrCore_GetScannedDeviceAddress (stBTRCoreHdl* apsthBTRCore, tBTRCoreDevId aBTRCoreDevId);
static const char* btrCore_GetKnownDeviceMac (stBTRCoreHdl* apsthBTRCore, tBTRCoreDevId aBTRCoreDevId);
static enBTRCoreRet btrCore_GetDeviceInfo (stBTRCoreHdl* apsthBTRCore, tBTRCoreDevId aBTRCoreDevId, enBTRCoreDeviceType aenBTRCoreDevType,
                                             enBTDeviceType* apenBTDeviceType, stBTRCoreBTDevice** appstBTRCoreBTDevice,
                                               stBTRCoreDevStateInfo** appstBTRCoreDevStateInfo, const char** appcBTRCoreBTDevicePath, const char** appcBTRCoreBTDeviceName);
static enBTRCoreRet btrCore_GetDeviceInfoKnown (stBTRCoreHdl* apsthBTRCore, tBTRCoreDevId aBTRCoreDevId, enBTRCoreDeviceType aenBTRCoreDevType,
                                                  enBTDeviceType* apenBTDeviceType, stBTRCoreBTDevice** appstBTRCoreBTDevice,
                                                    stBTRCoreDevStateInfo** appstBTRCoreDevStateInfo, const char** appcBTRCoreBTDevicePath);
static void btrCore_ShowSignalStrength (short strength);
static unsigned int btrCore_BTParseUUIDValue (const char *pUUIDString, char* pServiceNameOut);
static enBTRCoreDeviceState btrCore_BTParseDeviceState (const char* pcStateValue);
#ifndef LE_MODE
static eBTRCoreMedElementType btrCore_GetMediaElementType (eBTRCoreAVMElementType aeMediaElementType);
#endif

static enBTRCoreRet btrCore_RunTaskAddOp (GAsyncQueue* apRunTaskGAq, enBTRCoreTaskOp aenRunTaskOp, enBTRCoreTaskProcessType aenRunTaskPT, void* apvRunTaskInData);
static enBTRCoreRet btrCore_OutTaskAddOp (GAsyncQueue* apOutTaskGAq, enBTRCoreTaskOp aenOutTaskOp, enBTRCoreTaskProcessType aenOutTaskPT, void* apvOutTaskInData);
static enBTRCoreRet btrCore_updateBatteryLevelsForConnectedDevices ( stBTRCoreHdl* apsthBTRCore, unsigned char * ui8NumberDevicesAvailable, BOOLEAN * bLowBatteryDeviceFound);

/* Local Op Threads Prototypes */
static gpointer btrCore_RunTask (gpointer apsthBTRCore);
static gpointer btrCore_OutTask (gpointer apsthBTRCore);
static gpointer btrCore_BatteryLevelThread( gpointer apsthBTRCore);

/* Incoming Callbacks Prototypes */
STATIC  int btrCore_BTAdapterStatusUpdateCb (enBTAdapterProp aeBtAdapterProp, stBTAdapterInfo* apstBTAdapterInfo,  void* apUserData);
STATIC  int btrCore_BTDeviceStatusUpdateCb (enBTDeviceType aeBtDeviceType, enBTDeviceState aeBtDeviceState, stBTDeviceInfo* apstBTDeviceInfo,  void* apUserData);
STATIC  int btrCore_BTDeviceConnectionIntimationCb (enBTDeviceType  aeBtDeviceType, stBTDeviceInfo* apstBTDeviceInfo, unsigned int aui32devPassKey, unsigned char ucIsReqConfirmation, void* apUserData);
STATIC  int btrCore_BTDeviceAuthenticationCb (enBTDeviceType  aeBtDeviceType, stBTDeviceInfo* apstBTDeviceInfo, void* apUserData);
#ifndef LE_MODE
STATIC  enBTRCoreRet btrCore_BTMediaStatusUpdateCb (stBTRCoreAVMediaStatusUpdate* apMediaStreamStatus, const char*  apBtdevAddr, void* apUserData);
#endif
STATIC  enBTRCoreRet btrCore_BTLeStatusUpdateCb (stBTRCoreLeGattInfo* apstBtrLeInfo, const char*  apcBtdevAddr, void* apvUserData);

/* Static Function Definition */
static void
btrCore_InitDataSt (
    stBTRCoreHdl*   apsthBTRCore
) {
    int i;


    apsthBTRCore->avMediaHdl    = NULL;
    apsthBTRCore->leHdl         = NULL;
    apsthBTRCore->agentPath     = NULL;


    /* Current Adapter Path */
    apsthBTRCore->curAdapterPath = NULL;

    /* Current Adapter */
    apsthBTRCore->curAdapterAddr = NULL;
    apsthBTRCore->curAdapterAddr = (char*)g_malloc0(sizeof(char) * (BD_NAME_LEN + 1));
    MEMSET_S(apsthBTRCore->curAdapterAddr, sizeof(char) * (BD_NAME_LEN + 1), '\0', sizeof(char) * (BD_NAME_LEN + 1));

    /* Adapters */
    for (i = 0; i < BTRCORE_MAX_NUM_BT_ADAPTERS; i++) {
        apsthBTRCore->adapterPath[i] = NULL;
        apsthBTRCore->adapterPath[i] = (char*)g_malloc0(sizeof(char) * (BD_NAME_LEN + 1));
        MEMSET_S(apsthBTRCore->adapterPath[i], sizeof(char) * (BD_NAME_LEN + 1), '\0', sizeof(char) * (BD_NAME_LEN + 1));

        apsthBTRCore->adapterAddr[i] = NULL;
        apsthBTRCore->adapterAddr[i] = (char*)g_malloc0(sizeof(char) * (BD_NAME_LEN + 1));
        MEMSET_S(apsthBTRCore->adapterAddr[i], sizeof(char) * (BD_NAME_LEN + 1), '\0', sizeof(char) * (BD_NAME_LEN + 1));
    }

    /* Scanned Devices */
    for (i = 0; i < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES; i++) {
        apsthBTRCore->stScannedDevicesArr[i].tDeviceId          = 0;
        apsthBTRCore->stScannedDevicesArr[i].enDeviceType       = enBTRCore_DC_Unknown;
        apsthBTRCore->stScannedDevicesArr[i].bFound             = FALSE;
        apsthBTRCore->stScannedDevicesArr[i].bDeviceConnected   = FALSE;
        apsthBTRCore->stScannedDevicesArr[i].i32RSSI            = INT_MIN;

        MEMSET_S(apsthBTRCore->stScannedDevicesArr[i].pcDeviceName, sizeof(char) * (BD_NAME_LEN + 1),      '\0', sizeof(char) * (BD_NAME_LEN + 1));
        MEMSET_S(apsthBTRCore->stScannedDevicesArr[i].pcDeviceAddress, sizeof(char) * (BD_NAME_LEN + 1),   '\0', sizeof(char) * (BD_NAME_LEN + 1));
        MEMSET_S(apsthBTRCore->stScannedDevicesArr[i].pcDevicePath, sizeof(char) * (BD_NAME_LEN + 1),      '\0', sizeof(char) * (BD_NAME_LEN + 1));

        MEMSET_S(apsthBTRCore->stScannedDevicesArr[i].stAdServiceData, (BTRCORE_MAX_DEVICE_PROFILE * sizeof(stBTRCoreAdServiceData)), 0, (BTRCORE_MAX_DEVICE_PROFILE * sizeof(stBTRCoreAdServiceData)));

        apsthBTRCore->stScannedDevStInfoArr[i].eDevicePrevState = enBTRCoreDevStInitialized;
        apsthBTRCore->stScannedDevStInfoArr[i].eDeviceCurrState = enBTRCoreDevStInitialized;
    }

    apsthBTRCore->numOfScannedDevices = 0;
    apsthBTRCore->numOfPairedDevices  = 0;

    
    /* Known Devices */
    for (i = 0; i < BTRCORE_MAX_NUM_BT_DEVICES; i++) {
        apsthBTRCore->stKnownDevicesArr[i].tDeviceId            = 0;
        apsthBTRCore->stKnownDevicesArr[i].enDeviceType         = enBTRCore_DC_Unknown;
        apsthBTRCore->stKnownDevicesArr[i].bFound               = FALSE;
        apsthBTRCore->stKnownDevicesArr[i].bDeviceConnected     = FALSE;
        apsthBTRCore->stKnownDevicesArr[i].i32RSSI              = INT_MIN;

        MEMSET_S(apsthBTRCore->stKnownDevicesArr[i].pcDeviceName, sizeof(char) * (BD_NAME_LEN + 1),    '\0', sizeof(char) * (BD_NAME_LEN + 1));
        MEMSET_S(apsthBTRCore->stKnownDevicesArr[i].pcDeviceAddress, sizeof(char) * (BD_NAME_LEN + 1), '\0', sizeof(char) * (BD_NAME_LEN + 1));
        MEMSET_S(apsthBTRCore->stKnownDevicesArr[i].pcDevicePath, sizeof(char) * (BD_NAME_LEN + 1),    '\0', sizeof(char) * (BD_NAME_LEN + 1));

        MEMSET_S(apsthBTRCore->stKnownDevicesArr[i].stAdServiceData, (BTRCORE_MAX_DEVICE_PROFILE * sizeof(stBTRCoreAdServiceData)), 0, (BTRCORE_MAX_DEVICE_PROFILE * sizeof(stBTRCoreAdServiceData)));

        apsthBTRCore->stKnownDevStInfoArr[i].eDevicePrevState = enBTRCoreDevStInitialized;
        apsthBTRCore->stKnownDevStInfoArr[i].eDeviceCurrState = enBTRCoreDevStInitialized;
    }

    /* Callback Info */
    apsthBTRCore->stDevStatusCbInfo.eDevicePrevState = enBTRCoreDevStInitialized;
    apsthBTRCore->stDevStatusCbInfo.eDeviceCurrState = enBTRCoreDevStInitialized;


    MEMSET_S(&apsthBTRCore->stConnCbInfo, sizeof(stBTRCoreConnCBInfo), 0, sizeof(stBTRCoreConnCBInfo));

    apsthBTRCore->fpcBBTRCoreDeviceDisc     = NULL;
    apsthBTRCore->fpcBBTRCoreStatus         = NULL;
    apsthBTRCore->fpcBBTRCoreMediaStatus    = NULL;
    apsthBTRCore->fpcBBTRCoreConnIntim      = NULL;
    apsthBTRCore->fpcBBTRCoreConnAuth       = NULL;

    apsthBTRCore->pvcBDevDiscUserData       = NULL;
    apsthBTRCore->pvcBStatusUserData        = NULL;
    apsthBTRCore->pvcBMediaStatusUserData   = NULL;
    apsthBTRCore->pvcBConnIntimUserData     = NULL;
    apsthBTRCore->pvcBConnAuthUserData      = NULL;

    apsthBTRCore->pThreadRunTask            = NULL;
    apsthBTRCore->pGAQueueRunTask           = NULL;
                     
    apsthBTRCore->pThreadOutTask            = NULL;
    apsthBTRCore->pGAQueueOutTask           = NULL;

    /* Always safer to initialze Global variables, init if any left or added */
}


static tBTRCoreDevId
btrCore_GenerateUniqueDeviceID (
    const char* apcDeviceMac
) {
    tBTRCoreDevId   lBTRCoreDevId = 0;
    char            lcDevHdlArr[13] = {'\0'};

    // MAC Address Format 
    // AA:BB:CC:DD:EE:FF\0
    if (apcDeviceMac && (strlen(apcDeviceMac) >= 17)) {
        lcDevHdlArr[0]  = apcDeviceMac[0];
        lcDevHdlArr[1]  = apcDeviceMac[1];
        lcDevHdlArr[2]  = apcDeviceMac[3];
        lcDevHdlArr[3]  = apcDeviceMac[4];
        lcDevHdlArr[4]  = apcDeviceMac[6];
        lcDevHdlArr[5]  = apcDeviceMac[7];
        lcDevHdlArr[6]  = apcDeviceMac[9];
        lcDevHdlArr[7]  = apcDeviceMac[10];
        lcDevHdlArr[8]  = apcDeviceMac[12];
        lcDevHdlArr[9]  = apcDeviceMac[13];
        lcDevHdlArr[10] = apcDeviceMac[15];
        lcDevHdlArr[11] = apcDeviceMac[16];

        lBTRCoreDevId = (tBTRCoreDevId) strtoll(lcDevHdlArr, NULL, 16);
    }

    return lBTRCoreDevId;
}

static void btrCore_RemoveUnstableDeviceFromActionList (
    stBTRCoreBTDevice *DeviceInfo
) {
    char lcpAddDeviceCmd[BT_MAX_STR_LEN/2] = {'\0'};
    char lcBtmgmtResponse[BT_MAX_STR_LEN/2] = {'\0'};
    FILE* lpcBtmgmtCmd = NULL;
    snprintf(lcpAddDeviceCmd, BT_MAX_STR_LEN/2, "btmgmt add-device -t 1 -a 0 %s", DeviceInfo->pcDeviceAddress);
    BTRCORELOG_INFO ("lcpAddDeviceCmd: %s\n", lcpAddDeviceCmd);
    lpcBtmgmtCmd = popen(lcpAddDeviceCmd, "r");
    if ((lpcBtmgmtCmd == NULL)) {
        BTRCORELOG_ERROR ("Failed to run lcpAddDeviceCmd command\n");
    }
    else {
        if (fgets(lcBtmgmtResponse, sizeof(lcBtmgmtResponse)-1, lpcBtmgmtCmd) == NULL) {
            BTRCORELOG_ERROR ("Failed to Output of lcpAddDeviceCmd\n");
        }
        else {
            BTRCORELOG_WARN ("Output of lcpAddDeviceCmd =  %s\n", lcBtmgmtResponse);
        }
        pclose(lpcBtmgmtCmd);
    }
}

static BOOLEAN
btrCore_CheckLeHidConnectionStability (
    stBTRCoreBTDevice *DeviceInfo
) {
    DeviceInfo->last_disconnect_ts = DeviceInfo->disconnect_ts;
    DeviceInfo->disconnect_ts = g_get_monotonic_time();

    if (DeviceInfo->last_disconnect_ts > 0 && ((DeviceInfo->disconnect_ts - DeviceInfo->last_disconnect_ts) < G_TIME_SPAN_SECOND)) {
        BTRCORELOG_ERROR("HID device detected as unstable; scheduling for removal from action list\n");
        return TRUE;
    }
    return FALSE;
}

static BOOLEAN
btrCore_IsDevNameSameAsAddress (
    const stBTRCoreBTDevice *dev
) {
    const char *name = &dev->pcDeviceName[0];
    const char *mac = &dev->pcDeviceAddress[0];

    // comparing
    //   "14:CB:65:F9:57:FC" with
    //   "14-CB-65-F9-57-FC"
    #define btrCore_ByteCompare(A) (name[A] == mac[A])

    BOOLEAN same =
      btrCore_ByteCompare(0) && btrCore_ByteCompare(1) &&
      // skip 2
      btrCore_ByteCompare(3) && btrCore_ByteCompare(4) &&
      // skip 5
      btrCore_ByteCompare(6) && btrCore_ByteCompare(7) &&
      // skip 8
      btrCore_ByteCompare(9) && btrCore_ByteCompare(10) &&
      // skip 11
      btrCore_ByteCompare(12) && btrCore_ByteCompare(13) &&
      // skip 14
      btrCore_ByteCompare(15) && btrCore_ByteCompare(16);

    return same;
}

static enBTRCoreDeviceClass
btrCore_MapClassIDtoAVDevClass (
    unsigned int aui32ClassId
) {
    enBTRCoreDeviceClass rc = enBTRCore_DC_Unknown;

    if (((aui32ClassId & 0x400u) == 0x400u) || ((aui32ClassId & 0x200u) == 0x200u) || ((aui32ClassId & 0x100u) == 0x100u)) {
        unsigned int ui32DevClassID = aui32ClassId & 0xFFFu;
        BTRCORELOG_TRACE ("ui32DevClassID = 0x%x\n", ui32DevClassID);

        if (ui32DevClassID == enBTRCore_DC_Tablet) {
            BTRCORELOG_INFO ("Its a enBTRCore_DC_Tablet\n");
            rc = enBTRCore_DC_Tablet;
        }
        else if (ui32DevClassID == enBTRCore_DC_SmartPhone) {
            BTRCORELOG_INFO ("enBTRCore_DC_SmartPhone\n");
            rc = enBTRCore_DC_SmartPhone;
        }
        else if (ui32DevClassID == enBTRCore_DC_WearableHeadset) {
            BTRCORELOG_INFO ("enBTRCore_DC_WearableHeadset\n");
            rc = enBTRCore_DC_WearableHeadset;
        }
        else if (ui32DevClassID == enBTRCore_DC_Handsfree) {
            BTRCORELOG_INFO ("enBTRCore_DC_Handsfree\n");
            rc = enBTRCore_DC_Handsfree;
        }
        else if (ui32DevClassID == enBTRCore_DC_Reserved) {
            BTRCORELOG_INFO ("enBTRCore_DC_Reserved\n");
            rc = enBTRCore_DC_Reserved;
        }
        else if (ui32DevClassID == enBTRCore_DC_Microphone) {
            BTRCORELOG_INFO ("enBTRCore_DC_Microphone\n");
            rc = enBTRCore_DC_Microphone;
        }
        else if (ui32DevClassID == enBTRCore_DC_Loudspeaker) {
            BTRCORELOG_INFO ("enBTRCore_DC_Loudspeaker\n");
            rc = enBTRCore_DC_Loudspeaker;
        }
        else if (ui32DevClassID == enBTRCore_DC_Headphones) {
            BTRCORELOG_INFO ("enBTRCore_DC_Headphones\n");
            rc = enBTRCore_DC_Headphones;
        }
        else if (ui32DevClassID == enBTRCore_DC_HID_AudioRemote) {
            BTRCORELOG_INFO ("enBTRCore_DC_HID_AudioRemote\n");
            rc = enBTRCore_DC_HID_AudioRemote;
        }
        else if (ui32DevClassID == enBTRCore_DC_PortableAudio) {
            BTRCORELOG_INFO ("enBTRCore_DC_PortableAudio\n");
            rc = enBTRCore_DC_PortableAudio;
        }
        else if (ui32DevClassID == enBTRCore_DC_CarAudio) {
            BTRCORELOG_INFO ("enBTRCore_DC_CarAudio\n");
            rc = enBTRCore_DC_CarAudio;
        }
        else if (ui32DevClassID == enBTRCore_DC_STB) {
            BTRCORELOG_INFO ("enBTRCore_DC_STB\n");
            rc = enBTRCore_DC_STB;
        }
        else if (ui32DevClassID == enBTRCore_DC_HIFIAudioDevice) {
            BTRCORELOG_INFO ("enBTRCore_DC_HIFIAudioDevice\n");
            rc = enBTRCore_DC_HIFIAudioDevice;
        }
        else if (ui32DevClassID == enBTRCore_DC_VCR) {
            BTRCORELOG_INFO ("enBTRCore_DC_VCR\n");
            rc = enBTRCore_DC_VCR;
        }
        else if (ui32DevClassID == enBTRCore_DC_VideoCamera) {
            BTRCORELOG_INFO ("enBTRCore_DC_VideoCamera\n");
            rc = enBTRCore_DC_VideoCamera;
        }
        else if (ui32DevClassID == enBTRCore_DC_Camcoder) {
            BTRCORELOG_INFO ("enBTRCore_DC_Camcoder\n");
            rc = enBTRCore_DC_Camcoder;
        }
        else if (ui32DevClassID == enBTRCore_DC_VideoMonitor) {
            BTRCORELOG_INFO ("enBTRCore_DC_VideoMonitor\n");
            rc = enBTRCore_DC_VideoMonitor;
        }
        else if (ui32DevClassID == enBTRCore_DC_TV) {
            BTRCORELOG_INFO ("enBTRCore_DC_TV\n");
            rc = enBTRCore_DC_TV;
        }
        else if (ui32DevClassID == enBTRCore_DC_VideoConference) {
            BTRCORELOG_INFO ("enBTRCore_DC_VideoConference\n");
            rc = enBTRCore_DC_TV;
        }
    }

    return rc;
}

static enBTRCoreDeviceClass
btrCore_MapServiceClasstoDevType (
    unsigned int aui32ClassId
) {
    enBTRCoreDeviceClass rc = enBTRCore_DC_Unknown;

    /* Refer https://www.bluetooth.com/specifications/assigned-numbers/baseband
     * The bit 18 set to represent AUDIO OUT service Devices.
     * The bit 19 can be set to represent AUDIO IN Service devices
     * The bit 21 set to represent AUDIO Services (Mic, Speaker, headset).
     * The bit 22 set to represent Telephone Services (headset).
     */

    if (0x40000u & aui32ClassId) {
        BTRCORELOG_TRACE ("Its a Rendering Class of Service.\n");
        if ((rc = btrCore_MapClassIDtoAVDevClass(aui32ClassId)) == enBTRCore_DC_Unknown) {
            BTRCORELOG_TRACE ("Its a Rendering Class of Service. But no Audio Device Class defined\n");
        }
    }
    else if (0x80000u & aui32ClassId) {
        BTRCORELOG_TRACE ("Its a Capturing Service.\n");
        if ((rc = btrCore_MapClassIDtoAVDevClass(aui32ClassId)) == enBTRCore_DC_Unknown) {
            BTRCORELOG_TRACE ("Its a Capturing Service. But no Audio Device Class defined\n");
        }
    }
    else if (0x200000u & aui32ClassId) {
        BTRCORELOG_TRACE ("Its a Audio Class of Service.\n");
        if ((rc = btrCore_MapClassIDtoAVDevClass(aui32ClassId)) == enBTRCore_DC_Unknown) {
            BTRCORELOG_TRACE ("Its a Audio Class of Service. But no Audio Device Class defined; Lets assume its Loud Speaker\n");
            rc = enBTRCore_DC_Loudspeaker;
        }
    }
    else if (0x400000u & aui32ClassId) {
        BTRCORELOG_TRACE ("Its a Telephony Class of Service. So, enBTDevAudioSink\n");
        if ((rc = btrCore_MapClassIDtoAVDevClass(aui32ClassId)) == enBTRCore_DC_Unknown) {
            BTRCORELOG_TRACE ("Its a Telephony Class of Service. But no Audio Device Class defined;\n");
        }
    }

    return rc;
}

static enBTRCoreDeviceClass
btrCore_MapClassIDtoDevClass (
    unsigned int aui32ClassId
) {
    enBTRCoreDeviceClass rc = enBTRCore_DC_Unknown;
    BTRCORELOG_TRACE ("classID = 0x%x\n", aui32ClassId);

    if (aui32ClassId == enBTRCore_DC_Tile) {
        BTRCORELOG_INFO ("enBTRCore_DC_Tile\n");
        rc = enBTRCore_DC_Tile;
    }

    if (rc == enBTRCore_DC_Unknown)
        rc = btrCore_MapServiceClasstoDevType(aui32ClassId);

    /* If the Class of Service is not audio, lets parse the COD */
    if (rc == enBTRCore_DC_Unknown) {
        if ((aui32ClassId & 0x500u) == 0x500u) {
            unsigned int ui32DevClassID = aui32ClassId & 0xFFFu;
            BTRCORELOG_TRACE ("ui32DevClassID = 0x%x\n", ui32DevClassID);

            if (ui32DevClassID == enBTRCore_DC_HID_Keyboard) {
                BTRCORELOG_DEBUG ("Its a enBTRCore_DC_HID_Keyboard\n");
                rc = enBTRCore_DC_HID_Keyboard;
            }
            else if (ui32DevClassID == enBTRCore_DC_HID_Mouse) {
                BTRCORELOG_DEBUG ("Its a enBTRCore_DC_HID_Mouse\n");
                rc = enBTRCore_DC_HID_Mouse;
            }
            else if (ui32DevClassID == enBTRCore_DC_HID_MouseKeyBoard) {
                BTRCORELOG_DEBUG ("Its a enBTRCore_DC_HID_MouseKeyBoard\n");
                rc = enBTRCore_DC_HID_MouseKeyBoard;
            }
            else if (ui32DevClassID == enBTRCore_DC_HID_AudioRemote) {
                BTRCORELOG_DEBUG ("Its a enBTRCore_DC_HID_AudioRemote\n");
                rc = enBTRCore_DC_HID_AudioRemote;
            }
            else if (ui32DevClassID == enBTRCore_DC_HID_Joystick) {
                BTRCORELOG_DEBUG ("Its a enBTRCore_DC_HID_Joystick\n");
                rc = enBTRCore_DC_HID_Joystick;
            }
            else if (ui32DevClassID == enBTRCore_DC_HID_GamePad) {
                BTRCORELOG_DEBUG ("Its a enBTRCore_DC_HID_GamePad\n");
                rc = enBTRCore_DC_HID_GamePad;
            }
        }
        else
        {
            rc = btrCore_MapClassIDtoAVDevClass(aui32ClassId);
        }
    }

    return rc;
}

static enBTRCoreDeviceType
btrCore_MapClassIDToDevType (
    unsigned int    aui32ClassId,
    enBTDeviceType  aeBtDeviceType
) {
    enBTRCoreDeviceType  lenBTRCoreDevType  = enBTRCoreUnknown;
    enBTRCoreDeviceClass lenBTRCoreDevCl    = enBTRCore_DC_Unknown;

    switch (aeBtDeviceType) {
    case enBTDevAudioSink:
        lenBTRCoreDevCl = btrCore_MapClassIDtoDevClass(aui32ClassId);
        if (lenBTRCoreDevCl == enBTRCore_DC_WearableHeadset) {
           lenBTRCoreDevType =  enBTRCoreHeadSet;
        }
        else if (lenBTRCoreDevCl == enBTRCore_DC_Headphones) {
           lenBTRCoreDevType = enBTRCoreHeadSet;
        }
        else if (lenBTRCoreDevCl == enBTRCore_DC_Loudspeaker) {
           lenBTRCoreDevType = enBTRCoreSpeakers;
        }
        else if (lenBTRCoreDevCl == enBTRCore_DC_HIFIAudioDevice) {
           lenBTRCoreDevType = enBTRCoreSpeakers;
        }
        else {
           lenBTRCoreDevType = enBTRCoreSpeakers;
        }
        break;
    case enBTDevAudioSource:
        lenBTRCoreDevCl = btrCore_MapClassIDtoDevClass(aui32ClassId);
        if (lenBTRCoreDevCl == enBTRCore_DC_SmartPhone) {
           lenBTRCoreDevType =  enBTRCoreMobileAudioIn;
        }
        else if (lenBTRCoreDevCl == enBTRCore_DC_Tablet) {
           lenBTRCoreDevType = enBTRCorePCAudioIn;
        }
        else {
           lenBTRCoreDevType = enBTRCoreMobileAudioIn;
        }
        break;
    case enBTDevHFPHeadset:
        lenBTRCoreDevType =  enBTRCoreHeadSet;
        break;
    case enBTDevHFPAudioGateway:
        lenBTRCoreDevType =  enBTRCoreHeadSet;
        break;
    case enBTDevLE:
        lenBTRCoreDevType = enBTRCoreLE;
        break;
    case enBTDevHID:
        lenBTRCoreDevType = enBTRCoreHID;
        break;
    case enBTDevUnknown:
    default:
        lenBTRCoreDevType = enBTRCoreUnknown;
        break;
    }

    return lenBTRCoreDevType;
}


static enBTRCoreDeviceType
btrCore_MapDevClassToDevType (
    enBTRCoreDeviceClass    aenBTRCoreDevCl
) {
    enBTRCoreDeviceType  lenBTRCoreDevType  = enBTRCoreUnknown;

    if (aenBTRCoreDevCl == enBTRCore_DC_WearableHeadset) {
       lenBTRCoreDevType =  enBTRCoreHeadSet;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_Headphones) {
       lenBTRCoreDevType = enBTRCoreHeadSet;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_Loudspeaker) {
       lenBTRCoreDevType = enBTRCoreSpeakers;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_HIFIAudioDevice) {
       lenBTRCoreDevType = enBTRCoreSpeakers;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_PortableAudio) {
       lenBTRCoreDevType = enBTRCoreSpeakers;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_CarAudio) {
       lenBTRCoreDevType = enBTRCoreSpeakers;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_SmartPhone) {
       lenBTRCoreDevType =  enBTRCoreMobileAudioIn;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_Tablet) {
       lenBTRCoreDevType = enBTRCorePCAudioIn;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_HID_Keyboard) {
       lenBTRCoreDevType = enBTRCoreHID;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_HID_Mouse) {
       lenBTRCoreDevType = enBTRCoreHID;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_HID_MouseKeyBoard) {
       lenBTRCoreDevType = enBTRCoreHID;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_HID_AudioRemote) {
       lenBTRCoreDevType = enBTRCoreHID;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_HID_Joystick) {
       lenBTRCoreDevType = enBTRCoreHID;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_HID_GamePad) {
       lenBTRCoreDevType = enBTRCoreHID;
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_Tile) {
        lenBTRCoreDevType = enBTRCoreLE;
        //TODO: May be use should have AudioDeviceClass & LE DeviceClass 
        //      will help us to identify the device Type as LE
    }
    else if (aenBTRCoreDevCl == enBTRCore_DC_XBB) {
        lenBTRCoreDevType = enBTRCoreLE;
    }
    else {
        lenBTRCoreDevType = enBTRCoreUnknown; 
    }

    return lenBTRCoreDevType;
}

#ifndef LE_MODE
static eBTRCoreMedElementType
btrCore_GetMediaElementType (
    eBTRCoreAVMElementType  aeMediaElementType
) {
    eBTRCoreMedElementType lenMedElementType;

    switch (aeMediaElementType) {
    case eBTRCoreAVMETypeAlbum:
        lenMedElementType = enBTRCoreMedETypeAlbum;
        break;
    case eBTRCoreAVMETypeArtist:
        lenMedElementType = enBTRCoreMedETypeArtist;
        break;
    case eBTRCoreAVMETypeGenre:
        lenMedElementType = enBTRCoreMedETypeGenre;
        break;
    case eBTRCoreAVMETypeCompilation:
        lenMedElementType = enBTRCoreMedETypeCompilation;
        break;
    case eBTRCoreAVMETypePlayList:
        lenMedElementType = enBTRCoreMedETypePlayList;
        break;
    case eBTRCoreAVMETypeTrackList:
        lenMedElementType = enBTRCoreMedETypeTrackList;
        break;
    case eBTRCoreAVMETypeTrack:
        lenMedElementType = enBTRCoreMedETypeTrack;
        break;
    default:
        lenMedElementType = enBTRCoreMedETypeTrackList;
    }

    return lenMedElementType;
}
#endif

static void
btrCore_ClearScannedDevicesList (
    stBTRCoreHdl* apsthBTRCore
) {
    int i;

    for (i = 0; i < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES; i++) {
        apsthBTRCore->stScannedDevicesArr[i].tDeviceId          = 0;
        apsthBTRCore->stScannedDevicesArr[i].i32RSSI            = INT_MIN;
        apsthBTRCore->stScannedDevicesArr[i].bFound             = FALSE;
        apsthBTRCore->stScannedDevicesArr[i].bDeviceConnected   = FALSE;
        apsthBTRCore->stScannedDevicesArr[i].enDeviceType       = enBTRCore_DC_Unknown;

        MEMSET_S(apsthBTRCore->stScannedDevicesArr[i].pcDeviceName, sizeof(apsthBTRCore->stScannedDevicesArr[i].pcDeviceName),     '\0', sizeof(apsthBTRCore->stScannedDevicesArr[i].pcDeviceName));
        MEMSET_S(apsthBTRCore->stScannedDevicesArr[i].pcDeviceAddress, sizeof(apsthBTRCore->stScannedDevicesArr[i].pcDeviceAddress),  '\0', sizeof(apsthBTRCore->stScannedDevicesArr[i].pcDeviceAddress));
        MEMSET_S(apsthBTRCore->stScannedDevicesArr[i].pcDevicePath, sizeof(apsthBTRCore->stScannedDevicesArr[i].pcDevicePath),     '\0', sizeof(apsthBTRCore->stScannedDevicesArr[i].pcDevicePath));

        apsthBTRCore->stScannedDevStInfoArr[i].eDevicePrevState = enBTRCoreDevStInitialized;
        apsthBTRCore->stScannedDevStInfoArr[i].eDeviceCurrState = enBTRCoreDevStInitialized;
    }

    apsthBTRCore->skipDeviceDiscUpdate = 0;
    apsthBTRCore->numOfScannedDevices = 0;
}

static BOOLEAN btrCore_IsStadiaGamepad(
    char * pcAddress
) {
    unsigned char i;
    if (pcAddress == NULL) {
        BTRCORELOG_ERROR("Received NULL mac address\n");
        return FALSE;
    }

    for (i=0; BTRCORE_GOOGLE_OUI_VALUES[i] != NULL; i++) {
        if (!strncmp(pcAddress, BTRCORE_GOOGLE_OUI_VALUES[i], BTRCORE_GOOGLE_OUI_LENGTH)) {
            BTRCORELOG_DEBUG(" Device OUI matches Google stadia gamepad\n");
            return TRUE;
        }
    }
    return FALSE;
}

static BOOLEAN btrCore_IsLunaGamepad(
    char * pcAddress
) {
    unsigned char i;
    if (pcAddress == NULL) {
        BTRCORELOG_ERROR("Received NULL mac address\n");
        return FALSE;
    }

    for (i=0; BTRCORE_AMAZON_OUI_VALUES[i] != NULL; i++) {
        if (!strncmp(pcAddress, BTRCORE_AMAZON_OUI_VALUES[i], BTRCORE_AMAZON_OUI_LENGTH)) {
            BTRCORELOG_DEBUG(" Device OUI matches amazon gamepad\n");
            return TRUE;
        }
    }
    return FALSE;
}
static BOOLEAN btrCore_IsDeviceRdkRcu(
    char * pcAddress, 
    unsigned short ui16Appearance
) {
    unsigned char i;
    if (pcAddress == NULL)
    {
        BTRCORELOG_ERROR("Received NULL mac address\n");
        return FALSE;
    }

    if (ui16Appearance == BTRCORE_REMOTE_CONTROL_APPEARANCE)
    {
        BTRCORELOG_DEBUG("Device appearance is remote control\n");
        return TRUE;
    }

    for (i = 0; BTRCORE_REMOTE_OUI_VALUES[i] != NULL; i++)
    {
        if(!strncmp(pcAddress, BTRCORE_REMOTE_OUI_VALUES[i], BTRCORE_REMOTE_OUI_LENGTH))
        {
            BTRCORELOG_DEBUG("Device OUI matches remote control\n");
            return TRUE;
        }
    }
    return FALSE;
}

static int
btrCore_AddDeviceToScannedDevicesArr (
    stBTRCoreHdl*       apsthBTRCore,
    stBTDeviceInfo*     apstBTDeviceInfo,
    stBTRCoreBTDevice*  apstFoundDevice
) {
    int                 i;
    int                 count = 0;
    errno_t safec_rc = -1;
    stBTRCoreBTDevice   lstFoundDevice;

    MEMSET_S(&lstFoundDevice, sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));

    //TODO: this should be removed after these tickets are fixed: DEXP-234 and DEXP-575
    if (btrCore_IsDeviceRdkRcu (apstBTDeviceInfo->pcAddress, apstBTDeviceInfo->ui16Appearance))
    {
        BTRCORELOG_INFO("Skipping detected RCU: %s (%s)", lstFoundDevice.pcDeviceName, apstBTDeviceInfo->pcAddress);
        return -1;
    }

    lstFoundDevice.bFound               = FALSE;
    lstFoundDevice.i32RSSI              = apstBTDeviceInfo->i32RSSI;
    lstFoundDevice.ui32VendorId         = apstBTDeviceInfo->ui16Vendor;
    lstFoundDevice.tDeviceId            = btrCore_GenerateUniqueDeviceID(apstBTDeviceInfo->pcAddress);
    lstFoundDevice.enDeviceType         = btrCore_MapClassIDtoDevClass(apstBTDeviceInfo->ui32Class);
    lstFoundDevice.ui32DevClassBtSpec   = apstBTDeviceInfo->ui32Class;
    lstFoundDevice.ui16DevAppearanceBleSpec = apstBTDeviceInfo->ui16Appearance;

    btrCore_PopulateModaliasValues(apstBTDeviceInfo->pcModalias, &lstFoundDevice.ui32ModaliasVendorId, &lstFoundDevice.ui32ModaliasProductId, &lstFoundDevice.ui32ModaliasDeviceId);

    safec_rc = strcpy_s(lstFoundDevice.pcDeviceName,    BD_NAME_LEN,      apstBTDeviceInfo->pcName);
    ERR_CHK(safec_rc);
    safec_rc = strcpy_s(lstFoundDevice.pcDeviceAddress, BD_NAME_LEN,    apstBTDeviceInfo->pcAddress); // CID 340831: String not null terminated (STRING_NULL)
    ERR_CHK(safec_rc);
    safec_rc = strcpy_s(lstFoundDevice.pcDevicePath,    BD_NAME_LEN,  apstBTDeviceInfo->pcDevicePath);
    ERR_CHK(safec_rc);

    if (lstFoundDevice.enDeviceType == enBTRCore_DC_Unknown && !strncmp(apstBTDeviceInfo->pcIcon,"input-gaming",strlen("input-gaming"))) {
        BTRCORELOG_INFO("HID Device detected based on PcIcon ...\n");
        lstFoundDevice.enDeviceType = enBTRCore_DC_HID_GamePad;
    }

    if (lstFoundDevice.enDeviceType == enBTRCore_DC_Unknown &&
        lstFoundDevice.pcDeviceName[0] != '\0' &&
        (strstr(lstFoundDevice.pcDeviceName,"Luna") ||
         strstr(lstFoundDevice.pcDeviceName,"Amazon") ||
         btrCore_IsLunaGamepad(lstFoundDevice.pcDeviceAddress))) {
         lstFoundDevice.enDeviceType = enBTRCore_DC_HID_GamePad;
         lstFoundDevice.ui16DevAppearanceBleSpec = 0x3c4;
    }

    /* Populate the profile supported */
    for (i = 0; i < BT_MAX_DEVICE_PROFILE; i++) {
        if (apstBTDeviceInfo->aUUIDs[i][0] == '\0')
            break;
        else
            lstFoundDevice.stDeviceProfile.profile[i].uuid_value = btrCore_BTParseUUIDValue(apstBTDeviceInfo->aUUIDs[i],
                                                                                            lstFoundDevice.stDeviceProfile.profile[i].profile_name);
    }
    lstFoundDevice.stDeviceProfile.numberOfService = i;

    if (lstFoundDevice.enDeviceType == enBTRCore_DC_Unknown) {
        for (i = 0; i < lstFoundDevice.stDeviceProfile.numberOfService; i++) {
            if (lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_A2SNK, NULL, 16)) {
                lstFoundDevice.enDeviceType = enBTRCore_DC_Loudspeaker;
            }
            else if (lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_A2SRC, NULL, 16)) {
                lstFoundDevice.enDeviceType = enBTRCore_DC_SmartPhone;
            }
            else if ((lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_GATT_TILE_1, NULL, 16)) ||
                     (lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_GATT_TILE_2, NULL, 16)) ||
                     (lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_GATT_TILE_3, NULL, 16))) {
                lstFoundDevice.enDeviceType = enBTRCore_DC_Tile;
            }
            else if (lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_HID_1, NULL, 16) ||
                     lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_HID_2, NULL, 16)) {
                lstFoundDevice.enDeviceType = enBTRCore_DC_HID_Keyboard;
            }
            else if ((lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_GATT_XBB_1, NULL, 16)) ||
                     (lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_1, NULL, 16)) ||
                     (lstFoundDevice.stDeviceProfile.profile[i].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_3, NULL, 16))) {
                lstFoundDevice.enDeviceType = enBTRCore_DC_XBB;
            }
        }
    }

    // It seems like the GAP appearance is known well before the GATT service discovery is
    // completed. If we can identify the device as a gamepad now, then let's go ahead and
    // do that. The result is that gamepad discovery takes place much more quickly.
    if (lstFoundDevice.enDeviceType == enBTRCore_DC_Unknown && !btrCore_IsDevNameSameAsAddress(&lstFoundDevice)) {
        if (((apstBTDeviceInfo->ui16Appearance & 0xffc0) >> 6) == 0x0f)  {
            if ((apstBTDeviceInfo->ui16Appearance & 0x3f) == 0x03 || (apstBTDeviceInfo->ui16Appearance & 0x3f) == 0x04) {
                lstFoundDevice.enDeviceType = enBTRCore_DC_HID_Joystick;
                BTRCORELOG_INFO ("Setting device type to Joystick based on GAP appearance\n");
            }
        }
    }

    /* Populate the Ad Service Data */
    for(count = 0; count < lstFoundDevice.stDeviceProfile.numberOfService; count++)
    {
        strncpy(lstFoundDevice.stAdServiceData[count].pcUUIDs, apstBTDeviceInfo->saServices[count].pcUUIDs, (BT_MAX_UUID_STR_LEN - 1));

        if(0 != apstBTDeviceInfo->saServices[count].len)
        {
            lstFoundDevice.stAdServiceData[count].len = apstBTDeviceInfo->saServices[count].len;
            MEMCPY_S(lstFoundDevice.stAdServiceData[count].pcData,lstFoundDevice.stAdServiceData[count].len, apstBTDeviceInfo->saServices[count].pcData, BTRCORE_MAX_SERVICE_DATA_LEN);

            BTRCORELOG_TRACE ("ServiceData from %s\n", __FUNCTION__);
            for (int i =0; i < apstBTDeviceInfo->saServices[count].len; i++){
                BTRCORELOG_TRACE ("ServiceData[%d] = [%x]\n ", i, lstFoundDevice.stAdServiceData[count].pcData[i]);
            }
        }
    }

	/* HID devices are discovered without a valid appearance value. So ome device types are currently skipped during HID discovery.
     * TODO : Improve filtering logic to ensure only HID-specific devices are allowed during HID discovery.
     */
    if (apsthBTRCore->aenDeviceDiscoveryType == enBTRCoreHID) {
        if ((lstFoundDevice.enDeviceType == enBTRCore_DC_Loudspeaker) ||
            (lstFoundDevice.enDeviceType == enBTRCore_DC_Headphones) ||
            (lstFoundDevice.enDeviceType  == enBTRCore_DC_PortableAudio) ||
            (lstFoundDevice.enDeviceType == enBTRCore_DC_WearableHeadset) ||
            (lstFoundDevice.ui16DevAppearanceBleSpec == BTRCORE_REMOTE_CONTROL_APPEARANCE)) {
            BTRCORELOG_INFO("Skipping device %s (%s) as there is a audio/remote devices detected on HID scan\n", lstFoundDevice.pcDeviceName, lstFoundDevice.pcDeviceAddress);
            return -1;
         }
    }

    if ((apsthBTRCore->aenDeviceDiscoveryType == enBTRCoreSpeakers ||
        apsthBTRCore->aenDeviceDiscoveryType == enBTRCoreAudioAndHID) &&
        (lstFoundDevice.enDeviceType == enBTRCore_DC_Unknown)) {
        BTRCORELOG_WARN("Skipping device %s addr %s as the device type is not properly detected \n", lstFoundDevice.pcDeviceName, lstFoundDevice.pcDeviceAddress);
        return -1;
    }

    for (i = 0; i < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES; i++) {
        if (((lstFoundDevice.tDeviceId == apsthBTRCore->stScannedDevicesArr[i].tDeviceId) && (lstFoundDevice.ui32DevClassBtSpec == apsthBTRCore->stScannedDevicesArr[i].ui32DevClassBtSpec) ) || (apsthBTRCore->stScannedDevicesArr[i].bFound == FALSE)) {
            BTRCORELOG_INFO ("Unique DevID = %lld\n",      lstFoundDevice.tDeviceId);
            BTRCORELOG_INFO ("Adding %s at location %d\n", lstFoundDevice.pcDeviceAddress, i);

            lstFoundDevice.bFound   = TRUE;     //mark the record as found

            MEMCPY_S(&apsthBTRCore->stScannedDevicesArr[i],sizeof(apsthBTRCore->stScannedDevicesArr[0]), &lstFoundDevice, sizeof(stBTRCoreBTDevice));

            apsthBTRCore->stScannedDevStInfoArr[i].eDevicePrevState = apsthBTRCore->stScannedDevStInfoArr[i].eDeviceCurrState;
            apsthBTRCore->stScannedDevStInfoArr[i].eDeviceCurrState = enBTRCoreDevStFound;

            apsthBTRCore->numOfScannedDevices++;

            break;
        }
    }


    if ((i < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES) || (lstFoundDevice.enDeviceType == enBTRCore_DC_Tile)) {

        if (lstFoundDevice.enDeviceType == enBTRCore_DC_Tile) {
            lstFoundDevice.bFound   = TRUE;     //mark the record as found
        }

        MEMCPY_S(apstFoundDevice,sizeof(stBTRCoreBTDevice), &lstFoundDevice, sizeof(stBTRCoreBTDevice));
        return i;
    }

    BTRCORELOG_INFO ("Skipped %s DevID = %lld\n", lstFoundDevice.pcDeviceAddress, lstFoundDevice.tDeviceId);
    return -1;
}

static enBTRCoreRet
btrCore_RemoveDeviceFromScannedDevicesArr (
    stBTRCoreHdl*       apstlhBTRCore,
    tBTRCoreDevId       aBTRCoreDevId,
    stBTRCoreBTDevice*  astRemovedDevice
) {
    enBTRCoreRet    retResult   = enBTRCoreSuccess;
    int             i32LoopIdx  = -1;

    for (i32LoopIdx = 0; i32LoopIdx < apstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
        if (apstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId == aBTRCoreDevId) {
            break;
        }
    }

    if (i32LoopIdx != apstlhBTRCore->numOfScannedDevices) {
        BTRCORELOG_TRACE ("i32ScannedDevIdx = %d\n", i32LoopIdx);
        BTRCORELOG_TRACE ("pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].eDeviceCurrState = %d\n", apstlhBTRCore->stScannedDevStInfoArr[i32LoopIdx].eDeviceCurrState);
        BTRCORELOG_TRACE ("pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].eDevicePrevState = %d\n", apstlhBTRCore->stScannedDevStInfoArr[i32LoopIdx].eDevicePrevState);

        MEMCPY_S(astRemovedDevice,sizeof(stBTRCoreBTDevice), &apstlhBTRCore->stScannedDevicesArr[i32LoopIdx], sizeof(stBTRCoreBTDevice));
        astRemovedDevice->bFound = FALSE;

        // Clean flipping logic. This will suffice
        if (i32LoopIdx != apstlhBTRCore->numOfScannedDevices - 1) {
            MEMCPY_S(&apstlhBTRCore->stScannedDevicesArr[i32LoopIdx],sizeof(apstlhBTRCore->stScannedDevicesArr[0]), &apstlhBTRCore->stScannedDevicesArr[apstlhBTRCore->numOfScannedDevices - 1], sizeof(stBTRCoreBTDevice));
            MEMCPY_S(&apstlhBTRCore->stScannedDevStInfoArr[i32LoopIdx],sizeof(apstlhBTRCore->stScannedDevStInfoArr[0]), &apstlhBTRCore->stScannedDevStInfoArr[apstlhBTRCore->numOfScannedDevices - 1], sizeof(stBTRCoreDevStateInfo));
        }

        MEMSET_S(&apstlhBTRCore->stScannedDevicesArr[apstlhBTRCore->numOfScannedDevices - 1], sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
        MEMSET_S(&apstlhBTRCore->stScannedDevStInfoArr[apstlhBTRCore->numOfScannedDevices - 1],sizeof(stBTRCoreDevStateInfo),  0, sizeof(stBTRCoreDevStateInfo));

        apstlhBTRCore->numOfScannedDevices--;
    }
    else {
        BTRCORELOG_ERROR ("Device %lld not found in Scanned List!\n", aBTRCoreDevId);
        retResult = enBTRCoreDeviceNotFound;
    }

    return retResult;
}

static int
btrCore_AddDeviceToKnownDevicesArr (
    stBTRCoreHdl*   apsthBTRCore,
    stBTDeviceInfo* apstBTDeviceInfo
) {
    tBTRCoreDevId   ltDeviceId;
    int             i32LoopIdx      = 0;
    int             i32KnownDevIdx  = -1;
    int             i32ScannedDevIdx= -1;


    ltDeviceId = btrCore_GenerateUniqueDeviceID(apstBTDeviceInfo->pcAddress);

    for (i32LoopIdx = 0; i32LoopIdx < apsthBTRCore->numOfPairedDevices; i32LoopIdx++) {
        if (ltDeviceId == apsthBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
            i32KnownDevIdx = i32LoopIdx;
            break;
        }
    }

    if (i32KnownDevIdx != -1) {
        BTRCORELOG_INFO ("Already Present in stKnownDevicesArr - DevID = %lld\n", ltDeviceId);
        return i32KnownDevIdx;
    }

    if (apsthBTRCore->numOfPairedDevices >= BTRCORE_MAX_NUM_BT_DEVICES) {
        BTRCORELOG_ERROR ("No Space in stKnownDevicesArr - DevID = %lld\n", ltDeviceId);
        return i32KnownDevIdx;
    }

    for (i32LoopIdx = 0; i32LoopIdx < apsthBTRCore->numOfScannedDevices; i32LoopIdx++) {
        if (ltDeviceId == apsthBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
            i32ScannedDevIdx = i32LoopIdx;
            break;
        }
    }

    if (i32ScannedDevIdx == -1) {
        BTRCORELOG_INFO ("Not Present in stScannedDevicesArr - DevID = %lld\n", ltDeviceId);
        return i32ScannedDevIdx;
    }


    i32KnownDevIdx = apsthBTRCore->numOfPairedDevices++;

    MEMCPY_S(&apsthBTRCore->stKnownDevicesArr[i32KnownDevIdx],sizeof(apsthBTRCore->stKnownDevicesArr[0]), &apsthBTRCore->stScannedDevicesArr[i32ScannedDevIdx], sizeof(stBTRCoreBTDevice));
    apsthBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected   = apstBTDeviceInfo->bConnected;

    BTRCORELOG_TRACE ("Added in stKnownDevicesArr - DevID = %lld  i32KnownDevIdx = %d  NumOfPairedDevices = %d\n", ltDeviceId, i32KnownDevIdx, apsthBTRCore->numOfPairedDevices);

    return i32KnownDevIdx;
}


static enBTRCoreRet
btrCore_RemoveDeviceFromKnownDevicesArr (
    stBTRCoreHdl*       apstlhBTRCore,
    tBTRCoreDevId       aBTRCoreDevId
) {
    enBTRCoreRet    retResult   = enBTRCoreSuccess;
    int             i32LoopIdx  = -1;

    for (i32LoopIdx = 0; i32LoopIdx < apstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
        if (apstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId == aBTRCoreDevId) {
            break;
        }
    }

    if (i32LoopIdx != apstlhBTRCore->numOfPairedDevices) {
        BTRCORELOG_TRACE ("i32ScannedDevIdx = %d\n", i32LoopIdx);
        BTRCORELOG_TRACE ("pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].eDeviceCurrState = %d\n", apstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState);
        BTRCORELOG_TRACE ("pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].eDevicePrevState = %d\n", apstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDevicePrevState);

        // Clean flipping logic. This will suffice
        /* During unpairing of device, Removing the known device by comparing the index with
         * numOfScannedDevices leads to the removal of a unpaired device.
         */
        if (i32LoopIdx != apstlhBTRCore->numOfPairedDevices - 1) {
            BTRCORELOG_TRACE("Removing the entry from known device array ...\n");
            MEMCPY_S(&apstlhBTRCore->stKnownDevicesArr[i32LoopIdx],sizeof(apstlhBTRCore->stKnownDevicesArr[0]), &apstlhBTRCore->stKnownDevicesArr[apstlhBTRCore->numOfPairedDevices - 1], sizeof(stBTRCoreBTDevice));
            MEMCPY_S(&apstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx],sizeof(apstlhBTRCore->stKnownDevStInfoArr[0]), &apstlhBTRCore->stKnownDevStInfoArr[apstlhBTRCore->numOfPairedDevices - 1], sizeof(stBTRCoreDevStateInfo));
        }

        MEMSET_S(&apstlhBTRCore->stKnownDevicesArr[apstlhBTRCore->numOfPairedDevices - 1], sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
        MEMSET_S(&apstlhBTRCore->stScannedDevStInfoArr[apstlhBTRCore->numOfPairedDevices - 1], sizeof(stBTRCoreDevStateInfo),  0, sizeof(stBTRCoreDevStateInfo));

        apstlhBTRCore->numOfPairedDevices--;
    }
    else {
        BTRCORELOG_ERROR ("Device %lld not found in Paired List!\n", aBTRCoreDevId);
        retResult = enBTRCoreDeviceNotFound;
    }

    return retResult;
}

static enBTRCoreRet
btrCore_PopulateModaliasValues (
    char * pcModalias,
    unsigned int * ui32ModVendor,
    unsigned int * ui32ModProduct,
    unsigned int * ui32ModDevice
) {
    if (!pcModalias || !ui32ModVendor || !ui32ModProduct || !ui32ModDevice )
    {
        BTRCORELOG_ERROR("Invalid args");
        return enBTRCoreInvalidArg;
    }
    if (pcModalias[0] != '\0') {
        if (strstr(pcModalias, "usb"))
        {
            sscanf(pcModalias,"usb:v%04Xp%04Xd%04X", ui32ModVendor,ui32ModProduct, ui32ModDevice);
        }
        else if (strstr(pcModalias, "bluetooth")) 
        {
            sscanf(pcModalias,"bluetooth:v%04Xp%04Xd%04X", ui32ModVendor,ui32ModProduct, ui32ModDevice);
        }
        BTRCORELOG_TRACE("MVendor:%X MProduct:%X MDevice:%X\n", *ui32ModVendor, *ui32ModProduct, *ui32ModDevice);
    }
    return enBTRCoreSuccess;
}

static void
btrCore_MapKnownDeviceListFromPairedDeviceInfo (
    stBTRCoreBTDevice*      knownDevicesArr,
    stBTPairedDeviceInfo*   pairedDeviceInfo
) {
    unsigned char i_idx = 0;
    unsigned char j_idx = 0;
  
    for (i_idx = 0; i_idx < pairedDeviceInfo->numberOfDevices; i_idx++) {
        knownDevicesArr[i_idx].ui32VendorId         = pairedDeviceInfo->deviceInfo[i_idx].ui16Vendor;
        knownDevicesArr[i_idx].bDeviceConnected     = pairedDeviceInfo->deviceInfo[i_idx].bConnected;
        knownDevicesArr[i_idx].tDeviceId            = btrCore_GenerateUniqueDeviceID(pairedDeviceInfo->deviceInfo[i_idx].pcAddress);
        knownDevicesArr[i_idx].enDeviceType         = btrCore_MapClassIDtoDevClass(pairedDeviceInfo->deviceInfo[i_idx].ui32Class);
        knownDevicesArr[i_idx].ui32DevClassBtSpec   = pairedDeviceInfo->deviceInfo[i_idx].ui32Class;
        knownDevicesArr[i_idx].ui16DevAppearanceBleSpec = pairedDeviceInfo->deviceInfo[i_idx].ui16Appearance;

        btrCore_PopulateModaliasValues(pairedDeviceInfo->deviceInfo[i_idx].pcModalias, &knownDevicesArr[i_idx].ui32ModaliasVendorId,&knownDevicesArr[i_idx].ui32ModaliasProductId, &knownDevicesArr[i_idx].ui32ModaliasDeviceId);

        strncpy(knownDevicesArr[i_idx].pcDeviceName,    pairedDeviceInfo->deviceInfo[i_idx].pcName,     BD_NAME_LEN);
        strncpy(knownDevicesArr[i_idx].pcDeviceAddress, pairedDeviceInfo->deviceInfo[i_idx].pcAddress,  BD_NAME_LEN);
        strncpy(knownDevicesArr[i_idx].pcDevicePath,    pairedDeviceInfo->devicePath[i_idx],            BD_NAME_LEN);
   
        BTRCORELOG_TRACE ("Unique DevID = %lld\n", knownDevicesArr[i_idx].tDeviceId);

        for (j_idx = 0; j_idx < BT_MAX_DEVICE_PROFILE; j_idx++) {
            if (pairedDeviceInfo->deviceInfo[i_idx].aUUIDs[j_idx][0] == '\0')
                break;
            else
                knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].uuid_value = btrCore_BTParseUUIDValue (
                                                                                        pairedDeviceInfo->deviceInfo[i_idx].aUUIDs[j_idx],
                                                                                        knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].profile_name);
        }
        knownDevicesArr[i_idx].stDeviceProfile.numberOfService   =  j_idx;

        if (knownDevicesArr[i_idx].enDeviceType == enBTRCore_DC_Unknown) {
            for (j_idx = 0; j_idx < knownDevicesArr[i_idx].stDeviceProfile.numberOfService; j_idx++) {
                if (knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].uuid_value == strtol(BTR_CORE_A2SNK, NULL, 16)) {
                    knownDevicesArr[i_idx].enDeviceType = enBTRCore_DC_Loudspeaker;
                }
                else if ((knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].uuid_value == strtol(BTR_CORE_HID_1, NULL, 16)) ||
                         (knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].uuid_value == strtol(BTR_CORE_HID_2, NULL, 16))) {
                    knownDevicesArr[i_idx].enDeviceType = enBTRCore_DC_HID_Keyboard;
                }
                else if (knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].uuid_value == strtol(BTR_CORE_GATT_XBB_1, NULL, 16) ||
                         knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_1, NULL, 16)||
                         knownDevicesArr[i_idx].stDeviceProfile.profile[j_idx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_3, NULL, 16)) {
                    knownDevicesArr[i_idx].enDeviceType = enBTRCore_DC_XBB;
                }
            }
        }
    }
}
  
 
static enBTRCoreRet
btrCore_PopulateListOfPairedDevices (
    stBTRCoreHdl*   apsthBTRCore,
    const char*     pAdapterPath
) {
    unsigned char           i_idx = 0;
    unsigned char           j_idx = 0;
    enBTRCoreRet            retResult = enBTRCoreSuccess;
    stBTPairedDeviceInfo*   pstBTPairedDeviceInfo = NULL;
    stBTRCoreBTDevice       knownDevicesArr[BTRCORE_MAX_NUM_BT_DEVICES];


    if ((pstBTPairedDeviceInfo = g_malloc0(sizeof(stBTPairedDeviceInfo))) == NULL)
        return enBTRCoreFailure;


    pstBTPairedDeviceInfo->numberOfDevices = 0;
    for (i_idx = 0; i_idx < BT_MAX_NUM_DEVICE; i_idx++) {
        MEMSET_S(&pstBTPairedDeviceInfo->devicePath[i_idx][0], BT_MAX_DEV_PATH_LEN, '\0', BT_MAX_DEV_PATH_LEN);
        MEMSET_S(&pstBTPairedDeviceInfo->deviceInfo[i_idx], sizeof(stBTDeviceInfo), 0, sizeof(stBTDeviceInfo));
    }

    if (BtrCore_BTGetPairedDeviceInfo (apsthBTRCore->connHdl, pAdapterPath, pstBTPairedDeviceInfo) != 0) {
        BTRCORELOG_ERROR ("Failed to populate List Of Paired Devices\n");
        g_free(pstBTPairedDeviceInfo);
        pstBTPairedDeviceInfo = NULL;
        return enBTRCoreFailure;
    }

    for (j_idx = 0; j_idx < BTRCORE_MAX_NUM_BT_DEVICES; j_idx++) {
        MEMSET_S(&knownDevicesArr[j_idx], sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
    }

    /* Initially stBTRCoreKnownDevice List is populated from pstBTPairedDeviceInfo(bluez i/f) directly *********/  
    if (!apsthBTRCore->numOfPairedDevices) { 
        apsthBTRCore->numOfPairedDevices = pstBTPairedDeviceInfo->numberOfDevices;
        btrCore_MapKnownDeviceListFromPairedDeviceInfo (knownDevicesArr, pstBTPairedDeviceInfo); 

        for (i_idx = 0; i_idx < pstBTPairedDeviceInfo->numberOfDevices; i_idx++) {
            MEMCPY_S(&apsthBTRCore->stKnownDevicesArr[i_idx],sizeof(apsthBTRCore->stKnownDevicesArr[0]), &knownDevicesArr[i_idx], sizeof(stBTRCoreBTDevice));
            if ((knownDevicesArr[i_idx].bDeviceConnected == TRUE) &&
                (knownDevicesArr[i_idx].enDeviceType == enBTRCore_DC_HID_GamePad ||
                 knownDevicesArr[i_idx].enDeviceType == enBTRCore_DC_HID_Keyboard)) {
                BTRCORELOG_INFO("Updaing the current state of the device as connected ...\n");
                apsthBTRCore->stKnownDevStInfoArr[i_idx].eDevicePrevState = enBTRCoreDevStPaired;
                apsthBTRCore->stKnownDevStInfoArr[i_idx].eDeviceCurrState = enBTRCoreDevStConnected;
            } else {
                apsthBTRCore->stKnownDevStInfoArr[i_idx].eDevicePrevState = enBTRCoreDevStInitialized;
                apsthBTRCore->stKnownDevStInfoArr[i_idx].eDeviceCurrState = enBTRCoreDevStPaired;
            }
        }
    } 
    else {
        /**************************************************************************************************
        stBTRCoreKnownDevice in stBTRCoreHdl is handled seperately, instead of populating directly from bluez i/f
        pstBTPairedDeviceInfo list as it causes inconsistency in indices of stKnownDevStInfoArr during pair and unpair
        of devices.
        This case of the addition of Dev6, Dev7 and removal of Dev5 in stBTRCoreKnownDevice list using index
        arrays shows the working of the below algorithm.

         stBTPairedDeviceInfo List from bluez i/f               stBTRCoreKnownDevice List maintained in BTRCore
        +------+------+------+------+------+------+                   +------+------+------+------+------+
        |      |      |      |      |      |      |                   |      |      |      |      |      |      
        | Dev7 | Dev1 | Dev2 | Dev4 | Dev3 | Dev6 |                   | Dev3 | Dev1 | Dev4 | Dev5 | Dev2 |
        |      |      |      |      |      |      |                   |      |      |      |      |      |   
        +------+------+------+------+------+------+                   +------+------+------+------+------+
        +------+------+------+------+------+------+                   +------+------+------+------+------+
        |  0   |  1   |  1   |  1   |  1   |  0   |                   |  1   |  1   |  1   |  0   |  1   |   
        +------+------+------+------+------+------+                   +------+------+------+------+------+
          |                                  |                          |      |      |             |    
          +---------------------------+      |      +-------------------+      |      |             | 
                                      |      |      |      +-------------------+      |             |
                                      |      |      |      |      +-------------------+             |
                                      |      |      |      |      |      +--------------------------+                              
                                      |      |      |      |      |      |
                                   +------+------+------+------+------+------+
                                   |      |      |      |      |      |      |
                                   | Dev7 | Dev6 | Dev3 | Dev1 | Dev4 | Dev2 |
                                   |      |      |      |      |      |      |
                                   +------+------+------+------+------+------+       
                                   -----Updated stBTRCoreKnownDevice List-----
        Now as the change of indexes is known, stKnownDevStInfoArr is also handled  accordingly.******************/
        
        unsigned char count = 0;
        unsigned char k_idx = 0;
        unsigned char numOfDevices = 0;
        unsigned char knownDev_index_array[BTRCORE_MAX_NUM_BT_DEVICES];
        unsigned char pairedDev_index_array[BTRCORE_MAX_NUM_BT_DEVICES];

        MEMSET_S(knownDev_index_array, sizeof(knownDev_index_array),  0, sizeof(knownDev_index_array));
        MEMSET_S(pairedDev_index_array, sizeof(pairedDev_index_array), 0, sizeof(pairedDev_index_array));

        for (j_idx = 0; j_idx < BTRCORE_MAX_NUM_BT_DEVICES; j_idx++) {
            MEMCPY_S(&knownDevicesArr[j_idx],sizeof(knownDevicesArr[0]), &apsthBTRCore->stKnownDevicesArr[j_idx], sizeof(stBTRCoreBTDevice));
            MEMSET_S(&apsthBTRCore->stKnownDevicesArr[j_idx], sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
        }

        /*Loops through to mark the new added and removed device entries in the list              */
        for (i_idx = 0, j_idx = 0; i_idx < pstBTPairedDeviceInfo->numberOfDevices && j_idx < apsthBTRCore->numOfPairedDevices; j_idx++) {
            if (btrCore_GenerateUniqueDeviceID(pstBTPairedDeviceInfo->deviceInfo[i_idx].pcAddress) == knownDevicesArr[j_idx].tDeviceId) {
                knownDev_index_array[j_idx] = 1;
                pairedDev_index_array[i_idx]= 1;
                //make sure if a modalias is avaliable it is populated in the list
                if (knownDevicesArr[j_idx].ui32ModaliasVendorId == 0 && pstBTPairedDeviceInfo->deviceInfo[i_idx].pcModalias[0] != '\0')
                {
                    BTRCORELOG_INFO("Modalias found from bluez, but not in known devices, populating\n");
                    btrCore_PopulateModaliasValues(pstBTPairedDeviceInfo->deviceInfo[i_idx].pcModalias, &knownDevicesArr[j_idx].ui32ModaliasVendorId, &knownDevicesArr[j_idx].ui32ModaliasProductId, &knownDevicesArr[j_idx].ui32ModaliasDeviceId);
                }
                i_idx++;    
            }
            else {
                for (k_idx = i_idx + 1; k_idx < pstBTPairedDeviceInfo->numberOfDevices; k_idx++) {
                    if (btrCore_GenerateUniqueDeviceID(pstBTPairedDeviceInfo->deviceInfo[k_idx].pcAddress) == knownDevicesArr[j_idx].tDeviceId) {
                        knownDev_index_array[j_idx] = 1;
                        pairedDev_index_array[k_idx]= 1;
                        break;
                    }
                }
            }
        }

        numOfDevices = apsthBTRCore->numOfPairedDevices;

        /*Loops through to check for the removal of Device entries from the list during Unpairing */ 
        for (i_idx = 0; i_idx < numOfDevices; i_idx++) {
            if (knownDev_index_array[i_idx]) {
                MEMCPY_S(&apsthBTRCore->stKnownDevicesArr[i_idx - count],sizeof(apsthBTRCore->stKnownDevicesArr[0]), &knownDevicesArr[i_idx], sizeof(stBTRCoreBTDevice));
                apsthBTRCore->stKnownDevStInfoArr[i_idx - count].eDevicePrevState = apsthBTRCore->stKnownDevStInfoArr[i_idx].eDevicePrevState;
                apsthBTRCore->stKnownDevStInfoArr[i_idx - count].eDeviceCurrState = apsthBTRCore->stKnownDevStInfoArr[i_idx].eDeviceCurrState;
            }
            else {
                count++; 
                apsthBTRCore->numOfPairedDevices--;
            }
        }
        btrCore_MapKnownDeviceListFromPairedDeviceInfo (knownDevicesArr, pstBTPairedDeviceInfo);

        /*Loops through to checks for the addition of Device entries to the list during paring */
        for (i_idx = 0; i_idx < pstBTPairedDeviceInfo->numberOfDevices; i_idx++) {
            if (!pairedDev_index_array[i_idx]) {
                MEMCPY_S(&apsthBTRCore->stKnownDevicesArr[apsthBTRCore->numOfPairedDevices],sizeof(apsthBTRCore->stKnownDevicesArr[0]), &knownDevicesArr[i_idx], sizeof(stBTRCoreBTDevice));
                if (apsthBTRCore->stKnownDevStInfoArr[apsthBTRCore->numOfPairedDevices].eDeviceCurrState != enBTRCoreDevStConnected) {
                    apsthBTRCore->stKnownDevStInfoArr[apsthBTRCore->numOfPairedDevices].eDevicePrevState = enBTRCoreDevStInitialized;
                    apsthBTRCore->stKnownDevStInfoArr[apsthBTRCore->numOfPairedDevices].eDeviceCurrState = enBTRCoreDevStPaired;
                    apsthBTRCore->stKnownDevicesArr[apsthBTRCore->numOfPairedDevices].bDeviceConnected   = FALSE;
                    apsthBTRCore->stKnownDevicesArr[apsthBTRCore->numOfPairedDevices].bFound             = TRUE; // Paired Now
                }
                apsthBTRCore->numOfPairedDevices++;
            }
        }
    }
    
    g_free(pstBTPairedDeviceInfo);
    pstBTPairedDeviceInfo = NULL;
    return retResult;
}


static const char*
btrCore_GetScannedDeviceAddress (
    stBTRCoreHdl*   apsthBTRCore,
    tBTRCoreDevId   aBTRCoreDevId
) {
    int i = 0;

    if (apsthBTRCore->numOfScannedDevices) {
        for (i = 0; i < apsthBTRCore->numOfScannedDevices; i++) {
            if (aBTRCoreDevId == apsthBTRCore->stScannedDevicesArr[i].tDeviceId)
                return apsthBTRCore->stScannedDevicesArr[i].pcDevicePath;
        }
    }

    return NULL;
}


static const char*
btrCore_GetKnownDeviceMac (
    stBTRCoreHdl*   apsthBTRCore,
    tBTRCoreDevId   aBTRCoreDevId
) {
    int i32LoopIdx = -1;

    if (apsthBTRCore->numOfPairedDevices) {
        for (i32LoopIdx = 0; i32LoopIdx < apsthBTRCore->numOfPairedDevices; i32LoopIdx++) {
            if (aBTRCoreDevId == apsthBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId)
                return apsthBTRCore->stKnownDevicesArr[i32LoopIdx].pcDeviceAddress;
        }
    }

    return NULL;
}


static enBTRCoreRet
btrCore_GetDeviceInfo (
    stBTRCoreHdl*           apsthBTRCore,
    tBTRCoreDevId           aBTRCoreDevId,
    enBTRCoreDeviceType     aenBTRCoreDevType,
    enBTDeviceType*         apenBTDeviceType,
    stBTRCoreBTDevice**     appstBTRCoreBTDevice,
    stBTRCoreDevStateInfo** appstBTRCoreDevStateInfo,
    const char**            appcBTRCoreBTDevicePath,
    const char**            appcBTRCoreBTDeviceName
) {
    stBTRCoreBTDevice*      lpstBTRCoreBTDeviceArr  = NULL;
    stBTRCoreDevStateInfo*  lpstBTRCoreDevStInfoArr = NULL;
    unsigned int            ui32NumOfDevices        = 0;
    unsigned int            ui32LoopIdx             = 0;

    if (!apsthBTRCore->numOfPairedDevices) {
        BTRCORELOG_INFO ("Possibly the list is not populated; like booted and connecting\n");
        btrCore_PopulateListOfPairedDevices(apsthBTRCore, apsthBTRCore->curAdapterPath);    /* Keep the list upto date */
    }

#ifdef LE_MODE
    gboolean lbBatteryDevFound = FALSE;
    gboolean pairedDevFound = FALSE;

    for (ui32LoopIdx = 0; ui32LoopIdx < apsthBTRCore->numOfScannedDevices; ui32LoopIdx++) {
        if (aBTRCoreDevId == apsthBTRCore->stScannedDevicesArr[ui32LoopIdx].tDeviceId) {
            lpstBTRCoreBTDeviceArr = &apsthBTRCore->stScannedDevicesArr[ui32LoopIdx];
            break;
        }
    }

    /* Added the below if check to handle the case where we try to get the device info
     * by using device index from btrCoreTest
     */
    if (aBTRCoreDevId < apsthBTRCore->numOfPairedDevices) {
        lpstBTRCoreBTDeviceArr = &apsthBTRCore->stKnownDevicesArr[aBTRCoreDevId];
    }
    else {
        for (ui32LoopIdx = 0; ui32LoopIdx < apsthBTRCore->numOfPairedDevices; ui32LoopIdx++) {
            if (aBTRCoreDevId == apsthBTRCore->stKnownDevicesArr[ui32LoopIdx].tDeviceId) {
                lpstBTRCoreBTDeviceArr = &apsthBTRCore->stKnownDevicesArr[ui32LoopIdx];
                pairedDevFound = TRUE;
                break;
            }
        }
    }

    for (ui32LoopIdx = 0; (lpstBTRCoreBTDeviceArr != NULL) && (ui32LoopIdx < lpstBTRCoreBTDeviceArr->stDeviceProfile.numberOfService); ui32LoopIdx++) {
        if (lpstBTRCoreBTDeviceArr->stDeviceProfile.profile[ui32LoopIdx].uuid_value == strtol(BTR_CORE_GATT_XBB_1, NULL, 16) ||
            lpstBTRCoreBTDeviceArr->stDeviceProfile.profile[ui32LoopIdx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_1, NULL, 16) ||
            lpstBTRCoreBTDeviceArr->stDeviceProfile.profile[ui32LoopIdx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_3, NULL, 16)) {
            lbBatteryDevFound = TRUE;
            break;
        }
    }


    if ((aenBTRCoreDevType == enBTRCoreLE) && (lbBatteryDevFound == TRUE)) {
        ui32NumOfDevices        = apsthBTRCore->numOfPairedDevices;
        lpstBTRCoreBTDeviceArr  = apsthBTRCore->stKnownDevicesArr;
        lpstBTRCoreDevStInfoArr = apsthBTRCore->stKnownDevStInfoArr;
    }
    else {
        if((aenBTRCoreDevType == enBTRCoreUnknown) && (pairedDevFound == TRUE)) {
           ui32NumOfDevices        = apsthBTRCore->numOfPairedDevices;
           lpstBTRCoreBTDeviceArr  = apsthBTRCore->stKnownDevicesArr;
           lpstBTRCoreDevStInfoArr = apsthBTRCore->stKnownDevStInfoArr;
        }
        else {
           ui32NumOfDevices        = apsthBTRCore->numOfScannedDevices;
           lpstBTRCoreBTDeviceArr  = apsthBTRCore->stScannedDevicesArr;
           lpstBTRCoreDevStInfoArr = apsthBTRCore->stScannedDevStInfoArr;

           if (lbBatteryDevFound == FALSE) {
              aenBTRCoreDevType   = enBTRCoreUnknown;
           }
        }
    }
#else
    if (aenBTRCoreDevType == enBTRCoreLE) {
        ui32NumOfDevices        = apsthBTRCore->numOfScannedDevices;
        lpstBTRCoreBTDeviceArr  = apsthBTRCore->stScannedDevicesArr;
        lpstBTRCoreDevStInfoArr = apsthBTRCore->stScannedDevStInfoArr;
    }
    else {
        for (ui32LoopIdx = 0; ui32LoopIdx < apsthBTRCore->numOfScannedDevices; ui32LoopIdx++) {
            if (aBTRCoreDevId == apsthBTRCore->stScannedDevicesArr[ui32LoopIdx].tDeviceId) {
                ui32NumOfDevices        = apsthBTRCore->numOfScannedDevices;
                lpstBTRCoreBTDeviceArr  = apsthBTRCore->stScannedDevicesArr;
                lpstBTRCoreDevStInfoArr = apsthBTRCore->stScannedDevStInfoArr;
                break;
            }
        }

        /* Added the below if check to handle the case where we try to get the device info by using device index from btrCoreTest */
        if (aBTRCoreDevId < apsthBTRCore->numOfPairedDevices) {
            ui32NumOfDevices        = apsthBTRCore->numOfPairedDevices;
            lpstBTRCoreBTDeviceArr  = apsthBTRCore->stKnownDevicesArr;
            lpstBTRCoreDevStInfoArr = apsthBTRCore->stKnownDevStInfoArr;
        }
        else {
            for (ui32LoopIdx = 0; ui32LoopIdx < apsthBTRCore->numOfPairedDevices; ui32LoopIdx++) {
                if (aBTRCoreDevId == apsthBTRCore->stKnownDevicesArr[ui32LoopIdx].tDeviceId) {
                    ui32NumOfDevices        = apsthBTRCore->numOfPairedDevices;
                    lpstBTRCoreBTDeviceArr  = apsthBTRCore->stKnownDevicesArr;
                    lpstBTRCoreDevStInfoArr = apsthBTRCore->stKnownDevStInfoArr;
                    break;
                }
            }
        }
    }
#endif


    if (!ui32NumOfDevices) {
        BTRCORELOG_ERROR ("There is no device paried/scanned for this adapter\n");
        return enBTRCoreFailure;
    }


    if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
        *appstBTRCoreBTDevice       = &lpstBTRCoreBTDeviceArr[aBTRCoreDevId];
        *appstBTRCoreDevStateInfo   = &lpstBTRCoreDevStInfoArr[aBTRCoreDevId];
    }
    else {
        for (ui32LoopIdx = 0; ui32LoopIdx < ui32NumOfDevices; ui32LoopIdx++) {
            if (aBTRCoreDevId == lpstBTRCoreBTDeviceArr[ui32LoopIdx].tDeviceId) {
                *appstBTRCoreBTDevice       = &lpstBTRCoreBTDeviceArr[ui32LoopIdx];
                *appstBTRCoreDevStateInfo   = &lpstBTRCoreDevStInfoArr[ui32LoopIdx];
                break;
            }
        }
    }

    if (*appstBTRCoreBTDevice) {
        *appcBTRCoreBTDevicePath    = (*appstBTRCoreBTDevice)->pcDevicePath;
        *appcBTRCoreBTDeviceName    = (*appstBTRCoreBTDevice)->pcDeviceName;
    }

    if (!(*appcBTRCoreBTDevicePath) || !strlen(*appcBTRCoreBTDevicePath)) {
        BTRCORELOG_ERROR ("Failed to find device in paired/scanned devices list\n");
        return enBTRCoreDeviceNotFound;
    }


    switch (aenBTRCoreDevType) {
    case enBTRCoreSpeakers:
    case enBTRCoreHeadSet:
        *apenBTDeviceType = enBTDevAudioSink;
        break;
    case enBTRCoreMobileAudioIn:
    case enBTRCorePCAudioIn:
        *apenBTDeviceType = enBTDevAudioSource;
        break;
    case enBTRCoreLE:
        *apenBTDeviceType = enBTDevLE;
        break;
    case enBTRCoreHID:
        *apenBTDeviceType = enBTDevHID;
        break;
    case enBTRCoreUnknown:
    default:
        *apenBTDeviceType = enBTDevUnknown;
        break;
    }

    return enBTRCoreSuccess;
}


static enBTRCoreRet
btrCore_GetDeviceInfoKnown (
    stBTRCoreHdl*           apsthBTRCore,
    tBTRCoreDevId           aBTRCoreDevId,
    enBTRCoreDeviceType     aenBTRCoreDevType,
    enBTDeviceType*         apenBTDeviceType,
    stBTRCoreBTDevice**     appstBTRCoreBTDevice,
    stBTRCoreDevStateInfo** appstBTRCoreDevStateInfo,
    const char**            appcBTRCoreBTDevicePath
) {
    unsigned int            ui32NumOfDevices        = 0;
    unsigned int            ui32LoopIdx             = 0;

    if (!apsthBTRCore->numOfPairedDevices) {
        BTRCORELOG_INFO ("Possibly the list is not populated; like booted and connecting\n");
        btrCore_PopulateListOfPairedDevices(apsthBTRCore, apsthBTRCore->curAdapterPath); /* Keep the list upto date */
    }


    ui32NumOfDevices = apsthBTRCore->numOfPairedDevices;
    if (!ui32NumOfDevices) {
        BTRCORELOG_ERROR ("There is no device paried for this adapter\n");
        return enBTRCoreFailure;
    }


    if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
        *appstBTRCoreBTDevice       = &apsthBTRCore->stKnownDevicesArr[aBTRCoreDevId];
        *appstBTRCoreDevStateInfo   = &apsthBTRCore->stKnownDevStInfoArr[aBTRCoreDevId];
    }
    else {
        for (ui32LoopIdx = 0; ui32LoopIdx < ui32NumOfDevices; ui32LoopIdx++) {
            if (aBTRCoreDevId == apsthBTRCore->stKnownDevicesArr[ui32LoopIdx].tDeviceId) {
                *appstBTRCoreBTDevice       = &apsthBTRCore->stKnownDevicesArr[ui32LoopIdx];
                *appstBTRCoreDevStateInfo   = &apsthBTRCore->stKnownDevStInfoArr[ui32LoopIdx];
            }
        }
    }

    if (*appstBTRCoreBTDevice) {
        *appcBTRCoreBTDevicePath = (*appstBTRCoreBTDevice)->pcDevicePath;
    }

    if (!(*appcBTRCoreBTDevicePath) || !strlen(*appcBTRCoreBTDevicePath)) {
        BTRCORELOG_ERROR ("Failed to find device in paired devices list\n");
        return enBTRCoreDeviceNotFound;
    }


    switch (aenBTRCoreDevType) {
    case enBTRCoreSpeakers:
    case enBTRCoreHeadSet:
        *apenBTDeviceType  = enBTDevAudioSink;
        break;
    case enBTRCoreMobileAudioIn:
    case enBTRCorePCAudioIn:
        *apenBTDeviceType = enBTDevAudioSource;
        break;
    case enBTRCoreHID:
        *apenBTDeviceType = enBTDevHID;
        break;
    case enBTRCoreUnknown:
    default:
        *apenBTDeviceType = enBTDevUnknown;
        break;
    }

    return enBTRCoreSuccess;
}

static void 
btrCore_ShowSignalStrength (
    short strength
) {
    short pos_str;

    pos_str = 100 + strength;//strength is usually negative with number meaning more strength

    BTRCORELOG_TRACE (" Signal Strength: %d dbmv  \n", strength);

    if (pos_str > 70) {
        BTRCORELOG_TRACE ("++++\n");
    }

    if ((pos_str > 50) && (pos_str <= 70)) {
        BTRCORELOG_TRACE ("+++\n");
    }

    if ((pos_str > 37) && (pos_str <= 50)) {
        BTRCORELOG_TRACE ("++\n");
    }

    if (pos_str <= 37) {
        BTRCORELOG_TRACE ("+\n");
    } 
}


static unsigned int
btrCore_BTParseUUIDValue (
    const char* pUUIDString,
    char*       pServiceNameOut
) {
    char aUUID[8];
    unsigned int uuid_value = 0;


    if (pUUIDString) {
        /* Arrive at short form of UUID */
        aUUID[0] = '0';
        aUUID[1] = 'x';
        aUUID[2] = pUUIDString[4];
        aUUID[3] = pUUIDString[5];
        aUUID[4] = pUUIDString[6];
        aUUID[5] = pUUIDString[7];
        aUUID[6] = '\0';

        uuid_value = strtol(aUUID, NULL, 16);

        /* Have the name by list comparision */
        if (!strcasecmp(aUUID, BTR_CORE_SP)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_SP_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_HEADSET)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_HEADSET_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_A2SRC)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_A2SRC_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_A2SNK)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_A2SNK_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_AVRTG)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_AVRTG_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_AAD)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_AAD_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_AVRCT)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_AVRCT_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_AVREMOTE)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_AVREMOTE_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_HS_AG)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_HS_AG_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_HANDSFREE)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_HANDSFREE_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_HAG)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_HAG_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_HEADSET2)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_HEADSET2_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GEN_AUDIO)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GEN_AUDIO_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_PNP)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_PNP_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GEN_ATRIB)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GEN_ATRIB_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GATT_TILE_1)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GATT_TILE_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GATT_TILE_2)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GATT_TILE_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GATT_TILE_3)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GATT_TILE_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GEN_ACCESS)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GEN_ACCESS_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GEN_ATTRIBUTE)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GEN_ATTRIBUTE_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_DEVICE_INFO)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_DEVICE_INFO_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_BATTERY_SERVICE)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_BATTERY_SERVICE_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_HID_1)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_HID_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_HID_2)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_HID_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_GATT_XBB_1) || 
                 !strcasecmp(aUUID, BTR_CORE_BATTERY_SERVICE_XBB_1) ||
                 !strcasecmp(aUUID, BTR_CORE_BATTERY_SERVICE_XBB_3)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_GATT_XBB_TEXT);
        }
        else if (!strcasecmp(aUUID, BTR_CORE_REMOTE_SERVICE_1) ||
                 !strcasecmp(aUUID, BTR_CORE_REMOTE_SERVICE_2) ||
                 !strcasecmp(aUUID, BTR_CORE_REMOTE_SERVICE_3) ||
                 !strcasecmp(aUUID, BTR_CORE_REMOTE_SERVICE_4)) {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, BTR_CORE_REMOTE_SERVICE_TEXT);
        }
        else {
            STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, "Not Identified");
        }
    }
    else {
        STRCPY_S(pServiceNameOut, BTRCORE_STR_LEN, "Not Identified");
    }

    return uuid_value;
}


static enBTRCoreDeviceState
btrCore_BTParseDeviceState (
    const char* pcStateValue
) {
    enBTRCoreDeviceState rc = enBTRCoreDevStInitialized;

    if ((pcStateValue) && (pcStateValue[0] != '\0')) {
        BTRCORELOG_INFO ("Current State of this connection is @@%s@@\n", pcStateValue);

        if (!strcasecmp("unpaired", pcStateValue)) {
            rc = enBTRCoreDevStUnpaired;
        }
        else if (!strcasecmp("paired", pcStateValue)) {
            rc = enBTRCoreDevStPaired;
        }
        else if (!strcasecmp("disconnected", pcStateValue)) {
            rc = enBTRCoreDevStDisconnected;
        }
        else if (!strcasecmp("connecting", pcStateValue)) {
            rc = enBTRCoreDevStConnecting;
        }
        else if (!strcasecmp("connected", pcStateValue)) {
            rc = enBTRCoreDevStConnected;
        }
        else if (!strcasecmp("playing", pcStateValue)) {
            rc = enBTRCoreDevStPlaying;
        }
	else if (!strcasecmp("suspended", pcStateValue)) {
            rc = enBTRCoreDevStSuspended;
        }
    }

    return rc;
}


static enBTRCoreRet
btrCore_RunTaskAddOp (
    GAsyncQueue*                apRunTaskGAq,
    enBTRCoreTaskOp             aenRunTaskOp,
    enBTRCoreTaskProcessType    aenRunTaskPT,
    void*                       apvRunTaskInData
) {
    stBTRCoreTaskGAqData*       lpstRunTaskGAqData = NULL;

    if ((apRunTaskGAq == NULL) || (aenRunTaskOp == enBTRCoreTaskOpUnknown)) {
        return enBTRCoreInvalidArg;
    }


    if (!(lpstRunTaskGAqData = g_malloc0(sizeof(stBTRCoreTaskGAqData)))) {
        return enBTRCoreFailure;
    }


    lpstRunTaskGAqData->enBTRCoreTskOp = aenRunTaskOp;
    switch (lpstRunTaskGAqData->enBTRCoreTskOp) {
    case enBTRCoreTaskOpStart:
        break;
    case enBTRCoreTaskOpStop:
        break;
    case enBTRCoreTaskOpIdle:
        break;
    case enBTRCoreTaskOpProcess:
        break;
    case enBTRCoreTaskOpExit:
        break;
    case enBTRCoreTaskOpUnknown:
    default:
        break;
    }


    lpstRunTaskGAqData->enBTRCoreTskPT = aenRunTaskPT;
    if (lpstRunTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTUnknown) {
    }
    else {
    }


    BTRCORELOG_TRACE ("g_async_queue_push %d %d %p\n", lpstRunTaskGAqData->enBTRCoreTskOp, lpstRunTaskGAqData->enBTRCoreTskPT, lpstRunTaskGAqData->pvBTRCoreTskInData);
    g_async_queue_push(apRunTaskGAq, lpstRunTaskGAqData);


    return enBTRCoreSuccess;
}

static enBTRCoreRet
btrCore_OutTaskAddOp (
    GAsyncQueue*                apOutTaskGAq,
    enBTRCoreTaskOp             aenOutTaskOp,
    enBTRCoreTaskProcessType    aenOutTaskPT,
    void*                       apvOutTaskInData
) {
    stBTRCoreTaskGAqData*       lpstOutTaskGAqData = NULL;

    if ((apOutTaskGAq == NULL) || (aenOutTaskOp == enBTRCoreTaskOpUnknown)) {
        return enBTRCoreInvalidArg;
    }


    if (!(lpstOutTaskGAqData = g_malloc0(sizeof(stBTRCoreTaskGAqData)))) {
        return enBTRCoreFailure;
    }


    lpstOutTaskGAqData->enBTRCoreTskOp = aenOutTaskOp;
    switch (lpstOutTaskGAqData->enBTRCoreTskOp) {
    case enBTRCoreTaskOpStart:
        break;
    case enBTRCoreTaskOpStop:
        break;
    case enBTRCoreTaskOpIdle:
        break;
    case enBTRCoreTaskOpProcess:
        break;
    case enBTRCoreTaskOpExit:
        break;
    case enBTRCoreTaskOpUnknown:
    default:
        break;
    }


    lpstOutTaskGAqData->enBTRCoreTskPT = aenOutTaskPT;
    if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBAdapterStatus) {
        if ((lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreAdapter)))) {
            MEMCPY_S(lpstOutTaskGAqData->pvBTRCoreTskInData, sizeof(stBTRCoreAdapter), (stBTRCoreAdapter*)apvOutTaskInData, sizeof(stBTRCoreAdapter));
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBDeviceDisc) {
        if ((apvOutTaskInData) &&
            (lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreOTskInData))) &&
            (((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo = g_malloc0(sizeof(stBTDeviceInfo)))) {
            MEMCPY_S(((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo,sizeof(stBTDeviceInfo), (stBTDeviceInfo*)((stBTRCoreOTskInData*)apvOutTaskInData)->pstBTDevInfo, sizeof(stBTDeviceInfo));
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->bTRCoreDevId    = ((stBTRCoreOTskInData*)apvOutTaskInData)->bTRCoreDevId;
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->enBTRCoreDevType= ((stBTRCoreOTskInData*)apvOutTaskInData)->enBTRCoreDevType;
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBDeviceRemoved) {
        if ((apvOutTaskInData) &&
            (lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreOTskInData))) &&
            (((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo = g_malloc0(sizeof(stBTDeviceInfo)))) {
            MEMCPY_S(((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo,sizeof(stBTDeviceInfo), (stBTDeviceInfo*)((stBTRCoreOTskInData*)apvOutTaskInData)->pstBTDevInfo, sizeof(stBTDeviceInfo));
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->bTRCoreDevId    = ((stBTRCoreOTskInData*)apvOutTaskInData)->bTRCoreDevId;
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->enBTRCoreDevType= ((stBTRCoreOTskInData*)apvOutTaskInData)->enBTRCoreDevType;
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBDeviceLost) {
        if ((apvOutTaskInData) &&
            (lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreOTskInData))) &&
            (((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo = g_malloc0(sizeof(stBTDeviceInfo)))) {
            MEMCPY_S(((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo,sizeof(stBTDeviceInfo), (stBTDeviceInfo*)((stBTRCoreOTskInData*)apvOutTaskInData)->pstBTDevInfo, sizeof(stBTDeviceInfo));
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->bTRCoreDevId    = ((stBTRCoreOTskInData*)apvOutTaskInData)->bTRCoreDevId;
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->enBTRCoreDevType= ((stBTRCoreOTskInData*)apvOutTaskInData)->enBTRCoreDevType;
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBDeviceStatus) {
        if ((apvOutTaskInData) &&
            (lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreOTskInData))) &&
            (((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo = g_malloc0(sizeof(stBTDeviceInfo)))) {
            MEMCPY_S(((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo,sizeof(stBTDeviceInfo), (stBTDeviceInfo*)((stBTRCoreOTskInData*)apvOutTaskInData)->pstBTDevInfo, sizeof(stBTDeviceInfo));
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->bTRCoreDevId    = ((stBTRCoreOTskInData*)apvOutTaskInData)->bTRCoreDevId;
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->enBTRCoreDevType= ((stBTRCoreOTskInData*)apvOutTaskInData)->enBTRCoreDevType;
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBMediaStatus) {
        if ((lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreMediaStatusCBInfo)))) {
            MEMCPY_S(lpstOutTaskGAqData->pvBTRCoreTskInData,sizeof(stBTRCoreMediaStatusCBInfo), (stBTRCoreMediaStatusCBInfo*)apvOutTaskInData, sizeof(stBTRCoreMediaStatusCBInfo));
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBDevOpInfoStatus) {
        if ((apvOutTaskInData) &&
            (lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreDevStatusCBInfo)))) {
            MEMCPY_S(lpstOutTaskGAqData->pvBTRCoreTskInData,sizeof(stBTRCoreDevStatusCBInfo), (stBTRCoreDevStatusCBInfo*)apvOutTaskInData, sizeof(stBTRCoreDevStatusCBInfo));
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBConnIntim) {
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBConnAuth) {
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTcBModaliasUpdate) {
        if ((apvOutTaskInData) &&
            (lpstOutTaskGAqData->pvBTRCoreTskInData = g_malloc0(sizeof(stBTRCoreOTskInData))) &&
            (((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo = g_malloc0(sizeof(stBTDeviceInfo)))) {
            MEMCPY_S(((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->pstBTDevInfo,sizeof(stBTDeviceInfo), (stBTDeviceInfo*)((stBTRCoreOTskInData*)apvOutTaskInData)->pstBTDevInfo, sizeof(stBTDeviceInfo));
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->bTRCoreDevId    = ((stBTRCoreOTskInData*)apvOutTaskInData)->bTRCoreDevId;
            ((stBTRCoreOTskInData*)lpstOutTaskGAqData->pvBTRCoreTskInData)->enBTRCoreDevType= ((stBTRCoreOTskInData*)apvOutTaskInData)->enBTRCoreDevType;
        }
    }
    else if (lpstOutTaskGAqData->enBTRCoreTskPT == enBTRCoreTaskPTUnknown) {
    }
    else {
    }


    BTRCORELOG_TRACE ("g_async_queue_push %d %d %p\n", lpstOutTaskGAqData->enBTRCoreTskOp, lpstOutTaskGAqData->enBTRCoreTskPT, lpstOutTaskGAqData->pvBTRCoreTskInData);
    g_async_queue_push(apOutTaskGAq, lpstOutTaskGAqData);


    return enBTRCoreSuccess;
}

static enBTRCoreRet
btrCore_updateBatteryLevelsForConnectedDevices (
    stBTRCoreHdl* apsthBTRCore,
    unsigned char * ui8NumberDevicesAvailable,
    BOOLEAN * bLowBatteryDeviceFound
)
{
    unsigned char batteryLevel, i, prevBatteryLevel;
    enBTRCoreRet enBTRCoreRetVal;
    if (!apsthBTRCore || !ui8NumberDevicesAvailable ||  !bLowBatteryDeviceFound)
    {
        BTRCORELOG_ERROR("Invalid arguments, can't update battery levels\n");
        return enBTRCoreInvalidArg;
    }
    *ui8NumberDevicesAvailable = 0;
    *bLowBatteryDeviceFound = FALSE;
    for (i = 0; i < BTRCORE_MAX_NUM_BT_DEVICES; i++) {
        if ((apsthBTRCore->stKnownDevicesArr[i].tDeviceId != 0) && 
            (apsthBTRCore->stKnownDevicesArr[i].bDeviceConnected == 1) && 
            (btrCore_MapDevClassToDevType(apsthBTRCore->stKnownDevicesArr[i].enDeviceType) == enBTRCoreHID))
            {
                (*ui8NumberDevicesAvailable)++;
                enBTRCoreRetVal = BTRCore_GetDeviceBatteryLevel(apsthBTRCore, apsthBTRCore->stKnownDevicesArr[i].tDeviceId, enBTRCoreHID, &batteryLevel);
                if (enBTRCoreRetVal == enBTRCoreSuccess)
                {
                    g_mutex_lock(&apsthBTRCore->batteryLevelMutex);
                    prevBatteryLevel = apsthBTRCore->stKnownDevicesArr[i].ui8batteryLevel;
                    apsthBTRCore->stKnownDevicesArr[i].ui8batteryLevel = batteryLevel;
                    g_mutex_unlock(&apsthBTRCore->batteryLevelMutex);
                    if (prevBatteryLevel != apsthBTRCore->stKnownDevicesArr[i].ui8batteryLevel)
                    {
                        BTRCORELOG_INFO("updating battery level for device mac,level: %s,%d\n", apsthBTRCore->stKnownDevicesArr[i].pcDeviceAddress, batteryLevel);
                    }
                    else
                    {
                        BTRCORELOG_TRACE("Battery level for %lld is %d\n", apsthBTRCore->stKnownDevicesArr[i].tDeviceId, batteryLevel);
                    }
                    if (batteryLevel <= BTRCORE_LOW_BATTERY_THRESHOLD)
                    {
                        *bLowBatteryDeviceFound = TRUE;
                    }
                } 
                else 
                {
                    BTRCORELOG_TRACE("Failed to read battery level for %lld - not updating\n", apsthBTRCore->stKnownDevicesArr[i].tDeviceId);
                }
            }
            else if (apsthBTRCore->stKnownDevicesArr[i].tDeviceId != 0)
            {
                BTRCORELOG_TRACE("Not trying to read battery level for %lld - either not connected or supported\n", apsthBTRCore->stKnownDevicesArr[i].tDeviceId);
            }
    }
    return enBTRCoreSuccess;
}


/*  Local Op Threads */
static gpointer
btrCore_RunTask (
    gpointer        apsthBTRCore
) {
    stBTRCoreHdl*   pstlhBTRCore        = NULL;
    enBTRCoreRet*   penExitStatusRunTask= NULL;
    enBTRCoreRet    lenBTRCoreRet       = enBTRCoreSuccess;

    gint64                      li64usTimeout       = 0;
    guint16                     lui16msTimeout      = 20;
    BOOLEAN                     lbRunTaskExit       = FALSE;
    enBTRCoreTaskOp             lenRunTskOpPrv      = enBTRCoreTaskOpUnknown;
    enBTRCoreTaskOp             lenRunTskOpCur      = enBTRCoreTaskOpUnknown;
    enBTRCoreTaskProcessType    lenRunTskPTCur      = enBTRCoreTaskPTUnknown;
    void*                       lpvRunTskInData     = NULL;
    stBTRCoreTaskGAqData*       lpstRunTaskGAqData  = NULL;


    pstlhBTRCore = (stBTRCoreHdl*)apsthBTRCore;
    
    if (!(penExitStatusRunTask = g_malloc0(sizeof(enBTRCoreRet)))) {
        BTRCORELOG_ERROR ("RunTask failure - No Memory\n");
        return NULL;
    }

    if (!pstlhBTRCore || !pstlhBTRCore->connHdl) {
        BTRCORELOG_ERROR ("RunTask failure - BTRCore not initialized\n");
        *penExitStatusRunTask = enBTRCoreNotInitialized;
        return (gpointer)penExitStatusRunTask;
    }


    BTRCORELOG_INFO ("%s \n", "RunTask Started");

    do {

        /* Process incoming task-ops */
        {
            li64usTimeout = lui16msTimeout * G_TIME_SPAN_MILLISECOND;
            if ((pstlhBTRCore->pGAQueueRunTask) &&
                (lpstRunTaskGAqData = g_async_queue_timeout_pop(pstlhBTRCore->pGAQueueRunTask, li64usTimeout))) {
                lenRunTskOpCur = lpstRunTaskGAqData->enBTRCoreTskOp;
                lenRunTskPTCur = lpstRunTaskGAqData->enBTRCoreTskPT;
                lpvRunTskInData= lpstRunTaskGAqData->pvBTRCoreTskInData;
                g_free(lpstRunTaskGAqData);
                lpstRunTaskGAqData = NULL;
                BTRCORELOG_DEBUG ("g_async_queue_timeout_pop %d %d %p\n", lenRunTskOpCur, lenRunTskPTCur, lpvRunTskInData);
            }
        }


        /* Set up operation - Schedule state changes for next interation */
        if (lenRunTskOpPrv != lenRunTskOpCur) {
            lenRunTskOpPrv = lenRunTskOpCur;

            switch (lenRunTskOpCur) {
            case enBTRCoreTaskOpStart: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpStart\n");
                if ((lenBTRCoreRet = btrCore_RunTaskAddOp(pstlhBTRCore->pGAQueueRunTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTUnknown, NULL)) != enBTRCoreSuccess) {
                    BTRCORELOG_ERROR("Failure btrCore_RunTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTUnknown = %d\n", lenBTRCoreRet);
                }

                break;
            }
            case enBTRCoreTaskOpStop: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpStop\n");
                
                break;
            }
            case enBTRCoreTaskOpIdle: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpIdle\n");
                
                break;
            }
            case enBTRCoreTaskOpProcess: {
                BTRCORELOG_DEBUG ("enBTRCoreTaskOpProcess\n");

                break;
            }
            case enBTRCoreTaskOpExit: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpExit\n");

                break;
            }
            case enBTRCoreTaskOpUnknown: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpUnknown\n");

                break;
            }
            default:
                BTRCORELOG_INFO ("default\n");
                break;
            }

        }


        /* Process Operations */
        {
            switch (lenRunTskOpCur) {
            case enBTRCoreTaskOpStart: {

                break;
            }
            case enBTRCoreTaskOpStop: {
                break;
            }
            case enBTRCoreTaskOpIdle: {
                break;
            }
            case enBTRCoreTaskOpProcess: {
                if (BtrCore_BTSendReceiveMessages(pstlhBTRCore->connHdl) != 0) {
                    *penExitStatusRunTask = enBTRCoreFailure;
                    lbRunTaskExit = TRUE;
                }

                break;
            }
            case enBTRCoreTaskOpExit: {
                *penExitStatusRunTask = enBTRCoreSuccess;
                lbRunTaskExit = TRUE;

                break;
            }
            case enBTRCoreTaskOpUnknown: {
                break;
            }
            default:
                break;
            }
            
        }

    } while (lbRunTaskExit == FALSE);

    BTRCORELOG_INFO ("RunTask Exiting\n");


    return (gpointer)penExitStatusRunTask;
}


static gpointer
btrCore_OutTask (
    gpointer        apsthBTRCore
) {
    stBTRCoreHdl*   pstlhBTRCore        = NULL;
    enBTRCoreRet*   penExitStatusOutTask= NULL;
    enBTRCoreRet    lenBTRCoreRet       = enBTRCoreSuccess;

    gint64                      li64usTimeout       = 0;
    guint16                     lui16msTimeout      = 50;
    BOOLEAN                     lbOutTaskExit       = FALSE;
    enBTRCoreTaskOp             lenOutTskOpPrv      = enBTRCoreTaskOpUnknown;
    enBTRCoreTaskOp             lenOutTskOpCur      = enBTRCoreTaskOpUnknown;
    enBTRCoreTaskProcessType    lenOutTskPTCur      = enBTRCoreTaskPTUnknown;
    stBTRCoreOTskInData*        lpstOutTskInData    = NULL;
    stBTRCoreTaskGAqData*       lpstOutTaskGAqData  = NULL;



    pstlhBTRCore = (stBTRCoreHdl*)apsthBTRCore;

    if (!(penExitStatusOutTask = g_malloc0(sizeof(enBTRCoreRet)))) {
        BTRCORELOG_ERROR ("OutTask failure - No Memory\n");
        return NULL;
    }

    if (!pstlhBTRCore || !pstlhBTRCore->connHdl) {
        BTRCORELOG_ERROR ("OutTask failure - BTRCore not initialized\n");
        *penExitStatusOutTask = enBTRCoreNotInitialized;
        return (gpointer)penExitStatusOutTask;
    }


    BTRCORELOG_DEBUG ("OutTask Started\n");

    do {

        /* Process incoming task-ops */
        {
            li64usTimeout = lui16msTimeout * G_TIME_SPAN_MILLISECOND;
            if ((pstlhBTRCore->pGAQueueOutTask) &&
                (lpstOutTaskGAqData = (stBTRCoreTaskGAqData*)(g_async_queue_timeout_pop(pstlhBTRCore->pGAQueueOutTask, li64usTimeout)))) {
                lenOutTskOpCur  = lpstOutTaskGAqData->enBTRCoreTskOp;
                lenOutTskPTCur  = lpstOutTaskGAqData->enBTRCoreTskPT;
                lpstOutTskInData= lpstOutTaskGAqData->pvBTRCoreTskInData;
                g_free(lpstOutTaskGAqData);
                lpstOutTaskGAqData = NULL;
                BTRCORELOG_DEBUG ("g_async_queue_timeout_pop %d %d %p\n", lenOutTskOpCur, lenOutTskPTCur, lpstOutTskInData);
            }
        }


        /* Set up operation - Schedule state changes for next interation */
        if (lenOutTskOpPrv != lenOutTskOpCur) {
            lenOutTskOpPrv = lenOutTskOpCur;

            switch (lenOutTskOpCur) {
            case enBTRCoreTaskOpStart: {
                BTRCORELOG_DEBUG ("enBTRCoreTaskOpStart\n");
                if ((lenBTRCoreRet = btrCore_OutTaskAddOp(pstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpIdle, enBTRCoreTaskPTUnknown, NULL)) != enBTRCoreSuccess) {
                    BTRCORELOG_ERROR("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTUnknown = %d\n", lenBTRCoreRet);
                }
                
                break;
            }
            case enBTRCoreTaskOpStop: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpStop\n");
                
                break;
            }
            case enBTRCoreTaskOpIdle: {
                BTRCORELOG_DEBUG ("enBTRCoreTaskOpIdle\n");
                
                break;
            }
            case enBTRCoreTaskOpProcess: {
                BTRCORELOG_TRACE ("enBTRCoreTaskOpProcess\n");
                if ((lenBTRCoreRet = btrCore_OutTaskAddOp(pstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpIdle, enBTRCoreTaskPTUnknown, NULL)) != enBTRCoreSuccess) {
                    BTRCORELOG_ERROR("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTUnknown = %d\n", lenBTRCoreRet);
                }

                break;
            }
            case enBTRCoreTaskOpExit: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpExit\n");

                break;
            }
            case enBTRCoreTaskOpUnknown: {
                BTRCORELOG_INFO ("enBTRCoreTaskOpUnknown\n");

                break;
            }
            default:
                BTRCORELOG_INFO ("default\n");
                break;
            }

        }


        /* Process Operations */
        /* Should handle all State updates - Handle with care */
        {
            switch (lenOutTskOpCur) {
            case enBTRCoreTaskOpStart: {

                break;
            }
            case enBTRCoreTaskOpStop: {
                break;
            }
            case enBTRCoreTaskOpIdle: {
                break;
            }
            case enBTRCoreTaskOpProcess: {

                if (lenOutTskPTCur == enBTRCoreTaskPTcBDeviceDisc) {

                    if (lpstOutTskInData && lpstOutTskInData->pstBTDevInfo) {
                        stBTDeviceInfo*     lpstBTDeviceInfo = (stBTDeviceInfo*)lpstOutTskInData->pstBTDevInfo;
                        stBTRCoreBTDevice   lstFoundDevice;
                        int                 i32ScannedDevIdx = -1;

                        if ((i32ScannedDevIdx = btrCore_AddDeviceToScannedDevicesArr(pstlhBTRCore, lpstBTDeviceInfo, &lstFoundDevice)) != -1) {
                            BTRCORELOG_INFO ("btrCore_AddDeviceToScannedDevicesArr - Success Index = %d\n", i32ScannedDevIdx);

                            pstlhBTRCore->stDiscoveryCbInfo.type = enBTRCoreOpTypeDevice;
                            MEMCPY_S(&pstlhBTRCore->stDiscoveryCbInfo.device,sizeof(pstlhBTRCore->stDiscoveryCbInfo.device), &lstFoundDevice, sizeof(stBTRCoreBTDevice));

                            if (pstlhBTRCore->fpcBBTRCoreDeviceDisc) {
                                if ((lenBTRCoreRet = pstlhBTRCore->fpcBBTRCoreDeviceDisc(&pstlhBTRCore->stDiscoveryCbInfo,
                                                                                          pstlhBTRCore->pvcBDevDiscUserData)) != enBTRCoreSuccess) {
                                    BTRCORELOG_ERROR ("Failure fpcBBTRCoreDeviceDisc Ret = %d\n", lenBTRCoreRet);
                                }
                            }
                        }

                        g_free(lpstOutTskInData->pstBTDevInfo);
                        lpstOutTskInData->pstBTDevInfo = NULL;
                    }

                    if (lpstOutTskInData) {
                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBDeviceRemoved) {

                    if (lpstOutTskInData && lpstOutTskInData->pstBTDevInfo) {
                        stBTDeviceInfo*     lpstBTDeviceInfo = (stBTDeviceInfo*)lpstOutTskInData->pstBTDevInfo;
                        stBTRCoreBTDevice   lstRemovedDevice;
                        tBTRCoreDevId       lBTRCoreDevId = lpstOutTskInData->bTRCoreDevId;

                        (void)lpstBTDeviceInfo;
                        MEMSET_S(&lstRemovedDevice, sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));

                        lenBTRCoreRet = btrCore_RemoveDeviceFromScannedDevicesArr (pstlhBTRCore, lBTRCoreDevId, &lstRemovedDevice);

                        if (lenBTRCoreRet == enBTRCoreSuccess && lstRemovedDevice.tDeviceId) {

                            pstlhBTRCore->stDiscoveryCbInfo.type = enBTRCoreOpTypeDevice;
                            MEMCPY_S(&pstlhBTRCore->stDiscoveryCbInfo.device,sizeof(pstlhBTRCore->stDiscoveryCbInfo.device), &lstRemovedDevice, sizeof(stBTRCoreBTDevice));

                            if (pstlhBTRCore->fpcBBTRCoreDeviceDisc) {
                                if ((lenBTRCoreRet = pstlhBTRCore->fpcBBTRCoreDeviceDisc(&pstlhBTRCore->stDiscoveryCbInfo,
                                        pstlhBTRCore->pvcBDevDiscUserData)) != enBTRCoreSuccess) {
                                    BTRCORELOG_ERROR ("Failure fpcBBTRCoreDeviceDisc Ret = %d\n", lenBTRCoreRet);
                                }
                            }
                        }
                        else {
                            BTRCORELOG_ERROR ("Failed to remove dev %lld from Scanned List | Ret = %d\n", lBTRCoreDevId, lenBTRCoreRet);
                        }


                        if (!pstlhBTRCore->numOfScannedDevices) {
                            BTRCORELOG_INFO ("\nClearing Scanned Device List...\n");
                            btrCore_ClearScannedDevicesList(pstlhBTRCore);

                            pstlhBTRCore->stDevStatusCbInfo.deviceId         = 0; // Need to have any special IDs for this purpose like 0xFFFFFFFF
                            pstlhBTRCore->stDevStatusCbInfo.eDevicePrevState = enBTRCoreDevStFound;
                            pstlhBTRCore->stDevStatusCbInfo.eDeviceCurrState = enBTRCoreDevStLost;
                            pstlhBTRCore->stDevStatusCbInfo.isPaired         = 0;           

                            if (pstlhBTRCore->fpcBBTRCoreStatus) {
                                // We are already in OutTask - But use btrCore_OutTaskAddOp - Dont trigger Status callbacks from DeviceRemoved Process Type
                                //if (pstlhBTRCore->fpcBBTRCoreStatus(&pstlhBTRCore->stDevStatusCbInfo, pstlhBTRCore->pvcBStatusUserData) != enBTRCoreSuccess) {
                                //}      
                            }
                        }

                        g_free(lpstOutTskInData->pstBTDevInfo);
                        lpstOutTskInData->pstBTDevInfo = NULL;
                    }

                    if (lpstOutTskInData) {
                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBDeviceLost) {

                    if (lpstOutTskInData && lpstOutTskInData->pstBTDevInfo) {
                        stBTDeviceInfo*     lpstBTDeviceInfo = (stBTDeviceInfo*)lpstOutTskInData->pstBTDevInfo;
                        tBTRCoreDevId       lBTRCoreDevId = lpstOutTskInData->bTRCoreDevId;
                        enBTRCoreDeviceType  lenBTRCoreDevType = lpstOutTskInData->enBTRCoreDevType;
                        int                 i32LoopIdx = -1;
                        int                 i32KnownDevIdx  = -1, i32ScannedDevIdx = -1;
                        BOOLEAN             postEvent = FALSE;
                        
                        (void)lpstBTDeviceInfo;

                        for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                            if (lBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                                i32KnownDevIdx = i32LoopIdx;
                                break;
                            }
                        }


                        if ((i32KnownDevIdx != -1) && (pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].tDeviceId == lBTRCoreDevId)) {
                            BTRCORELOG_INFO ("Device %llu power state Off or OOR\n", pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].tDeviceId);
                            BTRCORELOG_TRACE ("i32LoopIdx = %d\n", i32KnownDevIdx);
                            BTRCORELOG_TRACE ("pstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState);
                            BTRCORELOG_TRACE ("pstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDevicePrevState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState);

                            if (((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStConnected) ||
                                 (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStPlaying)) &&
                                 (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStDisconnecting)) {

                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = pstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState;
                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = enBTRCoreDevStDisconnected;
                                pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected = FALSE;
                                postEvent = TRUE;
                            }
                            else if (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPlaying   ||
                                     pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStConnected ) {

                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState =  pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState;
                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = enBTRCoreDevStLost;
                                pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected = FALSE;
                                postEvent = TRUE;
                            }
                            else if (((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStSuspended &&
                                     pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStDisconnecting) ||
                                   (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStDisconnecting &&
                                     pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStSuspended)) &&
                                    (pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected == FALSE)) {
                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = pstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState;
                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = enBTRCoreDevStDisconnected;
                                postEvent = TRUE;
                                BTRCORELOG_TRACE("Device went to suspended state, so marking the device state as - %d \n",pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState);
                            }
                            // move this out of if block. populating stDevStatusCbInfo should be done common for both paired and scanned devices 
                            if (postEvent) {
                                pstlhBTRCore->stDevStatusCbInfo.deviceId           = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].tDeviceId;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceClass       = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType;
                                pstlhBTRCore->stDevStatusCbInfo.ui32DevClassBtSpec = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32DevClassBtSpec;
                                pstlhBTRCore->stDevStatusCbInfo.ui16DevAppearanceBleSpec = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui16DevAppearanceBleSpec;
                                pstlhBTRCore->stDevStatusCbInfo.eDevicePrevState   = pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceCurrState   = pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceType        = btrCore_MapDevClassToDevType(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType);
                                pstlhBTRCore->stDevStatusCbInfo.isPaired           = 1;
                                pstlhBTRCore->stDevStatusCbInfo.ui32VendorId       = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasVendorId;
                                pstlhBTRCore->stDevStatusCbInfo.ui32ProductId      = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasProductId;
                                pstlhBTRCore->stDevStatusCbInfo.ui32DeviceId       = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasDeviceId;
                                strncpy(pstlhBTRCore->stDevStatusCbInfo.deviceName, pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].pcDeviceName, BD_NAME_LEN);
                                strncpy(pstlhBTRCore->stDevStatusCbInfo.deviceAddress, pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].pcDeviceAddress, BD_NAME_LEN);

                                if (pstlhBTRCore->fpcBBTRCoreStatus) {
                                    if ((lenBTRCoreRet = pstlhBTRCore->fpcBBTRCoreStatus(&pstlhBTRCore->stDevStatusCbInfo, pstlhBTRCore->pvcBStatusUserData)) != enBTRCoreSuccess) {
                                        BTRCORELOG_ERROR ("Failure fpcBBTRCoreStatus Ret = %d\n", lenBTRCoreRet);
                                    }
                                }
                            }

                            /* We usually remove the devices when we do the unpair. But xbox gen4 f/w 5.0.9 device immediately removed
                             * from bluez except Bluetooth 5.2, 5.3 and above. So we are remove the device from known and scanned list.
                             * If Bluetooth version is 5.2, 5.3 or above, || privacy is enabled, we need to add the disable_unsupported_gamepad DISTRO feature in the builds.
                             * We have enabled BT_UNSUPPORTED_GAMEPAD_ENABLED macro based on this distro feature */
                            if(BTRCore_IsUnsupportedGamepad(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasVendorId,
                                                pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasProductId,
                                                pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasDeviceId)) {
                                lenBTRCoreDevType = btrCore_MapDevClassToDevType(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType);
                                BTRCORELOG_INFO ("Device type %d Modalias v%04Xp%04Xd%04X\n",
                                        btrCore_MapDevClassToDevType(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType),
                                        pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasVendorId,
                                        pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasProductId,
                                        pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasDeviceId);
                                if (lenBTRCoreDevType == enBTRCoreHID) {
                                    BTRCORELOG_INFO ("Remove the device from known list %llu\n",pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].tDeviceId);
                                    btrCore_RemoveDeviceFromKnownDevicesArr(pstlhBTRCore, pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].tDeviceId);
                                }
                            }
                        }

                        /* We usually remove the devices when we do the unpair. But xbox gen4 f/w 5.0.9 device immediately removed
                         * from bluez except Bluetooth 5.2, 5.3 and above. So we are remove the device from known and scanned list.
                         * If Bluetooth version is 5.2, 5.3 or above, || privacy is enabled, we need to add the disable_unsupported_gamepad DISTRO feature in the builds.
                         * We have enabled BT_UNSUPPORTED_GAMEPAD_ENABLED macro based on this distro feature */
                        if (pstlhBTRCore->numOfScannedDevices) {
                            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
                                if (lBTRCoreDevId == pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
                                    i32ScannedDevIdx = i32LoopIdx;
                                    break;
                                }
                            }
                        }

                        if (i32ScannedDevIdx != -1) {
                            if (BTRCore_IsUnsupportedGamepad(pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasVendorId,
                                                pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasProductId,
                                                pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasDeviceId)) {
                                BTRCORELOG_DEBUG ("device available in scanned list\n");
                                lenBTRCoreDevType = btrCore_MapDevClassToDevType(pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].enDeviceType);

                                BTRCORELOG_INFO ("Device type %d Modalias v%04Xp%04Xd%04X\n",
                                        btrCore_MapDevClassToDevType(pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].enDeviceType),
                                        pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasVendorId,
                                        pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasProductId,
                                        pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasDeviceId);

                                if (lenBTRCoreDevType == enBTRCoreHID) {
                                    stBTRCoreBTDevice       stScannedDevice;
                                    MEMSET_S(&stScannedDevice,sizeof(stBTRCoreBTDevice), 0 ,sizeof(stBTRCoreBTDevice));

                                    BTRCORELOG_INFO ("Remove the device from scan list %llu\n",lBTRCoreDevId);
                                    btrCore_RemoveDeviceFromScannedDevicesArr (pstlhBTRCore, lBTRCoreDevId, &stScannedDevice);
                                }
                            }
                        }

                        g_free(lpstOutTskInData->pstBTDevInfo);
                        lpstOutTskInData->pstBTDevInfo = NULL;
                    }

                    if (lpstOutTskInData) {
                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBDeviceStatus) {

                    if (lpstOutTskInData && lpstOutTskInData->pstBTDevInfo) {
                        stBTDeviceInfo* lpstBTDeviceInfo = (stBTDeviceInfo*)lpstOutTskInData->pstBTDevInfo;

                        int             i32LoopIdx       = -1;
                        int             i32KnownDevIdx   = -1;
                        int             i32ScannedDevIdx = -1;

                        tBTRCoreDevId        lBTRCoreDevId = lpstOutTskInData->bTRCoreDevId;
                        enBTRCoreDeviceType  lenBTRCoreDevType = lpstOutTskInData->enBTRCoreDevType;
                        enBTRCoreDeviceState leBTDevState  = btrCore_BTParseDeviceState(lpstBTDeviceInfo->pcDeviceCurrState);


                        if (pstlhBTRCore->numOfPairedDevices) {
                            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                                if (lBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                                    i32KnownDevIdx = i32LoopIdx;
                                    break;
                                }
                            }
                        }

                        if (pstlhBTRCore->numOfScannedDevices) {
                            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
                                if (lBTRCoreDevId == pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
                                    i32ScannedDevIdx = i32LoopIdx;
                                    break;
                                }
                            }
                        }


                        // Current device for which Property has changed must be either in Found devices or Paired devices
                        // TODO: if-else's for SM or HSM are bad. Find a better way
                        if (((i32ScannedDevIdx != -1) || (i32KnownDevIdx != -1)) && (leBTDevState != enBTRCoreDevStInitialized)) {
                            BOOLEAN                 bTriggerDevStatusChangeCb   = FALSE;
                            stBTRCoreBTDevice*      lpstBTRCoreBTDevice         = NULL;
                            stBTRCoreDevStateInfo*  lpstBTRCoreDevStateInfo     = NULL;

                            if ((i32KnownDevIdx != -1) && (leBTDevState != enBTRCoreDevStPaired)) {
                                enBTRCoreDeviceType  lenBTRCoreMapDevType = btrCore_MapDevClassToDevType(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType);
                                BTRCORELOG_TRACE ("DevType: %d\n",lenBTRCoreMapDevType);
                                BTRCORELOG_TRACE ("i32KnownDevIdx = %d\n", i32KnownDevIdx);
                                BTRCORELOG_TRACE ("leBTDevState = %d\n", leBTDevState);
                                BTRCORELOG_TRACE ("lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState);
                                BTRCORELOG_TRACE ("lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState);
                                if ((lenBTRCoreDevType == enBTRCoreUnknown) && (leBTDevState == enBTRCoreDevStConnected) &&
                                    (btrCore_IsStadiaGamepad(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].pcDeviceAddress) ||
                                     (pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].pcDeviceName[0] != '\0' &&
                                      strstr(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].pcDeviceName, "Stadia")))) {
                                      BTRCORELOG_INFO("Identified the device as stadia based on OUI values/device name ....\n");
                                      pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui16DevAppearanceBleSpec = BTRCORE_LE_HID_DEVICE_APPEARANCE;
                                      pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType = enBTRCore_DC_HID_GamePad;
                                }

                                if ((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState != leBTDevState) &&
                                    (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState != enBTRCoreDevStInitialized)) {

                                    if ((enBTRCoreMobileAudioIn != lenBTRCoreDevType) && (enBTRCorePCAudioIn != lenBTRCoreDevType)) {

                                        if ( !(((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState != enBTRCoreDevStPlaying) &&
                                                (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStConnected) && (leBTDevState == enBTRCoreDevStDisconnected)) ||
                                               ((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStDisconnected) && (leBTDevState == enBTRCoreDevStConnected) &&
                                                ((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState != enBTRCoreDevStPaired) ||
                                                 (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState != enBTRCoreDevStConnecting))) ||
                                                ((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPaired) &&
                                                 (leBTDevState == enBTRCoreDevStDisconnected) && (lenBTRCoreMapDevType == enBTRCoreHID)))) {
                                            bTriggerDevStatusChangeCb = TRUE;
                                        }

                                        if (((enBTRCoreHID == lenBTRCoreDevType) || (enBTRCoreUnknown == lenBTRCoreDevType) || (enBTRCoreLE == lenBTRCoreDevType)) &&
                                            (((enBTRCoreDevStConnected == pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState) && (enBTRCoreDevStDisconnected == leBTDevState)) ||
                                             ((enBTRCoreDevStDisconnected == pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState) && (enBTRCoreDevStConnected == leBTDevState)))) {
                                            BTRCORELOG_INFO ("leBTDevState = %d lenBTRCoreDevType =%d \n", leBTDevState,lenBTRCoreDevType);
                                            bTriggerDevStatusChangeCb = TRUE;

                                            if (strstr(pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].pcDeviceName, "Xbox")) {
                                                if (leBTDevState == enBTRCoreDevStConnected) {
                                                    if (BtrCore_BTDisableEnhancedRetransmissionMode(pstlhBTRCore->connHdl) != 0) {
                                                        BTRCORELOG_ERROR ("Failed to Disable ERTM\n");
                                                    }
                                                }
                                                else if (leBTDevState == enBTRCoreDevStDisconnected) {
                                                    if (BtrCore_BTEnableEnhancedRetransmissionMode(pstlhBTRCore->connHdl) != 0) {
                                                        BTRCORELOG_ERROR ("Failed to Enable ERTM\n");
                                                    }
                                                }
                                            }
                                        }

                                        // To make the state changes in a better logical way once the BTRCore dev structures are unified further

                                        //workaround for notifying the power Up event of a <paired && !connected> devices, as we are not able to track the
                                        //power Down event of such devices as per the current analysis
                                        if ((leBTDevState == enBTRCoreDevStDisconnected) &&
                                            ((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStConnected) ||
                                             (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStDisconnecting))) {
                                            pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = enBTRCoreDevStPaired;
                                        }
                                        else if ( !((leBTDevState == enBTRCoreDevStConnected) &&
                                                    ((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStDisconnecting) ||
                                                     (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStDisconnected)  ||
                                                     (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStInitialized)))) {
                                                if (!((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPaired) &&
                                                    (leBTDevState == enBTRCoreDevStDisconnected) && (lenBTRCoreMapDevType == enBTRCoreHID))) {
                                                     if (!(leBTDevState == enBTRCoreDevStConnected &&
                                                         pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPlaying)) {
                                                         pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState;
                                                     }
                                                }
                                        }
                                     

                                        if ((leBTDevState == enBTRCoreDevStDisconnected) &&
                                            (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPlaying)) {
                                            leBTDevState = enBTRCoreDevStLost;
                                            pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected = FALSE;
                                        }

                                        if((enBTRCoreHID == lenBTRCoreMapDevType) &&
                                            (leBTDevState == enBTRCoreDevStDisconnected) &&
                                            (enBTRCoreDevStConnected == pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState)) {
                                                lpstBTRCoreBTDevice     = &pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx];
                                                if ((lpstBTRCoreBTDevice->ui16DevAppearanceBleSpec == BTRCORE_LE_HID_DEVICE_APPEARANCE) && (!btrCore_IsDeviceRdkRcu(lpstBTRCoreBTDevice->pcDeviceAddress,lpstBTRCoreBTDevice->ui16DevAppearanceBleSpec))) {
                                                    gboolean IsDeviceStable = FALSE;
                                                    IsDeviceStable = btrCore_CheckLeHidConnectionStability (lpstBTRCoreBTDevice);
                                                    if (IsDeviceStable) {
                                                        BTRCORELOG_INFO("KDA : HID device identified as unstable, removing from action list. It may need repair....\n");
                                                        btrCore_RemoveUnstableDeviceFromActionList (lpstBTRCoreBTDevice);
                                                    }
                                                }
                                                leBTDevState = enBTRCoreDevStLost;
                                                pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected = FALSE;
                                        }

                                        if ( !((leBTDevState == enBTRCoreDevStDisconnected) &&
                                               (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStLost) &&
                                               (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStPlaying))) {

                                            if ( !((leBTDevState == enBTRCoreDevStConnected) &&
                                                 (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStLost) &&
                                                 (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState != enBTRCoreDevStPlaying))) {
                                                 if (!((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPaired) &&
                                                      (leBTDevState == enBTRCoreDevStDisconnected) && (lenBTRCoreMapDevType == enBTRCoreHID))) {
                                                       if (!(leBTDevState == enBTRCoreDevStConnected &&
                                                           pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPlaying)) {
                                                           pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState;
                                                           pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = leBTDevState;
                                                       }
                                                 }
                                            }
                                            else {
                                                if((enBTRCoreHID == lenBTRCoreMapDevType) && (enBTRCoreDevStConnected == leBTDevState) &&
                                                    (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStLost)) {
                                                    pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState;
                                                    pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = leBTDevState;
                                                }
                                                else {
                                                    pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = enBTRCoreDevStConnecting;
                                                }
                                            }
                                        }
                                        else {
                                            leBTDevState = enBTRCoreDevStConnected;
                                            pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = enBTRCoreDevStConnecting;
                                        }

                                    }
                                    else {
                                        bTriggerDevStatusChangeCb = TRUE;

                                        if (enBTRCoreDevStInitialized != pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState) {
                                            pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState =
                                                                           pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState;
                                        }
                                        else {
                                            pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = enBTRCoreDevStConnecting;
                                        }

                                        pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = leBTDevState;

                                        //TODO: There should be no need to do this. Find out why the enDeviceType = 0;
                                        pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType = btrCore_MapClassIDtoDevClass(lpstBTDeviceInfo->ui32Class);
                                    }

                                    pstlhBTRCore->stDevStatusCbInfo.isPaired = 1;
                                    lpstBTRCoreBTDevice     = &pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx];
                                    lpstBTRCoreDevStateInfo = &pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx];


                                    if ((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStConnected) &&
                                        (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStPaired)) {
                                        pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = enBTRCoreDevStPaired;
                                        pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = enBTRCoreDevStConnected;
                                    }

                                    BTRCORELOG_TRACE ("i32KnownDevIdx = %d\n", i32KnownDevIdx);
                                    BTRCORELOG_TRACE ("leBTDevState = %d\n", leBTDevState);
                                    BTRCORELOG_TRACE ("lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState);
                                    BTRCORELOG_TRACE ("lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState);
                                    BTRCORELOG_TRACE ("lpstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType       = %x\n", pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType);

                                    if (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStDisconnected) {
                                        pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected = FALSE;
                                    }
                                    else if (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStConnected) {
                                        pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected = TRUE;
                                    }

                                    pstlhBTRCore->stDevStatusCbInfo.ui32VendorId       = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasVendorId;
                                    pstlhBTRCore->stDevStatusCbInfo.ui32ProductId      = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasProductId;
                                    pstlhBTRCore->stDevStatusCbInfo.ui32DeviceId       = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasDeviceId;
                                }
                            }
                            else if (i32ScannedDevIdx != -1) {

                                BTRCORELOG_TRACE ("i32ScannedDevIdx = %d\n", i32ScannedDevIdx);
                                BTRCORELOG_TRACE ("leBTDevState = %d\n", leBTDevState);
                                BTRCORELOG_TRACE ("lpstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState = %d\n", pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState);
                                BTRCORELOG_TRACE ("lpstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState = %d\n", pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState);
                                BTRCORELOG_TRACE ("i32KnownDevIdx = %d\n", i32KnownDevIdx);

                                enBTRCoreDeviceType  lenBTRCoreMapDevType = btrCore_MapDevClassToDevType(pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].enDeviceType);
                                BTRCORELOG_INFO ("DevType: %d\n",lenBTRCoreMapDevType);
                                lpstBTRCoreBTDevice     = &pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx];

                                if ((lenBTRCoreMapDevType == enBTRCoreHID) && (lpstBTRCoreBTDevice->ui16DevAppearanceBleSpec == BTRCORE_LE_HID_DEVICE_APPEARANCE) && (leBTDevState == enBTRCoreDevStDisconnected) && !btrCore_IsDeviceRdkRcu(lpstBTRCoreBTDevice->pcDeviceAddress,lpstBTRCoreBTDevice->ui16DevAppearanceBleSpec)) {
                                    gboolean IsDeviceStable = FALSE;
                                    IsDeviceStable = btrCore_CheckLeHidConnectionStability (lpstBTRCoreBTDevice);
                                    if (IsDeviceStable) {
                                        BTRCORELOG_INFO("SDA : HID device identified as unstable, removing from action list. It may need repair....\n");
                                        btrCore_RemoveUnstableDeviceFromActionList (lpstBTRCoreBTDevice);
                                    }
                                }

                                char lpcBtVersion[BTRCORE_STR_LEN] = {'\0'};
                                if (enBTRCoreSuccess == BTRCore_GetVersionInfo(pstlhBTRCore, lpcBtVersion)) {
                                    BTRCORELOG_DEBUG("Bluez Version - %s \n",lpcBtVersion);
                                }
                                if (i32KnownDevIdx != -1 && pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStConnected && leBTDevState == enBTRCoreDevStPaired)
                                {
                                    BTRCORELOG_INFO("Device already in known devices array - check if connected\n");
                                    BTRCORELOG_TRACE ("lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState);
                                    BTRCORELOG_TRACE ("lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = %d\n", pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState);
                                    pstlhBTRCore->stKnownDevStInfoArr[i32ScannedDevIdx].eDevicePrevState = enBTRCoreDevStPaired;
                                    pstlhBTRCore->stKnownDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState = enBTRCoreDevStConnected;

                                    /* Before send paired event, device state is changed as paired when we have received characteristic events.
                                     * so paired event is not send to UI*/
                                    if((leBTDevState == enBTRCoreDevStPaired) && (enBTRCoreHID == lenBTRCoreMapDevType)) {
                                        pstlhBTRCore->stDevStatusCbInfo.isPaired = lpstBTDeviceInfo->bPaired;

                                        lpstBTRCoreBTDevice = &pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx];
                                        lpstBTRCoreDevStateInfo = &pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx];

                                        pstlhBTRCore->stDevStatusCbInfo.ui32VendorId  = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasVendorId;
                                        pstlhBTRCore->stDevStatusCbInfo.ui32ProductId = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasProductId;
                                        pstlhBTRCore->stDevStatusCbInfo.ui32DeviceId  = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].ui32ModaliasDeviceId;

                                        bTriggerDevStatusChangeCb = TRUE;
                                    }
                                }
                                else if ((pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState != leBTDevState) &&
                                    (pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState != enBTRCoreDevStInitialized) &&
                                    !((pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState == enBTRCoreDevStFound) &&
                                    (leBTDevState == enBTRCoreDevStConnected) && (enBTRCoreHID == lenBTRCoreMapDevType) &&
                                    !((lpstBTRCoreBTDevice->ui32ModaliasVendorId == BT_PS4_HID_VENDOR_ID) && (lpstBTRCoreBTDevice->ui32ModaliasProductId == BT_PS4_HID_PRODUCT_ID_1 || lpstBTRCoreBTDevice->ui32ModaliasProductId == BT_PS4_HID_PRODUCT_ID_2) && (0 == strncmp(lpcBtVersion, "Bluez-5.54", strlen("Bluez-5.54")))))) {

                                    pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState = pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState;
                                    pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState = leBTDevState;
                                    pstlhBTRCore->stDevStatusCbInfo.isPaired = lpstBTDeviceInfo->bPaired;

                                    lpstBTRCoreDevStateInfo = &pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx];

                                    if (lpstBTDeviceInfo->bPaired       &&
                                        lpstBTDeviceInfo->bConnected    &&
                                        (((pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState == enBTRCoreDevStFound)     &&
                                          (pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState == enBTRCoreDevStConnected))  ||
                                         ((pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState == enBTRCoreDevStFound)     &&
                                          (pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState == enBTRCoreDevStPaired))     ||
                                         ((pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState == enBTRCoreDevStConnected) &&
                                          (pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState == enBTRCoreDevStPaired)))) {

                                        if ((i32KnownDevIdx = btrCore_AddDeviceToKnownDevicesArr(pstlhBTRCore, lpstBTDeviceInfo)) != -1) {

                                            if ((pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState == enBTRCoreDevStConnected) &&
                                                (pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState == enBTRCoreDevStPaired)) {
                                                pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState = enBTRCoreDevStPaired;
                                                pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState = enBTRCoreDevStConnected;
                                            }

                                            if ((pstlhBTRCore->stKnownDevStInfoArr[i32ScannedDevIdx].eDevicePrevState == enBTRCoreDevStConnected) &&
                                                (pstlhBTRCore->stKnownDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState == enBTRCoreDevStPaired)) {
                                                pstlhBTRCore->stKnownDevStInfoArr[i32ScannedDevIdx].eDevicePrevState = enBTRCoreDevStPaired;
                                                pstlhBTRCore->stKnownDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState = enBTRCoreDevStConnected;
                                            }

                                            if (!((pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState == enBTRCoreDevStPaired) &&
                                                  (pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState == enBTRCoreDevStConnected))) {
                                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState = pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState;
                                                pstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState = pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState;
                                            }

                                            BTRCORELOG_INFO ("btrCore_AddDeviceToKnownDevicesArr - Success Index = %d\n", i32KnownDevIdx);
                                        }

                                        //TODO: Really really dont like this - Live with it for now
                                        if (pstlhBTRCore->numOfPairedDevices) {
                                            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                                                if (lBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                                                    //i32KnownDevIdx = i32LoopIdx;
                                                    lpstBTRCoreBTDevice     = &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx];
                                                    lpstBTRCoreDevStateInfo = &pstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx];
                                                }
                                            }
                                        }
                                    }

                                    if (lenBTRCoreDevType   == enBTRCoreLE &&
                                        (leBTDevState       == enBTRCoreDevStConnected   ||
                                         leBTDevState       == enBTRCoreDevStDisconnected)) {
                                        lpstBTRCoreBTDevice     = &pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx];
                                        lpstBTRCoreDevStateInfo = &pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx];

                                        if (leBTDevState == enBTRCoreDevStDisconnected) {
                                            lpstBTRCoreBTDevice->bDeviceConnected = FALSE;
                                        }

                                        if (pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState == enBTRCoreDevStInitialized) {
                                            pstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState = enBTRCoreDevStConnecting;
                                        }
                                    }

                                    pstlhBTRCore->stDevStatusCbInfo.ui32VendorId    = pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasVendorId;
                                    pstlhBTRCore->stDevStatusCbInfo.ui32ProductId   = pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasProductId;
                                    pstlhBTRCore->stDevStatusCbInfo.ui32DeviceId    = pstlhBTRCore->stScannedDevicesArr[i32ScannedDevIdx].ui32ModaliasDeviceId;

                                    bTriggerDevStatusChangeCb = TRUE;
                                }
                            }

                            if (bTriggerDevStatusChangeCb == TRUE) {

                                if (lpstBTRCoreBTDevice->enDeviceType == enBTRCore_DC_Unknown) {
                                    for (i32LoopIdx = 0; i32LoopIdx < lpstBTRCoreBTDevice->stDeviceProfile.numberOfService; i32LoopIdx++) {
                                        if (lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_A2SNK, NULL, 16)) {
                                            lpstBTRCoreBTDevice->enDeviceType = enBTRCore_DC_Loudspeaker;
                                        }
                                        else if (lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_A2SRC, NULL, 16)) {
                                            lpstBTRCoreBTDevice->enDeviceType = enBTRCore_DC_SmartPhone;
                                        }
                                        else if ((lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_GATT_TILE_1, NULL, 16)) ||
                                                 (lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_GATT_TILE_2, NULL, 16)) ||
                                                 (lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_GATT_TILE_3, NULL, 16))) {
                                            lpstBTRCoreBTDevice->enDeviceType = enBTRCore_DC_Tile;
                                        }
                                        else if (lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_HID_1, NULL, 16) ||
                                                 lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_HID_2, NULL, 16)) {
                                            lpstBTRCoreBTDevice->enDeviceType = enBTRCore_DC_HID_Keyboard;
                                        }
                                        else if ((lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_GATT_XBB_1, NULL, 16)) ||
                                                 (lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_1, NULL, 16)) ||
                                                 (lpstBTRCoreBTDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_3, NULL, 16))) {
                                            lpstBTRCoreBTDevice->enDeviceType = enBTRCore_DC_XBB;
                                        }
                                    }
                                }

                                pstlhBTRCore->stDevStatusCbInfo.deviceId           = lBTRCoreDevId;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceType        = btrCore_MapDevClassToDevType(lpstBTRCoreBTDevice->enDeviceType);
                                pstlhBTRCore->stDevStatusCbInfo.eDevicePrevState   = lpstBTRCoreDevStateInfo->eDevicePrevState;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceCurrState   = leBTDevState;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceClass       = lpstBTRCoreBTDevice->enDeviceType;
                                pstlhBTRCore->stDevStatusCbInfo.ui32DevClassBtSpec = lpstBTRCoreBTDevice->ui32DevClassBtSpec;
                                pstlhBTRCore->stDevStatusCbInfo.ui16DevAppearanceBleSpec = lpstBTRCoreBTDevice->ui16DevAppearanceBleSpec;
                                strncpy(pstlhBTRCore->stDevStatusCbInfo.deviceName, lpstBTRCoreBTDevice->pcDeviceName, BD_NAME_LEN);
                                strncpy(pstlhBTRCore->stDevStatusCbInfo.deviceAddress, lpstBTRCoreBTDevice->pcDeviceAddress, BD_NAME_LEN);                                

                                if (pstlhBTRCore->fpcBBTRCoreStatus) {
                                    if (pstlhBTRCore->fpcBBTRCoreStatus(&pstlhBTRCore->stDevStatusCbInfo, pstlhBTRCore->pvcBStatusUserData) != enBTRCoreSuccess) {
                                        /* Invoke the callback */
                                    }
                                }
                            }
                        }

                        g_free(lpstOutTskInData->pstBTDevInfo);
                        lpstOutTskInData->pstBTDevInfo = NULL;
                    }

                    if (lpstOutTskInData) {
                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBAdapterStatus) {
                    if (lpstOutTskInData) {
                        stBTRCoreAdapter* lpstBTRCoreAdapter = (stBTRCoreAdapter*)lpstOutTskInData;

                        BTRCORELOG_TRACE ("stDiscoveryCbInfo.adapter.bDiscovering = %d, lpstBTRCoreAdapter->bDiscovering = %d\n",
                                pstlhBTRCore->stDiscoveryCbInfo.adapter.bDiscovering, lpstBTRCoreAdapter->bDiscovering);
                        BTRCORELOG_TRACE("Adv timeout: %d\n",lpstBTRCoreAdapter->bAdvertisingTimedout);
                        // invoke the callback to mgr only if the adapter's properties (such as its discovering state) changed
                        if ((pstlhBTRCore->stDiscoveryCbInfo.adapter.bDiscovering != lpstBTRCoreAdapter->bDiscovering) || lpstBTRCoreAdapter->bAdvertisingTimedout)
                        {
                            pstlhBTRCore->stDiscoveryCbInfo.type = enBTRCoreOpTypeAdapter;
                            MEMCPY_S(&pstlhBTRCore->stDiscoveryCbInfo.adapter, sizeof(pstlhBTRCore->stDiscoveryCbInfo.adapter),lpstBTRCoreAdapter, sizeof(stBTRCoreAdapter));
                            if (pstlhBTRCore->fpcBBTRCoreDeviceDisc) {
                                if ((lenBTRCoreRet = pstlhBTRCore->fpcBBTRCoreDeviceDisc(&pstlhBTRCore->stDiscoveryCbInfo,
                                        pstlhBTRCore->pvcBDevDiscUserData)) != enBTRCoreSuccess) {
                                    BTRCORELOG_ERROR ("Failure fpcBBTRCoreDeviceDisc Ret = %d\n", lenBTRCoreRet);
                                }
                            }
                        }

                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBMediaStatus) {
                    if (lpstOutTskInData) {
                        stBTRCoreMediaStatusCBInfo* lpstMediaStatusUpdateCbInfo = (stBTRCoreMediaStatusCBInfo*)lpstOutTskInData;
                        tBTRCoreDevId               lBTRCoreDevId   = lpstMediaStatusUpdateCbInfo->deviceId;
                        int                         i32LoopIdx      = -1;
                        int                         i32KnownDevIdx  = -1;

                        for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                            if (lBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                                i32KnownDevIdx = i32LoopIdx;
                                break;
                            }
                        }

                        BTRCORELOG_TRACE ("i32KnownDevIdx = %d\n", i32KnownDevIdx);

                        if ((i32KnownDevIdx != -1) &&
                            ((pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].bDeviceConnected == TRUE) ||
                             (lpstMediaStatusUpdateCbInfo->m_mediaStatusUpdate.bIsMediaCtrlAvailable == TRUE))) {
                            MEMCPY_S(&pstlhBTRCore->stMediaStatusCbInfo,sizeof(pstlhBTRCore->stMediaStatusCbInfo), lpstMediaStatusUpdateCbInfo, sizeof(stBTRCoreMediaStatusCBInfo));
                            pstlhBTRCore->stMediaStatusCbInfo.deviceId      = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].tDeviceId;
                            pstlhBTRCore->stMediaStatusCbInfo.eDeviceClass  = pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].enDeviceType;
                            strncpy(pstlhBTRCore->stMediaStatusCbInfo.deviceName, pstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx].pcDeviceName, BD_NAME_LEN);

                            if (pstlhBTRCore->fpcBBTRCoreMediaStatus) {
                                if ((lenBTRCoreRet = pstlhBTRCore->fpcBBTRCoreMediaStatus(&pstlhBTRCore->stMediaStatusCbInfo, pstlhBTRCore->pvcBMediaStatusUserData)) != enBTRCoreSuccess) {
                                    BTRCORELOG_ERROR ("Failure fpcBBTRCoreMediaStatus Ret = %d\n", lenBTRCoreRet);
                                }
                            }
                        }

                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBDevOpInfoStatus) {
                    if (lpstOutTskInData) {
                        stBTRCoreDevStatusCBInfo* lpstDevStatusCbInfo = (stBTRCoreDevStatusCBInfo*)lpstOutTskInData;
                        MEMCPY_S(&pstlhBTRCore->stDevStatusCbInfo,sizeof(pstlhBTRCore->stDevStatusCbInfo), lpstDevStatusCbInfo,sizeof(stBTRCoreDevStatusCBInfo));
                        
                        if (pstlhBTRCore->fpcBBTRCoreStatus) {
                            if ((lenBTRCoreRet = pstlhBTRCore->fpcBBTRCoreStatus(&pstlhBTRCore->stDevStatusCbInfo, pstlhBTRCore->pvcBStatusUserData)) != enBTRCoreSuccess) {
                                BTRCORELOG_ERROR ("Failure fpcBBTRCoreStatus Ret = %d\n", lenBTRCoreRet);
                            }
                        }

                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBConnIntim) {
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBConnAuth) {
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTcBModaliasUpdate) {
                    if (lpstOutTskInData && lpstOutTskInData->pstBTDevInfo) {
                        BTRCORELOG_INFO ("btcore out task enBTRCoreTaskPTcBModaliasUpdate\n");
                        stBTDeviceInfo* lpstBTDeviceInfo = (stBTDeviceInfo*)lpstOutTskInData->pstBTDevInfo;
                        int             i32LoopIdx       = -1;
                        tBTRCoreDevId   lBTRCoreDevId = lpstOutTskInData->bTRCoreDevId;
                        unsigned char   bIsDeviceExist = 0;

                        if (pstlhBTRCore->numOfPairedDevices) {
                            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                                if (lBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                                    BTRCORELOG_INFO ("Modalias updated in known list %s\n", lpstBTDeviceInfo->pcModalias);
                                    btrCore_PopulateModaliasValues (lpstBTDeviceInfo->pcModalias, &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].ui32ModaliasVendorId,
                                        &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].ui32ModaliasProductId,
                                        &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].ui32ModaliasDeviceId);
                                        break;
                                }
                            }
                        }

                        if (pstlhBTRCore->numOfScannedDevices) {
                            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
                                if (lBTRCoreDevId == pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
                                    BTRCORELOG_INFO ("Modalias updated in scanned list %s\n", lpstBTDeviceInfo->pcModalias);
                                    btrCore_PopulateModaliasValues (lpstBTDeviceInfo->pcModalias, &pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasVendorId,
                                                &pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasProductId,
                                                &pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasDeviceId);
                                    bIsDeviceExist = 1;
                                    break;
                                }
                            }
                        }

                        if (bIsDeviceExist) {
                            if(BTRCore_IsUnsupportedGamepad(pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasVendorId,
                                            pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasProductId,
                                            pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasDeviceId)) {

                                BTRCORELOG_INFO ("Unsupported device detected: %s\n", lpstBTDeviceInfo->pcModalias);

                                //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
                                BTRCORELOG_ERROR ("Failed to pair a device name,class,apperance,modalias: %s,%u,%u,v%04Xp%04Xd%04X\n",
                                pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].pcDeviceName, pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32DevClassBtSpec,
                                pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui16DevAppearanceBleSpec,
                                pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasVendorId,
                                pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasProductId,
                                pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasDeviceId);

                                pstlhBTRCore->stDevStatusCbInfo.deviceId           = lBTRCoreDevId;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceType        = btrCore_MapDevClassToDevType(pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].enDeviceType);
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceCurrState   = enBTRCoreDevStUnsupported;
                                pstlhBTRCore->stDevStatusCbInfo.eDeviceClass       = pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].enDeviceType;
                                pstlhBTRCore->stDevStatusCbInfo.ui32DevClassBtSpec = pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32DevClassBtSpec;
                                pstlhBTRCore->stDevStatusCbInfo.ui16DevAppearanceBleSpec = pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui16DevAppearanceBleSpec;

                                strncpy(pstlhBTRCore->stDevStatusCbInfo.deviceName, pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].pcDeviceName, BD_NAME_LEN);
                                strncpy(pstlhBTRCore->stDevStatusCbInfo.deviceAddress,pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].pcDeviceAddress,BD_NAME_LEN);

                                pstlhBTRCore->stDevStatusCbInfo.ui32VendorId       = pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasVendorId;
                                pstlhBTRCore->stDevStatusCbInfo.ui32ProductId      = pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasProductId;
                                pstlhBTRCore->stDevStatusCbInfo.ui32DeviceId       = pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasDeviceId;

                                if (pstlhBTRCore->fpcBBTRCoreStatus) {
                                    if ((lenBTRCoreRet = pstlhBTRCore->fpcBBTRCoreStatus(&pstlhBTRCore->stDevStatusCbInfo, pstlhBTRCore->pvcBStatusUserData)) != enBTRCoreSuccess) {
                                        BTRCORELOG_ERROR ("Failure fpcBBTRCoreStatus Ret = %d\n", lenBTRCoreRet);
                                    }
                                    else {
                                        //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
                                        BTRCORELOG_INFO ("Unsupport BT device detected v%04Xp%04Xd%04X\n",
                                        pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasVendorId,
                                        pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasProductId,
                                        pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].ui32ModaliasDeviceId);
                                    }
                                }
                            }
                        }

                        g_free(lpstOutTskInData->pstBTDevInfo);
                        lpstOutTskInData->pstBTDevInfo = NULL;

                        g_free(lpstOutTskInData);
                        lpstOutTskInData = NULL;
                    }
                }
                else if (lenOutTskPTCur == enBTRCoreTaskPTUnknown) {
                }
                else {
                }

                break;
            }
            case enBTRCoreTaskOpExit: {
                lbOutTaskExit = TRUE;
                break;
            }
            case enBTRCoreTaskOpUnknown: {

                break;
            }
            default:
                break;
            }

        }

    } while (lbOutTaskExit == FALSE);

    BTRCORELOG_INFO ("OutTask Exiting\n");


    *penExitStatusOutTask = enBTRCoreSuccess;
    return (gpointer)penExitStatusOutTask;
}

static gpointer
btrCore_BatteryLevelThread (
    gpointer        apsthBTRCore
) {
    stBTRCoreHdl* lpstlhBTRCore = (stBTRCoreHdl*)apsthBTRCore;
    unsigned char ui8NumberDevicesAvailable = 1;
    BOOLEAN bLowBatteryDeviceFound = FALSE;
    int64_t i64EndWaitTime;
    uint8_t ui8RetriesForNewThread = BATTERY_LEVEL_RETRY_ATTEMPTS;
    BTRCORELOG_INFO("Battery thread started");
    while (ui8NumberDevicesAvailable != 0)
    {
        BTRCORELOG_TRACE("Battery thread woken");
        g_mutex_lock(&lpstlhBTRCore->batteryLevelMutex);
        if(lpstlhBTRCore->batteryLevelThreadExit)
        {
            g_mutex_unlock(&lpstlhBTRCore->batteryLevelMutex);
            break;
        }
        g_mutex_unlock(&lpstlhBTRCore->batteryLevelMutex);

        sleep(1); //sleep for 1 second to ensure connection parameters completely
        if( btrCore_updateBatteryLevelsForConnectedDevices(lpstlhBTRCore, &ui8NumberDevicesAvailable, &bLowBatteryDeviceFound) == enBTRCoreSuccess)
        {
            BTRCORELOG_DEBUG("Battery levels successfully updated\n");
            lpstlhBTRCore->batteryLevelRefreshInterval = (bLowBatteryDeviceFound) ? BTRCORE_LOW_BATTERY_REFRESH_INTERVAL : BTRCORE_BATTERY_REFRESH_INTERVAL;
            if (!ui8NumberDevicesAvailable)
            {
                // if the thread is just started but can't read battery level try again every 5 seconds for 30 seconds
                if (ui8RetriesForNewThread)
                {
                    ui8RetriesForNewThread--;
                    lpstlhBTRCore->batteryLevelRefreshInterval = BATTERY_LEVEL_NOT_FOUND_REFRESH_INTERVAL;
                }
                else
                {
                    break;
                }
            }
            else 
            {
                ui8RetriesForNewThread = 0;
            }
        }
        i64EndWaitTime = g_get_monotonic_time() + lpstlhBTRCore->batteryLevelRefreshInterval * G_TIME_SPAN_SECOND;
        g_mutex_lock(&lpstlhBTRCore->batteryLevelMutex);
        g_cond_wait_until(&lpstlhBTRCore->batteryLevelCond, &lpstlhBTRCore->batteryLevelMutex, i64EndWaitTime);
        g_mutex_unlock(&lpstlhBTRCore->batteryLevelMutex);
    }
    if (!ui8NumberDevicesAvailable)
    {
        BTRCORELOG_INFO("No available devices to read battery level, exiting thread\n");
        //if there is no join waiting for this to exit, handle own resources
        g_mutex_lock(&lpstlhBTRCore->batteryLevelMutex);
        if(lpstlhBTRCore->batteryLevelThread)
        {
            g_thread_unref(lpstlhBTRCore->batteryLevelThread);
            lpstlhBTRCore->batteryLevelThread = NULL;
        }
        g_mutex_unlock(&lpstlhBTRCore->batteryLevelMutex);
    }
    else 
    {
        BTRCORELOG_INFO("Thread exited due to modeule deInit\n");
        //there is a thread waiting, don't free own resources
    }
    
    return NULL;
}

/*  Interfaces  */
enBTRCoreRet
BTRCore_Init (
    tBTRCoreHandle* phBTRCore
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL; 
    unBTOpIfceProp  lunBtOpAdapProp;

#ifdef RDK_LOGGER_ENABLED
    const char* pDebugConfig = NULL;
    const char* BTRCORE_DEBUG_ACTUAL_PATH    = "/etc/debug.ini";
    const char* BTRCORE_DEBUG_OVERRIDE_PATH  = "/opt/debug.ini";

    /* Init the logger */
    if (access(BTRCORE_DEBUG_OVERRIDE_PATH, F_OK) != -1 ) {
        pDebugConfig = BTRCORE_DEBUG_OVERRIDE_PATH;
    }
    else {
        pDebugConfig = BTRCORE_DEBUG_ACTUAL_PATH;
    }

    if (rdk_logger_init(pDebugConfig) == 0) {
       b_rdk_logger_enabled = 1; 
    }
#endif

    BTRCORELOG_INFO ("BTRCore_Init\n");

    if (!phBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }


    pstlhBTRCore = (stBTRCoreHdl*)g_malloc0(sizeof(stBTRCoreHdl));
    if (!pstlhBTRCore) {
        BTRCORELOG_ERROR ("Insufficient memory - enBTRCoreInitFailure\n");
        return enBTRCoreInitFailure;
    }
    MEMSET_S(pstlhBTRCore, sizeof(stBTRCoreHdl), 0, sizeof(stBTRCoreHdl));


    pstlhBTRCore->connHdl = BtrCore_BTInitGetConnection();
    if (!pstlhBTRCore->connHdl) {
        BTRCORELOG_ERROR ("Can't get on system bus - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

    //init array of scanned , known & found devices
    btrCore_InitDataSt(pstlhBTRCore);

    pstlhBTRCore->agentPath = BtrCore_BTGetAgentPath(pstlhBTRCore->connHdl);
    if (!pstlhBTRCore->agentPath) {
        BTRCORELOG_ERROR ("Can't get agent path - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }


    if (!(pstlhBTRCore->pGAQueueOutTask = g_async_queue_new()) ||
        !(pstlhBTRCore->pThreadOutTask  = g_thread_new("btrCore_OutTask", btrCore_OutTask, (gpointer)pstlhBTRCore))) {
        BTRCORELOG_ERROR ("Failed to create btrCore_OutTask Thread - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

    if (!(pstlhBTRCore->pGAQueueRunTask = g_async_queue_new()) ||
        !(pstlhBTRCore->pThreadRunTask  = g_thread_new("btrCore_RunTask", btrCore_RunTask, (gpointer)pstlhBTRCore))) {
        BTRCORELOG_ERROR ("Failed to create btrCore_RunTask Thread - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }
   
    if (btrCore_OutTaskAddOp(pstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpStart, enBTRCoreTaskPTUnknown, NULL) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpStart enBTRCoreTaskPTUnknown\n");
    }

    if (btrCore_RunTaskAddOp(pstlhBTRCore->pGAQueueRunTask, enBTRCoreTaskOpStart, enBTRCoreTaskPTUnknown, NULL) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR("Failure btrCore_RunTaskAddOp enBTRCoreTaskOpStart enBTRCoreTaskPTUnknown\n");
    }

    g_mutex_init(&pstlhBTRCore->batteryLevelMutex);
    g_cond_init(&pstlhBTRCore->batteryLevelCond);


    pstlhBTRCore->curAdapterPath = BtrCore_BTGetAdapterPath(pstlhBTRCore->connHdl, NULL); //mikek hard code to default adapter for now
    if (!pstlhBTRCore->curAdapterPath) {
        BTRCORELOG_ERROR ("Failed to get BT Adapter - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropAddress;
    if (BtrCore_BTGetProp(pstlhBTRCore->connHdl,
                          pstlhBTRCore->curAdapterPath,
                          enBTAdapter,
                          lunBtOpAdapProp,
                          pstlhBTRCore->curAdapterAddr)) {
        BTRCORELOG_ERROR ("Failed to get BT Adapter Address - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

    BTRCORELOG_DEBUG ("Adapter path %s - Adapter Address %s \n", pstlhBTRCore->curAdapterPath, pstlhBTRCore->curAdapterAddr);


#ifndef LE_MODE
    /* Initialize BTRCore SubSystems - AVMedia/Telemetry..etc. */
    if (BTRCore_AVMedia_Init(&pstlhBTRCore->avMediaHdl, pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Init AV Media Subsystem - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }
#endif

    /* Initialize BTRCore SubSystems - LE Gatt profile. */
    if (BTRCore_LE_Init(&pstlhBTRCore->leHdl, pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Init LE Subsystem - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }


    if(BtrCore_BTRegisterAdapterStatusUpdateCb(pstlhBTRCore->connHdl, &btrCore_BTAdapterStatusUpdateCb, pstlhBTRCore)) {
        BTRCORELOG_ERROR ("Failed to Register Adapter Status CB - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

    if(BtrCore_BTRegisterDevStatusUpdateCb(pstlhBTRCore->connHdl, &btrCore_BTDeviceStatusUpdateCb, pstlhBTRCore)) {
        BTRCORELOG_ERROR ("Failed to Register Device Status CB - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

    if(BtrCore_BTRegisterConnIntimationCb(pstlhBTRCore->connHdl, &btrCore_BTDeviceConnectionIntimationCb, pstlhBTRCore)) {
        BTRCORELOG_ERROR ("Failed to Register Connection Intimation CB - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

    if(BtrCore_BTRegisterConnAuthCb(pstlhBTRCore->connHdl, &btrCore_BTDeviceAuthenticationCb, pstlhBTRCore)) {
        BTRCORELOG_ERROR ("Failed to Register Connection Authentication CB - enBTRCoreInitFailure\n");
        BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
        return enBTRCoreInitFailure;
    }

#ifndef LE_MODE
    if(BTRCore_AVMedia_RegisterMediaStatusUpdateCb(pstlhBTRCore->avMediaHdl, &btrCore_BTMediaStatusUpdateCb, pstlhBTRCore) != enBTRCoreSuccess) {
       BTRCORELOG_ERROR ("Failed to Register Media Status CB - enBTRCoreInitFailure\n");
       BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
       return enBTRCoreInitFailure;
    }
#endif

    if(BTRCore_LE_RegisterStatusUpdateCb(pstlhBTRCore->leHdl, &btrCore_BTLeStatusUpdateCb, pstlhBTRCore) != enBTRCoreSuccess) {
       BTRCORELOG_ERROR ("Failed to Register LE Status CB - enBTRCoreInitFailure\n");
       BTRCore_DeInit((tBTRCoreHandle)pstlhBTRCore);
       return enBTRCoreInitFailure;
    }

    *phBTRCore  = (tBTRCoreHandle)pstlhBTRCore;

    //Initialize array of known devices so we can use it for stuff
    btrCore_PopulateListOfPairedDevices(*phBTRCore, pstlhBTRCore->curAdapterPath);
    
    /* Discovery Type */
    pstlhBTRCore->aenDeviceDiscoveryType = enBTRCoreUnknown;

    /* Initialize the skip update count to 0 */
    pstlhBTRCore->skipDeviceDiscUpdate = 0;

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_DeInit (
    tBTRCoreHandle  hBTRCore
) {
    gpointer        penExitStatusRunTask = NULL;
    enBTRCoreRet    lenExitStatusRunTask = enBTRCoreSuccess;

    gpointer        penExitStatusOutTask = NULL;
    enBTRCoreRet    lenExitStatusOutTask = enBTRCoreSuccess;

    enBTRCoreRet    lenBTRCoreRet = enBTRCoreSuccess;

    stBTRCoreHdl*   pstlhBTRCore = NULL;
    GThread *       batteryLevelThreadCopy;
    int             i;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    BTRCORELOG_INFO ("hBTRCore   =   %8p\n", hBTRCore);


    /* Stop BTRCore Task Threads */
    if (pstlhBTRCore->pThreadRunTask) {

        if (pstlhBTRCore->pGAQueueRunTask) {
            if (btrCore_RunTaskAddOp(pstlhBTRCore->pGAQueueRunTask, enBTRCoreTaskOpExit, enBTRCoreTaskPTUnknown, NULL) != enBTRCoreSuccess) {
                BTRCORELOG_ERROR("Failure btrCore_RunTaskAddOp enBTRCoreTaskOpExit enBTRCoreTaskPTUnknown\n");
            }
        }

        penExitStatusRunTask = g_thread_join(pstlhBTRCore->pThreadRunTask);
        pstlhBTRCore->pThreadRunTask = NULL;
    }

    if (penExitStatusRunTask) {
        BTRCORELOG_INFO ("BTRCore_DeInit - RunTask Exiting BTRCore - %d\n", *((enBTRCoreRet*)penExitStatusRunTask));
        lenExitStatusRunTask = *((enBTRCoreRet*)penExitStatusRunTask);
        g_free(penExitStatusRunTask);
        penExitStatusRunTask = NULL;
    }

    if (pstlhBTRCore->pGAQueueRunTask) {
        g_async_queue_unref(pstlhBTRCore->pGAQueueRunTask);
        pstlhBTRCore->pGAQueueRunTask = NULL;
    }


    if (pstlhBTRCore->pThreadOutTask) {

        if (pstlhBTRCore->pGAQueueOutTask) {
            if (btrCore_OutTaskAddOp(pstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpExit, enBTRCoreTaskPTUnknown, NULL) != enBTRCoreSuccess) {
                BTRCORELOG_ERROR("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpExit enBTRCoreTaskPTUnknown\n");
            }
        }

        penExitStatusOutTask = g_thread_join(pstlhBTRCore->pThreadOutTask);
        pstlhBTRCore->pThreadOutTask = NULL;
    }

    if (penExitStatusOutTask) {
        BTRCORELOG_INFO ("BTRCore_DeInit - OutTask Exiting BTRCore - %d\n", *((enBTRCoreRet*)penExitStatusOutTask));
        lenExitStatusOutTask = *((enBTRCoreRet*)penExitStatusOutTask);
        g_free(penExitStatusOutTask);
        penExitStatusOutTask = NULL;
    }

    if (pstlhBTRCore->pGAQueueOutTask) {
        g_async_queue_unref(pstlhBTRCore->pGAQueueOutTask);
        pstlhBTRCore->pGAQueueOutTask = NULL;
    }
    g_mutex_lock(&pstlhBTRCore->batteryLevelMutex);
    if (pstlhBTRCore->batteryLevelThread)
    {
        pstlhBTRCore->batteryLevelThreadExit = TRUE;
        batteryLevelThreadCopy = pstlhBTRCore->batteryLevelThread;
        pstlhBTRCore->batteryLevelThread = NULL;
        g_cond_signal(&pstlhBTRCore->batteryLevelCond);
        g_mutex_unlock(&pstlhBTRCore->batteryLevelMutex);
        g_thread_join(batteryLevelThreadCopy);
        batteryLevelThreadCopy = NULL;
        
    }
    else
    {
        g_mutex_unlock(&pstlhBTRCore->batteryLevelMutex);
    }
    g_mutex_clear(&pstlhBTRCore->batteryLevelMutex);
    g_cond_clear(&pstlhBTRCore->batteryLevelCond);
    /* Free any memory allotted for use in BTRCore */

    /* DeInitialize BTRCore SubSystems - AVMedia/Telemetry..etc. */
    if (pstlhBTRCore->leHdl && (BTRCore_LE_DeInit(pstlhBTRCore->leHdl, pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath) != enBTRCoreSuccess)) {
        BTRCORELOG_ERROR ("Failed to DeInit LE Subsystem\n");
        lenBTRCoreRet = enBTRCoreFailure;
    }

#ifndef LE_MODE
    if (pstlhBTRCore->avMediaHdl && (BTRCore_AVMedia_DeInit(pstlhBTRCore->avMediaHdl, pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath) != enBTRCoreSuccess)) {
        BTRCORELOG_ERROR ("Failed to DeInit AV Media Subsystem\n");
        lenBTRCoreRet = enBTRCoreFailure;
    }
#endif

    if (pstlhBTRCore->curAdapterPath) {
        if (BtrCore_BTReleaseAdapterPath(pstlhBTRCore->connHdl, NULL)) {
            BTRCORELOG_ERROR ("Failure BtrCore_BTReleaseAdapterPath\n");
            lenBTRCoreRet = enBTRCoreFailure;
        }
        pstlhBTRCore->curAdapterPath = NULL;
    }

    /* Adapters */
    for (i = 0; i < BTRCORE_MAX_NUM_BT_ADAPTERS; i++) {
        if (pstlhBTRCore->adapterPath[i]) {
            g_free(pstlhBTRCore->adapterPath[i]);
            pstlhBTRCore->adapterPath[i] = NULL;
        }

        if (pstlhBTRCore->adapterAddr[i]) {
            g_free(pstlhBTRCore->adapterAddr[i]);
            pstlhBTRCore->adapterAddr[i] = NULL;
        }
    }

    if (pstlhBTRCore->curAdapterAddr) {
        g_free(pstlhBTRCore->curAdapterAddr);
        pstlhBTRCore->curAdapterAddr = NULL;
    }

    if (pstlhBTRCore->agentPath) {
        if (BtrCore_BTReleaseAgentPath(pstlhBTRCore->connHdl)) {
            BTRCORELOG_ERROR ("Failure BtrCore_BTReleaseAgentPath\n");
            lenBTRCoreRet = enBTRCoreFailure;
        }
        pstlhBTRCore->agentPath = NULL;
    }

    if (pstlhBTRCore->connHdl) {
        if (BtrCore_BTDeInitReleaseConnection(pstlhBTRCore->connHdl)) {
            BTRCORELOG_ERROR ("Failure BtrCore_BTDeInitReleaseConnection\n");
            lenBTRCoreRet = enBTRCoreFailure;
        }
        pstlhBTRCore->connHdl = NULL;
    }

    if (hBTRCore) {
        g_free(hBTRCore);
        hBTRCore = NULL;
    }

    lenBTRCoreRet = ((lenExitStatusRunTask == enBTRCoreSuccess) &&
                     (lenExitStatusOutTask == enBTRCoreSuccess) &&
                     (lenBTRCoreRet == enBTRCoreSuccess)) ? enBTRCoreSuccess : enBTRCoreFailure;
    BTRCORELOG_INFO ("Exit Status = %d\n", lenBTRCoreRet);


    return lenBTRCoreRet;
}


enBTRCoreRet
BTRCore_RegisterAgent (
    tBTRCoreHandle  hBTRCore,
    int             iBTRCapMode
) {
    char            capabilities[32] = {'\0'};
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    // 0 = DisplayOnly, 1 = DisplayYesNo, 2 = KeyboardOnly, 3 = NoInputNoOutput and 4 = KeyboardDisplay
    switch (iBTRCapMode)
    {
    case 0:
        STRCPY_S(capabilities, sizeof(capabilities), "DisplayOnly");
        break;
    case 1:
        STRCPY_S(capabilities, sizeof(capabilities), "DisplayYesNo");
        break;
    case 2:
        STRCPY_S(capabilities, sizeof(capabilities), "KeyboardOnly");
        break;
    case 4:
        STRCPY_S(capabilities, sizeof(capabilities), "KeyboardDisplay");
        break;
    case 3:
    default:
        STRCPY_S(capabilities, sizeof(capabilities), "NoInputNoOutput");
        break;
    }
    BTRCORELOG_TRACE("Registering Agent with capability : %s\n", capabilities);

    if (BtrCore_BTRegisterAgent(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, pstlhBTRCore->agentPath, capabilities) < 0) {
        BTRCORELOG_ERROR ("Agent registration ERROR occurred\n");
        return enBTRCoreFailure;
    }

    BTRCORELOG_TRACE ("Starting Agent in mode %s\n", capabilities);
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_UnregisterAgent (
    tBTRCoreHandle  hBTRCore
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (BtrCore_BTUnregisterAgent(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, pstlhBTRCore->agentPath) < 0) {
        BTRCORELOG_ERROR ("Agent unregistration  ERROR occurred\n");
        return enBTRCoreFailure;
    }

    BTRCORELOG_TRACE ("Stopping Agent\n");
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetListOfAdapters (
    tBTRCoreHandle          hBTRCore,
    stBTRCoreListAdapters*  pstListAdapters
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    rc = enBTRCoreFailure;
    unBTOpIfceProp  lunBtOpAdapProp;
    int i;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pstListAdapters) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!BtrCore_BTGetAdapterList(pstlhBTRCore->connHdl, &pstlhBTRCore->numOfAdapters, pstlhBTRCore->adapterPath)) {
        pstListAdapters->number_of_adapters = pstlhBTRCore->numOfAdapters;
        for (i = 0; i < pstListAdapters->number_of_adapters; i++) {
            MEMSET_S(&pstListAdapters->adapter_path[i][0], BD_NAME_LEN + 1, '\0', BD_NAME_LEN + 1);
            strncpy(&pstListAdapters->adapter_path[i][0], pstlhBTRCore->adapterPath[i], BD_NAME_LEN);

            MEMSET_S(&pstListAdapters->adapterAddr[i][0], BD_NAME_LEN + 1, '\0', BD_NAME_LEN + 1);
            lunBtOpAdapProp.enBtAdapterProp = enBTAdPropAddress;
            if (!BtrCore_BTGetProp(pstlhBTRCore->connHdl, pstlhBTRCore->adapterPath[i], enBTAdapter, lunBtOpAdapProp, pstlhBTRCore->adapterAddr[i])) {
                strncpy(&pstListAdapters->adapterAddr[i][0], pstlhBTRCore->adapterAddr[i], BD_NAME_LEN);
            }
            rc = enBTRCoreSuccess;
        }
    }

    return rc;
}


enBTRCoreRet
BTRCore_GetAdapters (
    tBTRCoreHandle          hBTRCore,
    stBTRCoreGetAdapters*   pstGetAdapters
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pstGetAdapters) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!BtrCore_BTGetAdapterList(pstlhBTRCore->connHdl, &pstlhBTRCore->numOfAdapters, pstlhBTRCore->adapterPath)) {
        pstGetAdapters->number_of_adapters = pstlhBTRCore->numOfAdapters;
        return enBTRCoreSuccess;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetAdapter (
    tBTRCoreHandle      hBTRCore,
    stBTRCoreAdapter*   apstBTRCoreAdapter
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!pstlhBTRCore->curAdapterPath) {
        if ((pstlhBTRCore->curAdapterPath = BtrCore_BTGetAdapterPath(pstlhBTRCore->connHdl, NULL)) == NULL) { //mikek hard code to default adapter for now
            BTRCORELOG_ERROR ("Failed to get BT Adapter");
            return enBTRCoreInvalidAdapter;
        }
    }


    if (apstBTRCoreAdapter) {
        apstBTRCoreAdapter->adapter_number   = 0; //hard code to default adapter for now
        apstBTRCoreAdapter->pcAdapterPath    = pstlhBTRCore->curAdapterPath;
        apstBTRCoreAdapter->pcAdapterDevName = NULL;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_SetAdapter (
    tBTRCoreHandle  hBTRCore,
    int             adapter_number
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    int pathlen;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    pathlen = strlen(pstlhBTRCore->curAdapterPath);
    switch (adapter_number) {
    case 0:
        pstlhBTRCore->curAdapterPath[pathlen-1]='0';
        break;
    case 1:
        pstlhBTRCore->curAdapterPath[pathlen-1]='1';
        break;
    case 2:
        pstlhBTRCore->curAdapterPath[pathlen-1]='2';
        break;
    case 3:
        pstlhBTRCore->curAdapterPath[pathlen-1]='3';
        break;
    case 4:
        pstlhBTRCore->curAdapterPath[pathlen-1]='4';
        break;
    case 5:
        pstlhBTRCore->curAdapterPath[pathlen-1]='5';
        break;
    default:
        BTRCORELOG_INFO ("max adapter value is 5, setting default\n");//6 adapters seems like plenty for now
        pstlhBTRCore->curAdapterPath[pathlen-1]='0';
        break;
    }

    BTRCORELOG_INFO ("Now current adatper is %s\n", pstlhBTRCore->curAdapterPath);

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_EnableAdapter (
    tBTRCoreHandle      hBTRCore,
    stBTRCoreAdapter*   apstBTRCoreAdapter
) {
    stBTRCoreHdl*       pstlhBTRCore = NULL;
    unBTOpIfceProp      lunBtOpAdapProp;
    int                 powered = 1;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    BTRCORELOG_ERROR ("BTRCore_EnableAdapter\n");
    apstBTRCoreAdapter->enable = TRUE; //does this even mean anything?
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropPowered;

    if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &powered)) {
        BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropPowered - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_DisableAdapter (
    tBTRCoreHandle      hBTRCore,
    stBTRCoreAdapter*   apstBTRCoreAdapter
) {
    stBTRCoreHdl*       pstlhBTRCore = NULL;
    unBTOpIfceProp      lunBtOpAdapProp;
    int                 powered = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    BTRCORELOG_ERROR ("BTRCore_DisableAdapter\n");
    apstBTRCoreAdapter->enable = FALSE;
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropPowered;

    if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &powered)) {
        BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropPowered - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetAdapterAddr (
    tBTRCoreHandle  hBTRCore,
    unsigned char   aui8adapterIdx,
    char*           apui8adapterAddr
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet= enBTRCoreFailure;
    int             i = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!apui8adapterAddr) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    for (i = 0; i < pstlhBTRCore->numOfAdapters; i++) {
        if (aui8adapterIdx == i) {
            strncpy(apui8adapterAddr, pstlhBTRCore->adapterAddr[i], (strlen(pstlhBTRCore->adapterAddr[i]) < BD_NAME_LEN) ? strlen(pstlhBTRCore->adapterAddr[i]) : BD_NAME_LEN - 1);
            lenBTRCoreRet = enBTRCoreSuccess;
            break;
        }
    }

    return lenBTRCoreRet;
}


enBTRCoreRet
BTRCore_SetAdapterDiscoverable (
    tBTRCoreHandle  hBTRCore,
    const char*     pAdapterPath,
    unsigned char   discoverable
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    unBTOpIfceProp  lunBtOpAdapProp;
    int             isDiscoverable = (int) discoverable;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pAdapterPath) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropDiscoverable;

    if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &isDiscoverable)) {
        BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropDiscoverable - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_SetAdapterDiscoverableTimeout (
    tBTRCoreHandle  hBTRCore,
    const char*     pAdapterPath,
    unsigned short  timeout
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    unBTOpIfceProp  lunBtOpAdapProp;
    unsigned int    givenTimeout = (unsigned int)timeout;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pAdapterPath) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropDiscoverableTimeOut;

    if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &givenTimeout)) {
        BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropDiscoverableTimeOut - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetAdapterDiscoverableStatus (
    tBTRCoreHandle  hBTRCore,
    const char*     pAdapterPath,
    unsigned char*  pDiscoverable
) {
    stBTRCoreHdl*   pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    unBTOpIfceProp  lunBtOpAdapProp;
    int             discoverable = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if ((!pAdapterPath) || (!pDiscoverable)) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropDiscoverable;
    if (!BtrCore_BTGetProp(pstlhBTRCore->connHdl, pAdapterPath, enBTAdapter, lunBtOpAdapProp, &discoverable)) {
        BTRCORELOG_INFO ("Get value for org.bluez.Adapter.Discoverable = %d\n", discoverable);
        *pDiscoverable = (unsigned char) discoverable;
        return enBTRCoreSuccess;
    }

    return enBTRCoreFailure;
}


enBTRCoreRet 
BTRCore_SetAdapterDeviceName (
    tBTRCoreHandle      hBTRCore,
    stBTRCoreAdapter*   apstBTRCoreAdapter,
    char*               apcAdapterDeviceName
) {
    stBTRCoreHdl*       pstlhBTRCore = NULL;
    unBTOpIfceProp      lunBtOpAdapProp;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!apcAdapterDeviceName) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if(apstBTRCoreAdapter->pcAdapterDevName) {
        g_free(apstBTRCoreAdapter->pcAdapterDevName);
        apstBTRCoreAdapter->pcAdapterDevName = NULL;
    }

    apstBTRCoreAdapter->pcAdapterDevName = g_strndup(apcAdapterDeviceName, BTRCORE_STR_LEN - 1); //TODO: Free this memory
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropName;

    if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &(apstBTRCoreAdapter->pcAdapterDevName))) {
        BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropName - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_SetAdapterName (
    tBTRCoreHandle      hBTRCore,
    const char*         pAdapterPath,
    const char*         pAdapterName
) {
    stBTRCoreHdl*       pstlhBTRCore = NULL;
    unBTOpIfceProp      lunBtOpAdapProp;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if ((!pAdapterPath) ||(!pAdapterName)) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropName;

    if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &pAdapterName)) {
        BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropName - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetAdapterName (
    tBTRCoreHandle  hBTRCore,
    const char*     pAdapterPath,
    char*           pAdapterName
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    unBTOpIfceProp  lunBtOpAdapProp;

    char name[BD_NAME_LEN + 1] = {'\0'};
    MEMSET_S(name, sizeof(char) * (BD_NAME_LEN + 1), '\0', sizeof(char) * (BD_NAME_LEN + 1));

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if ((!pAdapterPath) || (!pAdapterName)) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropName;
    if (!BtrCore_BTGetProp(pstlhBTRCore->connHdl, pAdapterPath, enBTAdapter, lunBtOpAdapProp, name)) {
        BTRCORELOG_INFO ("Get value for org.bluez.Adapter.Name = %s\n", name);
        strncpy(pAdapterName, name, strlen(name) < BD_NAME_LEN ? strlen(name) : BD_NAME_LEN -1);
        return enBTRCoreSuccess;
    }

    return enBTRCoreFailure;
}


enBTRCoreRet
BTRCore_SetAdapterPower (
    tBTRCoreHandle  hBTRCore,
    const char*     pAdapterPath,
    unsigned char   powerStatus
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    unBTOpIfceProp  lunBtOpAdapProp;
    int             power = powerStatus;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pAdapterPath) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropPowered;

    if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &power)) {
        BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropPowered - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetAdapterPower (
    tBTRCoreHandle  hBTRCore,
    const char*     pAdapterPath,
    unsigned char*  pAdapterPower
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    unBTOpIfceProp  lunBtOpAdapProp;
    int             powerStatus = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if ((!pAdapterPath) || (!pAdapterPower)) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    lunBtOpAdapProp.enBtAdapterProp = enBTAdPropPowered;
    if (!BtrCore_BTGetProp(pstlhBTRCore->connHdl, pAdapterPath, enBTAdapter, lunBtOpAdapProp, &powerStatus)) {
        BTRCORELOG_INFO ("Get value for org.bluez.Adapter.powered = %d\n", powerStatus);
        *pAdapterPower = (unsigned char) powerStatus;
        return enBTRCoreSuccess;
    }

    return enBTRCoreFailure;
}


enBTRCoreRet
BTRCore_GetVersionInfo (
    tBTRCoreHandle  hBTRCore,
    char*           apcBtVersion
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    char            lBtIfceName[BTRCORE_STR_LEN];
    char            lBtVersion[BTRCORE_STR_LEN];
    errno_t safec_rc = -1;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!apcBtVersion) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    MEMSET_S(lBtIfceName, BTRCORE_STR_LEN, '\0', BTRCORE_STR_LEN);
    MEMSET_S(lBtVersion, BTRCORE_STR_LEN,  '\0', BTRCORE_STR_LEN);

    if (!BtrCore_BTGetIfceNameVersion(pstlhBTRCore->connHdl, lBtIfceName, lBtVersion)) {
        safec_rc = strcpy_s(apcBtVersion, BTRCORE_STR_LEN-1, lBtIfceName);
        ERR_CHK(safec_rc);
        safec_rc = strncat_s(apcBtVersion, BTRCORE_STR_LEN-1, "-", strlen("-")); // CID 340879: String not null terminated (STRING_NULL)
        ERR_CHK(safec_rc);
        safec_rc = strncat_s(apcBtVersion, BTRCORE_STR_LEN-1, lBtVersion, strlen(lBtVersion));
        ERR_CHK(safec_rc);
        BTRCORELOG_DEBUG ("Ifce: %s Version: %s", lBtIfceName, lBtVersion);
        BTRCORELOG_INFO ("Out:  %s\n", apcBtVersion);
        return enBTRCoreSuccess;
    }

    return enBTRCoreFailure;
}


enBTRCoreRet
BTRCore_StartDiscovery (
    tBTRCoreHandle      hBTRCore,
    const char*         pAdapterPath,
    enBTRCoreDeviceType aenBTRCoreDevType,
    unsigned int        aui32DiscDuration
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pAdapterPath) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    btrCore_ClearScannedDevicesList(pstlhBTRCore);

    /* Discovery Type */
    pstlhBTRCore->aenDeviceDiscoveryType = aenBTRCoreDevType;

    if (aenBTRCoreDevType == enBTRCoreLE)  {
        if (BtrCore_BTStartLEDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
            return enBTRCoreDiscoveryFailure;
        }

        if (aui32DiscDuration) {
            sleep(aui32DiscDuration); //TODO: Better to setup a timer which calls BTStopDiscovery
            if (BtrCore_BTStopLEDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
                return enBTRCoreDiscoveryFailure;
            }
        }
    }
#ifndef LE_MODE
    else if ((aenBTRCoreDevType == enBTRCoreSpeakers) || (aenBTRCoreDevType == enBTRCoreHeadSet) ||
             (aenBTRCoreDevType == enBTRCoreMobileAudioIn) || (aenBTRCoreDevType == enBTRCorePCAudioIn)) {
        if (BtrCore_BTStartClassicDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
            return enBTRCoreDiscoveryFailure;
        }

        if (aui32DiscDuration) {
            sleep(aui32DiscDuration); //TODO: Better to setup a timer which calls BTStopDiscovery
            if (BtrCore_BTStopClassicDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
                return enBTRCoreDiscoveryFailure;
            }
        }
    }
#endif
    else {
        if (BtrCore_BTStartDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
            return enBTRCoreDiscoveryFailure;
        }

        if (aui32DiscDuration) {
            sleep(aui32DiscDuration); //TODO: Better to setup a timer which calls BTStopDiscovery
            if (BtrCore_BTStopDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
                return enBTRCoreDiscoveryFailure;
            }
        }
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_StopDiscovery (
    tBTRCoreHandle      hBTRCore,
    const char*         pAdapterPath,
    enBTRCoreDeviceType aenBTRCoreDevType
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pAdapterPath) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if (aenBTRCoreDevType == enBTRCoreLE)  {
        if (BtrCore_BTStopLEDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
            return enBTRCoreDiscoveryFailure;
        }
    }
#ifndef LE_MODE
    else if ((aenBTRCoreDevType == enBTRCoreSpeakers) || (aenBTRCoreDevType == enBTRCoreHeadSet) ||
             (aenBTRCoreDevType == enBTRCoreMobileAudioIn) || (aenBTRCoreDevType == enBTRCorePCAudioIn)) {
        if (BtrCore_BTStopClassicDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
            return enBTRCoreDiscoveryFailure;
        }
    }
#endif
    else {
        if (BtrCore_BTStopDiscovery(pstlhBTRCore->connHdl, pAdapterPath, pstlhBTRCore->agentPath)) {
            return enBTRCoreDiscoveryFailure;
        }
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetListOfScannedDevices (
    tBTRCoreHandle                  hBTRCore,
    stBTRCoreScannedDevicesCount*   pListOfScannedDevices
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    int i, j;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pListOfScannedDevices) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    MEMSET_S(pListOfScannedDevices, sizeof(stBTRCoreScannedDevicesCount), 0, sizeof(stBTRCoreScannedDevicesCount));

    BTRCORELOG_TRACE ("adapter path is %s\n", pstlhBTRCore->curAdapterPath);
    for (i = 0; i < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES; i++) {
        if (pstlhBTRCore->stScannedDevicesArr[i].bFound) {
            BTRCORELOG_TRACE ("Device : %d\n", i);
            BTRCORELOG_TRACE ("Name   : %s\n", pstlhBTRCore->stScannedDevicesArr[i].pcDeviceName);
            BTRCORELOG_TRACE ("Mac Ad : %s\n", pstlhBTRCore->stScannedDevicesArr[i].pcDeviceAddress);
            BTRCORELOG_TRACE ("Rssi   : %d dbmV\n", pstlhBTRCore->stScannedDevicesArr[i].i32RSSI);
            btrCore_ShowSignalStrength(pstlhBTRCore->stScannedDevicesArr[i].i32RSSI);

            MEMCPY_S(&pListOfScannedDevices->devices[pListOfScannedDevices->numberOfDevices],sizeof(pListOfScannedDevices->devices[0]), &pstlhBTRCore->stScannedDevicesArr[i], sizeof (stBTRCoreBTDevice)); // CID 330348 : Evaluation order violation (EVALUATION_ORDER)
            pListOfScannedDevices->numberOfDevices++;

            for (j = 0; j < BTRCORE_MAX_NUM_BT_DEVICES; j++) {
                if (pListOfScannedDevices->devices[i].tDeviceId == pstlhBTRCore->stKnownDevicesArr[j].tDeviceId) {
                    pListOfScannedDevices->devices[i].bDeviceConnected = pstlhBTRCore->stKnownDevicesArr[j].bDeviceConnected;
                    break;
                }
            }
        }
    }   

    BTRCORELOG_TRACE ("Copied scanned details of %d devices\n", pListOfScannedDevices->numberOfDevices);

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_PairDevice (
    tBTRCoreHandle  hBTRCore,
    tBTRCoreDevId   aBTRCoreDevId
) {
    stBTRCoreHdl*           pstlhBTRCore        = NULL;
    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstScannedDev       = NULL;
    int                     i32LoopIdx          = 0;
    enBTAdapterOp           pairingOp           = enBTAdpOpCreatePairedDev;
    unBTOpIfceProp          lunBtOpAdapProp;
    int                     ispairable = 1;
    int                     PairableMode = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES) {
        pstScannedDev   = &pstlhBTRCore->stScannedDevicesArr[aBTRCoreDevId];
    }
    else {
        for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
            if (aBTRCoreDevId == pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
                pstScannedDev   = &pstlhBTRCore->stScannedDevicesArr[i32LoopIdx];
                break;
            }
        }
    }


    if (pstScannedDev)
        pDeviceAddress  = pstScannedDev->pcDeviceAddress;


    if (!pstScannedDev || !pDeviceAddress || !strlen(pDeviceAddress)) {
        BTRCORELOG_ERROR ("Failed to find device in Scanned devices list\n");
        return enBTRCoreDeviceNotFound;
    }

    BTRCORELOG_DEBUG ("We will pair     %s\n", pstScannedDev->pcDeviceName);
    BTRCORELOG_DEBUG ("We will address  %s\n", pDeviceAddress);

    if ((pstScannedDev->enDeviceType == enBTRCore_DC_HID_Keyboard)      ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_HID_Mouse)         ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_HID_MouseKeyBoard) ||
        //(pstScannedDev->enDeviceType == enBTRCore_DC_HID_AudioRemote)   ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_HID_Joystick)      ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_HID_GamePad)) {

    
        BTRCORELOG_DEBUG ("Setting the adapter to pairable mode\n");
        lunBtOpAdapProp.enBtAdapterProp = enBTAdPropPairable;

        if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &ispairable)) {
            BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropPairable - FAILED\n");
            return enBTRCoreFailure;
        }
      
        BTRCORELOG_DEBUG ("We will do a Async Pairing for the HID Devices\n");
        pairingOp = enBTAdpOpCreatePairedDevASync;
        /* During pairing process, after unregistering the agent adapter pairable mode
         * was set to true in dbus_bluez5. Adding the below check to avoid the cases
         * when we try to pair before the pairable mode was changed.
         */
    } else if ((pstScannedDev->enDeviceType == enBTRCore_DC_Loudspeaker)     ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_WearableHeadset)        ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_Headphones)             ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_HIFIAudioDevice)        ||
        (pstScannedDev->enDeviceType == enBTRCore_DC_PortableAudio)) {
        lunBtOpAdapProp.enBtAdapterProp = enBTAdPropPairable;

        if (BtrCore_BTGetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &PairableMode)) {
             BTRCORELOG_ERROR ("Get Adapter Property enBTAdPropPairable - FAILED\n");
             return enBTRCoreFailure;
        }

        if (PairableMode == 0) {
            if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, enBTAdapter, lunBtOpAdapProp, &ispairable)) {
                BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropPairable - FAILED\n");
                return enBTRCoreFailure;
            } else {
                BTRCORELOG_INFO ("Set Adapter Pairable Mode Success\n");
            }
        }
    }

    if (BtrCore_BTPerformAdapterOp( pstlhBTRCore->connHdl,
                                    pstlhBTRCore->curAdapterPath,
                                    pstlhBTRCore->agentPath,
                                    pDeviceAddress,
                                    pairingOp) < 0) {
        BTRCORELOG_ERROR ("Failed to pair a device\n");
        return enBTRCorePairingFailed;
    }

    //Calling this api will update the KnownDevList appropriately
    btrCore_PopulateListOfPairedDevices(pstlhBTRCore, pstlhBTRCore->curAdapterPath);

    BTRCORELOG_INFO ("Pairing Success\n");
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_UnPairDevice (
    tBTRCoreHandle  hBTRCore,
    tBTRCoreDevId   aBTRCoreDevId
) {
    stBTRCoreHdl*           pstlhBTRCore    = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

    enBTRCoreDeviceType     aenBTRCoreDevType = enBTRCoreUnknown;
    stBTRCoreBTDevice       pstScannedDevice;

    /* We can enhance the BTRCore with passcode support later point in time */
    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    BTRCORELOG_INFO("Product - %u Vendor - %u Firmware - %u \n",pstKnownDevice->ui32ModaliasProductId,pstKnownDevice->ui32ModaliasVendorId,pstKnownDevice->ui32ModaliasDeviceId);
    if (pstKnownDevice->ui32ModaliasProductId == BTRCORE_XBOX_ELITE_PRODUCT_ID &&
        pstKnownDevice->ui32ModaliasVendorId == BTRCORE_XBOX_VENDOR_ID &&
        pstKnownDevice->ui32ModaliasDeviceId == BTRCORE_XBOX_ELITE_DEF_FIRMWARE) {
        unBTOpIfceProp lunBtOpDevProp;
        lunBtOpDevProp.enBtDeviceProp = enBTDevPropBlocked;
        BTRCORELOG_INFO("Identified xbox elite with basic firmware , so blocking the incoming connection before unpairing ...\n");
        int BlockDevice = 1;
        if (BtrCore_BTSetProp(pstlhBTRCore->connHdl, pDeviceAddress, enBTDevice, lunBtOpDevProp, &BlockDevice)) {
	    BTRCORELOG_ERROR ("Set Device Property enBTDevPropBlocked - FAILED\n");
        }
        sleep(2);
    }

    if (BtrCore_BTPerformAdapterOp( pstlhBTRCore->connHdl,
                                    pstlhBTRCore->curAdapterPath,
                                    pstlhBTRCore->agentPath,
                                    pDeviceAddress,
                                    enBTAdpOpRemovePairedDev) != 0) {
        BTRCORELOG_ERROR ("Failed to unpair a device\n");
        return enBTRCorePairingFailed;
    }


    btrCore_RemoveDeviceFromKnownDevicesArr(pstlhBTRCore, aBTRCoreDevId);

    //Calling this api will update the KnownDevList appropriately
    btrCore_PopulateListOfPairedDevices(pstlhBTRCore, pstlhBTRCore->curAdapterPath);

    MEMSET_S(&pstScannedDevice,sizeof(stBTRCoreBTDevice), 0 ,sizeof(stBTRCoreBTDevice));
    //Clear corresponding  device entry from Scanned List if any
    if (btrCore_GetScannedDeviceAddress(pstlhBTRCore, aBTRCoreDevId)) {
        lenBTRCoreRet = btrCore_RemoveDeviceFromScannedDevicesArr (pstlhBTRCore, aBTRCoreDevId, &pstScannedDevice);

        if (!(enBTRCoreSuccess == lenBTRCoreRet && pstScannedDevice.tDeviceId)) {
            BTRCORELOG_ERROR ("Remove device %lld from Scanned List Failed!\n", aBTRCoreDevId);
        }
    }

    BTRCORELOG_INFO ("UnPairing Success\n");
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetListOfPairedDevices (
    tBTRCoreHandle                  hBTRCore,
    stBTRCorePairedDevicesCount*    pListOfDevices
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    int             i32DevIdx = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!pListOfDevices) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (btrCore_PopulateListOfPairedDevices(pstlhBTRCore, pstlhBTRCore->curAdapterPath) == enBTRCoreSuccess) {
        pListOfDevices->numberOfDevices = pstlhBTRCore->numOfPairedDevices;
        g_mutex_lock(&pstlhBTRCore->batteryLevelMutex);
        for (i32DevIdx = 0; i32DevIdx < pListOfDevices->numberOfDevices; i32DevIdx++) {
            MEMCPY_S(&pListOfDevices->devices[i32DevIdx],sizeof(pListOfDevices->devices[0]), &pstlhBTRCore->stKnownDevicesArr[i32DevIdx], sizeof(stBTRCoreBTDevice));
        }
        g_mutex_unlock(&pstlhBTRCore->batteryLevelMutex);
        return enBTRCoreSuccess;
    }

    return enBTRCoreFailure;
}
enBTRCoreRet
BTRCore_GetDeviceBatteryLevel (
    tBTRCoreHandle     hBTRCore,
    tBTRCoreDevId      aBTRCoreDevId,
    enBTRCoreDeviceType     aenBTRCoreDevType,
    unsigned char *    pDeviceBatteryLevel
)
{
    stBTRCoreHdl*           pstlhBTRCore            = NULL;
    enBTRCoreRet            lenBTRCoreRet           = enBTRCoreFailure;
    enBTDeviceType          lenBTDeviceType         = enBTDevUnknown;
    stBTRCoreBTDevice*      lpstBTRCoreBTDevice     = NULL;
    stBTRCoreDevStateInfo*  lpstBTRCoreDevStateInfo = NULL;
    const char*             lpcBTRCoreBTDevicePath  = NULL;
    const char*             lpcBTRCoreBTDeviceName  = NULL;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    if (!pDeviceBatteryLevel) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }
    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if ((lenBTRCoreRet = btrCore_GetDeviceInfo(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                               &lenBTDeviceType, &lpstBTRCoreBTDevice, &lpstBTRCoreDevStateInfo,
                                               &lpcBTRCoreBTDevicePath, &lpcBTRCoreBTDeviceName)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information - %llu\n", aBTRCoreDevId);
        return lenBTRCoreRet;
    }

    if(BtrCore_BTGetBatteryLevel(pstlhBTRCore->connHdl, lpcBTRCoreBTDevicePath, pDeviceBatteryLevel))
    {
        BTRCORELOG_ERROR ("Failed to Get Device Battery level - %llu\n", aBTRCoreDevId);
        return enBTRCoreFailure;
    }
    return enBTRCoreSuccess;
}

enBTRCoreRet
BTRCore_FindDevice (
    tBTRCoreHandle  hBTRCore,
    tBTRCoreDevId   aBTRCoreDevId
) {
    stBTRCoreHdl*           pstlhBTRCore = NULL;
    stBTRCoreBTDevice* pstScannedDevice = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    pstScannedDevice = &pstlhBTRCore->stScannedDevicesArr[aBTRCoreDevId];

    BTRCORELOG_DEBUG (" We will try to find %s\n"
                     " address %s\n",
                     pstlhBTRCore->stScannedDevicesArr[aBTRCoreDevId].pcDeviceName,
                     pstlhBTRCore->stScannedDevicesArr[aBTRCoreDevId].pcDeviceAddress);

    if (BtrCore_BTPerformAdapterOp( pstlhBTRCore->connHdl,
                                    pstlhBTRCore->curAdapterPath,
                                    pstlhBTRCore->agentPath,
                                    pstScannedDevice->pcDeviceAddress,
                                    enBTAdpOpFindPairedDev) < 0) {
        BTRCORELOG_ERROR ("device not found\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


/*BTRCore_FindService, other inputs will include string and boolean pointer for returning*/
enBTRCoreRet
BTRCore_FindService (
    tBTRCoreHandle  hBTRCore,
    tBTRCoreDevId   aBTRCoreDevId,
    const char*     UUID,
    char*           XMLdata,
    int*            found
) {
    stBTRCoreHdl*           pstlhBTRCore    = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

    enBTRCoreDeviceType     aenBTRCoreDevType = enBTRCoreUnknown;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    BTRCORELOG_INFO ("Checking for service %s on %s\n", UUID, pDeviceAddress);
    *found = BtrCore_BTFindServiceSupported (pstlhBTRCore->connHdl, pDeviceAddress, UUID, XMLdata);
    if (*found < 0) {
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetSupportedServices (
    tBTRCoreHandle                  hBTRCore,
    tBTRCoreDevId                   aBTRCoreDevId,
    stBTRCoreSupportedServiceList*  pProfileList
) {
    stBTRCoreHdl*           pstlhBTRCore            = NULL;

    const char*             lpcBTRCoreBTDevicePath  = NULL;
    stBTRCoreBTDevice*      lpstBTRCoreBTDevice     = NULL;
    stBTRCoreDevStateInfo*  lpstBTRCoreDevStateInfo = NULL;
    enBTDeviceType          lenBTDeviceType         = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet           = enBTRCoreFailure;

    enBTRCoreDeviceType     aenBTRCoreDevType   = enBTRCoreUnknown;
    unsigned int            ui32LoopIdx         = 0;

    stBTDeviceSupportedServiceList profileList;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if ((!pProfileList) || (!aBTRCoreDevId)) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &lpstBTRCoreBTDevice, &lpstBTRCoreDevStateInfo, &lpcBTRCoreBTDevicePath)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }


    /* Initialize the array */
    MEMSET_S(pProfileList, sizeof(stBTRCoreSupportedServiceList), 0 , sizeof(stBTRCoreSupportedServiceList));
    MEMSET_S(&profileList, sizeof(stBTDeviceSupportedServiceList), 0 , sizeof(stBTDeviceSupportedServiceList));


    if (BtrCore_BTDiscoverDeviceServices(pstlhBTRCore->connHdl, lpcBTRCoreBTDevicePath, &profileList) != 0) {
        return enBTRCoreFailure;
    }

    BTRCORELOG_INFO ("Successfully received the supported services... \n");

    pProfileList->numberOfService = profileList.numberOfService;
    for (ui32LoopIdx = 0; ui32LoopIdx < profileList.numberOfService; ui32LoopIdx++) {
        pProfileList->profile[ui32LoopIdx].uuid_value = profileList.profile[ui32LoopIdx].uuid_value;
        strncpy (pProfileList->profile[ui32LoopIdx].profile_name,  profileList.profile[ui32LoopIdx].profile_name, 30);
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_IsDeviceConnectable (
    tBTRCoreHandle      hBTRCore,
    tBTRCoreDevId       aBTRCoreDevId
) {
    stBTRCoreHdl*       pstlhBTRCore = NULL;
    const char*         pDeviceMac = NULL;
    stBTRCoreBTDevice*  pstKnownDevice = NULL;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!pstlhBTRCore->numOfPairedDevices) {
        BTRCORELOG_DEBUG ("Possibly the list is not populated; like booted and connecting\n");
        /* Keep the list upto date */
        btrCore_PopulateListOfPairedDevices(pstlhBTRCore, pstlhBTRCore->curAdapterPath);
    }

    if (!pstlhBTRCore->numOfPairedDevices) {
        BTRCORELOG_ERROR ("There is no device paired for this adapter\n");
        return enBTRCoreFailure;
    }

    if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
        pstKnownDevice  = &pstlhBTRCore->stKnownDevicesArr[aBTRCoreDevId];
        pDeviceMac      = pstKnownDevice->pcDeviceAddress;
    }
    else {
        pDeviceMac      = btrCore_GetKnownDeviceMac(pstlhBTRCore, aBTRCoreDevId);
    }


    if (!pDeviceMac || !strlen(pDeviceMac)) {
        BTRCORELOG_ERROR ("Failed to find device in paired devices list\n");
        return enBTRCoreDeviceNotFound;
    }


    if (BtrCore_BTIsDeviceConnectable(pstlhBTRCore->connHdl, pDeviceMac) != 0) {
        BTRCORELOG_ERROR ("Device NOT CONNECTABLE\n");
        return enBTRCoreFailure;
    }

    BTRCORELOG_INFO ("Device CONNECTABLE\n");
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_ConnectDevice (
    tBTRCoreHandle          hBTRCore,
    tBTRCoreDevId           aBTRCoreDevId,
    enBTRCoreDeviceType     aenBTRCoreDevType
) {
    stBTRCoreHdl*           pstlhBTRCore            = NULL;
    enBTRCoreRet            lenBTRCoreRet           = enBTRCoreFailure;
    enBTDeviceType          lenBTDeviceType         = enBTDevUnknown;
    stBTRCoreBTDevice*      lpstBTRCoreBTDevice     = NULL;
    stBTRCoreDevStateInfo*  lpstBTRCoreDevStateInfo = NULL;
    const char*             lpcBTRCoreBTDevicePath  = NULL;
    const char*             lpcBTRCoreBTDeviceName  = NULL;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if ((lenBTRCoreRet = btrCore_GetDeviceInfo(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                               &lenBTDeviceType, &lpstBTRCoreBTDevice, &lpstBTRCoreDevStateInfo,
                                               &lpcBTRCoreBTDevicePath, &lpcBTRCoreBTDeviceName)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information - %llu\n", aBTRCoreDevId);
        return lenBTRCoreRet;
    }


    if ((lenBTDeviceType == enBTDevHID) &&
        strstr(lpcBTRCoreBTDeviceName, "Xbox")) {
        if (BtrCore_BTDisableEnhancedRetransmissionMode(pstlhBTRCore->connHdl) != 0) {
            BTRCORELOG_ERROR ("Failed to Disable ERTM\n");
        }
    }


    // TODO: Implement a Device State Machine and Check whether the device is in a Connectable State
    // before making the connect call
    if (BtrCore_BTConnectDevice(pstlhBTRCore->connHdl, lpcBTRCoreBTDevicePath, lenBTDeviceType) != 0) {
        BTRCORELOG_ERROR ("Connect to device failed - %llu\n", aBTRCoreDevId);
        return enBTRCoreFailure;
    }

    BTRCORELOG_INFO ("Connected to device %s Successfully. = %llu\n", lpcBTRCoreBTDeviceName, aBTRCoreDevId);
    /* Should think on moving a connected LE device from scanned list to paired list */

    if (lenBTDeviceType != enBTDevHID) {
        lpstBTRCoreBTDevice->bDeviceConnected = TRUE;
    }

    if ((lpstBTRCoreDevStateInfo->eDevicePrevState != enBTRCoreDevStConnected) &&
        (lpstBTRCoreDevStateInfo->eDeviceCurrState != enBTRCoreDevStPlaying)) {
        lpstBTRCoreDevStateInfo->eDevicePrevState  = lpstBTRCoreDevStateInfo->eDeviceCurrState;
    }

     if ((lpstBTRCoreDevStateInfo->eDeviceCurrState  != enBTRCoreDevStConnected) &&
         (lpstBTRCoreDevStateInfo->eDeviceCurrState  != enBTRCoreDevStPlaying) ) {
         lpstBTRCoreDevStateInfo->eDeviceCurrState   = enBTRCoreDevStConnecting;

        lenBTRCoreRet = enBTRCoreSuccess;
     }


    BTRCORELOG_DEBUG ("Ret - %d - %llu\n", lenBTRCoreRet, aBTRCoreDevId);
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_DisconnectDevice (
    tBTRCoreHandle          hBTRCore,
    tBTRCoreDevId           aBTRCoreDevId,
    enBTRCoreDeviceType     aenBTRCoreDevType
) {
    stBTRCoreHdl*           pstlhBTRCore            = NULL;
    enBTRCoreRet            lenBTRCoreRet           = enBTRCoreFailure;
    enBTDeviceType          lenBTDeviceType         = enBTDevUnknown;
    stBTRCoreBTDevice*      lpstBTRCoreBTDevice     = NULL;
    stBTRCoreDevStateInfo*  lpstBTRCoreDevStateInfo = NULL;
    const char*             lpcBTRCoreBTDevicePath  = NULL;
    const char*             lpcBTRCoreBTDeviceName  = NULL;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if ((lenBTRCoreRet = btrCore_GetDeviceInfo(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                               &lenBTDeviceType, &lpstBTRCoreBTDevice, &lpstBTRCoreDevStateInfo,
                                               &lpcBTRCoreBTDevicePath, &lpcBTRCoreBTDeviceName)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information - %llu\n", aBTRCoreDevId);
        return lenBTRCoreRet;
    }


    // TODO: Implement a Device State Machine and Check whether the device is in a Disconnectable State
    // before making the connect call
    if (BtrCore_BTDisconnectDevice(pstlhBTRCore->connHdl, lpcBTRCoreBTDevicePath, lenBTDeviceType) != 0) {
        BTRCORELOG_ERROR ("DisConnect from device failed - %llu\n", aBTRCoreDevId);
        return enBTRCoreFailure;
    }

    BTRCORELOG_INFO ("DisConnected from device %s Successfully.\n", lpcBTRCoreBTDeviceName);

    
    lpstBTRCoreBTDevice->bDeviceConnected = FALSE;

    if (lpstBTRCoreDevStateInfo->eDeviceCurrState   != enBTRCoreDevStDisconnected &&
        lpstBTRCoreDevStateInfo->eDeviceCurrState   != enBTRCoreDevStLost) {
        lpstBTRCoreDevStateInfo->eDevicePrevState    = lpstBTRCoreDevStateInfo->eDeviceCurrState;
        lpstBTRCoreDevStateInfo->eDeviceCurrState    = enBTRCoreDevStDisconnecting; 

        lenBTRCoreRet = enBTRCoreSuccess;
    }


    if ((lenBTDeviceType == enBTDevHID) &&
        strstr(lpcBTRCoreBTDeviceName, "Xbox")) {
        if (BtrCore_BTEnableEnhancedRetransmissionMode(pstlhBTRCore->connHdl) != 0) {
            BTRCORELOG_ERROR ("Failed to Enable ERTM\n");
        }
    }


    BTRCORELOG_DEBUG ("Ret - %d - %llu\n", lenBTRCoreRet, aBTRCoreDevId);
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_GetDeviceConnected (
    tBTRCoreHandle          hBTRCore, 
    tBTRCoreDevId           aBTRCoreDevId, 
    enBTRCoreDeviceType     aenBTRCoreDevType
) {
    stBTRCoreHdl*           pstlhBTRCore            = NULL;
    enBTRCoreRet            lenBTRCoreRet           = enBTRCoreFailure;
    enBTDeviceType          lenBTDeviceType         = enBTDevUnknown;
    stBTRCoreBTDevice*      lpstBTRCoreBTDevice     = NULL;
    stBTRCoreDevStateInfo*  lpstBTRCoreDevStateInfo = NULL;
    const char*             lpcBTRCoreBTDevicePath  = NULL;
    const char*             lpcBTRCoreBTDeviceName  = NULL;
    stBTPairedDeviceInfo*   pairedDeviceInfo = NULL;
    unsigned char i_idx = 0;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if ((lenBTRCoreRet = btrCore_GetDeviceInfo(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                               &lenBTDeviceType, &lpstBTRCoreBTDevice, &lpstBTRCoreDevStateInfo,
                                               &lpcBTRCoreBTDevicePath, &lpcBTRCoreBTDeviceName)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    (void)lenBTDeviceType;

    if ((lpstBTRCoreDevStateInfo->eDeviceCurrState == enBTRCoreDevStConnected) ||
        (lpstBTRCoreDevStateInfo->eDeviceCurrState == enBTRCoreDevStPlaying)) {
        BTRCORELOG_DEBUG ("enBTRCoreDevStConnected = %s\n", lpcBTRCoreBTDeviceName);
        lenBTRCoreRet = enBTRCoreSuccess;
    }
    else if (aenBTRCoreDevType == enBTRCoreHID) {
        if ((pairedDeviceInfo = g_malloc0(sizeof(stBTPairedDeviceInfo))) == NULL)
            return enBTRCoreFailure;

        if (BtrCore_BTGetPairedDeviceInfo (pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath, pairedDeviceInfo) != 0) {
            BTRCORELOG_ERROR("Getting the paired device info failed ...\n");
            g_free(pairedDeviceInfo);
            pairedDeviceInfo = NULL;
            return enBTRCoreFailure;
        }

        for (i_idx = 0; i_idx < pairedDeviceInfo->numberOfDevices; i_idx++) {
            if (btrCore_GenerateUniqueDeviceID(pairedDeviceInfo->deviceInfo[i_idx].pcAddress) == aBTRCoreDevId) {
                if (pairedDeviceInfo->deviceInfo[i_idx].bConnected == 1) {
                    BTRCORELOG_INFO("Updated the current state of the device now \n");
                    lenBTRCoreRet = enBTRCoreSuccess;
                    lpstBTRCoreDevStateInfo->eDevicePrevState = lpstBTRCoreDevStateInfo->eDeviceCurrState;
                    lpstBTRCoreDevStateInfo->eDeviceCurrState = enBTRCoreDevStConnected;
                    break;
                } else {
                    lenBTRCoreRet = enBTRCoreFailure;
                    break;
                }
            }
        }
        g_free(pairedDeviceInfo);
        pairedDeviceInfo = NULL;
    }
    else {
        lenBTRCoreRet = enBTRCoreFailure;
    }

    BTRCORELOG_INFO ("Ret - %d : CurrState - %d\n", lenBTRCoreRet, lpstBTRCoreDevStateInfo->eDeviceCurrState);
    return lenBTRCoreRet;
}


enBTRCoreRet
BTRCore_GetDeviceDisconnected (
    tBTRCoreHandle          hBTRCore, 
    tBTRCoreDevId           aBTRCoreDevId, 
    enBTRCoreDeviceType     aenBTRCoreDevType
) {
    stBTRCoreHdl*           pstlhBTRCore            = NULL;
    enBTRCoreRet            lenBTRCoreRet           = enBTRCoreFailure;
    enBTDeviceType          lenBTDeviceType         = enBTDevUnknown;
    stBTRCoreBTDevice*      lpstBTRCoreBTDevice     = NULL;
    stBTRCoreDevStateInfo*  lpstBTRCoreDevStateInfo = NULL;
    const char*             lpcBTRCoreBTDevicePath  = NULL;
    const char*             lpcBTRCoreBTDeviceName  = NULL;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if ((lenBTRCoreRet = btrCore_GetDeviceInfo(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                               &lenBTDeviceType, &lpstBTRCoreBTDevice, &lpstBTRCoreDevStateInfo,
                                               &lpcBTRCoreBTDevicePath, &lpcBTRCoreBTDeviceName)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }


    (void)lenBTDeviceType;

    if ((lpstBTRCoreDevStateInfo->eDeviceCurrState == enBTRCoreDevStDisconnected) ||
        (lpstBTRCoreDevStateInfo->eDeviceCurrState == enBTRCoreDevStLost)) {
        BTRCORELOG_DEBUG ("enBTRCoreDevStDisconnected = %s\n", lpcBTRCoreBTDeviceName);
        lenBTRCoreRet = enBTRCoreSuccess;
    }
    else {
        lenBTRCoreRet = enBTRCoreFailure;
    }

    BTRCORELOG_INFO ("Ret - %d : CurrState - %d\n", lenBTRCoreRet, lpstBTRCoreDevStateInfo->eDeviceCurrState);
    return lenBTRCoreRet;
}


enBTRCoreRet
BTRCore_GetDeviceTypeClass (
    tBTRCoreHandle          hBTRCore, 
    tBTRCoreDevId           aBTRCoreDevId, 
    enBTRCoreDeviceType*    apenBTRCoreDevTy,
    enBTRCoreDeviceClass*   apenBTRCoreDevCl
) {
    stBTRCoreHdl*           pstlhBTRCore    = NULL;
    stBTRCoreBTDevice*      pstBTDevice     = NULL;
    int                     i32LoopIdx      = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!apenBTRCoreDevTy || !apenBTRCoreDevCl) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }


    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (pstlhBTRCore->numOfPairedDevices) {
        if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
            pstBTDevice = &pstlhBTRCore->stKnownDevicesArr[aBTRCoreDevId];
        }
        else {
            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                if (aBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                    pstBTDevice = &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx];
                    break;
                }
            }
        }
    }


    if (!pstBTDevice && pstlhBTRCore->numOfScannedDevices) {
        if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES) {
            pstBTDevice = &pstlhBTRCore->stScannedDevicesArr[aBTRCoreDevId];
        }
        else {
            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
                if (aBTRCoreDevId == pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
                    pstBTDevice = &pstlhBTRCore->stScannedDevicesArr[i32LoopIdx];
                    break;
                }
            }
        }
    }

    
    if (!pstBTDevice) {
        *apenBTRCoreDevTy = enBTRCoreUnknown; 
        *apenBTRCoreDevCl = enBTRCore_DC_Unknown;
        return enBTRCoreFailure;
    }


    *apenBTRCoreDevCl = pstBTDevice->enDeviceType;
    *apenBTRCoreDevTy = btrCore_MapDevClassToDevType(pstBTDevice->enDeviceType);

    return enBTRCoreSuccess;
}

enBTRCoreRet BTRCore_refreshLEActionListForGamepads(tBTRCoreHandle hBTRCore)
{
    char lcpAddDeviceCmd[BT_MAX_STR_LEN/2] = {'\0'};
    char lcBtmgmtResponse[BT_MAX_STR_LEN/2] = {'\0'};
    FILE*           lpcBtmgmtCmd = NULL;
    int             i32DevIdx = 0;
    stBTRCoreHdl*   pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    if (!pstlhBTRCore)
    {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    if (btrCore_PopulateListOfPairedDevices(pstlhBTRCore, pstlhBTRCore->curAdapterPath) == enBTRCoreSuccess) {
        for (i32DevIdx = 0; i32DevIdx < pstlhBTRCore->numOfPairedDevices; i32DevIdx++) {
            //only refresh action list for LE gamepads
            if (((pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_Keyboard)      ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_Mouse)         ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_MouseKeyBoard) ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_Joystick)      ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_GamePad)       ||
            ((pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_Unknown) &&
             (btrCore_IsStadiaGamepad(pstlhBTRCore->stKnownDevicesArr[i32DevIdx].pcDeviceAddress) ||
             (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].pcDeviceName[0] != '\0' &&
              strstr(pstlhBTRCore->stKnownDevicesArr[i32DevIdx].pcDeviceName, "Stadia"))))) &&
            !btrCore_IsDeviceRdkRcu(pstlhBTRCore->stKnownDevicesArr[i32DevIdx].pcDeviceAddress, pstlhBTRCore->stKnownDevicesArr[i32DevIdx].ui16DevAppearanceBleSpec) &&
            pstlhBTRCore->stKnownDevicesArr[i32DevIdx].ui32DevClassBtSpec == 0 ) {

                //if the state is disconnected, refreshing the action list will not change anything as upper layers may not allow the autoconnection - reset the device to paired here
                if (pstlhBTRCore->stKnownDevStInfoArr[i32DevIdx].eDeviceCurrState == enBTRCoreDevStDisconnected)
                {
                    pstlhBTRCore->stKnownDevStInfoArr[i32DevIdx].eDevicePrevState = enBTRCoreDevStInitialized;
                    pstlhBTRCore->stKnownDevStInfoArr[i32DevIdx].eDeviceCurrState = enBTRCoreDevStPaired;
                }

                if (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_Unknown) {
                    BTRCORELOG_INFO("No valid device type and appearance values so marking as HID ...\n");
                    pstlhBTRCore->stKnownDevicesArr[i32DevIdx].ui16DevAppearanceBleSpec = BTRCORE_LE_HID_DEVICE_APPEARANCE;
                    pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType = enBTRCore_DC_HID_GamePad;
                }

                snprintf(lcpAddDeviceCmd, BT_MAX_STR_LEN/2, "btmgmt add-device -t 1 -a 2 %s", pstlhBTRCore->stKnownDevicesArr[i32DevIdx].pcDeviceAddress);
                BTRCORELOG_INFO ("lcpAddDeviceCmd: %s\n", lcpAddDeviceCmd);

                lpcBtmgmtCmd = popen(lcpAddDeviceCmd, "r");
                if ((lpcBtmgmtCmd == NULL)) {
                    BTRCORELOG_ERROR ("Failed to run lcpAddDeviceCmd command\n");
                }
                else {
                    if (fgets(lcBtmgmtResponse, sizeof(lcBtmgmtResponse)-1, lpcBtmgmtCmd) == NULL) {
                        BTRCORELOG_ERROR ("Failed to Output of lcpAddDeviceCmd\n");
                    }
                    else {
                        BTRCORELOG_WARN ("Output of lcpAddDeviceCmd =  %s\n", lcBtmgmtResponse);
                    }

                    pclose(lpcBtmgmtCmd);
                }
            }
        }
    }
    return enBTRCoreSuccess;
}

enBTRCoreRet BTRCore_clearLEActionListForGamepads(tBTRCoreHandle hBTRCore)
{
    char lcpAddDeviceCmd[BT_MAX_STR_LEN/2] = {'\0'};
    char lcBtmgmtResponse[BT_MAX_STR_LEN/2] = {'\0'};
    FILE*           lpcBtmgmtCmd = NULL;
    int             i32DevIdx = 0;
    stBTRCoreHdl*   pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    if (!pstlhBTRCore)
    {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    if (btrCore_PopulateListOfPairedDevices(pstlhBTRCore, pstlhBTRCore->curAdapterPath) == enBTRCoreSuccess) {
        for (i32DevIdx = 0; i32DevIdx < pstlhBTRCore->numOfPairedDevices; i32DevIdx++) {
            //only refresh action list for LE gamepads
            if (((pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_Keyboard)      ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_Mouse)         ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_MouseKeyBoard) ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_Joystick)      ||
            (pstlhBTRCore->stKnownDevicesArr[i32DevIdx].enDeviceType == enBTRCore_DC_HID_GamePad)) && 
            !btrCore_IsDeviceRdkRcu(pstlhBTRCore->stKnownDevicesArr[i32DevIdx].pcDeviceAddress, pstlhBTRCore->stKnownDevicesArr[i32DevIdx].ui16DevAppearanceBleSpec) &&
            pstlhBTRCore->stKnownDevicesArr[i32DevIdx].ui32DevClassBtSpec == 0 ) {
                snprintf(lcpAddDeviceCmd, BT_MAX_STR_LEN/2, "btmgmt add-device -t 1 -a 0 %s", pstlhBTRCore->stKnownDevicesArr[i32DevIdx].pcDeviceAddress);
                BTRCORELOG_INFO ("lcpAddDeviceCmd: %s\n", lcpAddDeviceCmd);

                lpcBtmgmtCmd = popen(lcpAddDeviceCmd, "r");
                if ((lpcBtmgmtCmd == NULL)) {
                    BTRCORELOG_ERROR ("Failed to run lcpAddDeviceCmd command\n");
                }
                else {
                    if (fgets(lcBtmgmtResponse, sizeof(lcBtmgmtResponse)-1, lpcBtmgmtCmd) == NULL) {
                        BTRCORELOG_ERROR ("Failed to Output of lcpAddDeviceCmd\n");
                    }
                    else {
                        BTRCORELOG_WARN ("Output of lcpAddDeviceCmd =  %s\n", lcBtmgmtResponse);
                    }

                    pclose(lpcBtmgmtCmd);
                }
            }
        }
    }
    return enBTRCoreSuccess;
}

enBTRCoreRet BTRCore_newBatteryLevelDevice (tBTRCoreHandle hBTRCore)
{

    stBTRCoreHdl* lpstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
    if (lpstlhBTRCore == NULL)
    {
        BTRCORELOG_ERROR("Invalid arguments, cannot operate on battery level\n");
        return enBTRCoreInvalidArg;
    }
    //if not already started initialise the update batterylevel thread
    if (lpstlhBTRCore->batteryLevelThread == NULL)
    {
        BTRCORELOG_TRACE("Starting battery level thread\n");
        lpstlhBTRCore->batteryLevelThread  = g_thread_new("btrCore_BatteryLevelThread", btrCore_BatteryLevelThread, (gpointer)lpstlhBTRCore);
        if (lpstlhBTRCore->batteryLevelThread != NULL)
        {
            BTRCORELOG_DEBUG("Battery thread created successfully\n");
        }
        else
        {
            BTRCORELOG_ERROR("Battery thread creation failed\n");
        }
        return enBTRCoreSuccess;
    }
    else
    {
        BTRCORELOG_INFO("Signalling battery level thread to refresh battery level\n");
        g_mutex_lock(&lpstlhBTRCore->batteryLevelMutex);
        g_cond_signal(&lpstlhBTRCore->batteryLevelCond); // wake battery level thread to refresh battery levels
        g_mutex_unlock(&lpstlhBTRCore->batteryLevelMutex);
    }
    return enBTRCoreSuccess;
}

enBTRCoreRet
BTRCore_GetDeviceMediaInfo (
    tBTRCoreHandle          hBTRCore,
    tBTRCoreDevId           aBTRCoreDevId,
    enBTRCoreDeviceType     aenBTRCoreDevType,
    stBTRCoreDevMediaInfo*  apstBTRCoreDevMediaInfo
) {
#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore    = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

    stBTRCoreAVMediaInfo    lstBtrCoreMediaInfo;
    unsigned int            ui32AVMCodecInfoSize = 0;

    
    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!apstBTRCoreDevMediaInfo || !apstBTRCoreDevMediaInfo->pstBtrCoreDevMCodecInfo) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    MEMSET_S(&lstBtrCoreMediaInfo, sizeof(stBTRCoreAVMediaInfo), 0, sizeof(stBTRCoreAVMediaInfo));
    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }


    BTRCORELOG_INFO (" We will get Media Info for %s - DevTy %d\n", pDeviceAddress, lenBTDeviceType);

    switch (lenBTDeviceType) {
    case enBTDevAudioSink:
        lstBtrCoreMediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowOut;
        break;
    case enBTDevAudioSource:
        lstBtrCoreMediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowIn;
        break;
    case enBTDevHFPHeadset:
        lstBtrCoreMediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowInOut;
        break;
    case enBTDevHFPAudioGateway:
        lstBtrCoreMediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowInOut;
        break;
    case enBTDevLE:
        lstBtrCoreMediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowUnknown;
        break;
    case enBTDevUnknown:
        lstBtrCoreMediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowUnknown;
        break;
    default:
        lstBtrCoreMediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowUnknown;
        break;
    }


    ui32AVMCodecInfoSize = sizeof(stBTRCoreAVMediaMpegInfo) > sizeof(stBTRCoreAVMediaSbcInfo) ? sizeof(stBTRCoreAVMediaMpegInfo) : sizeof(stBTRCoreAVMediaSbcInfo);
    ui32AVMCodecInfoSize = ui32AVMCodecInfoSize > sizeof(stBTRMgrAVMediaPcmInfo) ? ui32AVMCodecInfoSize : sizeof(stBTRMgrAVMediaPcmInfo);

    lstBtrCoreMediaInfo.eBtrCoreAVMType = eBTRCoreAVMTypeUnknown;
    if (!(lstBtrCoreMediaInfo.pstBtrCoreAVMCodecInfo = g_malloc0(ui32AVMCodecInfoSize))) {
        BTRCORELOG_ERROR ("AVMedia_GetCurMediaInfo - Unable to alloc Memory\n");
        return lenBTRCoreRet;
    }


    // TODO: Implement a Device State Machine and Check whether the device is Connected before making the call
    if ((lenBTRCoreRet = BTRCore_AVMedia_GetCurMediaInfo (pstlhBTRCore->avMediaHdl, pDeviceAddress, &lstBtrCoreMediaInfo)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("AVMedia_GetCurMediaInfo ERROR occurred\n");
        g_free(lstBtrCoreMediaInfo.pstBtrCoreAVMCodecInfo);
        lstBtrCoreMediaInfo.pstBtrCoreAVMCodecInfo = NULL;
        return lenBTRCoreRet;
    }

    switch (lstBtrCoreMediaInfo.eBtrCoreAVMType) {
    case eBTRCoreAVMTypePCM:
        apstBTRCoreDevMediaInfo->eBtrCoreDevMType = eBTRCoreDevMediaTypePCM;
        break;
    case eBTRCoreAVMTypeSBC: {
        stBTRCoreDevMediaSbcInfo*   lapstBtrCoreDevMCodecInfo = (stBTRCoreDevMediaSbcInfo*)(apstBTRCoreDevMediaInfo->pstBtrCoreDevMCodecInfo);
        stBTRCoreAVMediaSbcInfo*    lpstBtrCoreAVMSbcInfo = (stBTRCoreAVMediaSbcInfo*)(lstBtrCoreMediaInfo.pstBtrCoreAVMCodecInfo);

        apstBTRCoreDevMediaInfo->eBtrCoreDevMType = eBTRCoreDevMediaTypeSBC;

        switch (lpstBtrCoreAVMSbcInfo->eAVMAChan) {
        case eBTRCoreAVMAChanMono:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanMono;
            break;
        case eBTRCoreAVMAChanDualChannel:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanDualChannel;
            break;
        case eBTRCoreAVMAChanStereo:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanStereo;
            break;
        case eBTRCoreAVMAChanJointStereo:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanJointStereo;
            break;
        case eBTRCoreAVMAChan5_1:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChan5_1;
            break;
        case eBTRCoreAVMAChan7_1:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChan7_1;
            break;
        case eBTRCoreAVMAChanUnknown:
        default:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanUnknown;
            break;
        }

        lapstBtrCoreDevMCodecInfo->ui32DevMSFreq         = lpstBtrCoreAVMSbcInfo->ui32AVMSFreq;
        lapstBtrCoreDevMCodecInfo->ui8DevMSbcAllocMethod = lpstBtrCoreAVMSbcInfo->ui8AVMSbcAllocMethod;
        lapstBtrCoreDevMCodecInfo->ui8DevMSbcSubbands    = lpstBtrCoreAVMSbcInfo->ui8AVMSbcSubbands;
        lapstBtrCoreDevMCodecInfo->ui8DevMSbcBlockLength = lpstBtrCoreAVMSbcInfo->ui8AVMSbcBlockLength;
        lapstBtrCoreDevMCodecInfo->ui8DevMSbcMinBitpool  = lpstBtrCoreAVMSbcInfo->ui8AVMSbcMinBitpool;
        lapstBtrCoreDevMCodecInfo->ui8DevMSbcMaxBitpool  = lpstBtrCoreAVMSbcInfo->ui8AVMSbcMaxBitpool;
        lapstBtrCoreDevMCodecInfo->ui16DevMSbcFrameLen   = lpstBtrCoreAVMSbcInfo->ui16AVMSbcFrameLen;
        lapstBtrCoreDevMCodecInfo->ui16DevMSbcBitrate    = lpstBtrCoreAVMSbcInfo->ui16AVMSbcBitrate;

        break;
    }
    case eBTRCoreAVMTypeMPEG:
        apstBTRCoreDevMediaInfo->eBtrCoreDevMType = eBTRCoreDevMediaTypeMPEG;
        break;
    case eBTRCoreAVMTypeAAC: {
        stBTRCoreDevMediaMpegInfo* lapstBtrCoreDevMCodecInfo = (stBTRCoreDevMediaMpegInfo*)(apstBTRCoreDevMediaInfo->pstBtrCoreDevMCodecInfo);
        stBTRCoreAVMediaMpegInfo*  lpstBtrCoreAVMAacInfo = (stBTRCoreAVMediaMpegInfo*)(lstBtrCoreMediaInfo.pstBtrCoreAVMCodecInfo);

        apstBTRCoreDevMediaInfo->eBtrCoreDevMType = eBTRCoreDevMediaTypeAAC;

        switch (lpstBtrCoreAVMAacInfo->eAVMAChan) {
        case eBTRCoreAVMAChanMono:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanMono;
            break;
        case eBTRCoreAVMAChanDualChannel:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanDualChannel;
            break;
        case eBTRCoreAVMAChanStereo:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanStereo;
            break;
        case eBTRCoreAVMAChanJointStereo:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanJointStereo;
            break;
        case eBTRCoreAVMAChan5_1:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChan5_1;
            break;
        case eBTRCoreAVMAChan7_1:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChan7_1;
            break;
        case eBTRCoreAVMAChanUnknown:
        default:
            lapstBtrCoreDevMCodecInfo->eDevMAChan = eBTRCoreDevMediaAChanUnknown;
            break;
        }

        lapstBtrCoreDevMCodecInfo->ui32DevMSFreq        = lpstBtrCoreAVMAacInfo->ui32AVMSFreq;
        lapstBtrCoreDevMCodecInfo->ui8DevMMpegCrc       = lpstBtrCoreAVMAacInfo->ui8AVMMpegCrc;
        lapstBtrCoreDevMCodecInfo->ui8DevMMpegLayer     = lpstBtrCoreAVMAacInfo->ui8AVMMpegLayer;
        lapstBtrCoreDevMCodecInfo->ui8DevMMpegMpf       = lpstBtrCoreAVMAacInfo->ui8AVMMpegMpf;
        lapstBtrCoreDevMCodecInfo->ui8DevMMpegRfa       = lpstBtrCoreAVMAacInfo->ui8AVMMpegRfa;
        lapstBtrCoreDevMCodecInfo->ui16DevMMpegFrameLen = lpstBtrCoreAVMAacInfo->ui16AVMMpegFrameLen;
        lapstBtrCoreDevMCodecInfo->ui16DevMMpegBitrate  = lpstBtrCoreAVMAacInfo->ui16AVMMpegBitrate;

        break;
    }
    case eBTRCoreAVMTypeUnknown:
    default:
        apstBTRCoreDevMediaInfo->eBtrCoreDevMType = eBTRCoreDevMediaTypeUnknown;
        break;
    }

    g_free(lstBtrCoreMediaInfo.pstBtrCoreAVMCodecInfo);
    lstBtrCoreMediaInfo.pstBtrCoreAVMCodecInfo = NULL;
#endif

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_AcquireDeviceDataPath (
    tBTRCoreHandle      hBTRCore,
    tBTRCoreDevId       aBTRCoreDevId,
    enBTRCoreDeviceType aenBTRCoreDevType,
    int*                aiDataPath,
    int*                aidataReadMTU,
    int*                aidataWriteMTU,
    unsigned int*       apui32Delay
) {
#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;
    enBTRCoreDeviceState    lenDevPrevState     = enBTRCoreDevStUnknown;
    enBTRCoreDeviceState    lenDevCurrState     = enBTRCoreDevStUnknown;

    int                     liDataPath      = 0;
    int                     lidataReadMTU   = 0;
    int                     lidataWriteMTU  = 0;
    unsigned int            ui32Delay       = 0xFFFFu;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!aiDataPath || !aidataReadMTU || !aidataWriteMTU || !apui32Delay) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    lenDevPrevState = lpstKnownDevStInfo->eDevicePrevState;
    lenDevCurrState = lpstKnownDevStInfo->eDeviceCurrState;
    lpstKnownDevStInfo->eDevicePrevState = lpstKnownDevStInfo->eDeviceCurrState;
    if (lpstKnownDevStInfo->eDeviceCurrState != enBTRCoreDevStPlaying) {
        lpstKnownDevStInfo->eDeviceCurrState = enBTRCoreDevStPlaying;
    }

    BTRCORELOG_INFO (" We will Acquire Data Path for %s\n", pDeviceAddress);
    if (BTRCore_AVMedia_AcquireDataPath(pstlhBTRCore->avMediaHdl, pDeviceAddress, &liDataPath, &lidataReadMTU, &lidataWriteMTU, &ui32Delay) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("AVMedia_AcquireDataPath ERROR occurred\n");
        lpstKnownDevStInfo->eDevicePrevState = lenDevPrevState;
        lpstKnownDevStInfo->eDeviceCurrState = lenDevCurrState;
        return enBTRCoreFailure;
    }

    *aiDataPath     = liDataPath;
    *aidataReadMTU  = lidataReadMTU;
    *aidataWriteMTU = lidataWriteMTU;
    *apui32Delay    = ui32Delay;
#endif

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_ReleaseDeviceDataPath (
    tBTRCoreHandle      hBTRCore,
    tBTRCoreDevId       aBTRCoreDevId,
    enBTRCoreDeviceType aenBTRCoreDevType
) {
#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    
    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    //TODO: Make a Device specific call baced on lenBTDeviceType
    (void)lenBTDeviceType;

    BTRCORELOG_INFO (" We will Release Data Path for %s\n", pDeviceAddress);

    // TODO: Implement a Device State Machine and Check whether the device is in a State  to acquire Device Data path
    // before making the call
    if(BTRCore_AVMedia_ReleaseDataPath(pstlhBTRCore->avMediaHdl, pDeviceAddress) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("AVMedia_ReleaseDataPath ERROR occurred\n");
        return enBTRCoreFailure;
    }
#endif

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_SetDeviceDataAckTimeout (
    tBTRCoreHandle  hBTRCore,
    unsigned int    aui32AckTOutms
) {
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (BtrCore_BTSetDevDataAckTimeout(pstlhBTRCore->connHdl, aui32AckTOutms) != 0) {
        BTRCORELOG_ERROR ("Failed to SetDevDataAckTimeout\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_MediaControl (
    tBTRCoreHandle          hBTRCore, 
    tBTRCoreDevId           aBTRCoreDevId, 
    enBTRCoreDeviceType     aenBTRCoreDevType,
    enBTRCoreMediaCtrl      aenBTRCoreMediaCtrl,
    stBTRCoreMediaCtData*   apstBTRCoreMediaCData
) {
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;

    enBTRCoreAVMediaCtrl    lenBTRCoreAVMediaCtrl = 0;
    eBTRCoreAVMediaFlow     lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowUnknown;
    stBTRCoreAVMediaCtData* lpstBTRCoreAVMCtData  = NULL;


    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if (pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }


    switch (lenBTDeviceType) {
    case enBTDevAudioSink:
        lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowOut;
        break;
    case enBTDevAudioSource:
        lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowIn;
        break;
    case enBTDevHFPHeadset:
        lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowInOut;
        break;
    case enBTDevHFPAudioGateway:
        lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowInOut;
        break;
    case enBTDevLE:
        lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowUnknown;
        break;
    case enBTDevUnknown:
        lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowUnknown;
        break;
    default:
        lenBTRCoreAVMediaFlow = eBTRCoreAVMediaFlowUnknown;
        break;
    }

    switch (aenBTRCoreMediaCtrl) {
    case enBTRCoreMediaCtrlPlay:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlPlay;
        break;
    case enBTRCoreMediaCtrlPause:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlPause;
        break;
    case enBTRCoreMediaCtrlStop:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlStop;
        break;
    case enBTRCoreMediaCtrlNext:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlNext;
        break;
    case enBTRCoreMediaCtrlPrevious:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlPrevious;
        break;
    case enBTRCoreMediaCtrlFastForward:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlFastForward;
        break;
    case enBTRCoreMediaCtrlRewind:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlRewind;
        break;
    case enBTRCoreMediaCtrlVolumeUp:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlVolumeUp;
        break;
    case enBTRCoreMediaCtrlVolumeDown:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlVolumeDown;
        break;
    case enBTRCoreMediaCtrlEqlzrOff:
        lenBTRCoreAVMediaCtrl = enBTRcoreAVMediaCtrlEqlzrOff;
        break;
    case enBTRCoreMediaCtrlEqlzrOn:
        lenBTRCoreAVMediaCtrl = enBTRcoreAVMediaCtrlEqlzrOn;
        break;
    case enBTRCoreMediaCtrlShflOff:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlShflOff;
        break;
    case enBTRCoreMediaCtrlShflAllTracks:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlShflAllTracks; 
        break;
    case enBTRCoreMediaCtrlShflGroup:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlShflGroup;
        break;
    case enBTRCoreMediaCtrlRptOff:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlRptOff;
        break;
    case enBTRCoreMediaCtrlRptSingleTrack:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlRptSingleTrack;
        break;
    case enBTRCoreMediaCtrlRptAllTracks:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlRptAllTracks;
        break;
    case enBTRCoreMediaCtrlRptGroup:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlRptGroup;
        break;
    default:
        lenBTRCoreAVMediaCtrl = enBTRCoreAVMediaCtrlUnknown;
        break;
    }

    if (lenBTRCoreAVMediaCtrl == enBTRCoreAVMediaCtrlUnknown) {
        BTRCORELOG_ERROR ("Media Play Control Unknown!\n");
        lenBTRCoreRet = enBTRCoreFailure;
    }
    else {
        stBTRCoreAVMediaCtData lstBTRCoreAVMCtData;

        if (apstBTRCoreMediaCData != NULL) {
            lstBTRCoreAVMCtData.m_mediaAbsTransportVolume = apstBTRCoreMediaCData->m_mediaAbsoluteVolume;
            lpstBTRCoreAVMCtData = &lstBTRCoreAVMCtData;
        }

        BTRCORELOG_INFO (" We will Perform Media Control for %s - DevTy %d - DevName %s - Ctrl %d - Flow %d\n", pDeviceAddress, lenBTDeviceType, pstKnownDevice->pcDeviceName, lenBTRCoreAVMediaCtrl, lenBTRCoreAVMediaFlow);

        if ((lenBTRCoreRet = BTRCore_AVMedia_MediaControl(pstlhBTRCore->avMediaHdl,
                                                          pDeviceAddress,
                                                          lenBTRCoreAVMediaCtrl,
                                                          lenBTRCoreAVMediaFlow,
                                                          lpstBTRCoreAVMCtData,
                                                          pstKnownDevice->pcDeviceName)) != enBTRCoreSuccess) {
            BTRCORELOG_ERROR ("Media Play Control Failed!!!\n");
        }
    }
#endif

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_GetMediaTrackInfo (
    tBTRCoreHandle            hBTRCore,
    tBTRCoreDevId             aBTRCoreDevId,
    enBTRCoreDeviceType       aenBTRCoreDevType,
    stBTRCoreMediaTrackInfo*  apstBTMediaTrackInfo
) {
#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore    = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if (pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }


    if (BTRCore_AVMedia_GetTrackInfo(pstlhBTRCore->avMediaHdl,
                                     pDeviceAddress,
                                     (stBTRCoreAVMediaTrackInfo*)apstBTMediaTrackInfo) != enBTRCoreSuccess)  {
        BTRCORELOG_ERROR ("AVMedia get media track information Failed!!!\n");
        return enBTRCoreFailure;
    }
#endif

    return enBTRCoreSuccess;
}

enBTRCoreRet
BTRCore_GetMediaElementTrackInfo (
    tBTRCoreHandle            hBTRCore,
    tBTRCoreDevId             aBTRCoreDevId,
    enBTRCoreDeviceType       aenBTRCoreDevType,
    tBTRCoreMediaElementId  aBtrMediaElementId,
    stBTRCoreMediaTrackInfo*  apstBTMediaTrackInfo
) {
#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore    = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if(pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }


    if (BTRCore_AVMedia_GetElementTrackInfo(pstlhBTRCore->avMediaHdl,
                                            pDeviceAddress,
                                            aBtrMediaElementId,
                                            (stBTRCoreAVMediaTrackInfo*)apstBTMediaTrackInfo) != enBTRCoreSuccess)  {
        BTRCORELOG_ERROR ("AVMedia get media track information Failed!!!\n");
        return enBTRCoreFailure;
    }

#endif
    return enBTRCoreSuccess;
}



enBTRCoreRet
BTRCore_GetMediaPositionInfo (
    tBTRCoreHandle              hBTRCore,
    tBTRCoreDevId               aBTRCoreDevId,
    enBTRCoreDeviceType         aenBTRCoreDevType,
    stBTRCoreMediaPositionInfo* apstBTMediaPositionInfo
) {
#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if(pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }


   if (BTRCore_AVMedia_GetPositionInfo(pstlhBTRCore->avMediaHdl,
                                       pDeviceAddress,
                                       (stBTRCoreAVMediaPositionInfo*)apstBTMediaPositionInfo) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("AVMedia get Media Position Info Failed!!!\n");
        return enBTRCoreFailure;
    }

#endif
    return enBTRCoreSuccess;
}



enBTRCoreRet
BTRCore_GetMediaProperty (
    tBTRCoreHandle          hBTRCore,
    tBTRCoreDevId           aBTRCoreDevId,
    enBTRCoreDeviceType     aenBTRCoreDevType,
    const char*             mediaPropertyKey,
    void*                   mediaPropertyValue
) {
#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if (pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }


    if (BTRCore_AVMedia_GetMediaProperty(pstlhBTRCore->avMediaHdl,
                                         pDeviceAddress,
                                         mediaPropertyKey,
                                         mediaPropertyValue) != enBTRCoreSuccess)  {
        BTRCORELOG_ERROR ("AVMedia get property Failed!!!\n");
        return enBTRCoreFailure;
    }

#endif
    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_SelectMediaElement (
    tBTRCoreHandle          hBTRCore,
    tBTRCoreDevId           aBTRCoreDevId,
    tBTRCoreMediaElementId  aBtrMediaElementId,
    enBTRCoreDeviceType     aenBTRCoreDevType,
    eBTRCoreMedElementType  aenBTRCoreMedElementType
) {
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    eBTRCoreAVMElementType  lAVMediaElementType = eBTRCoreAVMETypeUnknown;
    char                    isPlayable          = 0;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    // CID 44523: Unsigned compared against 0 (NO_EFFECT)

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if (pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }

    switch (aenBTRCoreMedElementType) {
    case enBTRCoreMedETypeAlbum:
        lAVMediaElementType = eBTRCoreAVMETypeAlbum;
        break;
    case enBTRCoreMedETypeArtist:
        lAVMediaElementType = eBTRCoreAVMETypeArtist;
        break;
    case enBTRCoreMedETypeGenre:
        lAVMediaElementType = eBTRCoreAVMETypeGenre;
        break;
    case enBTRCoreMedETypeCompilation:
        lAVMediaElementType = eBTRCoreAVMETypeCompilation;
        break;
    case enBTRCoreMedETypePlayList:
        lAVMediaElementType = eBTRCoreAVMETypePlayList;
        break;
    case enBTRCoreMedETypeTrackList:
        lAVMediaElementType = eBTRCoreAVMETypeTrackList;
        break;
    case enBTRCoreMedETypeTrack:
        lAVMediaElementType = eBTRCoreAVMETypeTrack;
        break;
    default:
        break;
    }

    if (BTRCore_AVMedia_IsMediaElementPlayable (pstlhBTRCore->avMediaHdl,
                                                pDeviceAddress,
                                                aBtrMediaElementId,
                                                &isPlayable) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to MediaElement's playable state!\n");
        return enBTRCoreFailure;                
    }

    if (isPlayable) {

        if (BTRCore_AVMedia_PlayTrack (pstlhBTRCore->avMediaHdl, pDeviceAddress, aBtrMediaElementId) != enBTRCoreSuccess)  {
            BTRCORELOG_ERROR ("AVMedia Play Media Track by Element Id(%llu) Failed!!!\n", aBtrMediaElementId);
            return enBTRCoreFailure;
        }
    }
    else {

        if (BTRCore_AVMedia_ChangeBrowserLocation (pstlhBTRCore->avMediaHdl,
                                                   pDeviceAddress,
                                                   aBtrMediaElementId,
                                                   lAVMediaElementType) != enBTRCoreSuccess)  {
            BTRCORELOG_ERROR ("AVMedia change browser location to %llu Failed!\n", aBtrMediaElementId);
            return enBTRCoreFailure;    
        }
    }

    (void)lAVMediaElementType;
#endif

    return lenBTRCoreRet;
}


enBTRCoreRet
BTRCore_GetMediaElementList (
    tBTRCoreHandle                  hBTRCore,
    tBTRCoreDevId                   aBTRCoreDevId,
    tBTRCoreMediaElementId          aBtrMediaElementId,
    unsigned short                  aui16BtrMedElementStartIdx,
    unsigned short                  aui16BtrMedElementEndIdx,
    enBTRCoreDeviceType             aenBTRCoreDevType,
    eBTRCoreMedElementType          aenBTRCoreMedElementType,
    stBTRCoreMediaElementInfoList*  apstMediaElementListInfo
) {
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;

    stBTRCoreAVMediaElementInfoList  lpstAVMediaElementInfoList;

    if (!hBTRCore || !apstMediaElementListInfo) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    // CID 44534: Unsigned compared against 0 (NO_EFFECT)

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if (pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }

    /* TODO find if its current folder or not and prevent calling SelectMediaBrowserElements() for non current folder */

    if (BTRCore_AVMedia_SelectMediaBrowserElements (pstlhBTRCore->avMediaHdl,
                                                    pDeviceAddress,
                                                    aui16BtrMedElementStartIdx,
                                                    aui16BtrMedElementEndIdx) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("AVMedia Select Media Browser Elements Failed for %llu (%u - %u)!\n", aBtrMediaElementId
                          , aui16BtrMedElementStartIdx, aui16BtrMedElementEndIdx);
        //return enBTRCoreFailure;
    }

    MEMSET_S(&lpstAVMediaElementInfoList, sizeof(stBTRCoreAVMediaElementInfoList), 0, sizeof(stBTRCoreAVMediaElementInfoList));

    if (BTRCore_AVMedia_GetMediaElementList (pstlhBTRCore->avMediaHdl,
                                             pDeviceAddress,
                                             aBtrMediaElementId,
                                             aui16BtrMedElementStartIdx,
                                             aui16BtrMedElementEndIdx,
                                             aenBTRCoreMedElementType,
                                             &lpstAVMediaElementInfoList) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("AVMedia Get Media Item List Failed for %llu (%u - %u)!\n", aBtrMediaElementId, aui16BtrMedElementStartIdx, aui16BtrMedElementEndIdx);
        return enBTRCoreFailure;
    }

    MEMCPY_S(apstMediaElementListInfo,sizeof(stBTRCoreMediaElementInfoList), &lpstAVMediaElementInfoList, sizeof(stBTRCoreMediaElementInfoList));

#endif
    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_SetMediaElementActive (
    tBTRCoreHandle          hBTRCore, 
    tBTRCoreDevId           aBTRCoreDevId, 
    tBTRCoreMediaElementId  aBtrMediaElementId, 
    enBTRCoreDeviceType     aenBTRCoreDevType,
    eBTRCoreMedElementType  aenBTRCoreMedElementType
) {
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;

#ifndef LE_MODE
    stBTRCoreHdl*           pstlhBTRCore        = NULL;

    const char*             pDeviceAddress      = NULL;
    stBTRCoreBTDevice*      pstKnownDevice      = NULL;
    stBTRCoreDevStateInfo*  lpstKnownDevStInfo  = NULL;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    char                    isPlayable          = 0;
    
    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    // CID 44531: Unsigned compared against 0 (NO_EFFECT)

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;


    if ((lenBTRCoreRet = btrCore_GetDeviceInfoKnown(pstlhBTRCore, aBTRCoreDevId, aenBTRCoreDevType,
                                                    &lenBTDeviceType, &pstKnownDevice, &lpstKnownDevStInfo, &pDeviceAddress)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to Get Device Information\n");
        return lenBTRCoreRet;
    }

    if (pstKnownDevice->bDeviceConnected == FALSE) {
       BTRCORELOG_ERROR ("Device is not Connected!!!\n");
       return enBTRCoreFailure;
    }

    /* Enhance */

        BTRCORELOG_ERROR ("BTRCore_AVMedia_IsMediaElementPlayable calling !\n");
    if (BTRCore_AVMedia_IsMediaElementPlayable (pstlhBTRCore->avMediaHdl,
                                                pDeviceAddress,
                                                aBtrMediaElementId,
                                                &isPlayable) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to MediaElement's playable state!\n");
        return enBTRCoreFailure;
    }

    if (isPlayable) {

        if ((lenBTRCoreRet = BTRCore_AVMedia_SelectTrack (pstlhBTRCore->avMediaHdl, 
                                    pDeviceAddress, aBtrMediaElementId)) != enBTRCoreSuccess)  {
            BTRCORELOG_ERROR ("AVMedia Set Media Track by Element Id(%llu) Failed!!!\n", aBtrMediaElementId);
            return enBTRCoreFailure;
        }
    }
    else {
             // TODO
    }

#endif
    return lenBTRCoreRet;
}


enBTRCoreRet
BTRCore_GetLEProperty (
    tBTRCoreHandle     hBTRCore,
    tBTRCoreDevId      aBTRCoreDevId,
    const char*        apcBTRCoreLEUuid,
    enBTRCoreLeProp    aenBTRCoreLeProp,
    void*              apvBTRCorePropValue
) {

    if (!hBTRCore || !apcBTRCoreLEUuid) {
       BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    stBTRCoreHdl*         pstlhBTRCore   = (stBTRCoreHdl*)hBTRCore;
    tBTRCoreDevId         ltBTRCoreDevId = 0;
    int                   i32LoopIdx     = 0;

#ifndef LE_MODE
    stBTRCoreBTDevice*  pstScannedDev  = NULL;
    if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES) {
       pstScannedDev  = &pstlhBTRCore->stScannedDevicesArr[aBTRCoreDevId];
    }
    else {
       for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
           if (aBTRCoreDevId == pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
              pstScannedDev  = &pstlhBTRCore->stScannedDevicesArr[i32LoopIdx];
              break;
           }
       }
    }

    if (pstScannedDev) {
        ltBTRCoreDevId = pstScannedDev->tDeviceId;
    }

    if (!pstScannedDev || !ltBTRCoreDevId) {
       BTRCORELOG_ERROR ("Failed to find device in Scanned devices list\n");
       return enBTRCoreDeviceNotFound;
    }

    BTRCORELOG_DEBUG ("Get LE Property for Device : %s\n", pstScannedDev->pcDeviceName);
#else
    stBTRCoreBTDevice*  pstKnownDev  = NULL;
    if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
       pstKnownDev  = &pstlhBTRCore->stKnownDevicesArr[aBTRCoreDevId];
    }
    else {
       for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
           if (aBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
              pstKnownDev  = &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx];
              break;
           }
       }
    }

    if (pstKnownDev) {
        ltBTRCoreDevId = pstKnownDev->tDeviceId;
    }

    if (!pstKnownDev || !ltBTRCoreDevId) {
       BTRCORELOG_ERROR ("Failed to find device in paired devices list\n");
       return enBTRCoreDeviceNotFound;
    }

    BTRCORELOG_DEBUG ("Get LE Property for Device : %s\n", pstKnownDev->pcDeviceName);
#endif
    BTRCORELOG_DEBUG ("LE DeviceID  %llu\n", ltBTRCoreDevId);

    enBTRCoreLEGattProp  lenBTRCoreLEGattProp = enBTRCoreLEGPropUnknown;

    switch (aenBTRCoreLeProp) {

    case enBTRCoreLePropGUUID:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropUUID;
        break;
    case enBTRCoreLePropGPrimary:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropPrimary;
        break;
    case enBTRCoreLePropGDevice:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropDevice;
        break;
    case enBTRCoreLePropGService:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropService;
        break;
    case enBTRCoreLePropGValue:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropValue;
        break;
    case enBTRCoreLePropGNotifying:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropNotifying;
        break;
    case enBTRCoreLePropGFlags:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropFlags;
        break;
    case enBTRCoreLePropGChar:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropChar;
        break;
    case enBTRCoreLePropUnknown:
    default:
        lenBTRCoreLEGattProp = enBTRCoreLEGPropUnknown;
    }

    if (lenBTRCoreLEGattProp == enBTRCoreLEGPropUnknown || BTRCore_LE_GetGattProperty(pstlhBTRCore->leHdl,
                                                                                      ltBTRCoreDevId,
                                                                                      apcBTRCoreLEUuid,
                                                                                      lenBTRCoreLEGattProp,
                                                                                      apvBTRCorePropValue) != enBTRCoreSuccess) {
       BTRCORELOG_ERROR ("Failed to get Gatt Property %d!!!\n", lenBTRCoreLEGattProp);
      return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}

enBTRCoreRet
BTRCore_BatteryWriteOTAControl (
    tBTRCoreHandle    hBTRCore,
    tBTRCoreDevId     aBTRCoreDevId,
    const char*       apBtUuid,
    int value
) {
    
    BTRCORELOG_INFO("In BTRCore_BatteryWriteOTAControl\n");
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    stBTRCoreBTDevice* pstKnownDev  = NULL;
    tBTRCoreDevId       ltBTRCoreDevId = 0;
    int                 i32LoopIdx     = 0;

    if (!hBTRCore || !apBtUuid) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

     pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (pstlhBTRCore->numOfPairedDevices) {
        if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
            pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[aBTRCoreDevId];
        }
        else {
            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                if (aBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                    pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx];
                    break;
                }
            }
        }
    }

    if (pstKnownDev) {
        ltBTRCoreDevId = pstKnownDev->tDeviceId;
    }
    BtrCore_LE_BatteryWriteOTAControl(pstlhBTRCore->leHdl,ltBTRCoreDevId,apBtUuid,value);

    return 0;
}

enBTRCoreRet
BTRCore_BatterySetLED (
    tBTRCoreHandle    hBTRCore,
    tBTRCoreDevId     aBTRCoreDevId,
    const char*       apBtUuid
) {
    BTRCORELOG_INFO("In BTRCore_BatterySetLED\n");
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    stBTRCoreBTDevice* pstKnownDev  = NULL;
    tBTRCoreDevId       ltBTRCoreDevId = 0;
    int                 i32LoopIdx     = 0;

    if (!hBTRCore || !apBtUuid) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

     pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (pstlhBTRCore->numOfPairedDevices) {
        if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
            pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[aBTRCoreDevId];
        }
        else {
            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                if (aBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                    pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx];
                    break;
                }
            }
        }
    }

    if (pstKnownDev) {
        ltBTRCoreDevId = pstKnownDev->tDeviceId;
    }
    
    BtrCore_LE_BatterySetLED(pstlhBTRCore->leHdl,ltBTRCoreDevId,apBtUuid);
    return 0;
}

enBTRCoreRet
BTRCore_BatteryOTADataTransfer (
    tBTRCoreHandle    hBTRCore,
    tBTRCoreDevId     aBTRCoreDevId,
    const char*       apBtUuid,
    char* FileName
) {
  BTRCORELOG_INFO("In BatteryOTADataTransfer\n");
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    stBTRCoreBTDevice* pstKnownDev  = NULL;
    tBTRCoreDevId       ltBTRCoreDevId = 0;
    int                 i32LoopIdx     = 0;

    if (!hBTRCore || !apBtUuid) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

     pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (pstlhBTRCore->numOfPairedDevices) {
        if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
            pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[aBTRCoreDevId];
        }
        else {
            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                if (aBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                    BTRCORELOG_INFO("Device FOund in list \n");
                    pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx];
                    break;
                }
            }
        }
    }

    if (pstKnownDev) {
        ltBTRCoreDevId = pstKnownDev->tDeviceId;
    }

    BtrCore_LE_BatteryOTATransfer(pstlhBTRCore->leHdl,ltBTRCoreDevId,apBtUuid,FileName);
    return 0;
}


enBTRCoreRet 
BTRCore_PerformLEOp (
    tBTRCoreHandle    hBTRCore,
    tBTRCoreDevId     aBTRCoreDevId,
    const char*       apBtUuid,
    enBTRCoreLeOp     aenBTRCoreLeOp,
    char*             apLeOpArg,
    char*             rpLeOpRes
) {

    if (!hBTRCore || !apBtUuid) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    stBTRCoreHdl*       pstlhBTRCore   = (stBTRCoreHdl*)hBTRCore;
    tBTRCoreDevId       ltBTRCoreDevId = 0;
    int                 i32LoopIdx     = 0;

#ifndef LE_MODE
    stBTRCoreBTDevice*    pstScannedDev  = NULL;
    if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES) {
       pstScannedDev  = &pstlhBTRCore->stScannedDevicesArr[aBTRCoreDevId];
    }
    else {
       for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfScannedDevices; i32LoopIdx++) {
           if (aBTRCoreDevId == pstlhBTRCore->stScannedDevicesArr[i32LoopIdx].tDeviceId) {
              pstScannedDev  = &pstlhBTRCore->stScannedDevicesArr[i32LoopIdx];
              break;
           }
       }
    }

    if (pstScannedDev) {
        ltBTRCoreDevId = pstScannedDev->tDeviceId;
    }

    if (!pstScannedDev || !ltBTRCoreDevId) {
        BTRCORELOG_ERROR ("Failed to find device in Scanned devices list\n");
        return enBTRCoreDeviceNotFound;
    }

    if (!pstScannedDev->bFound || !pstScannedDev->bDeviceConnected) {
        BTRCORELOG_ERROR ("Le Device is not connected, Please connect and perform LE method operation\n");
        return enBTRCoreDeviceNotFound;
    }

    BTRCORELOG_DEBUG ("Perform LE Op for Device : %s\n", pstScannedDev->pcDeviceName);
#else
    stBTRCoreBTDevice* pstKnownDev  = NULL;

    if (pstlhBTRCore->numOfPairedDevices) {
        if (aBTRCoreDevId < BTRCORE_MAX_NUM_BT_DEVICES) {
            pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[aBTRCoreDevId];
        }
        else {
            for (i32LoopIdx = 0; i32LoopIdx < pstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                if (aBTRCoreDevId == pstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId) {
                    pstKnownDev = &pstlhBTRCore->stKnownDevicesArr[i32LoopIdx];
                    break;
                }
            }
        }
    }

    if (pstKnownDev) {
        ltBTRCoreDevId = pstKnownDev->tDeviceId;
    }

    if (!pstKnownDev || !ltBTRCoreDevId) {
        BTRCORELOG_ERROR ("Failed to find device in Paired devices list\n");
        return enBTRCoreDeviceNotFound;
    }

    if (!pstKnownDev->bDeviceConnected) {
        BTRCORELOG_ERROR ("Le Device is not connected, Please connect and perform LE method operation\n");
        return enBTRCoreDeviceNotFound;
    }

    BTRCORELOG_DEBUG ("Perform LE Op for Device : %s\n", pstKnownDev->pcDeviceName);
#endif
    BTRCORELOG_DEBUG ("LE DeviceID  %llu\n", ltBTRCoreDevId);

    enBTRCoreLEGattOp  lenBTRCoreLEGattOp = enBTRCoreLEGOpUnknown;

    switch (aenBTRCoreLeOp) {
   
    case enBTRCoreLeOpGReadValue:
         lenBTRCoreLEGattOp = enBTRCoreLEGOpReadValue;
         break;
    case enBTRCoreLeOpGWriteValue:
         lenBTRCoreLEGattOp =  enBTRCoreLEGOpWriteValue;
         break;
    case enBTRCoreLeOpGStartNotify:
         lenBTRCoreLEGattOp =  enBTRCoreLEGOpStartNotify; 
         break;
    case enBTRCoreLeOpGStopNotify:
         lenBTRCoreLEGattOp =  enBTRCoreLEGOpStopNotify;
         break;
    case enBTRCoreLeOpUnknown:
    default : 
         lenBTRCoreLEGattOp = enBTRCoreLEGOpUnknown;
    }

    if (lenBTRCoreLEGattOp == enBTRCoreLEGOpUnknown || BtrCore_LE_PerformGattOp(pstlhBTRCore->leHdl,
                                                                                ltBTRCoreDevId,
                                                                                apBtUuid,
                                                                                lenBTRCoreLEGattOp,
                                                                                apLeOpArg,
                                                                                rpLeOpRes) != enBTRCoreSuccess) {
       BTRCORELOG_ERROR ("Failed to Perform LE Method Op %d!!!\n", aenBTRCoreLeOp);
       return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}

enBTRCoreRet
BTRCore_StartAdvertisement (
    tBTRCoreHandle  hBTRCore
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreSuccess;

    if (NULL == hBTRCore) {
        BTRCORELOG_ERROR("enBTRCoreNotInitialized\n");
        lenBTRCoreRet = enBTRCoreNotInitialized;
    }
    else {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        lenBTRCoreRet = BTRCore_LE_StartAdvertisement(pstlhBTRCore->leHdl, pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath);
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_StopAdvertisement (
    tBTRCoreHandle  hBTRCore
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreSuccess;

    if (NULL == hBTRCore) {
        BTRCORELOG_ERROR("enBTRCoreNotInitialized\n");
        lenBTRCoreRet = enBTRCoreNotInitialized;
    }
    else {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        lenBTRCoreRet = BTRCore_LE_StopAdvertisement(pstlhBTRCore->leHdl, pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath);
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_ReleaseAdvertisement (
    tBTRCoreHandle  hBTRCore
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreSuccess;

    if (NULL == hBTRCore) {
        BTRCORELOG_ERROR("enBTRCoreNotInitialized\n");
        lenBTRCoreRet = enBTRCoreNotInitialized;
    }
    else {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        lenBTRCoreRet = BTRCore_LE_ReleaseAdvertisement(pstlhBTRCore->leHdl, pstlhBTRCore->connHdl, pstlhBTRCore->curAdapterPath);
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_GetPropertyValue (
    tBTRCoreHandle  hBTRCore,
    char*           aUUID,
    char*           aValue,
    enBTRCoreLeProp aElement
) {
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;
    //enBTRCoreLEGattProp lGattProp = enBTRCoreLEGPropValue;

    if ((NULL == hBTRCore) || (NULL == aUUID) || (NULL == aValue)) {
        BTRCORELOG_ERROR("enBTRCoreNotInitialized\n");
        lenBTRCoreRet = enBTRCoreNotInitialized;
    }
    else {
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_SetAdvertisementInfo (
    tBTRCoreHandle hBTRCore,
    char*          aAdvtType,
    char*          aAdvtBeaconName
) {
    stBTRCoreHdl* pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;

    if ((NULL != hBTRCore) && ( NULL != aAdvtType) && ( NULL != aAdvtBeaconName)) {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        lenBTRCoreRet = BTRCore_LE_SetAdvertisementInfo(pstlhBTRCore->leHdl, aAdvtType, aAdvtBeaconName);
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_SetServiceUUIDs (
    tBTRCoreHandle hBTRCore,
    char*          aUUID
) {
    stBTRCoreHdl* pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;

    if ((NULL != hBTRCore) && (NULL != aUUID)) {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        lenBTRCoreRet = BTRCore_LE_SetServiceUUIDs(pstlhBTRCore->leHdl, aUUID);
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_SetManufacturerData (
    tBTRCoreHandle hBTRCore,
    unsigned short aManfId,
    unsigned char* aDeviceDetails,
    int            aLenManfData
) {
    stBTRCoreHdl* pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;

    if ((NULL != hBTRCore) && (NULL != aDeviceDetails)) {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

        lenBTRCoreRet = BTRCore_LE_SetManufacturerData(pstlhBTRCore->leHdl, aManfId, aDeviceDetails, aLenManfData);
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_SetEnableTxPower (
    tBTRCoreHandle  hBTRCore,
    BOOLEAN         aTxPower
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;

    if (NULL != hBTRCore) {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        lenBTRCoreRet = BTRCore_LE_SetEnableTxPower(pstlhBTRCore->leHdl, aTxPower);
    }

    return lenBTRCoreRet;
}

enBTRCoreRet 
BTRCore_SetServiceInfo (
    tBTRCoreHandle  hBTRCore,
    char*           aUUID,
    BOOLEAN         aServiceType
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;
    int             lNumGattServices = 0;

    if ((NULL != hBTRCore) && (NULL != aUUID)) {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        if (NULL != BTRCore_LE_AddGattServiceInfo(pstlhBTRCore->leHdl, pstlhBTRCore->curAdapterPath, pstlhBTRCore->curAdapterAddr, aUUID, aServiceType, &lNumGattServices)) {
            //*aNumGattServices = lNumGattServices;
            lenBTRCoreRet = enBTRCoreSuccess;
        }
    }
    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_RemoveServiceInfo(
    tBTRCoreHandle  hBTRCore,
    char*           aUUID
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;
    int             lNumGattServices = 0;

    if ((NULL != hBTRCore) && (NULL != aUUID)) {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;
        if (NULL != BTRCore_LE_RemoveGattServiceInfo(pstlhBTRCore->leHdl, pstlhBTRCore->curAdapterPath, aUUID, &lNumGattServices)) {
            lenBTRCoreRet = enBTRCoreSuccess;
        }
    }
    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_SetGattInfo (
    tBTRCoreHandle  hBTRCore,
    char*           aParentUUID,
    char*           aUUID,
    unsigned short  aFlags,
    char*           aValue,
    enBTRCoreLeProp aElement
) {
    enBTRCoreRet    lenBTRCoreRet = enBTRCoreFailure;
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if ((NULL != hBTRCore) && (NULL != aParentUUID) && (NULL != aUUID) && (NULL != aValue)) {
        pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

        if (enBTRCoreLePropGChar == aElement) {
            if (NULL != BTRCore_LE_AddGattCharInfo(pstlhBTRCore->leHdl, pstlhBTRCore->curAdapterPath, pstlhBTRCore->curAdapterAddr, aParentUUID, aUUID, aFlags, aValue)) {
                lenBTRCoreRet = enBTRCoreSuccess;
            }
        }
        else {
            if (NULL != BTRCore_LE_AddGattDescInfo(pstlhBTRCore->leHdl, pstlhBTRCore->curAdapterPath, pstlhBTRCore->curAdapterAddr, aParentUUID, aUUID, aFlags, aValue)) {
                lenBTRCoreRet = enBTRCoreSuccess;
            }
        }
    }

    return lenBTRCoreRet;
}

enBTRCoreRet
BTRCore_SetPropertyValue (
    tBTRCoreHandle  hBTRCore,
    char*           aUUID,
    char*           aValue,
    enBTRCoreLeProp aElement
) {
    stBTRCoreHdl*       pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }
    else if (!aValue) {
        BTRCORELOG_ERROR("enBTRCoreInvalidArg\n");
        return enBTRCoreInvalidArg;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!BTRCore_LE_SetPropertyValue(pstlhBTRCore->leHdl, aUUID, aValue, aElement)) {
        BTRCORELOG_ERROR("Set Gatt Property value - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}

enBTRCoreRet
BTRCore_GetBluetoothVersion (
    char* version
) {
    if (version == NULL) {
        BTRCORELOG_ERROR("Invalid memory\n");
        return enBTRCoreFailure;
    }

    if (BtrCore_BTGetBluetoothVersion(version) !=0 ) {
        BTRCORELOG_ERROR("Get Bluetooth version - FAILED\n");
        return enBTRCoreFailure;
    }

    return enBTRCoreSuccess;
}

BOOLEAN BTRCore_IsUnsupportedGamepad (
        unsigned int ui32Vendor,
        unsigned int ui32Product,
        unsigned int ui32DeviceId) {
    BOOLEAN isUnsupported = FALSE;

    BTRCORELOG_DEBUG("Vendor:%04X product:%04X DeviceId:%04X\n",ui32Vendor, ui32Product, ui32DeviceId);
 /* If Bluetooth version is 5.2, 5.3 or above, || privacy is enabled, we need to add the disable_unsupported_gamepad DISTRO feature in the builds.
 * We have enabled BT_UNSUPPORTED_GAMEPAD_ENABLED macro based on this distro feature */
#ifdef BT_UNSUPPORTED_GAMEPAD_ENABLED
    if (BTRCORE_XBOX_VENDOR_ID == ui32Vendor && BTRCORE_XBOX_GEN4_PRODUCT_ID == ui32Product &&
       ((BTRCORE_XBOX_GEN4_DEF_FIRMWARE == ui32DeviceId) ||
	(BTRCORE_XBOX_GEN4_DEF_FIRMWARE_501 == ui32DeviceId))) {
        isUnsupported = TRUE;
    }
#endif
    return isUnsupported;
}

// Outgoing callbacks Registration Interfaces
enBTRCoreRet
BTRCore_RegisterDiscoveryCb (
    tBTRCoreHandle              hBTRCore,
    fPtr_BTRCore_DeviceDiscCb   afpcBBTRCoreDeviceDisc,
    void*                       apUserData
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!pstlhBTRCore->fpcBBTRCoreDeviceDisc) {
        pstlhBTRCore->fpcBBTRCoreDeviceDisc = afpcBBTRCoreDeviceDisc;
        pstlhBTRCore->pvcBDevDiscUserData   = apUserData;
        BTRCORELOG_INFO ("Device Discovery Callback Registered Successfully\n");
    }
    else {
        BTRCORELOG_INFO ("Device Discovery Callback Already Registered - Not Registering current CB\n");
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_RegisterStatusCb (
    tBTRCoreHandle          hBTRCore,
    fPtr_BTRCore_StatusCb   afpcBBTRCoreStatus,
    void*                   apUserData
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!pstlhBTRCore->fpcBBTRCoreStatus) {
        pstlhBTRCore->fpcBBTRCoreStatus = afpcBBTRCoreStatus;
        pstlhBTRCore->pvcBStatusUserData= apUserData; 
        BTRCORELOG_INFO ("BT Status Callback Registered Successfully\n");
    }
    else {
        BTRCORELOG_INFO ("BT Status Callback Already Registered - Not Registering current CB\n");
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_RegisterMediaStatusCb (
    tBTRCoreHandle              hBTRCore,
    fPtr_BTRCore_MediaStatusCb  afpcBBTRCoreMediaStatus,
    void*                       apUserData
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
       BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
       return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    if (!pstlhBTRCore->fpcBBTRCoreMediaStatus) {
        pstlhBTRCore->fpcBBTRCoreMediaStatus = afpcBBTRCoreMediaStatus;
        pstlhBTRCore->pvcBMediaStatusUserData= apUserData;
        BTRCORELOG_INFO ("BT Media Status Callback Registered Successfully\n");
    }
    else {
       BTRCORELOG_INFO ("BT Media Status Callback Already Registered - Not Registering current CB\n");
    }

    return enBTRCoreSuccess;
}

       
enBTRCoreRet
BTRCore_RegisterConnectionIntimationCb (
    tBTRCoreHandle           hBTRCore,
    fPtr_BTRCore_ConnIntimCb afpcBBTRCoreConnIntim,
    void*                    apUserData
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    (void)pstlhBTRCore;

    if (!pstlhBTRCore->fpcBBTRCoreConnIntim) {
        pstlhBTRCore->fpcBBTRCoreConnIntim = afpcBBTRCoreConnIntim;
        pstlhBTRCore->pvcBConnIntimUserData= apUserData;
        BTRCORELOG_INFO ("BT Conn In Intimation Callback Registered Successfully\n");
    }
    else {
        BTRCORELOG_INFO ("BT Conn In Intimation Callback Already Registered - Not Registering current CB\n");
    }

    return enBTRCoreSuccess;
}


enBTRCoreRet
BTRCore_RegisterConnectionAuthenticationCb (
    tBTRCoreHandle          hBTRCore,
    fPtr_BTRCore_ConnAuthCb afpcBBTRCoreConnAuth,
    void*                   apUserData
) {
    stBTRCoreHdl*   pstlhBTRCore = NULL;

    if (!hBTRCore) {
        BTRCORELOG_ERROR ("enBTRCoreNotInitialized\n");
        return enBTRCoreNotInitialized;
    }

    pstlhBTRCore = (stBTRCoreHdl*)hBTRCore;

    (void)pstlhBTRCore;

    if (!pstlhBTRCore->fpcBBTRCoreConnAuth) {
        pstlhBTRCore->fpcBBTRCoreConnAuth = afpcBBTRCoreConnAuth;
        pstlhBTRCore->pvcBConnAuthUserData= apUserData;
        BTRCORELOG_INFO ("BT Conn Auth Callback Registered Successfully\n");
    }
    else {
        BTRCORELOG_INFO ("BT Conn Auth Callback Already Registered - Not Registering current CB\n");
    }

    return enBTRCoreSuccess;
}

/*  Incoming Callbacks */
STATIC  int
btrCore_BTAdapterStatusUpdateCb (
    enBTAdapterProp  aeBtAdapterProp,
    stBTAdapterInfo* apstBTAdapterInfo,
    void*            apUserData
) {
    stBTRCoreHdl*   lpstlhBTRCore = (stBTRCoreHdl*)apUserData;
    enBTRCoreRet    lenBTRCoreRet  = enBTRCoreSuccess;
    stBTRCoreAdapter lstAdapterInfo;
    int pathlen;

    if (!apstBTAdapterInfo || !apUserData) {
       BTRCORELOG_ERROR ("enBTRCoreInvalidArg!!!");
       return -1;
    }

    if ((apstBTAdapterInfo->pcPath[0] == '\0') ||
        !(pathlen = strlen (apstBTAdapterInfo->pcPath)) ||
        strcmp(apstBTAdapterInfo->pcPath, lpstlhBTRCore->curAdapterPath)) {
        BTRCORELOG_INFO ("Dropping event for non-current adapter path %s\n", apstBTAdapterInfo->pcPath ? apstBTAdapterInfo->pcPath : "<null>");
        return -1;
    }

    memset(&lstAdapterInfo, 0, sizeof(stBTRCoreAdapter));
    lstAdapterInfo.adapter_number = atoi(apstBTAdapterInfo->pcPath + pathlen-1);

    BTRCORELOG_INFO ("adapter number = %d, path = %s, discovering = %d\n",
            lstAdapterInfo.adapter_number, apstBTAdapterInfo->pcPath, apstBTAdapterInfo->bDiscovering);

    switch (aeBtAdapterProp) {
    case enBTAdPropAdvTimeout:
        // tiemout release cb to up layer
        lstAdapterInfo.bAdvertisingTimedout = TRUE;
        lstAdapterInfo.discoverable = apstBTAdapterInfo->bDiscoverable;
        lstAdapterInfo.bDiscovering = apstBTAdapterInfo->bDiscovering;

        if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask,
                enBTRCoreTaskOpProcess,
                enBTRCoreTaskPTcBAdapterStatus,
                &lstAdapterInfo)) != enBTRCoreSuccess) {
            BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBAdapterStatus %d\n", lenBTRCoreRet);
        }
        break;
    case enBTAdPropDiscoveryStatus: {
        lstAdapterInfo.discoverable = apstBTAdapterInfo->bDiscoverable;
        lstAdapterInfo.bDiscovering = apstBTAdapterInfo->bDiscovering;

        if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask,
                enBTRCoreTaskOpProcess,
                enBTRCoreTaskPTcBAdapterStatus,
                &lstAdapterInfo)) != enBTRCoreSuccess) {
            BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBAdapterStatus %d\n", lenBTRCoreRet);
        }
        break;
    }

    case enBTAdPropUnknown: {
        break;
    }
    default:
        break;
    }

    return 0;
}


STATIC  int
btrCore_BTDeviceStatusUpdateCb (
    enBTDeviceType  aeBtDeviceType,
    enBTDeviceState aeBtDeviceState,
    stBTDeviceInfo* apstBTDeviceInfo,
    void*           apUserData
) {
    enBTRCoreRet         lenBTRCoreRet      = enBTRCoreFailure;
    enBTRCoreDeviceType  lenBTRCoreDevType  = enBTRCoreUnknown;

    BTRCORELOG_DEBUG ("enBTDeviceType = %d enBTDeviceState = %d apstBTDeviceInfo = %p\n", aeBtDeviceType, aeBtDeviceState, apstBTDeviceInfo);

    if (apstBTDeviceInfo) {
        lenBTRCoreDevType = btrCore_MapClassIDToDevType(apstBTDeviceInfo->ui32Class, aeBtDeviceType);
    }

    switch (aeBtDeviceState) {
    case enBTDevStCreated: {
        break;
    }
    case enBTDevStScanInProgress: {
        break;
    }
    case enBTDevStClassAppUpdate:
    //fall through if device changes class or appearance during discovery we want to treat it as a new device
    case enBTDevStFound: {
        stBTRCoreHdl*   lpstlhBTRCore = (stBTRCoreHdl*)apUserData;

        if (lpstlhBTRCore && apstBTDeviceInfo) {
            int j = 0;
            tBTRCoreDevId   lBTRCoreDevId = 0;

            BTRCORELOG_TRACE ("bPaired          = %d\n", apstBTDeviceInfo->bPaired);
            BTRCORELOG_TRACE ("bConnected       = %d\n", apstBTDeviceInfo->bConnected);
            BTRCORELOG_TRACE ("bTrusted         = %d\n", apstBTDeviceInfo->bTrusted);
            BTRCORELOG_TRACE ("bBlocked         = %d\n", apstBTDeviceInfo->bBlocked);
            BTRCORELOG_TRACE ("ui16Vendor       = %d\n", apstBTDeviceInfo->ui16Vendor);
            BTRCORELOG_TRACE ("ui16VendorSource = %d\n", apstBTDeviceInfo->ui16VendorSource);
            BTRCORELOG_TRACE ("ui16Product      = %d\n", apstBTDeviceInfo->ui16Product);
            BTRCORELOG_TRACE ("ui16Version      = %d\n", apstBTDeviceInfo->ui16Version);
            BTRCORELOG_TRACE ("ui32Class        = %d\n", apstBTDeviceInfo->ui32Class);
            BTRCORELOG_TRACE ("i32RSSI          = %d\n", apstBTDeviceInfo->i32RSSI);
            BTRCORELOG_TRACE ("pcName           = %s\n", apstBTDeviceInfo->pcName);
            BTRCORELOG_TRACE ("pcAddress        = %s\n", apstBTDeviceInfo->pcAddress);
            BTRCORELOG_TRACE ("pcAlias          = %s\n", apstBTDeviceInfo->pcAlias);
            BTRCORELOG_TRACE ("pcIcon           = %s\n", apstBTDeviceInfo->pcIcon);
            BTRCORELOG_TRACE ("pcDevicePath     = %s\n", apstBTDeviceInfo->pcDevicePath);

            for (j = 0; j < BT_MAX_DEVICE_PROFILE; j++) {
                if (apstBTDeviceInfo->aUUIDs[j][0] == '\0')
                    break;
                else
                    BTRCORELOG_TRACE ("aUUIDs = %s\n", apstBTDeviceInfo->aUUIDs[j]);
            }

            lBTRCoreDevId = btrCore_GenerateUniqueDeviceID(apstBTDeviceInfo->pcAddress);

            if (btrCore_GetScannedDeviceAddress(lpstlhBTRCore, lBTRCoreDevId) && (aeBtDeviceState != enBTDevStClassAppUpdate)) {
                BTRCORELOG_INFO ("Already we have a entry in the list; Skip Parsing now \n");
            }
            else {
                stBTRCoreOTskInData lstOTskInData;

                lstOTskInData.bTRCoreDevId      = lBTRCoreDevId;
                lstOTskInData.enBTRCoreDevType  = lenBTRCoreDevType;
                lstOTskInData.pstBTDevInfo      = apstBTDeviceInfo;

                stBTRCoreBTDevice   FoundDevice;

                MEMSET_S(&FoundDevice, sizeof(stBTRCoreBTDevice), 0, sizeof(stBTRCoreBTDevice));
                strncpy(FoundDevice.pcDeviceName,    apstBTDeviceInfo->pcName,       BD_NAME_LEN);
                strncpy(FoundDevice.pcDeviceAddress, apstBTDeviceInfo->pcAddress,    BD_NAME_LEN);

                if(btrCore_IsDevNameSameAsAddress(&FoundDevice)) {
                    if ((lenBTRCoreDevType == enBTRCoreSpeakers) || (lenBTRCoreDevType == enBTRCoreHeadSet) || (enBTRCoreHID == lenBTRCoreDevType)) {
                        BTRCORELOG_INFO("pcName - %s pcAddress - %s DeviceType - %d skipCount - %lld\n",apstBTDeviceInfo->pcName,apstBTDeviceInfo->pcAddress,lenBTRCoreDevType,lpstlhBTRCore->skipDeviceDiscUpdate);

                        // NOTE: This increments across devices (not on a per device basis), if we have 5 devices which broadcast name as MAC address
                        // then the 5th RSSI update across devices only will be processed. This is temporary fix. need to change the code for each device.
                        lpstlhBTRCore->skipDeviceDiscUpdate++;
                        if (lpstlhBTRCore->skipDeviceDiscUpdate % 5 == 0) {

                            if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBDeviceDisc,  &lstOTskInData)) != enBTRCoreSuccess) {
                                BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceDisc %d\n", lenBTRCoreRet);
                            }
                        }
                        else {
                            BTRCORELOG_INFO("Skipping the update ...\n");
                        }
                    }
                }
                else {
                    if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBDeviceDisc,  &lstOTskInData)) != enBTRCoreSuccess) {
                        BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceDisc %d\n", lenBTRCoreRet);
                    }
                }
            }
        }

        break;
    }
    case enBTDevStLost: {
        stBTRCoreHdl*   lpstlhBTRCore = (stBTRCoreHdl*)apUserData;

        if (lpstlhBTRCore && apstBTDeviceInfo) {
            tBTRCoreDevId   lBTRCoreDevId     = btrCore_GenerateUniqueDeviceID(apstBTDeviceInfo->pcAddress);

            if (btrCore_GetKnownDeviceMac(lpstlhBTRCore, lBTRCoreDevId)) {
                stBTRCoreOTskInData lstOTskInData;

                lstOTskInData.bTRCoreDevId      = lBTRCoreDevId;
                lstOTskInData.enBTRCoreDevType  = lenBTRCoreDevType;
                lstOTskInData.pstBTDevInfo      = apstBTDeviceInfo;

                if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBDeviceLost, &lstOTskInData)) != enBTRCoreSuccess) {
                    BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceLost%d\n", lenBTRCoreRet);
                }
            }
            else if (btrCore_GetScannedDeviceAddress(lpstlhBTRCore, lBTRCoreDevId)) {
                stBTRCoreOTskInData lstOTskInData;

                lstOTskInData.bTRCoreDevId      = lBTRCoreDevId;
                lstOTskInData.enBTRCoreDevType  = lenBTRCoreDevType;
                lstOTskInData.pstBTDevInfo      = apstBTDeviceInfo;

                if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBDeviceRemoved, &lstOTskInData)) != enBTRCoreSuccess) {
                    BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceRemoved %d\n", lenBTRCoreRet);
                }
            }
            else {
                BTRCORELOG_INFO ("We dont have a entry in the list; Skip Parsing now \n");
            }
        }
        break;
    }
    case enBTDevStPairingRequest: {
        break;
    }
    case enBTDevStPairingInProgress: {
        break;
    }
    case enBTDevStPaired: {
        break;
    }
    case enBTDevStUnPaired: {
        break;
    }
    case enBTDevStConnectInProgress: {
        break;
    }
    case enBTDevStConnected: {
        break;
    }
    case enBTDevStDisconnected: {
        break;
    }
    case enBTDevStPropChanged: {
        stBTRCoreHdl*   lpstlhBTRCore = (stBTRCoreHdl*)apUserData;

        if (lpstlhBTRCore && apstBTDeviceInfo) {
            tBTRCoreDevId   lBTRCoreDevId     = btrCore_GenerateUniqueDeviceID(apstBTDeviceInfo->pcAddress);

            if (btrCore_GetKnownDeviceMac(lpstlhBTRCore, lBTRCoreDevId)) {
                stBTRCoreOTskInData lstOTskInData;

                lstOTskInData.bTRCoreDevId      = lBTRCoreDevId;
                lstOTskInData.enBTRCoreDevType  = lenBTRCoreDevType;
                lstOTskInData.pstBTDevInfo      = apstBTDeviceInfo;

                if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBDeviceStatus, &lstOTskInData)) != enBTRCoreSuccess) {
                    BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceStatus %d\n", lenBTRCoreRet);
                }
            }
            else if (btrCore_GetScannedDeviceAddress(lpstlhBTRCore, lBTRCoreDevId)) {
                stBTRCoreOTskInData lstOTskInData; 
                
                lstOTskInData.bTRCoreDevId      = lBTRCoreDevId;
                lstOTskInData.enBTRCoreDevType  = lenBTRCoreDevType;
                lstOTskInData.pstBTDevInfo      = apstBTDeviceInfo;

                if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBDeviceStatus, &lstOTskInData)) != enBTRCoreSuccess) {
                    BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceStatus %d\n", lenBTRCoreRet);
                }
            }
        }

        break;
    }
    case enBTDevStRSSIUpdate: {
        BTRCORELOG_INFO ("Received RSSI Update...\n");
        break;
    }
    case enBTDevStModaliasChanged: {
        stBTRCoreHdl*   lpstlhBTRCore = (stBTRCoreHdl*)apUserData;

        BTRCORELOG_INFO ("Modalias update...\n");
        if (lpstlhBTRCore && apstBTDeviceInfo) {
            tBTRCoreDevId   lBTRCoreDevId = 0;

            lBTRCoreDevId = btrCore_GenerateUniqueDeviceID(apstBTDeviceInfo->pcAddress);

            if (btrCore_GetScannedDeviceAddress(lpstlhBTRCore, lBTRCoreDevId)) {
                BTRCORELOG_INFO ("Modalias update: btrCore_GetScannedDeviceAddress \n");
                stBTRCoreOTskInData lstOTskInData;

                lstOTskInData.bTRCoreDevId      = lBTRCoreDevId;
                lstOTskInData.enBTRCoreDevType  = lenBTRCoreDevType;
                lstOTskInData.pstBTDevInfo      = apstBTDeviceInfo;

                if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBModaliasUpdate, &lstOTskInData)) != enBTRCoreSuccess) {
                    BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceStatus %d\n", lenBTRCoreRet);
                }
            }
        }
        break;
    }
    case enBTDevStUnknown: {
        break;
    }
    default: {
        break;
    }
    }

    return 0;
}


STATIC  int
btrCore_BTDeviceConnectionIntimationCb (
    enBTDeviceType  aeBtDeviceType,
    stBTDeviceInfo* apstBTDeviceInfo,
    unsigned int    aui32devPassKey,
    unsigned char   ucIsReqConfirmation,
    void*           apUserData
) {
    int                  i32DevConnIntimRet = 0;
    stBTRCoreHdl*        lpstlhBTRCore      = (stBTRCoreHdl*)apUserData;
    enBTRCoreDeviceType  lenBTRCoreDevType  = enBTRCoreUnknown;

    lenBTRCoreDevType = btrCore_MapClassIDToDevType(apstBTDeviceInfo->ui32Class, aeBtDeviceType);


    if (lpstlhBTRCore) {
        stBTRCoreBTDevice   lstFoundDevice;
        int                 i32ScannedDevIdx = -1;

        if ((i32ScannedDevIdx = btrCore_AddDeviceToScannedDevicesArr(lpstlhBTRCore, apstBTDeviceInfo, &lstFoundDevice)) != -1) {
           BTRCORELOG_DEBUG ("btrCore_AddDeviceToScannedDevicesArr - Success Index = %d\n", i32ScannedDevIdx);
        }

        BTRCORELOG_DEBUG("btrCore_BTDeviceConnectionIntimationCb\n");
        lpstlhBTRCore->stConnCbInfo.ui32devPassKey = aui32devPassKey;
        lpstlhBTRCore->stConnCbInfo.ucIsReqConfirmation = ucIsReqConfirmation;

        if (apstBTDeviceInfo->pcName[0] != '\0')
            strncpy(lpstlhBTRCore->stConnCbInfo.cConnAuthDeviceName, apstBTDeviceInfo->pcName, (strlen(apstBTDeviceInfo->pcName) < (BTRCORE_STR_LEN - 1)) ? strlen(apstBTDeviceInfo->pcName) : BTRCORE_STR_LEN - 1);

        MEMCPY_S(&lpstlhBTRCore->stConnCbInfo.stFoundDevice,sizeof(lpstlhBTRCore->stConnCbInfo.stFoundDevice), &lstFoundDevice, sizeof(stBTRCoreBTDevice));
        lpstlhBTRCore->stConnCbInfo.stFoundDevice.bFound = TRUE;

        if ((lenBTRCoreDevType == enBTRCoreMobileAudioIn) || (lenBTRCoreDevType == enBTRCorePCAudioIn) || (lenBTRCoreDevType == enBTRCoreHID)) {
            if (lpstlhBTRCore->fpcBBTRCoreConnIntim) {
                if (lpstlhBTRCore->fpcBBTRCoreConnIntim(&lpstlhBTRCore->stConnCbInfo, &i32DevConnIntimRet, lpstlhBTRCore->pvcBConnIntimUserData) != enBTRCoreSuccess) {
                    //TODO: Triggering Outgoing callbacks from Incoming callbacks..aaaaaaaahhhh not a good idea
                }
            }
        }
        else if ((lenBTRCoreDevType == enBTRCoreSpeakers) || (lenBTRCoreDevType == enBTRCoreHeadSet)) {
            if (lpstlhBTRCore->numOfPairedDevices) {
                unsigned int i32LoopIdx = 0;

                //TODO: Even before we loop, check if we are already connected and playing Audio-Out 
                for (i32LoopIdx = 0; i32LoopIdx < lpstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                    if (lpstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId == lpstlhBTRCore->stConnCbInfo.stFoundDevice.tDeviceId) {
                        BTRCORELOG_INFO("ACCEPT INCOMING INTIMATION stKnownDevice : %s", lpstlhBTRCore->stKnownDevicesArr[i32LoopIdx].pcDeviceName);
                        i32DevConnIntimRet = 1;
                    }
                }
            }
        }   
#ifdef LE_MODE
        else if ((lenBTRCoreDevType == enBTRCoreMobileAudioIn) || (lenBTRCoreDevType == enBTRCorePCAudioIn) || (lenBTRCoreDevType == enBTRCoreUnknown)) {
            if (lpstlhBTRCore->fpcBBTRCoreConnIntim) {
                if ((lpstlhBTRCore->fpcBBTRCoreConnIntim(&lpstlhBTRCore->stConnCbInfo, &i32DevConnIntimRet, lpstlhBTRCore->pvcBConnIntimUserData) == enBTRCoreSuccess) && (i32DevConnIntimRet == 1)) {
                    //TODO: Triggering Outgoing callbacks from Incoming callbacks..aaaaaaaahhhh not a good idea
                    int i32KnownDevIdx   = -1;
                    if ((i32KnownDevIdx = btrCore_AddDeviceToKnownDevicesArr(lpstlhBTRCore, apstBTDeviceInfo)) != -1) {
                        MEMCPY_S(&lpstlhBTRCore->stConnCbInfo.stKnownDevice,sizeof(lpstlhBTRCore->stConnCbInfo.stKnownDevice), &lpstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx], sizeof(stBTRCoreBTDevice));
                        BTRCORELOG_INFO ("btrCore_AddDeviceToKnownDevicesArr - Success Index = %d Unique DevID = %lld\n", i32KnownDevIdx, lpstlhBTRCore->stConnCbInfo.stKnownDevice.tDeviceId);
                    }
                }
            }
        }
#endif
        else {
            i32DevConnIntimRet = 1; // SM - hack to allow incoming pairing request for LE devices/characteristics with secure write/read
        }
    }

    return i32DevConnIntimRet;
}

STATIC  int
btrCore_BTDeviceAuthenticationCb (
    enBTDeviceType  aeBtDeviceType,
    stBTDeviceInfo* apstBTDeviceInfo,
    void*           apUserData
) {
    int                  i32DevAuthRet      = 0;
    stBTRCoreHdl*        lpstlhBTRCore      = (stBTRCoreHdl*)apUserData;
    enBTRCoreDeviceType  lenBTRCoreDevType  = enBTRCoreUnknown;

    lenBTRCoreDevType = btrCore_MapClassIDToDevType(apstBTDeviceInfo->ui32Class, aeBtDeviceType);


    if (lpstlhBTRCore) {
        stBTRCoreBTDevice   lstFoundDevice;
        int                 i32ScannedDevIdx = -1;
        int                 i32KnownDevIdx   = -1;

        if ((i32ScannedDevIdx = btrCore_AddDeviceToScannedDevicesArr(lpstlhBTRCore, apstBTDeviceInfo, &lstFoundDevice)) != -1) {
           BTRCORELOG_INFO ("btrCore_AddDeviceToScannedDevicesArr - Success Index = %d\n", i32ScannedDevIdx);
        }

        BTRCORELOG_DEBUG("btrCore_BTDeviceAuthenticationCb\n");
        if (apstBTDeviceInfo->pcName[0] != '\0')
            strncpy(lpstlhBTRCore->stConnCbInfo.cConnAuthDeviceName, apstBTDeviceInfo->pcName, (strlen(apstBTDeviceInfo->pcName) < (BTRCORE_STR_LEN - 1)) ? strlen(apstBTDeviceInfo->pcName) : BTRCORE_STR_LEN - 1);


        if ((i32KnownDevIdx = btrCore_AddDeviceToKnownDevicesArr(lpstlhBTRCore, apstBTDeviceInfo)) != -1) {
            MEMCPY_S(&lpstlhBTRCore->stConnCbInfo.stKnownDevice,sizeof(lpstlhBTRCore->stConnCbInfo.stKnownDevice), &lpstlhBTRCore->stKnownDevicesArr[i32KnownDevIdx], sizeof(stBTRCoreBTDevice));
            lpstlhBTRCore->stConnCbInfo.eDevicePrevState = lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDevicePrevState;
            lpstlhBTRCore->stConnCbInfo.eDeviceCurrState = lpstlhBTRCore->stKnownDevStInfoArr[i32KnownDevIdx].eDeviceCurrState;
            BTRCORELOG_INFO ("btrCore_AddDeviceToKnownDevicesArr - Success Index = %d Unique DevID = %lld\n", i32KnownDevIdx, lpstlhBTRCore->stConnCbInfo.stKnownDevice.tDeviceId);
        }

        if (lpstlhBTRCore->fpcBBTRCoreConnAuth) {
            if (lpstlhBTRCore->fpcBBTRCoreConnAuth(&lpstlhBTRCore->stConnCbInfo, &i32DevAuthRet, lpstlhBTRCore->pvcBConnAuthUserData) != enBTRCoreSuccess) {
                //TODO: Triggering Outgoing callbacks from Incoming callbacks..aaaaaaaahhhh not a good idea
            }
        }


        if (lpstlhBTRCore->numOfPairedDevices && i32DevAuthRet) {

            if ((lenBTRCoreDevType == enBTRCoreMobileAudioIn) ||
                (lenBTRCoreDevType == enBTRCorePCAudioIn)) {
                unsigned int i32LoopIdx = 0;

                for (i32LoopIdx = 0; i32LoopIdx < lpstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                    if (lpstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId == lpstlhBTRCore->stConnCbInfo.stKnownDevice.tDeviceId) {

                        if ((i32ScannedDevIdx != -1) && (i32ScannedDevIdx < BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES) && (lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState == enBTRCoreDevStInitialized)) {
                            lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDevicePrevState = lpstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDevicePrevState;
                            lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState = lpstlhBTRCore->stScannedDevStInfoArr[i32ScannedDevIdx].eDeviceCurrState;
                        }

                        lpstlhBTRCore->stConnCbInfo.stKnownDevice.bDeviceConnected = TRUE;
                        lpstlhBTRCore->stKnownDevicesArr[i32LoopIdx].bDeviceConnected = TRUE;
                        BTRCORELOG_DEBUG("stKnownDevice.device_connected set : %d\n", lpstlhBTRCore->stKnownDevicesArr[i32LoopIdx].bDeviceConnected);
                    }
                }
            }
            else if ((lenBTRCoreDevType == enBTRCoreSpeakers) || 
                     (lenBTRCoreDevType == enBTRCoreHeadSet)) {
                unsigned int i32LoopIdx = 0;

                //TODO: Even before we loop, check if we are already connected and playing Audio-Out 
                for (i32LoopIdx = 0; i32LoopIdx < lpstlhBTRCore->numOfPairedDevices; i32LoopIdx++) {
                    if (lpstlhBTRCore->stKnownDevicesArr[i32LoopIdx].tDeviceId == lpstlhBTRCore->stConnCbInfo.stKnownDevice.tDeviceId) {
                        BTRCORELOG_INFO("ACCEPTED INCOMING CONNECT stKnownDevice : %s\n", lpstlhBTRCore->stKnownDevicesArr[i32LoopIdx].pcDeviceName);
                        if ((lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDevicePrevState != enBTRCoreDevStPlaying) &&
                            (lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState != enBTRCoreDevStConnected)) {
                            lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDevicePrevState = lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState;

                            if (lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState != enBTRCoreDevStPlaying)
                                lpstlhBTRCore->stKnownDevStInfoArr[i32LoopIdx].eDeviceCurrState = enBTRCoreDevStConnected;
                        }
                    }
                }
            }
            else if ((lenBTRCoreDevType == enBTRCoreHID) &&
                      strstr(apstBTDeviceInfo->pcName, "Xbox")) {
                if (BtrCore_BTDisableEnhancedRetransmissionMode(lpstlhBTRCore->connHdl) != 0) {
                    BTRCORELOG_ERROR ("Failed to Disable ERTM\n");
                }
            }
        }

    }

    return i32DevAuthRet;
}


#ifndef LE_MODE
STATIC  enBTRCoreRet
btrCore_BTMediaStatusUpdateCb (
    stBTRCoreAVMediaStatusUpdate*   apMediaStreamStatus,
    const char*                     apBtdevAddr,
    void*                           apUserData
) {
    stBTRCoreHdl*                   lpstlhBTRCore  = (stBTRCoreHdl*)apUserData;
    enBTRCoreRet                    lenBTRCoreRet  = enBTRCoreSuccess;
    tBTRCoreDevId                   lBTRCoreDevId  = 0;
    stBTRCoreMediaStatusCBInfo      lstMediaStatusUpdateCbInfo;


    if (!apMediaStreamStatus || !apBtdevAddr || !apUserData) {
       BTRCORELOG_ERROR ("enBTRCoreInvalidArg!!!");
       return enBTRCoreInvalidArg;
    }


    lBTRCoreDevId = btrCore_GenerateUniqueDeviceID(apBtdevAddr);
    if (!btrCore_GetKnownDeviceMac(lpstlhBTRCore, lBTRCoreDevId)) {
        BTRCORELOG_INFO ("We dont have a entry in the list; Skip Parsing now \n");
        return enBTRCoreDeviceNotFound;
    }

    MEMSET_S(&lstMediaStatusUpdateCbInfo, sizeof(stBTRCoreMediaStatusCBInfo), 0, sizeof(stBTRCoreMediaStatusCBInfo));

    switch (apMediaStreamStatus->eAVMediaState) {

    case eBTRCoreAVMediaTrkStStarted:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaTrkStStarted;
        MEMCPY_S(&lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo,sizeof(lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo), &apMediaStreamStatus->m_mediaPositionInfo, sizeof(stBTRCoreMediaPositionInfo));
        break;
    case eBTRCoreAVMediaTrkStPlaying:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaTrkStPlaying;
        MEMCPY_S(&lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo,sizeof(lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo), &apMediaStreamStatus->m_mediaPositionInfo, sizeof(stBTRCoreMediaPositionInfo));
        break;
    case eBTRCoreAVMediaTrkStPaused:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaTrkStPaused;
        MEMCPY_S(&lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo,sizeof(lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo), &apMediaStreamStatus->m_mediaPositionInfo, sizeof(stBTRCoreMediaPositionInfo));
        break;
    case eBTRCoreAVMediaTrkStStopped:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaTrkStStopped;
        MEMCPY_S(&lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo,sizeof(lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPositionInfo), &apMediaStreamStatus->m_mediaPositionInfo, sizeof(stBTRCoreMediaPositionInfo));
        break;
    case eBTRCoreAVMediaTrkStChanged:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaTrkStChanged;
        MEMCPY_S(&lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaTrackInfo,sizeof(lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaTrackInfo), &apMediaStreamStatus->m_mediaTrackInfo, sizeof(stBTRCoreMediaTrackInfo));
        break;
    case eBTRCoreAVMediaTrkPosition:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaTrkPosition;
        MEMCPY_S(&lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaTrackInfo,sizeof(lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaTrackInfo), &apMediaStreamStatus->m_mediaTrackInfo, sizeof(stBTRCoreMediaTrackInfo));
        break;
    case eBTRCoreAVMediaPlaybackEnded:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlaybackEnded;
        break;
    case eBTRCoreAVMediaPlyrName:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrName;
        strncpy (lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPlayerName, apMediaStreamStatus->m_mediaPlayerName, BTRCORE_MAX_STR_LEN -1);
        break;
    case eBTRCoreAVMediaPlyrEqlzrStOff:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrEqlzrStOff;
        break;
    case eBTRCoreAVMediaPlyrEqlzrStOn:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrEqlzrStOn;
        break;
    case eBTRCoreAVMediaPlyrShflStOff:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrShflStOff;
        break;
    case eBTRCoreAVMediaPlyrShflStAllTracks:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrShflStAllTracks;
        break;
    case eBTRCoreAVMediaPlyrShflStGroup:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrShflStGroup;
        break;
    case eBTRCoreAVMediaPlyrRptStOff:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrRptStOff;
        break;
    case eBTRCoreAVMediaPlyrRptStSingleTrack:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrRptStSingleTrack;
        break;
    case eBTRCoreAVMediaPlyrRptStAllTracks:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrRptStAllTracks;
        break;
    case eBTRCoreAVMediaPlyrRptStGroup:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrRptStGroup;
        break;
    case eBTRCoreAVMediaPlyrVolume:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrVolume;
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPlayerVolume = apMediaStreamStatus->m_mediaPlayerTransportVolume;
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.bIsMediaCtrlAvailable = (apMediaStreamStatus->bIsAVMediaCtrlAvail == 1) ? TRUE : FALSE;
        break;
    case eBTRCoreAVMediaPlyrDelay:
        BTRCORELOG_INFO("Received eBTRCoreAVMediaPlyrDelay event\n");
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaPlyrDelay;
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaPlayerDelay = apMediaStreamStatus->m_mediaPlayerTransportDelay;
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.bIsMediaCtrlAvailable = (apMediaStreamStatus->bIsAVMediaCtrlAvail == 1) ? TRUE : FALSE;
        break;
    case eBTRCoreAVMediaElementAdded:
        {
            stBTRCoreMediaElementInfo* mediaElement = &lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaElementInfo;

            lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaElementInScope;

            mediaElement->ui32MediaElementId   = apMediaStreamStatus->m_mediaElementInfo.ui32AVMediaElementId;
            mediaElement->eAVMedElementType = btrCore_GetMediaElementType (apMediaStreamStatus->m_mediaElementInfo.eAVMElementType);

            if ((mediaElement->bIsPlayable  = apMediaStreamStatus->m_mediaElementInfo.bIsPlayable)) {
                MEMCPY_S(&mediaElement->m_mediaTrackInfo,sizeof(stBTRCoreMediaTrackInfo), &apMediaStreamStatus->m_mediaElementInfo.m_mediaTrackInfo, sizeof(stBTRCoreMediaTrackInfo));
            }
            else {
                strncpy (mediaElement->m_mediaElementName, apMediaStreamStatus->m_mediaElementInfo.m_mediaElementName, BTRCORE_MAX_STR_LEN -1);
            }
        }
        break;
    case eBTRCoreAVMediaElementRemoved:
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.eBTMediaStUpdate = eBTRCoreMediaElementOofScope;
        lstMediaStatusUpdateCbInfo.m_mediaStatusUpdate.m_mediaElementInfo.ui32MediaElementId  = apMediaStreamStatus->m_mediaElementInfo.ui32AVMediaElementId;
        break;

    default:
        break;
    }

    lstMediaStatusUpdateCbInfo.deviceId = lBTRCoreDevId;

    if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBMediaStatus, &lstMediaStatusUpdateCbInfo)) != enBTRCoreSuccess) {
        BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBMediaStatus %d\n", lenBTRCoreRet);
        lenBTRCoreRet = enBTRCoreFailure;
    }


    return lenBTRCoreRet;
}
#endif


STATIC  enBTRCoreRet
btrCore_BTLeStatusUpdateCb (
    stBTRCoreLeGattInfo*    apstBtrLeInfo,
    const char*             apcBtdevAddr,
    void*                   apvUserData
) {
    stBTRCoreHdl*           lpstlhBTRCore       = (stBTRCoreHdl*)apvUserData;
    enBTRCoreRet            lenBTRCoreRet       = enBTRCoreFailure;
    enBTDeviceType          lenBTDeviceType     = enBTDevUnknown;
    stBTRCoreBTDevice*      lpstScannedDevice   = NULL;
    stBTRCoreDevStateInfo*  lpstScannedDevStInfo= NULL;
    const char*             pDevicePath         = NULL;
    const char*             pDeviceName         = NULL;
    tBTRCoreDevId           lBTRCoreDevId       = 0;
    stBTRCoreDevStatusCBInfo lstDevStatusCbInfo;
    int  i32LoopIdx  = -1;
    gboolean BatteryDevFound = FALSE;


    if (!apstBtrLeInfo || !apcBtdevAddr || !apvUserData) {
        BTRCORELOG_ERROR ("enBTRCoreInvalidArg!!!\n");
        return enBTRCoreInvalidArg;
    }


    //TODO : Look if we can get the infos from the CB itslef
    lBTRCoreDevId = btrCore_GenerateUniqueDeviceID(apcBtdevAddr);
    if ((lenBTRCoreRet = btrCore_GetDeviceInfo(lpstlhBTRCore, lBTRCoreDevId, enBTRCoreLE, &lenBTDeviceType,
                                   &lpstScannedDevice, &lpstScannedDevStInfo, &pDevicePath, &pDeviceName)) != enBTRCoreSuccess) {
        BTRCORELOG_ERROR ("Failed to find Device in ScannedList!\n");
        return enBTRCoreDeviceNotFound;
    }


    BTRCORELOG_DEBUG ("LE Dev %s Path %s\n", pDeviceName, pDevicePath);

    lstDevStatusCbInfo.deviceId             = lBTRCoreDevId;
    lstDevStatusCbInfo.eDeviceType          = enBTRCoreLE;

    for (i32LoopIdx = 0; i32LoopIdx < lpstScannedDevice->stDeviceProfile.numberOfService; i32LoopIdx++) {
        if (lpstScannedDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_GATT_XBB_1, NULL, 16) ||
            lpstScannedDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_1, NULL, 16) ||
            lpstScannedDevice->stDeviceProfile.profile[i32LoopIdx].uuid_value == strtol(BTR_CORE_BATTERY_SERVICE_XBB_3, NULL, 16)) {
            lstDevStatusCbInfo.eDeviceClass = enBTRCore_DC_XBB;
            BatteryDevFound = TRUE;
            break;
        }
    }

    if (!BatteryDevFound) {
        lstDevStatusCbInfo.eDeviceClass= lpstScannedDevice->enDeviceType;
    }

    lstDevStatusCbInfo.ui32DevClassBtSpec   = lpstScannedDevice->ui32DevClassBtSpec;
    lstDevStatusCbInfo.ui16DevAppearanceBleSpec = lpstScannedDevice->ui16DevAppearanceBleSpec;
    lstDevStatusCbInfo.isPaired             = 0;   
    lstDevStatusCbInfo.eDevicePrevState     = lpstScannedDevStInfo->eDeviceCurrState;
    strncpy(lstDevStatusCbInfo.deviceName, pDeviceName, BD_NAME_LEN);
    strncpy(lstDevStatusCbInfo.deviceAddress, apcBtdevAddr, BTRCORE_MAX_STR_LEN - 1);

    lstDevStatusCbInfo.eCoreLeProp          = apstBtrLeInfo->enLeProp;
    lstDevStatusCbInfo.eDeviceType          = enBTRCoreLE;
    lstDevStatusCbInfo.eCoreLeOper          = apstBtrLeInfo->enLeOper;

    switch (apstBtrLeInfo->enLeOper) {
    case enBTRCoreLEGOpReady:
        lstDevStatusCbInfo.eDeviceCurrState = enBTRCoreDevStOpReady;
        strncpy(lstDevStatusCbInfo.devOpResponse, apstBtrLeInfo->pui8Value, BTRCORE_MAX_DEV_OP_DATA_LEN - 1);
        break;
    case enBTRCoreLEGOpReadValue:
        lstDevStatusCbInfo.eDeviceCurrState = enBTRCoreDevStOpInfo;
        strncpy(lstDevStatusCbInfo.uuid, apstBtrLeInfo->pui8Uuid, BTRCORE_UUID_LEN - 1);
        MEMSET_S(lstDevStatusCbInfo.devOpResponse, BTRCORE_MAX_DEV_OP_DATA_LEN, '\0', BTRCORE_MAX_DEV_OP_DATA_LEN);
        strncpy(lstDevStatusCbInfo.devOpResponse, apstBtrLeInfo->pui8Value, BTRCORE_MAX_DEV_OP_DATA_LEN - 1);
        break;
    case enBTRCoreLEGOpWriteValue:
        lstDevStatusCbInfo.eDeviceCurrState = enBTRCoreDevStOpInfo;
        strncpy(lstDevStatusCbInfo.uuid, apstBtrLeInfo->pui8Uuid, BTRCORE_UUID_LEN - 1);
        MEMSET_S(lstDevStatusCbInfo.devOpResponse, BTRCORE_MAX_DEV_OP_DATA_LEN, '\0', BTRCORE_MAX_DEV_OP_DATA_LEN);
        strncpy(lstDevStatusCbInfo.devOpResponse, apstBtrLeInfo->pui8Value, BTRCORE_MAX_DEV_OP_DATA_LEN - 1);
        break;
    case enBTRCoreLEGOpStartNotify:
        lstDevStatusCbInfo.eDeviceCurrState = enBTRCoreDevStOpInfo;
        MEMSET_S(lstDevStatusCbInfo.uuid, BTRCORE_UUID_LEN, '\0', BTRCORE_UUID_LEN);
        strncpy(lstDevStatusCbInfo.uuid, apstBtrLeInfo->pui8Uuid, BTRCORE_UUID_LEN - 1);
        break;
    case enBTRCoreLEGOpStopNotify:
        lstDevStatusCbInfo.eDeviceCurrState = enBTRCoreDevStOpInfo;
        MEMSET_S(lstDevStatusCbInfo.uuid, BTRCORE_UUID_LEN, '\0', BTRCORE_UUID_LEN);
        strncpy(lstDevStatusCbInfo.uuid, apstBtrLeInfo->pui8Uuid, BTRCORE_UUID_LEN - 1);
        break;
    case enBTRCoreLEGOpUnknown:
    default:
        break;
    }

    if ((enBTRCoreLEGOpReadValue == apstBtrLeInfo->enLeOper) && (lpstScannedDevice->enDeviceType != enBTRCore_DC_Tile)) {
        if (lpstlhBTRCore->fpcBBTRCoreStatus) {
            BTRCORELOG_INFO(" Not adding to the task queue !!!!!!\n");
            /* Invoke the callback */
            if (enBTRCoreSuccess != lpstlhBTRCore->fpcBBTRCoreStatus(&lstDevStatusCbInfo, lpstlhBTRCore->pvcBStatusUserData) ) {
                BTRCORELOG_ERROR(" CallBack Error !!!!!!\n");
                lenBTRCoreRet = enBTRCoreFailure;
            }
            else {
                strncpy(apstBtrLeInfo->pui8Value, lstDevStatusCbInfo.devOpResponse,  BTRCORE_MAX_DEV_OP_DATA_LEN - 1);
            }
        }
    }
    else {
        if ((lenBTRCoreRet = btrCore_OutTaskAddOp(lpstlhBTRCore->pGAQueueOutTask, enBTRCoreTaskOpProcess, enBTRCoreTaskPTcBDevOpInfoStatus, &lstDevStatusCbInfo)) != enBTRCoreSuccess) {
            BTRCORELOG_WARN("Failure btrCore_OutTaskAddOp enBTRCoreTaskOpProcess enBTRCoreTaskPTcBDeviceStatus %d\n", lenBTRCoreRet);
        }
    }


    return lenBTRCoreRet;
}

/* End of File */
