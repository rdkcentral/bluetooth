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
/*
 * btrCore_gdbus_bluez5.c
 * Implementation of GDBus layer abstraction for BT functionality (BlueZ 5.45/5.48/5.54)
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>

/* External Library Headers */
#include <glib.h>
#include <gio/gio.h>

/* Interface lib Headers */
#include "btrCore_logger.h"
#include "bt-telemetry.h"

/* Local Headers */
#include "btrCore_bt_ifce.h"

#include "gdbusBluez.h"
#include "gdbusFreedesktop.h"
#include <bluezAPI/util/agentHelper.h>
#include <bleconfd/deviceinfo.h>
#include <bleconfd/gatt.h>
#include <btrCore_platform_spec.h>

#include "safec_lib.h"


#ifdef LIBSYSWRAPPER_BUILD
#include "secure_wrapper.h"
#endif

#define DBUS_INTERFACE_PROPERTIES           "org.freedesktop.DBus.Properties"
#define DBUS_INTERFACE_OBJECT_MANAGER       "org.freedesktop.DBus.ObjectManager"
#define BT_DBUS_BLUEZ_PATH                  "org.bluez"
#define BT_DBUS_BLUEZ_ADAPTER_PATH          "org.bluez.Adapter1"
#define BT_DBUS_BLUEZ_DEVICE_PATH           "org.bluez.Device1"
#define BT_DBUS_BLUEZ_GATT_SERVICE_PATH     "org.bluez.GattService1"
#define BT_DBUS_BLUEZ_GATT_CHAR_PATH        "org.bluez.GattCharacteristic1"
#define BT_DBUS_BLUEZ_GATT_DESCRIPTOR_PATH  "org.bluez.GattDescriptor1"
#define BT_DBUS_BLUEZ_BATTERY_PATH          "org.bluez.Battery1"

#define BT_MEDIA_SBC_A2DP_SINK_ENDPOINT     "/MediaEndpoint/SBC/A2DP/Sink"
#define BT_MEDIA_SBC_A2DP_SOURCE_ENDPOINT   "/MediaEndpoint/SBC/A2DP/Source"
#define BT_MEDIA_MP3_A2DP_SINK_ENDPOINT     "/MediaEndpoint/Mp3/A2DP/Sink"
#define BT_MEDIA_MP3_A2DP_SOURCE_ENDPOINT   "/MediaEndpoint/Mp3/A2DP/Source"
#define BT_MEDIA_AAC_A2DP_SINK_ENDPOINT     "/MediaEndpoint/AAC/A2DP/Sink"
#define BT_MEDIA_AAC_A2DP_SOURCE_ENDPOINT   "/MediaEndpoint/AAC/A2DP/Source"
#define BT_MEDIA_PCM_HFP_AG_ENDPOINT        "/MediaEndpoint/PCM/HFP/AudioGateway"
#define BT_MEDIA_SBC_HFP_AG_ENDPOINT        "/MediaEndpoint/SBC/HFP/AudioGateway"
#define BT_MEDIA_PCM_HFP_HS_ENDPOINT        "/MediaEndpoint/PCM/HFP/Headset"

#define BT_LE_GATT_SERVER_ENDPOINT          "/org/bluez/hci0/dev_XX_XX_XX_XX_XX_XX"
#define BT_LE_GATT_SERVER_ADVERTISEMENT     "/LeGattAdvert"

#define BT_BLUEZ_VERSION_5_45               "5.45"
#define BT_BLUEZ_VERSION_5_48               "5.48"
#define BT_BLUEZ_VERSION_5_54               "5.54"
#define BT_BLUEZ_VERSION_5_77               "5.77"

#define BT_MAX_ADAPTERS                      4
#define BT_MAX_DEVICES                       64


typedef struct _stBTMediaInfo {
    unsigned char   ui8Codec;
    char            pcState[BT_MAX_STR_LEN];
    char            pcUUID[BT_MAX_STR_LEN];
    unsigned short  ui16Delay;
    unsigned short  ui16Volume;
} stBTMediaInfo;


typedef struct _stBtIfceHdl {
    GDBusConnection                         *pGDBusConn;

    char*                                   pcBTAgentPath;
    char*                                   pcBTDAdapterPath;
    char*                                   pcBTDAdapterAddr;
    char*                                   pcBTAdapterPath;
    char*                                   pcDevTransportPath;

    char*                                   pcBTOutPassCode;

    void*                                   pcBAdapterStatusUserData;
    void*                                   pcBDevStatusUserData;
    void*                                   pcBMediaStatusUserData;
    void*                                   pcBNegMediaUserData;
    void*                                   pcBTransPathMediaUserData;
    void*                                   pcBMediaPlayerPathUserData;
    void*                                   pcBMediaBrowserPathUserData;
    void*                                   pcBConnIntimUserData;
    void*                                   pcBConnAuthUserData;
    void*                                   pcBLePathUserData;
    void*                                   pcBLeAdvUserData;

    int                                     i32DoReject;

    unsigned int                            ui32cBConnAuthPassKey;
    unsigned int                            ui32DevLost;

    unsigned int                            ui32IsAdapterDiscovering;

    char                                    pcBTVersion[BT_MAX_STR_LEN];
    char                                    pcDeviceCurrState[BT_MAX_STR_LEN];
    char                                    pcLeDeviceCurrState[BT_MAX_STR_LEN];
    char                                    pcLeDeviceAddress[BT_MAX_STR_LEN];
    char                                    pcMediaCurrState[BT_MAX_STR_LEN];
    char                                    BatteryOTAControlPath[BT_MAX_STR_LEN];

    stBTAdapterInfo                         lstBTAdapterInfo;
    stBTDeviceInfo                          lstBTDeviceInfo;
    stBTMediaInfo                           lstBTMediaInfo;

    stBTLeGattInfo                          stBTLeGatt;
    stBTLeCustomAdv                         stBTLeCustAdvt;
    char                                    pcBTAdapterGattSrvEpPath[BT_MAX_DEV_PATH_LEN];

    fPtr_BtrCore_BTAdapterStatusUpdateCb    fpcBAdapterStatusUpdate;
    fPtr_BtrCore_BTDevStatusUpdateCb        fpcBDevStatusUpdate;
    fPtr_BtrCore_BTMediaStatusUpdateCb      fpcBMediaStatusUpdate;
    fPtr_BtrCore_BTNegotiateMediaCb         fpcBNegotiateMedia;
    fPtr_BtrCore_BTTransportPathMediaCb     fpcBTransportPathMedia;
    fPtr_BtrCore_BTMediaPlayerPathCb        fpcBTMediaPlayerPath;
    fPtr_BtrCore_BTMediaBrowserPathCb       fpcBTMediaBrowserPath;
    fPtr_BtrCore_BTConnIntimCb              fpcBConnectionIntimation;
    fPtr_BtrCore_BTConnAuthCb               fpcBConnectionAuthentication;
    fPtr_BtrCore_BTLeGattPathCb             fpcBTLeGattPath;
    fPtr_BtrCore_BTLeAdvertisementCb        fpcBTLeAdvPath;

    char                                    AdapPathInfo[BT_MAX_ADAPTERS][BT_MAX_DEV_PATH_LEN];
    char                                    DevPathInfo[BT_MAX_DEVICES][BT_MAX_DEV_PATH_LEN];

    /*TODO : Include a seperate structure to store the GATT related
     * objects,Flags,proxies and signal handlers.
     */
    char                                    GattCharDevPath[BT_MAX_NUM_GATT_CHAR][BT_MAX_STR_LEN];
    char                                    GattSerDevPath[BT_MAX_NUM_GATT_SERVICE][BT_MAX_STR_LEN];
    char                                    GattDesDevPath[BT_MAX_NUM_GATT_DESC][BT_MAX_STR_LEN];
    char                                    GattCharNotifyPath[BT_MAX_NOTIFY_CHAR_PATH][BT_MAX_STR_LEN];

    int                                     DevCount;
    int                                     GattCharDevCount;
    int                                     GattSerDevCount;
    int                                     GattDesDevCount;
    int                                     GattCharNotifyDevCount;

    gulong                                  GattCharSignalHandlerID[BT_MAX_NUM_GATT_CHAR];
    gulong                                  GattSerSignalHandlerID[BT_MAX_NUM_GATT_SERVICE];
    gulong                                  GattDesSignalHandlerID[BT_MAX_NUM_GATT_DESC];
    gulong                                  DevSignalHandlerID[BT_MAX_DEVICES];

    DBusObjectManager                       *DBusObjManProxy;
    BluezAdapter1                           *DBusAdapProxy;
    BluezDevice1                            *DevProxyList[BT_MAX_DEVICES];
    BluezGattCharacteristic1                *GattCharProxyList[BT_MAX_NUM_GATT_CHAR];
    BluezGattService1                       *GattSerProxyList[BT_MAX_NUM_GATT_SERVICE];
    BluezGattDescriptor1                    *GattDesProxyList[BT_MAX_NUM_GATT_DESC];

    FILE                                    *BatteryFirmFilep;
    BluezGattCharacteristic1                *BatteryDevProxy;

} stBtIfceHdl;


/* Static Function Prototypes */
#ifdef GATT_CLIENT
static void btrCore_BTGattCharPropertyChangedCb(BluezGattCharacteristic1 *proxy,GParamSpec *spec,gpointer userdata);
static void btrCore_BTGattSerPropertyChangedCb(BluezGattService1 *proxy,GParamSpec *spec,gpointer userdata);
static void btrCore_BTGattDesPropertyChangedCb(BluezGattDescriptor1 *proxy,GParamSpec *spec,gpointer userdata);
#endif
static void btrCore_bluezDeviceAppearedCb(DBusObjectManager *proxy,const gchar *object,GVariant *value,gpointer userdata);
static void btrCore_bluezDeviceDisappearedCb(DBusObjectManager *proxy,const gchar *object,GDBusInterfaceInfo *value,void *userdata);
static void btrCore_bluezSignalDeviceChangedCb(BluezDevice1 *proxy,GParamSpec *spec,gpointer userdata);
static void btrCore_bluezSignalAdatperChangedCb (BluezAdapter1 *proxy,GParamSpec *spec,gpointer userdata);
static enBTDeviceType btrCore_BTMapDevClasstoDevType(unsigned int aui32Class);
static int btrCore_BTParseAdapter(BluezAdapter1 *adap_proxy,stBTAdapterInfo* apstBTAdapterInfo);
static int btrCore_BTGetDeviceInfo(stBtIfceHdl* apstlhBtIfce,stBTDeviceInfo* apstBTDeviceInfo,const char* apcIface);
static int btrCore_BTGetDevAddressFromDevPath (const char* deviceIfcePath, char* devAddr);
static char* btrCore_BTGetDefaultAdapterPath (stBtIfceHdl* apstlhBtIfce);
static int btrCore_BTReleaseDefaultAdapterPath (stBtIfceHdl* apstlhBtIfce);
static void btrCore_BTStartDiscoveryCb(BluezAdapter1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTStopDiscoveryCb(BluezAdapter1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTConnectDeviceCb(BluezDevice1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTDisconnectDeviceCb(BluezDevice1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTRemoveDeviceCb(BluezAdapter1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTStopDiscoveryCb(BluezAdapter1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTSetDiscoveryFilterCb(BluezAdapter1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTGattDesWriteValueCb(BluezGattDescriptor1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTGattCharWriteValueCb(BluezGattCharacteristic1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTGattCharStartNotifyCb(BluezGattCharacteristic1 *proxy,GAsyncResult *res,gpointer data);
static void btrCore_BTGattCharStopNotifyCb(BluezGattCharacteristic1 *proxy,GAsyncResult *res,gpointer data);
static int btrCore_BTReqPasskeyCB (const gchar *lpcPath, void *apvUserData);
static int btrCore_BTReqAutherizationCB (const gchar *lpcPath, void *apvUserData);
static int btrCore_BTReqConfirmationCB (const gchar *lpcPath, guint ui32PassCode, void *apvUserData);
static void btrCore_BTAdvReleaseCB(const char *apBtAdapter, gpointer apvUserData);
static void btrCore_BTFirmwareUpdateCb (BluezGattCharacteristic1 *proxy,GAsyncResult *res,gpointer data);

/*  Local Op Threads Prototypes */
static gpointer btrCore_g_main_loop_Task (gpointer appvMainLoop);

static void*                        gpvMainLoop                 = NULL;
static void*                        gpvMainLoopThread           = NULL;

// @brief Assign pairing agent method callback
// three callback so far, since it asks confirmation from user
static struct agentMethodCb agtIfaceCb = {
    .reqConfirmation = btrCore_BTReqConfirmationCB,
    .reqAuth = btrCore_BTReqAutherizationCB,
    .reqPasskey = btrCore_BTReqPasskeyCB,
};

static struct leadvertisement1Cb leAdvIfaceCb = {
    .advRelease = btrCore_BTAdvReleaseCB,
};

#if 0
static enBTDeviceType btrCore_BTMapServiceClasstoDevType(unsigned int aui32Class);
static enBTDeviceType btrCore_BTMapDevClasstoDevType(unsigned int aui32Class);
static tBTMediaItemId btrCore_BTGetMediaItemIdFromMediaPath (const char* mediaIfcePath);
static enBTMediaFolderType btrCore_BTGetMediaFolderType (const char* apcFolderType);
static const char* btrCore_BTGetMediaItemAbsoluteName (const char* apcMediaItemName);
#endif

#if 0
static int btrCore_BTGetGattInfo (enBTOpIfceType aenBTOpIfceType, void* apvGattInfo, const char* apcIface);
#endif

/* Incoming Callbacks Prototypes */



static enBTDeviceType
btrCore_BTMapServiceClasstoDevType (
    unsigned int aui32Class
) {
    enBTDeviceType lenBtDevType = enBTDevUnknown;

    /* Refer https://www.bluetooth.com/specifications/assigned-numbers/baseband
     * The bit 18 set to represent AUDIO OUT service Devices.
     * The bit 19 can be set to represent AUDIO IN Service devices
     * The bit 21 set to represent AUDIO Services (Mic, Speaker, headset).
     * The bit 22 set to represent Telephone Services (headset).
     */

    if (0x40000u & aui32Class) {
        BTRCORELOG_DEBUG ("Its a enBTDevAudioSink : Rendering Class of Service\n");
        lenBtDevType = enBTDevAudioSink;
    }
    else if (0x80000u & aui32Class) {
        if (enBTDCMicrophone & aui32Class) {
            BTRCORELOG_DEBUG ("Its a enBTDevAudioSource : Capturing Service and Mic Device\n");
            lenBtDevType = enBTDevAudioSource;
        }
    }
    //Audio device but not a peripheral  (mouse, joystick, keyboards etc )
    else if ((0x200000u & aui32Class) && ((0x500u & aui32Class) != 0x500u ))  {
        if (enBTDCMicrophone & aui32Class) {
            BTRCORELOG_DEBUG ("Its a enBTDevAudioSource : Audio Class of Service and Mic Device\n");
            lenBtDevType = enBTDevAudioSource;
        }
        else {
            BTRCORELOG_DEBUG ("Its a enBTDevAudioSink : Audio Class of Service. Not a Mic\n");
            lenBtDevType = enBTDevAudioSink;
        }
    }
    else if (0x400000u & aui32Class) {
        BTRCORELOG_DEBUG ("Its a enBTDevAudioSink : Telephony Class of Service\n");
        lenBtDevType = enBTDevAudioSink;
    }

    return lenBtDevType;
}


static enBTDeviceType
btrCore_BTMapDevClasstoDevType (
    unsigned int    aui32Class
) {
    enBTDeviceType lenBtDevType = enBTDevUnknown;

    if ((lenBtDevType = btrCore_BTMapServiceClasstoDevType(aui32Class)) != enBTDevUnknown)
        return lenBtDevType;


    if (((aui32Class & 0x100u) == 0x100u) ||
        ((aui32Class & 0x200u) == 0x200u) ||
        ((aui32Class & 0x400u) == 0x400u) ||
        ((aui32Class & 0x500u) == 0x500u) ||
        ((aui32Class & 0x540u) == 0x540u) ||
        ((aui32Class & 0x580u) == 0x580u) ||
        ((aui32Class & 0x5C0u) == 0x5C0u) ||
        ((aui32Class & 0x504u) == 0x504u) ||
        ((aui32Class & 0x508u) == 0x508u) || 
        ((aui32Class & 0x50Cu) == 0x50Cu)) {

        unsigned int ui32DevClassID = aui32Class & 0xFFFu;

        switch (ui32DevClassID){
        case enBTDCSmartPhone:
        case enBTDCTablet:
        case enBTDCMicrophone:
            BTRCORELOG_DEBUG ("Its a enBTDevAudioSource\n");
            lenBtDevType = enBTDevAudioSource;
            break;
        case enBTDCWearableHeadset:
        case enBTDCHeadphones:
        case enBTDCLoudspeaker:
        case enBTDCHIFIAudioDevice:
            BTRCORELOG_DEBUG ("Its a enBTDevAudioSink\n");
            lenBtDevType = enBTDevAudioSink;
            break;
        case enBTDCKeyboard:
        case enBTDCMouse:
        case enBTDCMouseKeyBoard:
        case enBTDCJoystick:
        case enBTDCGamePad:
        case enBTDCAudioRemote:
            BTRCORELOG_DEBUG ("Its a enBTDevHID\n");
            lenBtDevType = enBTDevHID;
            break;
        default:
            BTRCORELOG_INFO ("Its a enBTDevUnknown\n");                   
            lenBtDevType = enBTDevUnknown;
            break;
        }
    }

    return lenBtDevType;
}


static int
btrCore_BTGetDevAddressFromDevPath (
    const char*     deviceIfcePath,
    char*           devAddr
) {
    //  DevIfcePath format /org/bluez/hci0/dev_DC_1A_C5_62_F5_EA
    deviceIfcePath = strstr(deviceIfcePath, "dev") + 4;

    devAddr[0]   = deviceIfcePath[0];
    devAddr[1]   = deviceIfcePath[1];
    devAddr[2]   = ':';
    devAddr[3]   = deviceIfcePath[3];
    devAddr[4]   = deviceIfcePath[4];
    devAddr[5]   = ':';
    devAddr[6]   = deviceIfcePath[6];
    devAddr[7]   = deviceIfcePath[7];
    devAddr[8]   = ':';
    devAddr[9]   = deviceIfcePath[9];
    devAddr[10]  = deviceIfcePath[10];
    devAddr[11]  = ':';
    devAddr[12]  = deviceIfcePath[12];
    devAddr[13]  = deviceIfcePath[13];
    devAddr[14]  = ':';
    devAddr[15]  = deviceIfcePath[15];
    devAddr[16]  = deviceIfcePath[16];

    devAddr[17]  = '\0';

    return 0;
}


#if 0
static tBTMediaItemId
btrCore_BTGetMediaItemIdFromMediaPath (
    const char*     mediaIfcePath
) {
    tBTMediaItemId  mediaItemId = 0;
    char*           ptr         = 0;
    unsigned int    ui32Index   = 0;
    char            iArray[BT_MAX_STR_LEN]  = {'\0'};
    /* To change the logic later to enable index based searching for better performance */
    ptr = strstr(mediaIfcePath, "item");

    if (ptr != NULL) {

        while (ptr && *ptr) {
            if (*ptr<=57 && *ptr>=48) {
                iArray[ui32Index++] = *ptr;
            }
            ptr++;
        }

        iArray[ui32Index] = '\0';

        /* In mediaIfcePath if big value comes,
         * mediaItemId is limiting the range from the 1 to LLONG_MAX.
         * mediaItemId MSB bit is used for differentiating the items from which path
         * either NowPlaying or Filesystem.
         */
        ui32Index = 0;
        do {
            mediaItemId = strtoull (iArray + ui32Index++, NULL, 10);
        } while ((mediaItemId > LLONG_MAX) || (mediaItemId == 0));
    }

    if (strstr(mediaIfcePath, "NowPlaying")) {
        mediaItemId |= 0x8000000000000000;
    }
    else if (!strstr(mediaIfcePath, "Filesystem")) {
        mediaItemId  = 0xFFFFFFFFFFFFFFFF;
    }

    return mediaItemId;
}

static const char*
btrCore_BTGetMediaItemAbsoluteName (
    const char*     apcMediaItemName
) {
    const char*     ptr = 0;
    /* /Filesystem/Album/XYZ_Album   should return  XYZ_Album    */
    /* /Filesystem                   should return  Filesystem   */
    /* ABC_SongName                  should return  ABC_SongName */

    if (!apcMediaItemName) {
        return NULL;
    }

    ptr = &apcMediaItemName[strlen(apcMediaItemName)];

    while (--ptr >= apcMediaItemName && *ptr != '/');

    return ptr +1;
}

static enBTMediaFolderType
btrCore_BTGetMediaFolderType (
    const char*     apcFolderType
) {
    enBTMediaFolderType     eMediaFolderType;

    if (!strncmp(apcFolderType, "albums",      strlen("albums"))) {
        eMediaFolderType    = enBTMediaFldTypAlbum;
    }
    else if (!strncmp(apcFolderType, "artists",     strlen("artists"))) {
        eMediaFolderType    = enBTMediaFldTypArtist;
    }
    else if (!strncmp(apcFolderType, "genres",      strlen("genres"))) {
        eMediaFolderType    = enBTMediaFldTypGenre;
    }
    else if (!strncmp(apcFolderType, "compilation", strlen("compilation"))) {
        eMediaFolderType    = enBTMediaFldTypCompilation;
    }
    else if (!strncmp(apcFolderType, "playlists",    strlen("playlists"))) {
        eMediaFolderType    = enBTMediaFldTypPlayList;
    }
    else if (!strncmp(apcFolderType, "titles",      strlen("titles"))) {
        eMediaFolderType    = enBTMediaFldTypTrackList;
    }
    else if (!strncmp(apcFolderType, "mixed",       strlen("mixed"))) {
        eMediaFolderType    = enBTMediaFldTypTrackList;
    }
    else if (!strncmp(apcFolderType, "track",       strlen("track"))) {
        eMediaFolderType    = enBTMediaFldTypTrack;
    }
    else {
        eMediaFolderType    = enBTMediaFldTypTrackList;
    }

    return eMediaFolderType;
}
#endif

static char*
btrCore_BTGetDefaultAdapterPath (
    stBtIfceHdl*    pstlhBtIfce
) {
    bool  lbAdapterFound = FALSE;
    char *adapter_path = NULL;
    char  object_path[BT_MAX_DEV_PATH_LEN] = {'\0'};
    const gchar *interface_name = NULL;
    GError *error = NULL;
    gboolean result;
    GVariant *properties = NULL,*lOutObjects = NULL,*ifaces_and_properties = NULL;

    if (pstlhBtIfce->DBusObjManProxy != NULL) {
        result = dbus_object_manager_call_get_managed_objects_sync((DBusObjectManager *)pstlhBtIfce->DBusObjManProxy,
                &lOutObjects,
                NULL,
                &error);
    }
    else {
        BTRCORELOG_ERROR("Proxy not available to read managed objects\n");
        return 0;
    }

    if (result != TRUE) {
        BTRCORELOG_ERROR("Get Managed Objects method call failed \n");
    }

    if (lOutObjects != NULL) {
        while (!lbAdapterFound) {
            GVariantIter i;
            g_variant_iter_init(&i,lOutObjects);
            while(g_variant_iter_next(&i, "{&o@a{sa{sv}}}", &adapter_path, &ifaces_and_properties)) {
                GVariantIter ii;
                memcpy(object_path, adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
                g_variant_iter_init(&ii, ifaces_and_properties);
                while(g_variant_iter_next(&ii, "{&s@a{sv}}", &interface_name, &properties)) {
                    if (strncmp(interface_name, BT_DBUS_BLUEZ_ADAPTER_PATH, sizeof(BT_DBUS_BLUEZ_ADAPTER_PATH)) == 0) {
                        pstlhBtIfce->pcBTDAdapterPath = strndup(adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
                        lbAdapterFound = TRUE;
                        break;
                    }
                    g_variant_unref(properties);
                }
                g_variant_unref(ifaces_and_properties);
            }
        }
        g_variant_unref(lOutObjects);
    }

    if (pstlhBtIfce->pcBTDAdapterPath) {
        BTRCORELOG_WARN ("Default Adapter Path is : %s - %p\n", pstlhBtIfce->pcBTDAdapterPath, pstlhBtIfce->pcBTDAdapterPath);

    }
    return pstlhBtIfce->pcBTDAdapterPath;
}

static int
btrCore_BTReleaseDefaultAdapterPath (
    stBtIfceHdl*    apstlhBtIfce
) {
    if (apstlhBtIfce->pcBTDAdapterAddr) {
        BTRCORELOG_WARN ("Adapter Addr is : %s - %p\n", apstlhBtIfce->pcBTDAdapterAddr, apstlhBtIfce->pcBTDAdapterAddr);
        free(apstlhBtIfce->pcBTDAdapterAddr);
        apstlhBtIfce->pcBTDAdapterAddr = NULL;
    }

    if (apstlhBtIfce->pcBTDAdapterPath) {
        BTRCORELOG_WARN ("Current Adapter Path is : %s - %p\n", apstlhBtIfce->pcBTDAdapterPath, apstlhBtIfce->pcBTDAdapterPath);
        free(apstlhBtIfce->pcBTDAdapterPath);
        apstlhBtIfce->pcBTDAdapterPath = NULL;
    }

    return 0;
}

#if 0
/* Stub function to print gatt properties when the gatt object
 * added in lower layer. we will remove it in future.
 */

static void
btrCore_BTGetGattProperty (
    stBtIfceHdl *pstlhBtIfce,
    const char *object_path,
    const char *interface_name
) {
    BluezGattService1 *lDBusGattSerProxy = NULL;
    BluezGattCharacteristic1 *lDBusGattCharProxy = NULL;
    BluezGattDescriptor1 *lDBusGattDesProxy = NULL;
    GError *error = NULL;
    const gchar * const* arg = NULL;
    GVariant *var = NULL;
    const guint8 *array = NULL;
    gsize n_elements;
    int i;

    if (strcmp(interface_name,BT_DBUS_BLUEZ_GATT_CHAR_PATH) == 0) {

        lDBusGattCharProxy = bluez_gatt_characteristic1_proxy_new_sync((GDBusConnection *)pstlhBtIfce->pGDBusConn,
                G_DBUS_PROXY_FLAGS_NONE,
                BT_DBUS_BLUEZ_PATH,
                object_path,
                NULL,
                &error);

        /* UUID */
        BTRCORELOG_DEBUG("> Characteristic - uuid - %s <\n",bluez_gatt_characteristic1_get_uuid(lDBusGattCharProxy));
        /* Object Service */
        BTRCORELOG_DEBUG("> Characteristic - service - %s <\n",bluez_gatt_characteristic1_get_service(lDBusGattCharProxy));
        /* Notifying */
        BTRCORELOG_DEBUG("> Characteristic - Notifying - %d <\n",bluez_gatt_characteristic1_get_notifying(lDBusGattCharProxy));

        /* Flags */
        arg = bluez_gatt_characteristic1_get_flags(lDBusGattCharProxy);
        if (arg != NULL) {
            while (*arg != NULL) {
                BTRCORELOG_DEBUG("> Characteristic - flag - %s <\n",*arg);
                arg++;
            }
        }

        /* Value */
        var = bluez_gatt_characteristic1_get_value(lDBusGattCharProxy);

        array = g_variant_get_fixed_array (var,&n_elements,sizeof(guint8));
        for (i=0;i<n_elements;i++)
        {
            printf("%02X",array[i]);
        }

        BTRCORELOG_DEBUG("> Characteristic - Handle - %d <\n",bluez_gatt_characteristic1_get_handle(lDBusGattCharProxy));
        lDBusGattCharProxy = NULL;

    }
    else if (strcmp(interface_name,BT_DBUS_BLUEZ_GATT_SERVICE_PATH) == 0) {

        lDBusGattSerProxy = bluez_gatt_service1_proxy_new_sync((GDBusConnection *)pstlhBtIfce->pGDBusConn,
                G_DBUS_PROXY_FLAGS_NONE,
                BT_DBUS_BLUEZ_PATH,
                object_path,
                NULL,
                &error);

        /* UUID */
        BTRCORELOG_DEBUG("> Service - uuid - %s <\n",bluez_gatt_service1_get_uuid(lDBusGattSerProxy));
        /* Primary */
        BTRCORELOG_DEBUG("> Service - primary - %d <\n",bluez_gatt_service1_get_primary(lDBusGattSerProxy));
        /* Object Service */
        BTRCORELOG_DEBUG("> Service - object - %s <\n",bluez_gatt_service1_get_device(lDBusGattSerProxy));
#if 0
        /* Includes */
        arg = bluez_gatt_service1_get_includes(lDBusGattSerProxy);
        if (arg != NULL) {
            while (*arg != NULL) {
                BTRCORELOG_DEBUG("> service - includes - %s <\n",*arg);
                arg++;
            }
        }
#endif
        /* Handle */
        BTRCORELOG_DEBUG("> Service - Handle - %d <\n",bluez_gatt_service1_get_handle(lDBusGattSerProxy));
        lDBusGattSerProxy = NULL;

    }
    else if (strcmp(interface_name,BT_DBUS_BLUEZ_GATT_DESCRIPTOR_PATH) == 0) {

        lDBusGattDesProxy = bluez_gatt_descriptor1_proxy_new_sync((GDBusConnection *)pstlhBtIfce->pGDBusConn,
                G_DBUS_PROXY_FLAGS_NONE,
                BT_DBUS_BLUEZ_PATH,
                object_path,
                NULL,
                &error);

        /* UUID */
        BTRCORELOG_DEBUG("> Descriptor - uuid - %s <\n",bluez_gatt_descriptor1_get_uuid(lDBusGattDesProxy));
        /* Object characteristic */
        BTRCORELOG_DEBUG("> Descriptor - char - %s <\n",bluez_gatt_descriptor1_get_characteristic(lDBusGattDesProxy));
        /* Flags */
        arg = bluez_gatt_descriptor1_get_flags(lDBusGattDesProxy);
        if (arg != NULL) {
            while (*arg != NULL) {
                BTRCORELOG_DEBUG("> Descriptor - flags - %s <\n",*arg);
                arg++;
            }
        }

        /* Value */
        var = bluez_gatt_descriptor1_get_value(lDBusGattDesProxy);
        array = g_variant_get_fixed_array (var,&n_elements,sizeof(guint8));

        for (i=0;i<n_elements;i++)
        {
            printf("%02X",array[i]);
        }

        /* Handle */
        BTRCORELOG_DEBUG("> Descriptor - Handle - %d <\n",bluez_gatt_descriptor1_get_handle(lDBusGattDesProxy));
        lDBusGattDesProxy = NULL;
    }
}
#endif

#ifdef GATT_CLIENT
/* For monitoring gatt characteristic interface signals */
static void
btrCore_BTGattCharSignalInit(
    stBtIfceHdl* pstlhBtIfce
) {
    GError *error = NULL;

    pstlhBtIfce->GattCharProxyList[pstlhBtIfce->GattCharDevCount] = bluez_gatt_characteristic1_proxy_new_sync((GDBusConnection *)pstlhBtIfce->pGDBusConn,
                G_DBUS_PROXY_FLAGS_NONE,
                BT_DBUS_BLUEZ_PATH,
                pstlhBtIfce->GattCharDevPath[pstlhBtIfce->GattCharDevCount],
                NULL,
                &error);

    if (error == NULL) {
        BTRCORELOG_INFO("Gatt characteristic proxy created succesfully \n");
    }
    else {
        BTRCORELOG_INFO("Gatt characteristic proxy creation failed %s\n",error->message);
    }

    pstlhBtIfce->GattCharSignalHandlerID[pstlhBtIfce->GattCharDevCount] = g_signal_connect(pstlhBtIfce->GattCharProxyList[pstlhBtIfce->GattCharDevCount],
                "notify",
                (GCallback)btrCore_BTGattCharPropertyChangedCb,
                pstlhBtIfce);

    if (pstlhBtIfce->GattCharSignalHandlerID[pstlhBtIfce->GattCharDevCount] == 0) {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s failed\n",pstlhBtIfce->GattCharDevPath[pstlhBtIfce->GattCharDevCount]);
    }
    else {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s \n",pstlhBtIfce->GattCharDevPath[pstlhBtIfce->GattCharDevCount]);
    }
}

/* For monitoring gatt service interface signals */

static void
btrCore_BTGattSerSignalInit(
    stBtIfceHdl* pstlhBtIfce
) {
    GError *error = NULL;

    pstlhBtIfce->GattSerProxyList[pstlhBtIfce->GattSerDevCount] = bluez_gatt_service1_proxy_new_sync((GDBusConnection *)pstlhBtIfce->pGDBusConn,
            G_DBUS_PROXY_FLAGS_NONE,
            BT_DBUS_BLUEZ_PATH,
            pstlhBtIfce->GattSerDevPath[pstlhBtIfce->GattSerDevCount],
            NULL,
            &error);

    if (error == NULL) {
        BTRCORELOG_INFO("Gatt service proxy created succesfully \n");
    }
    else {
        BTRCORELOG_INFO("Gatt service proxy creation failed - %s\n",error->message);
    }

    pstlhBtIfce->GattSerSignalHandlerID[pstlhBtIfce->GattSerDevCount] = g_signal_connect(pstlhBtIfce->GattSerProxyList[pstlhBtIfce->GattSerDevCount],
            "notify",
            (GCallback)btrCore_BTGattSerPropertyChangedCb,
            pstlhBtIfce);

    if (pstlhBtIfce->GattSerSignalHandlerID[pstlhBtIfce->GattSerDevCount] == 0) {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s failed\n",pstlhBtIfce->GattSerDevPath[pstlhBtIfce->GattSerDevCount]);
    }
    else {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s \n",pstlhBtIfce->GattSerDevPath[pstlhBtIfce->GattSerDevCount]);
    }
}

/* For monitoring gatt descriptor interface signals */

static void
btrCore_BTGattDesSignalInit(
    stBtIfceHdl* pstlhBtIfce
) {
    GError *error = NULL;

    pstlhBtIfce->GattDesProxyList[pstlhBtIfce->GattDesDevCount] = bluez_gatt_descriptor1_proxy_new_sync((GDBusConnection *)pstlhBtIfce->pGDBusConn,
            G_DBUS_PROXY_FLAGS_NONE,
            BT_DBUS_BLUEZ_PATH,
            pstlhBtIfce->GattDesDevPath[pstlhBtIfce->GattDesDevCount],
            NULL,
            &error);

    if (error == NULL) {
        BTRCORELOG_INFO("Gatt descriptor proxy created succesfully \n");
    }
    else {
        BTRCORELOG_INFO("Gatt descriptor proxy creation failed - %s\n",error->message);
    }

    pstlhBtIfce->GattDesSignalHandlerID[pstlhBtIfce->GattDesDevCount] = g_signal_connect(pstlhBtIfce->GattDesProxyList[pstlhBtIfce->GattDesDevCount],
            "notify",
            (GCallback)btrCore_BTGattDesPropertyChangedCb,
            pstlhBtIfce);

    if (pstlhBtIfce->GattDesSignalHandlerID[pstlhBtIfce->GattDesDevCount] == 0) {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s failed\n",pstlhBtIfce->GattDesDevPath[pstlhBtIfce->GattDesDevCount]);
    }
    else {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s \n",pstlhBtIfce->GattDesDevPath[pstlhBtIfce->GattDesDevCount]);
    }
}
#endif
static int
btrCore_BTParseAdapter (
    BluezAdapter1 *adap_proxy,
    stBTAdapterInfo*  apstBTAdapterInfo
) {
    int count = 0;
    gchar ** uuid = NULL;

    BTRCORELOG_INFO("Parsing adapter info\n");

    memset(apstBTAdapterInfo->pcAddress, '\0', BT_MAX_STR_LEN);
    strncpy(apstBTAdapterInfo->pcAddress, bluez_adapter1_get_address(adap_proxy), BT_MAX_STR_LEN - 1 );
    BTRCORELOG_INFO ("pcAddress                  = %s\n", apstBTAdapterInfo->pcAddress);

    memset(apstBTAdapterInfo->pcName, '\0', BT_MAX_STR_LEN);
    strncpy(apstBTAdapterInfo->pcName, bluez_adapter1_get_name(adap_proxy), BT_MAX_STR_LEN - 1 );
    BTRCORELOG_INFO ("pcName                  = %s\n", apstBTAdapterInfo->pcName);

    memset(apstBTAdapterInfo->pcAlias, '\0', BT_MAX_STR_LEN);
    strncpy(apstBTAdapterInfo->pcAlias, bluez_adapter1_get_alias(adap_proxy), BT_MAX_STR_LEN - 1 );
    BTRCORELOG_DEBUG ("pcAlias                 = %s\n", apstBTAdapterInfo->pcAlias);

    apstBTAdapterInfo->ui32Class = bluez_adapter1_get_class(adap_proxy);
    BTRCORELOG_DEBUG ("ui32Class               = %u\n", apstBTAdapterInfo->ui32Class);

    apstBTAdapterInfo->bPowered = bluez_adapter1_get_powered(adap_proxy);
    BTRCORELOG_DEBUG ("bPowered                = %d\n", apstBTAdapterInfo->bPowered);

    apstBTAdapterInfo->bDiscoverable = bluez_adapter1_get_discoverable(adap_proxy);
    BTRCORELOG_INFO ("bDiscoverable           = %d\n", apstBTAdapterInfo->bDiscoverable);

    apstBTAdapterInfo->ui32DiscoverableTimeout = bluez_adapter1_get_discoverable_timeout(adap_proxy);
    BTRCORELOG_DEBUG ("ui32DiscoverableTimeout = %u\n", apstBTAdapterInfo->ui32DiscoverableTimeout);

    apstBTAdapterInfo->bPairable = bluez_adapter1_get_pairable(adap_proxy);
    BTRCORELOG_INFO ("bPairable               = %d\n", apstBTAdapterInfo->bPairable);

    apstBTAdapterInfo->ui32PairableTimeout = bluez_adapter1_get_pairable_timeout(adap_proxy);
    BTRCORELOG_DEBUG ("ui32PairableTimeout     = %u\n", apstBTAdapterInfo->ui32PairableTimeout);

    apstBTAdapterInfo->bDiscovering = bluez_adapter1_get_discovering(adap_proxy);
    BTRCORELOG_DEBUG ("bDiscovering            = %d\n", apstBTAdapterInfo->bDiscovering);

    strncpy(apstBTAdapterInfo->pcModalias, bluez_adapter1_get_modalias(adap_proxy),BT_MAX_STR_LEN - 1 );
    BTRCORELOG_DEBUG ("pcModalias              = %s\n", apstBTAdapterInfo->pcModalias);

    uuid = bluez_adapter1_dup_uuids(adap_proxy);
    while (uuid && (uuid[count] != NULL)) {
        strncpy(apstBTAdapterInfo->ppcUUIDs[count],uuid[count],BT_MAX_UUID_STR_LEN-1);
        BTRCORELOG_DEBUG ("UUID value is %s\n", apstBTAdapterInfo->ppcUUIDs[count]);
        count++;
    }
    g_strfreev(uuid);
    return 0;
}

static int
btrCore_BTParseDevice (
        BluezDevice1 *dev_proxy,
        stBTDeviceInfo* apstBTDeviceInfo
) {
    gchar **uuid = NULL;
    int count = 0;
    //GVariant *ser_data;

    BTRCORELOG_INFO("Parsing device info \n");

    if ((dev_proxy == NULL) || (apstBTDeviceInfo == NULL)) {
        return -1;
    }

    if (bluez_device1_get_address(dev_proxy) != NULL) {
        memset(apstBTDeviceInfo->pcAddress, '\0', BT_MAX_STR_LEN);
        strncpy(apstBTDeviceInfo->pcAddress,bluez_device1_get_address(dev_proxy),BT_MAX_STR_LEN-1); //CID-163720 -Buffer not null terminated
        BTRCORELOG_INFO ("pcAddress         = %s\n", apstBTDeviceInfo->pcAddress);
    }

    if (bluez_device1_get_name(dev_proxy) != NULL) {
        memset(apstBTDeviceInfo->pcName, '\0', BT_MAX_STR_LEN);
        strncpy(apstBTDeviceInfo->pcName, bluez_device1_get_name(dev_proxy), BT_MAX_STR_LEN-1); //CID-163720 -Buffer not null terminated
        BTRCORELOG_INFO ("pcName            = %s\n", apstBTDeviceInfo->pcName);
    }

    if (bluez_device1_get_icon(dev_proxy) != NULL) {
        memset(apstBTDeviceInfo->pcIcon, '\0', BT_MAX_STR_LEN);
        strncpy(apstBTDeviceInfo->pcIcon, bluez_device1_get_icon(dev_proxy), BT_MAX_STR_LEN-1); //CID-163720 -Buffer not null terminated
        BTRCORELOG_DEBUG ("pcIcon            = %s\n", apstBTDeviceInfo->pcIcon);
    }

    apstBTDeviceInfo->ui32Class = bluez_device1_get_class(dev_proxy);
    BTRCORELOG_DEBUG ("ui32Class         = %d\n", apstBTDeviceInfo->ui32Class);

    apstBTDeviceInfo->bPaired = bluez_device1_get_paired(dev_proxy);
    BTRCORELOG_INFO ("bPaired           = %d\n", apstBTDeviceInfo->bPaired);

    apstBTDeviceInfo->bConnected = bluez_device1_get_connected(dev_proxy);
    BTRCORELOG_INFO ("bConnected        = %d\n", apstBTDeviceInfo->bConnected);

    {
        //This is telemetry log. If we change this print,need to change and configure the telemetry string in xconf server.
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "bPaired = %d", apstBTDeviceInfo->bPaired);
        telemetry_event_s("BTpair_split", buffer);
        snprintf(buffer, sizeof(buffer), "bConnected = %d", apstBTDeviceInfo->bConnected);
        telemetry_event_s("BTconn_split", buffer);
    }

    apstBTDeviceInfo->ui16Appearance = bluez_device1_get_appearance(dev_proxy);
    BTRCORELOG_INFO ("ui16Appearance    = %d\n", apstBTDeviceInfo->ui16Appearance);

    apstBTDeviceInfo->i16TxPower = bluez_device1_get_tx_power(dev_proxy);
    BTRCORELOG_DEBUG ("i16TxPower        = %d\n", apstBTDeviceInfo->i16TxPower);

    apstBTDeviceInfo->bLegacyPairing = bluez_device1_get_legacy_pairing(dev_proxy);
    BTRCORELOG_DEBUG ("bLegacyPairing    = %d\n", apstBTDeviceInfo->bLegacyPairing);

    apstBTDeviceInfo->bTrusted = bluez_device1_get_trusted(dev_proxy);
    BTRCORELOG_DEBUG ("bTrusted          = %d\n", apstBTDeviceInfo->bTrusted);

    apstBTDeviceInfo->bBlocked = bluez_device1_get_blocked(dev_proxy);
    BTRCORELOG_DEBUG ("bBlocked          = %d\n", apstBTDeviceInfo->bBlocked);

    if (bluez_device1_get_alias(dev_proxy) != NULL) {
    strncpy(apstBTDeviceInfo->pcAlias, bluez_device1_get_alias(dev_proxy), BT_MAX_STR_LEN-1); //CID-163720 -Buffer not null terminated
    BTRCORELOG_INFO ("pcAlias           = %s\n", apstBTDeviceInfo->pcAlias);
    }

    apstBTDeviceInfo->bServicesResolved = bluez_device1_get_services_resolved(dev_proxy);
    BTRCORELOG_DEBUG ("bServicesResolved = %d\n", apstBTDeviceInfo->bServicesResolved);

    apstBTDeviceInfo->i32RSSI = bluez_device1_get_rssi(dev_proxy);
    BTRCORELOG_DEBUG ("i32RSSI           = %d\n", apstBTDeviceInfo->i32RSSI);

#if 0
    strncpy(apstBTDeviceInfo->pcModalias, bluez_device1_get_modalias(dev_proxy),BT_MAX_STR_LEN - 1 );
    BTRCORELOG_INFO ("pcModalias         = %s\n", apstBTDeviceInfo->pcModalias);

    if (bluez_device1_get_adapter(dev_proxy) != NULL) {
    strncpy(apstBTDeviceInfo->pcAdapter, bluez_device1_get_adapter(dev_proxy),BT_MAX_STR_LEN - 1 );
    BTRCORELOG_INFO ("pcAdapter         = %s\n", apstBTDeviceInfo->pcAdapter);
    }
#endif

    uuid = bluez_device1_dup_uuids(dev_proxy);
    while (uuid && (uuid[count] != NULL)) {
        strncpy(apstBTDeviceInfo->aUUIDs[count],uuid[count],BT_MAX_UUID_STR_LEN-1);
        BTRCORELOG_DEBUG("UUID value is %s\n", apstBTDeviceInfo->aUUIDs[count]);
        count++;
    }
    g_strfreev(uuid);

#if 0
    ser_data = bluez_device1_get_service_data(dev_proxy);
    GVariantIter i;
    const char *uuid_str = NULL;
    count = 0;
    g_variant_iter_init(&i,ser_data);
    while (g_variant_iter_next(&i,"s",&uuid_str)) {
        strncpy(apstBTDeviceInfo->saServices[count].pcUUIDs, uuid_str, (BT_MAX_UUID_STR_LEN - 1));
        BTRCORELOG_INFO ("UUID value is %s\n", apstBTDeviceInfo->saServices[count].pcUUIDs);
        count++;
    }

    count = 0;
    uint8_t *data = NULL;
    GVariantIter ii;
    while (g_variant_iter_next(&ii,"a",&data)) {
        memcpy(apstBTDeviceInfo->saServices[count].pcData,data,BT_MAX_SERVICE_DATA_LEN-1);
        BTRCORELOG_INFO ("pcData is %d\n", apstBTDeviceInfo->saServices[count].pcData[count]);
        count++;
    }
#endif

    if (strlen(apstBTDeviceInfo->pcAlias))
        memcpy(apstBTDeviceInfo->pcName, apstBTDeviceInfo->pcAlias, BT_MAX_STR_LEN-1);

    return 0;
}

static int
btrCore_BTGetDeviceInfo (
    stBtIfceHdl*        apstlhBtIfce,
    stBTDeviceInfo*     apstBTDeviceInfo,
    const char*         apcIface
) {
    stBtIfceHdl* pstlhBtIfce = (stBtIfceHdl*) apstlhBtIfce;
    BluezDevice1 *lDBusDevProxy = NULL;
    int index = 0;

    while (index < pstlhBtIfce->DevCount) {
        if (strcmp(pstlhBtIfce->DevPathInfo[index],apcIface) == 0) {
            break;
        }
        index++;
    }

    lDBusDevProxy = pstlhBtIfce->DevProxyList[index];

    if (lDBusDevProxy != NULL)
    {
        if (0 != btrCore_BTParseDevice(lDBusDevProxy,apstBTDeviceInfo)) {
            BTRCORELOG_ERROR ("Parsing the device %s failed..\n", apcIface);
            return -1;
        }
        else {
            memcpy(apstBTDeviceInfo->pcDevicePath, apcIface, (strlen(apcIface) < BT_MAX_DEV_PATH_LEN) ? strlen(apcIface) : BT_MAX_DEV_PATH_LEN - 1 );
            return 0;
        }
    }

    lDBusDevProxy = NULL;

    if (!apcIface)
        return 0;

    return 0;
}

/* For monitoring device interface signals */

static void
btrCore_BTDevSignalInit(
    stBtIfceHdl* pstlhBtIfce
) {
    GError *error = NULL;

    pstlhBtIfce->DevProxyList[pstlhBtIfce->DevCount] = bluez_device1_proxy_new_sync(pstlhBtIfce->pGDBusConn,
            G_DBUS_PROXY_FLAGS_NONE,
            BT_DBUS_BLUEZ_PATH,
            pstlhBtIfce->DevPathInfo[pstlhBtIfce->DevCount],
            NULL,
            &error);

    if (error == NULL) {
        BTRCORELOG_INFO("Device proxy created succesfully \n");
    }
    else {
        BTRCORELOG_INFO("Device service proxy creation failed - %s \n",error->message);
    }

    pstlhBtIfce->DevSignalHandlerID[pstlhBtIfce->DevCount] = g_signal_connect (G_OBJECT(pstlhBtIfce->DevProxyList[pstlhBtIfce->DevCount]),
            "notify",
            (GCallback)btrCore_bluezSignalDeviceChangedCb,
            pstlhBtIfce);


    if (pstlhBtIfce->DevSignalHandlerID[pstlhBtIfce->DevCount] == 0) {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s failed\n",pstlhBtIfce->DevPathInfo[pstlhBtIfce->DevCount]);
    }
    else {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s \n",pstlhBtIfce->DevPathInfo[pstlhBtIfce->DevCount]);
    }
}

/* Interfaces */
void*
BtrCore_BTInitGetConnection (
    void
) {
    stBtIfceHdl*    pstlhBtIfce = NULL;
    GDBusConnection *lcon = NULL;
    guint lIfceRmved = 0,lIfceAdded = 0,lAdapPropChanged = 0;
    GError *error = NULL;
    GMainLoop*      pMainLoop       = NULL;
    GThread*        pMainLoopThread = NULL;
    int i;

    lcon = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    pstlhBtIfce = (stBtIfceHdl*)malloc(sizeof(stBtIfceHdl));

    if (!pstlhBtIfce)
        return NULL;


    pstlhBtIfce->pGDBusConn                     = NULL;
    pstlhBtIfce->pcBTAgentPath                  = NULL;
    pstlhBtIfce->pcBTDAdapterPath               = NULL;
    pstlhBtIfce->pcBTDAdapterAddr               = NULL;
    pstlhBtIfce->pcBTAdapterPath                = NULL;
    pstlhBtIfce->pcDevTransportPath             = NULL;

    pstlhBtIfce->pcBTOutPassCode                = NULL;

    pstlhBtIfce->i32DoReject                    = 1;

    pstlhBtIfce->ui32cBConnAuthPassKey          = 0;
    pstlhBtIfce->ui32DevLost                    = 0;

    pstlhBtIfce->ui32IsAdapterDiscovering       = 0;
    memset(pstlhBtIfce->BatteryOTAControlPath,     '\0', sizeof(char) * BT_MAX_STR_LEN);

    memset(pstlhBtIfce->pcBTVersion,        '\0', sizeof(char) * BT_MAX_STR_LEN);
    memset(pstlhBtIfce->pcDeviceCurrState,  '\0', sizeof(char) * BT_MAX_STR_LEN);
    memset(pstlhBtIfce->pcLeDeviceCurrState,'\0', sizeof(char) * BT_MAX_STR_LEN);
    memset(pstlhBtIfce->pcLeDeviceAddress,  '\0', sizeof(char) * BT_MAX_STR_LEN);
    memset(pstlhBtIfce->pcMediaCurrState,   '\0', sizeof(char) * BT_MAX_STR_LEN);
    memset(pstlhBtIfce->BatteryOTAControlPath, '\0', sizeof(char) * BT_MAX_STR_LEN);

    memset(&pstlhBtIfce->lstBTAdapterInfo, 0, sizeof(stBTAdapterInfo));
    memset(&pstlhBtIfce->lstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));
    memset(&pstlhBtIfce->lstBTMediaInfo, 0, sizeof(stBTMediaInfo));

    memset(&pstlhBtIfce->stBTLeGatt, 0, sizeof(stBTLeGattInfo));
    memset(&pstlhBtIfce->stBTLeCustAdvt, 0, sizeof(stBTLeCustomAdv));
    memset(pstlhBtIfce->pcBTAdapterGattSrvEpPath, '\0', sizeof(char) * BT_MAX_DEV_PATH_LEN);
    for (i=0; i<BT_MAX_DEVICES ;i++) {
        memset(pstlhBtIfce->DevPathInfo[i],'\0',sizeof(pstlhBtIfce->DevPathInfo[i]));
    }

    for (i=0; i<BT_MAX_NUM_GATT_CHAR ;i++) {
        memset(pstlhBtIfce->GattCharDevPath[i],'\0',sizeof(pstlhBtIfce->GattCharDevPath[i]));
    }

    for (i=0; i<BT_MAX_NUM_GATT_SERVICE ;i++) {
        memset(pstlhBtIfce->GattSerDevPath[i],'\0',sizeof(pstlhBtIfce->GattSerDevPath[i]));
    }

    for (i=0; i<BT_MAX_NUM_GATT_DESC ;i++) {
        memset(pstlhBtIfce->GattDesDevPath[i],'\0',sizeof(pstlhBtIfce->GattDesDevPath[i]));
    }

    strncpy(pstlhBtIfce->pcDeviceCurrState,   "disconnected", BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcLeDeviceCurrState, "disconnected", BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcLeDeviceAddress,   "none",         BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcMediaCurrState,    "none",         BT_MAX_STR_LEN - 1);

    /* For now added for hci0, In future we will add for hci1,hci2 & hci3 */
    strncpy(pstlhBtIfce->AdapPathInfo[0], "/org/bluez/hci0",  BT_MAX_DEV_PATH_LEN);

    pstlhBtIfce->pcBAdapterStatusUserData       = NULL;
    pstlhBtIfce->pcBDevStatusUserData           = NULL;
    pstlhBtIfce->pcBMediaStatusUserData         = NULL;
    pstlhBtIfce->pcBNegMediaUserData            = NULL;
    pstlhBtIfce->pcBTransPathMediaUserData      = NULL;
    pstlhBtIfce->pcBMediaPlayerPathUserData     = NULL;
    pstlhBtIfce->pcBConnIntimUserData           = NULL;
    pstlhBtIfce->pcBConnAuthUserData            = NULL;
    pstlhBtIfce->pcBLePathUserData              = NULL;

    pstlhBtIfce->fpcBAdapterStatusUpdate        = NULL;
    pstlhBtIfce->fpcBDevStatusUpdate            = NULL;
    pstlhBtIfce->fpcBMediaStatusUpdate          = NULL;
    pstlhBtIfce->fpcBNegotiateMedia             = NULL;
    pstlhBtIfce->fpcBTransportPathMedia         = NULL;
    pstlhBtIfce->fpcBTMediaPlayerPath           = NULL;
    pstlhBtIfce->fpcBConnectionIntimation       = NULL;
    pstlhBtIfce->fpcBConnectionAuthentication   = NULL;
    pstlhBtIfce->fpcBTLeGattPath                = NULL;
    pstlhBtIfce->GattCharDevCount = 0;
    pstlhBtIfce->DevCount = 0;
    pstlhBtIfce->GattSerDevCount = 0;
    pstlhBtIfce->GattDesDevCount = 0;
    pstlhBtIfce->GattCharNotifyDevCount = 0;
    pstlhBtIfce->BatteryFirmFilep = NULL;
    pstlhBtIfce->BatteryDevProxy = NULL;

    BTRCORELOG_INFO ("GDBus Debug DBus Connection Name %s\n", g_dbus_connection_get_unique_name (lcon));
    pstlhBtIfce->pGDBusConn = lcon;


    pstlhBtIfce->DBusObjManProxy = dbus_object_manager_proxy_new_sync(pstlhBtIfce->pGDBusConn,
            G_DBUS_PROXY_FLAGS_NONE,
            BT_DBUS_BLUEZ_PATH,
            "/",
            NULL,
            &error);

    lIfceAdded = g_signal_connect(G_OBJECT(pstlhBtIfce->DBusObjManProxy),
            "interfaces-added",
            (GCallback)btrCore_bluezDeviceAppearedCb,
            pstlhBtIfce);

    if (lIfceAdded == 0) {
        BTRCORELOG_INFO ("Connect with signal - InterfacesAdded failed\n");
    }
    else {
        BTRCORELOG_INFO ("Connected with signal - InterfacesAdded\n");
    }

    lIfceRmved = g_signal_connect(G_OBJECT(pstlhBtIfce->DBusObjManProxy),
            "interfaces-removed",
            (GCallback)btrCore_bluezDeviceDisappearedCb,
            pstlhBtIfce);

    if (lIfceRmved == 0) {
        BTRCORELOG_INFO ("Connect with signal - InterfacesRemoved failed\n");
    }
    else {
        BTRCORELOG_INFO ("Connected with signal - InterfacesRemoved\n");
    }

    /* For now added for hci0, In future will add for hci1,hci2 & hci3 */

    pstlhBtIfce->DBusAdapProxy = bluez_adapter1_proxy_new_sync(pstlhBtIfce->pGDBusConn,
            G_DBUS_PROXY_FLAGS_NONE,
            BT_DBUS_BLUEZ_PATH,
            pstlhBtIfce->AdapPathInfo[0],
            NULL,
            &error);

    lAdapPropChanged = g_signal_connect(G_OBJECT(pstlhBtIfce->DBusAdapProxy),
            "notify",
            (GCallback)btrCore_bluezSignalAdatperChangedCb,
            pstlhBtIfce);

    if (lAdapPropChanged == 0) {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s failed\n",BT_DBUS_BLUEZ_ADAPTER_PATH);
    }
    else {
        BTRCORELOG_INFO ("Registered for signal - PropertiesChanged on %s \n",BT_DBUS_BLUEZ_ADAPTER_PATH);
    }

    /* Allocating dynamic memory to store the GATT server characteristic objects */
    gatt_char_obj_init();
    pMainLoop   = g_main_loop_new (NULL, FALSE);
    gpvMainLoop = (void*)pMainLoop;

    pMainLoopThread   = g_thread_new("btrCore_g_main_loop_Task", btrCore_g_main_loop_Task, gpvMainLoop);
    gpvMainLoopThread = (void*)pMainLoopThread;
    if ((pMainLoop == NULL) || (pMainLoopThread == NULL)) {
        BTRCORELOG_ERROR ("Could not initialize g_main module\n");
        BtrCore_BTDeInitReleaseConnection(pstlhBtIfce);
        return 0;
    }
    return (void*)pstlhBtIfce;
}

int
BtrCore_BTDeInitReleaseConnection (
    void* apstBtIfceHdl
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    int i;

    if (!apstBtIfceHdl)
        return -1;

    if (gpvMainLoop) {
        g_main_loop_quit(gpvMainLoop);
    }

    if (gpvMainLoopThread) {
        g_thread_join(gpvMainLoopThread);
        gpvMainLoopThread = NULL;
    }

    if (gpvMainLoop) {
        g_main_loop_unref(gpvMainLoop);
        gpvMainLoop = NULL;
    }

    if (pstlhBtIfce->pcBTAgentPath) {
        free(pstlhBtIfce->pcBTAgentPath);
        pstlhBtIfce->pcBTAgentPath = NULL;
    }

    if (pstlhBtIfce->pcBTDAdapterAddr) {
        free(pstlhBtIfce->pcBTDAdapterAddr);
        pstlhBtIfce->pcBTDAdapterAddr = NULL;
    }

    if (pstlhBtIfce->pcBTDAdapterPath) {
        free(pstlhBtIfce->pcBTDAdapterPath);
        pstlhBtIfce->pcBTDAdapterPath = NULL;
    }

    if (pstlhBtIfce->pcBTAdapterPath) {
        free(pstlhBtIfce->pcBTAdapterPath);
        pstlhBtIfce->pcBTAdapterPath = NULL;
    }

    gatt_char_obj_release();

    pstlhBtIfce->pcBLePathUserData              = NULL;
    pstlhBtIfce->pcBConnAuthUserData            = NULL;
    pstlhBtIfce->pcBConnIntimUserData           = NULL;
    pstlhBtIfce->pcBMediaPlayerPathUserData     = NULL;
    pstlhBtIfce->pcBTransPathMediaUserData      = NULL;
    pstlhBtIfce->pcBNegMediaUserData            = NULL;
    pstlhBtIfce->pcBMediaStatusUserData         = NULL;
    pstlhBtIfce->pcBDevStatusUserData           = NULL;
    pstlhBtIfce->pcBAdapterStatusUserData       = NULL;


    pstlhBtIfce->fpcBTLeGattPath                = NULL;
    pstlhBtIfce->fpcBConnectionAuthentication   = NULL;
    pstlhBtIfce->fpcBConnectionIntimation       = NULL;
    pstlhBtIfce->fpcBTMediaPlayerPath           = NULL;
    pstlhBtIfce->fpcBTransportPathMedia         = NULL;
    pstlhBtIfce->fpcBNegotiateMedia             = NULL;
    pstlhBtIfce->fpcBMediaStatusUpdate          = NULL;
    pstlhBtIfce->fpcBDevStatusUpdate            = NULL;
    pstlhBtIfce->fpcBAdapterStatusUpdate        = NULL;


    pstlhBtIfce->ui32IsAdapterDiscovering       = 0;
    pstlhBtIfce->ui32DevLost                    = 0;
    pstlhBtIfce->ui32cBConnAuthPassKey          = 0;
    pstlhBtIfce->i32DoReject                    = 0;


    for (i=0; i<pstlhBtIfce->DevCount ;i++) {
        if(pstlhBtIfce->DevProxyList[i]) {
           g_signal_handler_disconnect(pstlhBtIfce->DevProxyList[i],
                   pstlhBtIfce->DevSignalHandlerID[i]);
           g_object_unref(pstlhBtIfce->DevProxyList[i]);
           pstlhBtIfce->DevProxyList[i] = NULL;
        }
        memset(pstlhBtIfce->DevPathInfo[i],'\0',sizeof(pstlhBtIfce->DevPathInfo[i]));
    }

    for (i=0; i<pstlhBtIfce->GattCharDevCount ;i++) {
        if(pstlhBtIfce->GattCharProxyList[i]) {
           g_signal_handler_disconnect(pstlhBtIfce->GattCharProxyList[i],
                   pstlhBtIfce->GattCharSignalHandlerID[i]);
           g_object_unref(pstlhBtIfce->GattCharProxyList[i]);
           pstlhBtIfce->GattCharProxyList[i] = NULL;
        }
        memset(pstlhBtIfce->GattCharDevPath[i],'\0',sizeof(pstlhBtIfce->GattCharDevPath[i]));
    }

    for (i=0; i<pstlhBtIfce->GattSerDevCount ;i++) {
        if(pstlhBtIfce->GattSerProxyList[i]) {
           g_signal_handler_disconnect(pstlhBtIfce->GattSerProxyList[i],
                   pstlhBtIfce->GattSerSignalHandlerID[i]);
           g_object_unref(pstlhBtIfce->GattSerProxyList[i]);
           pstlhBtIfce->GattSerProxyList[i] = NULL;
        }
        memset(pstlhBtIfce->GattSerDevPath[i],'\0',sizeof(pstlhBtIfce->GattSerDevPath[i]));
    }

    for (i=0; i<pstlhBtIfce->GattDesDevCount ;i++) {
        if(pstlhBtIfce->GattDesProxyList[i]) {
           g_signal_handler_disconnect(pstlhBtIfce->GattDesProxyList[i],
                   pstlhBtIfce->GattDesSignalHandlerID[i]);
           g_object_unref(pstlhBtIfce->GattDesProxyList[i]);
           pstlhBtIfce->GattDesProxyList[i] = NULL;
        }
        memset(pstlhBtIfce->GattDesDevPath[i],'\0',sizeof(pstlhBtIfce->GattDesDevPath[i]));
    }

    g_object_unref(pstlhBtIfce->DBusObjManProxy);
    pstlhBtIfce->DBusObjManProxy = NULL;
    g_object_unref(pstlhBtIfce->DBusAdapProxy);
    pstlhBtIfce->DBusAdapProxy  = NULL;
    g_object_unref(pstlhBtIfce->pGDBusConn);
    pstlhBtIfce->pGDBusConn = NULL;

    free(pstlhBtIfce);
    pstlhBtIfce = NULL;


    return 0;
}


char*
BtrCore_BTGetAgentPath (
    void* apstBtIfceHdl
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    char            lDefaultBTPath[BT_MAX_DEV_PATH_LEN] = {'\0'};

    if (!apstBtIfceHdl)
        return NULL;


    snprintf(lDefaultBTPath, sizeof(lDefaultBTPath), "/org/bluez/agent_%d", getpid());

    if (pstlhBtIfce->pcBTAgentPath) {
        free(pstlhBtIfce->pcBTAgentPath);
        pstlhBtIfce->pcBTAgentPath = NULL;
    }

    pstlhBtIfce->pcBTAgentPath = strdup(lDefaultBTPath);
    BTRCORELOG_INFO ("Agent Path: %s\n", pstlhBtIfce->pcBTAgentPath);
    return pstlhBtIfce->pcBTAgentPath;
}


int
BtrCore_BTReleaseAgentPath (
    void* apstBtIfceHdl
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl)
        return -1;


    if (pstlhBtIfce->pcBTAgentPath) {
        free(pstlhBtIfce->pcBTAgentPath);
        pstlhBtIfce->pcBTAgentPath = NULL;
    }

    return 0;
}


int
BtrCore_BTRegisterAgent (
    void*       apstBtIfceHdl,
    const char* apBtAdapter,
    const char* apBtAgentPath,
    const char* capabilities
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl)
        return -1;


    BTRCORELOG_INFO("adapter path: %s, agent path: %s\n", apBtAdapter, apBtAgentPath);
    bluezAgentHelperRegister(apBtAdapter, apBtAgentPath, capabilities, &agtIfaceCb, pstlhBtIfce);

    return 0;
}


int
BtrCore_BTUnregisterAgent (
    void*       apstBtIfceHdl,
    const char* apBtAdapter,
    const char* apBtAgentPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl)
        return -1;

    /* UnregisterAgent in bluez5.54 internal it is setting Pairable false.
     * due to this pairing is failing, to avoid this implicitly setting
     * Pairable true here */
    if (!strncmp(pstlhBtIfce->pcBTVersion, BT_BLUEZ_VERSION_5_77, strlen(BT_BLUEZ_VERSION_5_77)) || !strncmp(pstlhBtIfce->pcBTVersion, BT_BLUEZ_VERSION_5_54, strlen(BT_BLUEZ_VERSION_5_54)) || !strncmp(pstlhBtIfce->pcBTVersion, BT_BLUEZ_VERSION_5_48, strlen(BT_BLUEZ_VERSION_5_48)))
    {
        unBTOpIfceProp  lunBtOpAdapProp;
        int             isPairable = 1;
        lunBtOpAdapProp.enBtAdapterProp = enBTAdPropPairable;
        if (BtrCore_BTSetProp(apstBtIfceHdl, apBtAdapter, enBTAdapter, lunBtOpAdapProp, &isPairable)) {
            BTRCORELOG_ERROR ("Set Adapter Property enBTAdPropPairable - FAILED\n");
            return -1;
        }
        bluezAgentHelperUnregister(apBtAdapter, apBtAgentPath);
    }

    return 0;
}


int
BtrCore_BTGetAdapterList (
    void*           apstBtIfceHdl,
    unsigned int*   apBtNumAdapters,
    char*           apcArrBtAdapterPath[BT_MAX_NUM_ADAPTERS]
 ) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    char*           adapter_path = NULL;
    char            object_path[BT_MAX_DEV_PATH_LEN] = {'\0'};
    char            paths[BT_MAX_NUM_ADAPTERS][BT_MAX_DEV_PATH_LEN] = {'\0'};
    GVariantIter    i,ii;
    const gchar*    interface_name = NULL;
    GVariant*       properties = NULL,*lOutObjects = NULL,*ifaces_and_properties = NULL;
    int             d = 0,c = 0,num = -1,index = 0,rc = -1;
    gboolean        result;
    GError          *error = NULL;

    if (pstlhBtIfce->DBusObjManProxy != NULL) {
        result = dbus_object_manager_call_get_managed_objects_sync((DBusObjectManager *)pstlhBtIfce->DBusObjManProxy,
                &lOutObjects,
                NULL,
                &error);
    }
    else {
        BTRCORELOG_ERROR("Proxy not available to read managed objects \n");
        return rc;
    }

    if (result != TRUE) {
        BTRCORELOG_ERROR("Failed to get the managed objects \n");
    }

    for (index = 0; index < BT_MAX_NUM_ADAPTERS; index++)
        memset (&paths[index], '\0',  BT_MAX_DEV_PATH_LEN);

    memset(object_path, '\0', BT_MAX_DEV_PATH_LEN);

    if (lOutObjects != NULL) {
        g_variant_iter_init(&i,lOutObjects);
        while(g_variant_iter_next(&i, "{&o@a{sa{sv}}}", &adapter_path, &ifaces_and_properties)) {
            memcpy(object_path, adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
            g_variant_iter_init(&ii, ifaces_and_properties);
            while(g_variant_iter_next(&ii, "{&s@a{sv}}", &interface_name, &properties)) {
                if (strcmp(interface_name, BT_DBUS_BLUEZ_ADAPTER_PATH) == 0) {
                    memcpy(&paths[d][0], adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
                    ++d;
                }
                g_variant_unref(properties);
            }
            g_variant_unref(ifaces_and_properties);
        }
        g_variant_unref(lOutObjects);
    }
    num = d;
    if (apBtNumAdapters && apcArrBtAdapterPath) {
        *apBtNumAdapters = num;
        for (c = 0; c < num; c++) {
            if (apcArrBtAdapterPath[c]) {
                if (*(apcArrBtAdapterPath[c]) == '\0') {
                    BTRCORELOG_DEBUG ("Adapter Path %d is: %s\n", c, &paths[c][0]);
                    memcpy(apcArrBtAdapterPath[c], &paths[c][0], (strlen(&paths[c][0]) < BT_MAX_DEV_PATH_LEN) ? strlen(&paths[c][0]) : BT_MAX_DEV_PATH_LEN - 1);
                    rc = 0;
                }
                else {
                    BTRCORELOG_DEBUG ("Adapter Path %d is: %s\n", c, &paths[c][0]);
                    BTRCORELOG_DEBUG ("Existing Adapter Path is: %s\n", apcArrBtAdapterPath[c]);
                    rc = 0;
                }
            }
        }
    }

    return rc;
}

char*
BtrCore_BTGetAdapterPath (
    void*       apstBtIfceHdl,
    const char* apBtAdapter
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    char*           pui8DefAdapterPath = NULL;
    char*           pui8AdapterPath1 = "/org/bluez/hci0";
    char*           pui8AdapterPath2 = "/org/bluez/hci1";
    char*           pui8AdapterPath3 = "/org/bluez/hci2";
    char*           bt1 = "hci0";
    char*           bt2 = "hci1";
    char*           bt3 = "hci2";

    if (!apstBtIfceHdl)
        return NULL;


    if (!apBtAdapter) {
        if (!pstlhBtIfce->pcBTAdapterPath && ((pui8DefAdapterPath = btrCore_BTGetDefaultAdapterPath(pstlhBtIfce)) != NULL)) {
            BTRCORELOG_WARN ("Got Default Adapter Path is : %s - %p\n", pui8DefAdapterPath, pui8DefAdapterPath);
            pstlhBtIfce->pcBTAdapterPath = strndup(pui8DefAdapterPath, strlen(pui8DefAdapterPath));
        }
    }
    else if (strcmp(apBtAdapter, bt1) == 0) {
        if (!pstlhBtIfce->pcBTAdapterPath) {
            pstlhBtIfce->pcBTAdapterPath = strndup(pui8AdapterPath1, strlen(pui8AdapterPath1));
        }
    }
    else if (strcmp(apBtAdapter, bt2) == 0) {
        if (!pstlhBtIfce->pcBTAdapterPath) {
            pstlhBtIfce->pcBTAdapterPath = strndup(pui8AdapterPath2, strlen(pui8AdapterPath2));
        }
    }
    else if (strcmp(apBtAdapter, bt3) == 0) {
        if (!pstlhBtIfce->pcBTAdapterPath) {
            pstlhBtIfce->pcBTAdapterPath = strndup(pui8AdapterPath3, strlen(pui8AdapterPath3));
        }
    }
    else {
        if (!pstlhBtIfce->pcBTAdapterPath && ((pui8DefAdapterPath = btrCore_BTGetDefaultAdapterPath(pstlhBtIfce)) != NULL)) {
            pstlhBtIfce->pcBTAdapterPath = strndup(pui8DefAdapterPath, strlen(pui8DefAdapterPath));
        }
    }


    BTRCORELOG_WARN ("Adapter Path is : %s - %p\n", pstlhBtIfce->pcBTAdapterPath, pstlhBtIfce->pcBTAdapterPath);
    return pstlhBtIfce->pcBTAdapterPath;
}


int
BtrCore_BTReleaseAdapterPath (
    void*       apstBtIfceHdl,
    const char* apBtAdapter
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl)
        return -1;


    if (!apBtAdapter) {
        btrCore_BTReleaseDefaultAdapterPath(pstlhBtIfce);
    }

    if (pstlhBtIfce->pcBTAdapterPath) {

        BTRCORELOG_WARN ("Adapter Path is : %s - %p\n", pstlhBtIfce->pcBTAdapterPath, pstlhBtIfce->pcBTAdapterPath);
        if (pstlhBtIfce->pcBTAdapterPath != apBtAdapter) {
            BTRCORELOG_ERROR ("ERROR: Looks like Adapter path has been changed by User\n");
        }

        free(pstlhBtIfce->pcBTAdapterPath);
        pstlhBtIfce->pcBTAdapterPath = NULL;
    }

    BTRCORELOG_WARN ("Freed Adapter\n");

    return 0;
}


int
BtrCore_BTGetIfceNameVersion (
    void* apstBtIfceHdl,
    char* apBtOutIfceName,
    char* apBtOutVersion
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    FILE*           lfpVersion = NULL;
    char            lcpVersion[BT_MAX_STR_LEN] = {'\0'};

    if (!apstBtIfceHdl || !apBtOutIfceName || !apBtOutVersion)
        return -1;


    (void)pstlhBtIfce;

    /* strncpy() throws format truncation error on gcc 9.x when same length of src string
       is given in 3rd argument. Also, strncpy() will fill null terminating characters if
       3rd argument length is more than the length of src string
       */
    lfpVersion = popen("/usr/lib/bluez5/bluetooth/bluetoothd --version", "r");
    if ((lfpVersion == NULL)) {
        BTRCORELOG_ERROR ("Failed to run Version command\n");
        strncpy(lcpVersion, "5.XXX", strlen("5.XXX") + 1);
    }
    else {
        do {
            if (fgets(lcpVersion, sizeof(lcpVersion)-1, lfpVersion) == NULL) {
                BTRCORELOG_ERROR ("Failed to Valid Version\n");
                strncpy(lcpVersion, "5.XXX", sizeof(lcpVersion));
                lcpVersion[sizeof(lcpVersion) - 1] = '\0';
            }
        } while (strstr(lcpVersion, "breakpad") || strstr(lcpVersion, "Breakpad"));

        pclose(lfpVersion);
    }


    strncpy(apBtOutIfceName, "Bluez", strlen("Bluez") + 1);
    memcpy(apBtOutVersion, lcpVersion, strlen(lcpVersion) + 1);
    memcpy(pstlhBtIfce->pcBTVersion, lcpVersion, strlen(lcpVersion));

    BTRCORELOG_DEBUG ("Bluez Version - %s\n", apBtOutVersion);

    return 0;
}


int
BtrCore_BTGetProp (
    void*               apstBtIfceHdl,
    const char*         apcBtOpIfcePath,
    enBTOpIfceType      aenBtOpIfceType,
    unBTOpIfceProp      aunBtOpIfceProp,
    void*               apvVal
) {
    stBtIfceHdl*        pstlhBtIfce              = (stBtIfceHdl*)apstBtIfceHdl;
    int                 rc                       = 0;
    int                 i32StoreDefAdpAddr       = 0;
    BluezDevice1        *lDBusDevProxy           = NULL;
    BluezGattService1   *lDBusGattSerProxy       = NULL;
    BluezGattCharacteristic1 *lDBusGattCharProxy = NULL;
    BluezGattDescriptor1 *lDBusGattDesProxy      = NULL;
    int                  index                   = 0;
    gboolean             PathFound               = false;

    if (!apstBtIfceHdl)
        return -1;

    if (aenBtOpIfceType == enBTAdapter) {

        if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropName) {
            char str[BT_MAX_STR_LEN];
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str,bluez_adapter1_get_alias(pstlhBtIfce->DBusAdapProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropAddress) {
            char str[BT_MAX_STR_LEN];
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str,bluez_adapter1_get_address(pstlhBtIfce->DBusAdapProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
            if (i32StoreDefAdpAddr && !pstlhBtIfce->pcBTDAdapterAddr) {
                pstlhBtIfce->pcBTDAdapterAddr = strndup(str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
            }
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropPowered) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_adapter1_get_powered(pstlhBtIfce->DBusAdapProxy);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropDiscoverable) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_adapter1_get_discoverable(pstlhBtIfce->DBusAdapProxy);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropDiscoverableTimeOut) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_adapter1_get_discoverable_timeout(pstlhBtIfce->DBusAdapProxy);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp ==  enBTAdPropPairable) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_adapter1_get_pairable(pstlhBtIfce->DBusAdapProxy);
        }
        else {
            BTRCORELOG_ERROR ("Invalid Adapter Property\n");
            return -1;
        }
    }

    if (aenBtOpIfceType == enBTDevice) {

        while (index < pstlhBtIfce->DevCount) {
            if (strcmp(pstlhBtIfce->DevPathInfo[index],apcBtOpIfcePath) == 0) {
                PathFound = true;
                break;
            }
            index++;
        }

        if (PathFound != true) {
            return -1;
        }

        lDBusDevProxy = pstlhBtIfce->DevProxyList[index];

        if (aunBtOpIfceProp.enBtDeviceProp == enBTDevPropPaired) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_device1_get_paired(lDBusDevProxy);
        }
        else if (aunBtOpIfceProp.enBtDeviceProp == enBTDevPropConnected) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_device1_get_connected(lDBusDevProxy);
        }
        else if (aunBtOpIfceProp.enBtDeviceProp == enBTDevPropSrvRslvd) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_device1_get_services_resolved(lDBusDevProxy);
        }
        else {
            BTRCORELOG_ERROR ("Invalid Adapter Property\n");
            return -1;

        }
        lDBusDevProxy = NULL;
    }

    if (aenBtOpIfceType == enBTGattCharacteristic) {

        while (index < pstlhBtIfce->GattCharDevCount) {
            if (strcmp(pstlhBtIfce->GattCharDevPath[index],apcBtOpIfcePath) == 0) {
                PathFound = true;
                break;
            }
            index++;
        }

        lDBusGattCharProxy = pstlhBtIfce->GattCharProxyList[index];

        if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropUUID) {
            char str[BT_MAX_STR_LEN];
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str,bluez_gatt_characteristic1_get_uuid(lDBusGattCharProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
        }
        else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropService) {
            char str[BT_MAX_STR_LEN];
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str,bluez_gatt_characteristic1_get_service(lDBusGattCharProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
        }
        else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropValue) {
            const guint8 *array = NULL;
            gsize n_elements;
            int i;
            char byteValue[BT_MAX_STR_LEN] = "\0";
            char hexValue[] = "0123456789abcdef";
            unsigned short u16idx = 0;
            GVariant *lOutObject = NULL;

            lOutObject = bluez_gatt_characteristic1_get_value(lDBusGattCharProxy);

            array = g_variant_get_fixed_array (lOutObject,&n_elements,sizeof(guint8));

            for (i=0; i<n_elements ;i++)
            {
                byteValue[u16idx++] = hexValue[array[i] >> 4];
                byteValue[u16idx++] = hexValue[array[i] &  0x0F];
            }
            byteValue[u16idx] = '\0';
            memcpy (apvVal, byteValue, BT_MAX_STR_LEN-1);
            g_variant_unref(lOutObject);

        }
        else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropNotifying) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_gatt_characteristic1_get_notifying(lDBusGattCharProxy);
        }
        else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropFlags) {
            gchar **arg = NULL,**p;
            char (*ptr)[BT_MAX_UUID_STR_LEN] = (char (*)[BT_MAX_UUID_STR_LEN])apvVal;
            int index = 0;
            arg = bluez_gatt_characteristic1_dup_flags(lDBusGattCharProxy);

            p = arg;
            if (arg != NULL) {
                while (*arg != NULL) {
                    strncpy (ptr[index++], *arg, BT_MAX_UUID_STR_LEN - 1);
                    arg++;
                }
            }
            g_strfreev(p);
        }
        lDBusGattCharProxy = NULL;
    }

    if (aenBtOpIfceType == enBTGattService) {

       while (index < pstlhBtIfce->GattSerDevCount) {
            if (strcmp(pstlhBtIfce->GattSerDevPath[index],apcBtOpIfcePath) == 0) {
                PathFound = true;
                break;
            }
            index++;
        }

        if (PathFound != true) {
            return -1;
        }

        lDBusGattSerProxy = pstlhBtIfce->GattSerProxyList[index];

        if (aunBtOpIfceProp.enBtGattServiceProp == enBTGattSPropUUID) {
            char str[BT_MAX_STR_LEN] ;
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str,bluez_gatt_service1_get_uuid(lDBusGattSerProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
        }
        else if (aunBtOpIfceProp.enBtGattServiceProp == enBTGattSPropPrimary) {
            int* ptr = (int*) apvVal;
            *ptr = bluez_gatt_service1_get_primary(lDBusGattSerProxy);
        }
        else if (aunBtOpIfceProp.enBtGattServiceProp == enBTGattSPropDevice) {
            char str[BT_MAX_STR_LEN] = {'\0'};
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str,bluez_gatt_service1_get_device(lDBusGattSerProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
        }
    }

    if (aenBtOpIfceType == enBTGattDescriptor) {

        while (index < pstlhBtIfce->GattDesDevCount) {
            if (strcmp(pstlhBtIfce->GattDesDevPath[index],apcBtOpIfcePath) == 0) {
                PathFound = true;
                break;
            }
            index++;
        }

        if (PathFound != true) {
            BTRCORELOG_INFO("Invalid property query\n");
            return -1;
        }

        lDBusGattDesProxy = pstlhBtIfce->GattDesProxyList[index];

        if (aunBtOpIfceProp.enBtGattDescProp == enBTGattDPropUUID) {
            char str[BT_MAX_STR_LEN];
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str, bluez_gatt_descriptor1_get_uuid(lDBusGattDesProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
        }
        else if (aunBtOpIfceProp.enBtGattDescProp == enBTGattDPropCharacteristic) {
            char str[BT_MAX_STR_LEN];
            memset(str,'\0',BT_MAX_STR_LEN);
            strncpy(str,bluez_gatt_descriptor1_get_characteristic(lDBusGattDesProxy), BT_MAX_STR_LEN - 1);
            memcpy(apvVal, str, strlen(str) < BT_MAX_STR_LEN ? strlen(str) : BT_MAX_STR_LEN - 1);
        }
        else if (aunBtOpIfceProp.enBtGattDescProp == enBTGattDPropValue) {
            const guint8 *array = NULL;
            gsize n_elements;
            int i;
            char byteValue[BT_MAX_STR_LEN] = "\0";
            char hexValue[] = "0123456789abcdef";
            unsigned short u16idx = 0;
            GVariant *lOutObject = NULL;

            lOutObject = bluez_gatt_descriptor1_get_value(lDBusGattDesProxy);

            array = g_variant_get_fixed_array (lOutObject,&n_elements,sizeof(guint8));

            for (i=0; i<n_elements ;i++)
            {
                byteValue[u16idx++] = hexValue[array[i] >> 4];
                byteValue[u16idx++] = hexValue[array[i] &  0x0F];
            }
            byteValue[u16idx] = '\0';
            memcpy (apvVal, byteValue, BT_MAX_STR_LEN-1);
            g_variant_unref(lOutObject);
        }
        else if (aunBtOpIfceProp.enBtGattDescProp == enBTGattDPropFlags) {
            gchar **arg,**p;
            char (*ptr)[BT_MAX_UUID_STR_LEN] = (char (*)[BT_MAX_UUID_STR_LEN])apvVal;
            int index = 0;
            arg = bluez_gatt_descriptor1_dup_flags(lDBusGattDesProxy);

            p = arg;
            if (arg != NULL) {
                while (*arg != NULL) {
                    strncpy (ptr[index++], *arg, BT_MAX_UUID_STR_LEN - 1);
                    arg++;
                }
            }
            g_strfreev(p);
        }
        else {
            BTRCORELOG_ERROR("Invalid property query \n");
            return -1;
        }
        lDBusGattDesProxy = NULL;
    }

    return rc;
}

int
BtrCore_BTSetProp (
    void*               apstBtIfceHdl,
    const char*         apcBtOpIfcePath,
    enBTOpIfceType      aenBtOpIfceType,
    unBTOpIfceProp      aunBtOpIfceProp,
    void*               apvVal
) {
    stBtIfceHdl*        pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    BluezDevice1*       lDBusDevProxy = NULL;
    int index = 0;
    BluezGattCharacteristic1 *lDBusGattCharProxy = NULL;

    if (aenBtOpIfceType == enBTAdapter) {

        if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropName) {
            char **pValue = (char **)apvVal;
            bluez_adapter1_set_alias(pstlhBtIfce->DBusAdapProxy,*pValue);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropPowered) {
            int *pValue = (int *)apvVal;
            bluez_adapter1_set_powered(pstlhBtIfce->DBusAdapProxy,*pValue);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropDiscoverable) {
            int *pValue = (int *)apvVal;
            bluez_adapter1_set_discoverable(pstlhBtIfce->DBusAdapProxy,*pValue);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropDiscoverableTimeOut) {
            int *pValue = (int *)apvVal;
            bluez_adapter1_set_discoverable_timeout(pstlhBtIfce->DBusAdapProxy,*pValue);
        }
        else if (aunBtOpIfceProp.enBtAdapterProp == enBTAdPropPairable) {
            int *pValue = (int *)apvVal;
            bluez_adapter1_set_pairable(pstlhBtIfce->DBusAdapProxy,*pValue);
        }
        else {
            BTRCORELOG_ERROR ("Invalid Adapter Property\n");
            return -1;
        }
    }

    if (aenBtOpIfceType == enBTDevice) {

        while (index < pstlhBtIfce->DevCount) {
            if (strcmp(pstlhBtIfce->DevPathInfo[index],apcBtOpIfcePath) == 0) {
                break;
            }
            index++;
        }

        lDBusDevProxy = pstlhBtIfce->DevProxyList[index];

        if (aunBtOpIfceProp.enBtDeviceProp == enBTDevPropName) {
            char **pValue = (char **)apvVal;
            bluez_device1_set_alias(lDBusDevProxy,*pValue);
        }
        else if (aunBtOpIfceProp.enBtDeviceProp == enBTDevPropBlocked) {
            int *pValue = (int *)apvVal;
            bluez_device1_set_blocked(lDBusDevProxy,*pValue);
        }
        else if (aunBtOpIfceProp.enBtDeviceProp == enBTDevPropTrusted) {
            int *pValue = (int *)apvVal;
            bluez_device1_set_trusted(lDBusDevProxy,*pValue);
        }
        else {
            BTRCORELOG_ERROR ("Invalid Device Property\n");
            return -1;
        }

        lDBusDevProxy = NULL;
    }

    if (aenBtOpIfceType == enBTGattCharacteristic) {
        lDBusGattCharProxy = get_gatt_ser_char_obj(apcBtOpIfcePath);
        if (lDBusGattCharProxy == NULL) {
            BTRCORELOG_ERROR ("Gatt characteristic object not found\n");
            return -1;
        }

        if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropUUID) {
            char **pValue = (char **)apvVal;
            bluez_gatt_characteristic1_set_uuid(lDBusGattCharProxy,*pValue);
        } else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropService) {
            char **pValue = (char **)apvVal;
            bluez_gatt_characteristic1_set_service(lDBusGattCharProxy,*pValue);
        } else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropNotifying) {
            int *pValue = (int *)apvVal;
            bluez_gatt_characteristic1_set_notifying(lDBusGattCharProxy,*pValue);
        } else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropValue) {
            if (bluez_gatt_characteristic1_get_notifying(lDBusGattCharProxy)){
                BTRCORELOG_INFO("Set prop at Char path : %s\n", apcBtOpIfcePath);
                bluez_gatt_characteristic1_set_value(lDBusGattCharProxy, g_variant_new_bytestring((char *)apvVal));
            }
            else
            {
                unsigned char lpcLeWriteByteData[BT_MAX_STR_LEN];
                int lWriteByteDataLen = 0;
                unsigned short u16idx = 0;
                unsigned char u8val = 0;
                unsigned char *pValue = (unsigned char *)apvVal;
                GVariantBuilder *builder = NULL;
                GVariant *value = NULL;

                builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
                memset(lpcLeWriteByteData, 0, BT_MAX_STR_LEN);
                while (*pValue)
                {
                    u8val = *pValue - ((*pValue <= 'f' && *pValue >= 'a') ? ('a' - 10) : (*pValue <= 'F' && *pValue >= 'A') ? ('A' - 10) : '0');
                    if (u16idx % 2)
                    {
                        lpcLeWriteByteData[u16idx++ / 2] |= u8val;
                    }
                    else
                    {
                        lpcLeWriteByteData[u16idx++ / 2] = u8val << 4;
                    }
                    pValue++;
                }
                lpcLeWriteByteData[u16idx] = '\0';
                lWriteByteDataLen = u16idx / 2;

                for (int i = 0; i < lWriteByteDataLen; i++)
                {
                    BTRCORELOG_INFO("lpcLeWriteByteData[%d] : %02x\n", i, lpcLeWriteByteData[i]);
                    g_variant_builder_add(builder, "y", lpcLeWriteByteData[i]);
                }

                value = g_variant_new("ay", builder);
                g_variant_builder_unref(builder);
                bluez_gatt_characteristic1_set_value(lDBusGattCharProxy, value);
                g_free(value);
            }
        } else if (aunBtOpIfceProp.enBtGattCharProp == enBTGattCPropFlags) {
             const gchar* const* pValue = (const gchar * const *) apvVal;
             bluez_gatt_characteristic1_set_flags(lDBusGattCharProxy,pValue);
        } else {
             BTRCORELOG_ERROR ("Invalid GATT characteristic Property\n");
                 return -1;
        }
        lDBusGattCharProxy = NULL;
    }

    return 0;
}

int
BtrCore_BTStartDiscovery (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    bluez_adapter1_call_start_discovery((BluezAdapter1 *)pstlhBtIfce->DBusAdapProxy,
            NULL,
            (GAsyncReadyCallback)btrCore_BTStartDiscoveryCb,
            pstlhBtIfce);

    return 0;
}


int
BtrCore_BTStopDiscovery (
    void*       apstBtIfceHdl,
    const char* apBtAdapter,
    const char* apBtAgentPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    bluez_adapter1_call_stop_discovery((BluezAdapter1 *)pstlhBtIfce->DBusAdapProxy,
            NULL,
            (GAsyncReadyCallback)btrCore_BTStopDiscoveryCb,
            pstlhBtIfce);

    return 0;
}


int
BtrCore_BTStartLEDiscovery (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(b, "{sv}", "Transport", g_variant_new_string("le"));


    /* For now adding XBB UUID and verified ,In Future we will add the
     * UUID of other devices which is required.
     */

#if 0
    GVariantBuilder *u = g_variant_builder_new(G_VARIANT_TYPE_STRING_ARRAY);
    g_variant_builder_add(u, "s", BT_UUID_GATT_TILE_1);
    g_variant_builder_add(u, "s", BT_UUID_GATT_TILE_2);
    g_variant_builder_add(u, "s", BT_UUID_GATT_TILE_3);
    g_variant_builder_add(b, "{sv}", "UUIDs", g_variant_builder_end(u));
    g_variant_builder_unref(u);
#endif

    GVariantBuilder *u = g_variant_builder_new(G_VARIANT_TYPE_STRING_ARRAY);
    g_variant_builder_add(u, "s", BT_UUID_GATT_XBB_1);
    g_variant_builder_add(b, "{sv}", "UUIDs", g_variant_builder_end(u));
    g_variant_builder_unref(u);

    GVariant *dict = g_variant_builder_end(b);
    g_variant_builder_unref(b);

    bluez_adapter1_call_set_discovery_filter(pstlhBtIfce->DBusAdapProxy,
            dict,
            NULL,
            (GAsyncReadyCallback)btrCore_BTSetDiscoveryFilterCb,
            pstlhBtIfce);

    return BtrCore_BTStartDiscovery(pstlhBtIfce, apBtAdapter, apBtAgentPath);
}


int
BtrCore_BTStopLEDiscovery (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl)
        return -1;

    if (BtrCore_BTStopDiscovery(pstlhBtIfce, apBtAdapter, apBtAgentPath)) {
        BTRCORELOG_WARN ("Failed to Stop Discovery\n");
    }

    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
    GVariant *dict = g_variant_builder_end(b);
    g_variant_builder_unref(b);

    bluez_adapter1_call_set_discovery_filter(pstlhBtIfce->DBusAdapProxy,
            dict,
            NULL,
            (GAsyncReadyCallback)btrCore_BTSetDiscoveryFilterCb,
            pstlhBtIfce);

    return 0;
}


int
BtrCore_BTStartClassicDiscovery (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(b, "{sv}", "Transport", g_variant_new_string("bredr"));

    GVariant *dict = g_variant_builder_end(b);
    g_variant_builder_unref(b);

    bluez_adapter1_call_set_discovery_filter(pstlhBtIfce->DBusAdapProxy,
            dict,
            NULL,
            (GAsyncReadyCallback)btrCore_BTSetDiscoveryFilterCb,
            pstlhBtIfce);

    return BtrCore_BTStartDiscovery(pstlhBtIfce, apBtAdapter, apBtAgentPath);

}


int
BtrCore_BTStopClassicDiscovery (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath
    ) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl)
        return -1;

    if (BtrCore_BTStopDiscovery(pstlhBtIfce, apBtAdapter, apBtAgentPath)) {
        BTRCORELOG_WARN ("Failed to Stop Discovery\n");
    }

    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
    GVariant *dict = g_variant_builder_end(b);
    g_variant_builder_unref(b);

    bluez_adapter1_call_set_discovery_filter(pstlhBtIfce->DBusAdapProxy,
            dict,
            NULL,
            (GAsyncReadyCallback)btrCore_BTSetDiscoveryFilterCb,
            pstlhBtIfce);

    return 0;
}

int
BtrCore_BTGetPairedDeviceInfo (
    void*                   apstBtIfceHdl,
    const char*             apBtAdapter,
    stBTPairedDeviceInfo*   pPairedDeviceInfo
) {
    char*           adapter_path = NULL;
    char            paths[BT_MAX_NUM_DEVICE][BT_MAX_DEV_PATH_LEN];
    char            objectPath[BT_MAX_DEV_PATH_LEN] = {'\0'};
    int             d = 0,j = 0,a = 0,num = 0,index = 0;
    GVariant*       prop_val = NULL,*lOutObjects = NULL,*properties = NULL,*ifaces_and_properties = NULL;
    GVariantIter    i,ii,iii;
    BluezDevice1*   lDBusDevProxy = NULL;
    const gchar*    property_name = NULL,*interface_name = NULL;
    GError*         error = NULL;
    gboolean        result;

    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !apBtAdapter || !pPairedDeviceInfo)
        return -1;

    if (pstlhBtIfce->DBusObjManProxy != NULL) {
        result = dbus_object_manager_call_get_managed_objects_sync((DBusObjectManager *)pstlhBtIfce->DBusObjManProxy,
                &lOutObjects,
                NULL,
                &error);
    }
    else {
        BTRCORELOG_ERROR("Proxy not available to get managed objects \n");
        return -1;
    }

    if (result != TRUE) {
        BTRCORELOG_ERROR("Failed to get the managed objects \n");
    }

    for ( index = 0; index < BT_MAX_NUM_DEVICE; index++) {
        memset(&paths[index][0], '\0', BT_MAX_DEV_PATH_LEN);
    }

    if (lOutObjects != NULL) {
        g_variant_iter_init(&i, lOutObjects);
        while(g_variant_iter_next(&i, "{&o@a{sa{sv}}}", &adapter_path, &ifaces_and_properties)) {
            memcpy(objectPath, adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
            ++a;
            g_variant_iter_init(&ii, ifaces_and_properties);
            while(g_variant_iter_next(&ii, "{&s@a{sv}}", &interface_name, &properties)) {
                if (strcmp (interface_name,BT_DBUS_BLUEZ_DEVICE_PATH) == 0) {
                    g_variant_iter_init(&iii, properties);
                    while(g_variant_iter_next(&iii, "{&sv}", &property_name, &prop_val)) {
                        const gchar *type = g_variant_get_type_string(prop_val);
                        if (*type == 'b') {
                            if (strcmp(property_name,"Paired") == 0 && g_variant_get_boolean(prop_val)) {
                                memcpy(&paths[d][0], adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
                                ++d;
                            }
                        }
                        g_variant_unref(prop_val);
                    }
                }
                g_variant_unref(properties);
            }
            g_variant_unref(ifaces_and_properties);
        }
        g_variant_unref(lOutObjects);
    }

    num = d;

    /* Update the number of devices */
    pPairedDeviceInfo->numberOfDevices = num;
    /* Update the paths of these devices */
    for ( index = 0; index < num; index++) {
        memset(&pPairedDeviceInfo->devicePath[index][0], '\0', BT_MAX_DEV_PATH_LEN);
        memcpy(&pPairedDeviceInfo->devicePath[index][0], &paths[index][0], (strlen(&paths[index][0]) < BT_MAX_DEV_PATH_LEN) ? strlen(&paths[index][0]) : BT_MAX_DEV_PATH_LEN - 1);
    }

    if ((pPairedDeviceInfo->numberOfDevices > 0) && (pstlhBtIfce->DevCount == 0)) {
        for (index = 0; index < num; index++) {
            strncpy(pstlhBtIfce->DevPathInfo[pstlhBtIfce->DevCount],pPairedDeviceInfo->devicePath[index], BT_MAX_DEV_PATH_LEN - 1);
            btrCore_BTDevSignalInit(pstlhBtIfce);
            pstlhBtIfce->DevCount++;
        }
    }

    for (index = 0; index < num; index++) {
        for (j=0; j<pstlhBtIfce->DevCount; j++) {
            if (strcmp(pstlhBtIfce->DevPathInfo[j],pPairedDeviceInfo->devicePath[index]) == 0) {
                break;
            }
        }

        lDBusDevProxy = pstlhBtIfce->DevProxyList[j];
        stBTDeviceInfo  apstBTDeviceInfo;
        memset (&apstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));

        if (0 != btrCore_BTParseDevice(lDBusDevProxy, &apstBTDeviceInfo)) {
            BTRCORELOG_ERROR ("Parsing the device %s failed..\n", pPairedDeviceInfo->devicePath[index]);
            return -1;
        }
        else {
            memcpy (&pPairedDeviceInfo->deviceInfo[index], &apstBTDeviceInfo, sizeof(stBTDeviceInfo));
        }
    }
    lDBusDevProxy = NULL;

    BTRCORELOG_INFO ("Exiting\n");
    return 0;
}


int
BtrCore_BTDiscoverDeviceServices (
    void*                           apstBtIfceHdl,
    const char*                     apcDevPath,
    stBTDeviceSupportedServiceList* pProfileList
) {
    stBtIfceHdl*        pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;
    return 0;
}


int
BtrCore_BTFindServiceSupported (
    void*           apstBtIfceHdl,
    const char*     apcDevPath,
    const char*     apcSearchString,
    char*           apcDataString
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    if (!apstBtIfceHdl || !apcDevPath)
        return -1;


    //BTRCORELOG_ERROR ("%d\t: %s - apcDevPath is %s\n and service UUID is %s", __LINE__, __FUNCTION__, apcDevPath, apcSearchString);

    return 0;
}


int
BtrCore_BTPerformAdapterOp (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath,
    const char*     apcDevPath,
    enBTAdapterOp   aenBTAdpOp
) {
    char            deviceObjectPath[BT_MAX_DEV_PATH_LEN] = {'\0'};
    char            deviceOpString[BT_MAX_STR_LEN/8] = {'\0'};
    char            objectData[BT_MAX_DEV_PATH_LEN] = {'\0'};
    int             rc = 0;
    BluezDevice1*   lDBusDevProxy = NULL;
    BluezAdapter1*  lDBusAdapProxy = NULL;
    bool            bAdapterFound = FALSE;
    char*           adapter_path = NULL;
    int             b = 0,index = 0;
    gboolean        result;
    GError          *error = NULL;
    GVariant        *lOutObjects = NULL;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !apBtAdapter || !apBtAgentPath || !apcDevPath || (aenBTAdpOp == enBTAdpOpUnknown))
        return -1;

    switch (aenBTAdpOp) {
    case enBTAdpOpFindPairedDev:
        strncpy(deviceOpString, "FindDevice",BT_MAX_STR_LEN/8 - 1);
        break;
    case enBTAdpOpCreatePairedDev:
    case enBTAdpOpCreatePairedDevASync:
        strncpy(deviceOpString, "Pair", BT_MAX_STR_LEN/8 - 1);
        break;
    case enBTAdpOpRemovePairedDev:
        strncpy(deviceOpString, "RemoveDevice", BT_MAX_STR_LEN/8 - 1);
        break;
    default:
        rc = -1;
        break;
    }

    if (rc == -1)
        return rc;

    if (aenBTAdpOp == enBTAdpOpFindPairedDev) {
        if (pstlhBtIfce->DBusObjManProxy != NULL) {
                result = dbus_object_manager_call_get_managed_objects_sync((DBusObjectManager *)pstlhBtIfce->DBusObjManProxy,
                    &lOutObjects,
                    NULL,
                    &error);
        }
        else {
            BTRCORELOG_ERROR("Proxy not available to get managed objects\n");
            return 0;
        }

        if (result != TRUE) {
            BTRCORELOG_ERROR("Failed to get the managed objects \n");
        }

        if (lOutObjects != NULL) {
            while (!bAdapterFound && strlen(apcDevPath) != 0) {
                GVariantIter i;
                GVariant *ifaces_and_properties = NULL;
                g_variant_iter_init(&i, lOutObjects);
                while(g_variant_iter_next(&i, "{&o@a{sa{sv}}}", &adapter_path, &ifaces_and_properties)) {
                    memcpy(objectData, adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
                    const gchar *interface_name = NULL;
                    GVariant *properties = NULL;
                    GVariantIter ii;
                    g_variant_iter_init(&ii, ifaces_and_properties);
                    while(g_variant_iter_next(&ii, "{&s@a{sv}}", &interface_name, &properties)) {
                        const gchar *property_name = NULL;
                        GVariantIter iii;
                        GVariant *prop_val = NULL;
                        g_variant_iter_init(&iii, properties);
                        while(g_variant_iter_next(&iii, "{&sv}", &property_name, &prop_val)) {
                            const gchar *type = g_variant_get_type_string(prop_val);
                            if (*type == 's') {
                                if (strcmp(property_name,"Address") == 0) {
                                    strncpy(objectData,g_variant_get_string(prop_val, NULL), BT_MAX_DEV_PATH_LEN - 1);
                                    if (strcmp(apcDevPath, objectData) == 0) {
                                        ++b;
                                        bAdapterFound = TRUE;
                                        break;
                                    }
                                }
                            }
                            g_variant_unref(prop_val);
                        }
                        g_variant_unref(properties);
                        if (bAdapterFound)
                            break;
                    }
                    if (bAdapterFound)
                        break;
                    g_variant_unref(ifaces_and_properties);
                }
            }
            g_variant_unref(lOutObjects);
        }
        if (bAdapterFound != TRUE) {
                return -1;
        }
    }
    else if (aenBTAdpOp == enBTAdpOpRemovePairedDev) {

        lDBusAdapProxy = pstlhBtIfce->DBusAdapProxy;
        bluez_adapter1_call_remove_device ((BluezAdapter1 *)lDBusAdapProxy,
                apcDevPath,
                NULL,
                (GAsyncReadyCallback)btrCore_BTRemoveDeviceCb,
                pstlhBtIfce);

    }
    else if ((aenBTAdpOp == enBTAdpOpCreatePairedDev) || (aenBTAdpOp == enBTAdpOpCreatePairedDevASync)) {

        if (pstlhBtIfce->DBusObjManProxy != NULL) {
            result = dbus_object_manager_call_get_managed_objects_sync((DBusObjectManager *)pstlhBtIfce->DBusObjManProxy,
                    &lOutObjects,
                    NULL,
                    &error);
        }
        else {
            BTRCORELOG_ERROR("Proxy not available to get managed objects\n");
            return 0;
        }

        if (result != TRUE) {
            BTRCORELOG_ERROR("Failed to get the managed objects \n");
        }

        if (lOutObjects != NULL) {
            while (!bAdapterFound && strlen(apcDevPath) != 0) {
                GVariantIter i;
                GVariant *ifaces_and_properties = NULL;
                g_variant_iter_init(&i, lOutObjects);
                while(g_variant_iter_next(&i, "{&o@a{sa{sv}}}", &adapter_path, &ifaces_and_properties)) {
                    memcpy(objectData, adapter_path, (strlen(adapter_path) < BT_MAX_DEV_PATH_LEN) ? strlen(adapter_path) : BT_MAX_DEV_PATH_LEN - 1);
                    const gchar *interface_name = NULL;
                    GVariant *properties = NULL;
                    GVariantIter ii;
                    g_variant_iter_init(&ii, ifaces_and_properties);
                    while(g_variant_iter_next(&ii, "{&s@a{sv}}", &interface_name, &properties)) {
                        const gchar *property_name = NULL;
                        GVariantIter iii;
                        GVariant *prop_val = NULL;
                        g_variant_iter_init(&iii, properties);
                        while(g_variant_iter_next(&iii, "{&sv}", &property_name, &prop_val)) {
                            const gchar *type = g_variant_get_type_string(prop_val);
                            if (*type == 's') {
                                if (strcmp(property_name,"Address") == 0) {
                                    strncpy(objectData,g_variant_get_string(prop_val, NULL), BT_MAX_DEV_PATH_LEN - 1);
                                    if (strcmp(apcDevPath, objectData) == 0) {
                                        ++b;
                                        strncpy(deviceObjectPath, adapter_path, BT_MAX_DEV_PATH_LEN - 1);
                                        bAdapterFound = TRUE;
                                        break;
                                    }
                                }
                            }
                            g_variant_unref(prop_val);
                        }
                        g_variant_unref(properties);
                        if (bAdapterFound)
                            break;
                    }
                    if (bAdapterFound)
                        break;
                    g_variant_unref(ifaces_and_properties);
                }
            }
            g_variant_unref(lOutObjects);
        }

        while (index < pstlhBtIfce->DevCount) {
            if (strcmp(pstlhBtIfce->DevPathInfo[index],deviceObjectPath) == 0) {
                break;
            }
            index++;
        }

        lDBusDevProxy = pstlhBtIfce->DevProxyList[index];

        result = bluez_device1_call_pair_sync ((BluezDevice1 *)lDBusDevProxy,
                NULL,
                &error);

        lDBusDevProxy = NULL;

        if (result == 1 && error == NULL) {
            BTRCORELOG_INFO("paired succesfully\n");
        }
        else {
            BTRCORELOG_INFO("pairing failed - %s\n",error->message);
        }
    }
    return 0;
}


int
BtrCore_BTIsDeviceConnectable (
    void*           apstBtIfceHdl,
    const char*     apcDevPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    FILE*           lfpL2Ping   = NULL;
    int             i32OpRet    = -1;
    char            lcpL2PingIp[BT_MAX_STR_LEN/2] = {'\0'};
    char            lcpL2PingOp[BT_MAX_STR_LEN] = {'\0'};

    if (!apstBtIfceHdl || !apcDevPath)
        return -1;

    (void)pstlhBtIfce;

    snprintf(lcpL2PingIp, BT_MAX_STR_LEN/2, "l2ping -i hci0 -c 3 -s 2 -d 2 %s", apcDevPath);
    BTRCORELOG_INFO ("lcpL2PingIp: %s\n", lcpL2PingIp);
#ifdef LIBSYSWRAPPER_BUILD
    lfpL2Ping = v_secure_popen("r","l2ping -i hci0 -c 3 -s 2 -d 2 %s", apcDevPath);
#else
    lfpL2Ping = popen(lcpL2PingIp, "r");
#endif
    if ((lfpL2Ping == NULL)) {
        BTRCORELOG_ERROR ("Failed to run BTIsDeviceConnectable command\n");
    }
    else {
        if (fgets(lcpL2PingOp, sizeof(lcpL2PingOp)-1, lfpL2Ping) == NULL) {
            BTRCORELOG_ERROR ("Failed to Output of l2ping\n");
        }
        else {
            BTRCORELOG_WARN ("Output of l2ping =  %s\n", lcpL2PingOp);
            if (!strstr(lcpL2PingOp, "Host is down")) {
                i32OpRet = 0;
            }
        }
#ifdef LIBSYSWRAPPER_BUILD
        v_secure_pclose(lfpL2Ping);
#else
        pclose(lfpL2Ping);
#endif
    }

    return i32OpRet;
}


int
BtrCore_BTConnectDevice (
    void*           apstBtIfceHdl,
    const char*     apDevPath,
    enBTDeviceType  aenBTDeviceType
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    BluezDevice1*   lDBusDevProxy = NULL;
    int index = 0;

    if (!apstBtIfceHdl || !apDevPath)
        return -1;

    for (index = 0; index < pstlhBtIfce->DevCount ; index++) {
        if (strcmp(pstlhBtIfce->DevPathInfo[index],apDevPath) == 0) {
            break;
        }
    }
    lDBusDevProxy = pstlhBtIfce->DevProxyList[index];

    bluez_device1_call_connect ((BluezDevice1 *)lDBusDevProxy,
            NULL,
            (GAsyncReadyCallback)btrCore_BTConnectDeviceCb,
            pstlhBtIfce);

    lDBusDevProxy = NULL;

    return 0;
}


int
BtrCore_BTDisconnectDevice (
    void*           apstBtIfceHdl,
    const char*     apDevPath,
    enBTDeviceType  aenBTDeviceType
) {
    BluezDevice1*   lDBusDevProxy = NULL;
    int index = 0;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !apDevPath)
        return -1;

    for (index = 0; index < pstlhBtIfce->DevCount ; index++) {
        if (strcmp(pstlhBtIfce->DevPathInfo[index],apDevPath) == 0) {
            break;
        }
    }
    lDBusDevProxy = pstlhBtIfce->DevProxyList[index];

    bluez_device1_call_disconnect ((BluezDevice1 *)lDBusDevProxy,
            NULL,
            (GAsyncReadyCallback)btrCore_BTDisconnectDeviceCb,
            pstlhBtIfce);

    lDBusDevProxy = NULL;

    return 0;
}


int
BtrCore_BTEnableEnhancedRetransmissionMode (
    void*           apstBtIfceHdl
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    FILE*           lfpBtErtm   = NULL;
    int             i32OpRet    = -1;
    char            lcpBtErtmIp[BT_MAX_STR_LEN/2] = {'\0'};
    char            lcpBtErtmOp[BT_MAX_STR_LEN] = {'\0'};

    if (!apstBtIfceHdl || !pstlhBtIfce)
        return -1;


    memset(lcpBtErtmIp, '\0', BT_MAX_STR_LEN/2);
    memset(lcpBtErtmOp, '\0', BT_MAX_STR_LEN);

    snprintf(lcpBtErtmIp, BT_MAX_STR_LEN/2, "echo 0 > /sys/module/bluetooth/parameters/disable_ertm");
    BTRCORELOG_INFO ("lcpBtErtmIp: %s\n", lcpBtErtmIp);

#ifdef LIBSYSWRAPPER_BUILD
    lfpBtErtm = v_secure_popen("r", lcpBtErtmIp);
#else
    lfpBtErtm = popen(lcpBtErtmIp, "r");
#endif
    if (lfpBtErtm == NULL) {
        BTRCORELOG_ERROR ("Failed to run BTEnableEnhancedRetransmissionMode command\n");
    }
    else {
        if (fgets(lcpBtErtmOp, sizeof(lcpBtErtmOp)-1, lfpBtErtm) == NULL) {
            BTRCORELOG_INFO ("Success  - Output of BtErtm\n");
            i32OpRet = 0;
        }
        else {
            BTRCORELOG_WARN ("Failed Output of BtErtm =  %s\n", lcpBtErtmOp);
            if (!strstr(lcpBtErtmOp, "Permission denied")) {
                BTRCORELOG_WARN ("Check path =  %s\n", lcpBtErtmIp);
            }
        }
#ifdef LIBSYSWRAPPER_BUILD
        v_secure_pclose(lfpBtErtm);
#else
        pclose(lfpBtErtm);
#endif
    }

    return i32OpRet;
}


int
BtrCore_BTDisableEnhancedRetransmissionMode (
    void*           apstBtIfceHdl
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    FILE*           lfpBtErtm   = NULL;
    int             i32OpRet    = -1;
    char            lcpBtErtmIp[BT_MAX_STR_LEN/2] = {'\0'};
    char            lcpBtErtmOp[BT_MAX_STR_LEN] = {'\0'};

    if (!apstBtIfceHdl || !pstlhBtIfce)
        return -1;


    memset(lcpBtErtmIp, '\0', BT_MAX_STR_LEN/2);
    memset(lcpBtErtmOp, '\0', BT_MAX_STR_LEN);

    snprintf(lcpBtErtmIp, BT_MAX_STR_LEN/2, "echo 1 > /sys/module/bluetooth/parameters/disable_ertm");
    BTRCORELOG_INFO ("lcpBtErtmIp: %s\n", lcpBtErtmIp);

#ifdef LIBSYSWRAPPER_BUILD
    lfpBtErtm = v_secure_popen("r", lcpBtErtmIp);
#else
    lfpBtErtm = popen(lcpBtErtmIp, "r");
#endif
    if ((lfpBtErtm == NULL)) {
        BTRCORELOG_ERROR ("Failed to run BTDisableEnhancedRetransmissionMode command\n");
    }
    else {
        if (fgets(lcpBtErtmOp, sizeof(lcpBtErtmOp)-1, lfpBtErtm) == NULL) {
            BTRCORELOG_INFO ("Success  - Output of BtErtm\n");
            i32OpRet = 0;
        }
        else {
            BTRCORELOG_WARN ("Failed Output of BtErtm =  %s\n", lcpBtErtmOp);
            if (!strstr(lcpBtErtmOp, "Permission denied")) {
                BTRCORELOG_WARN ("Check path =  %s\n", lcpBtErtmIp);
            }
        }
#ifdef LIBSYSWRAPPER_BUILD
        v_secure_pclose(lfpBtErtm);
#else
        pclose(lfpBtErtm);
#endif
    }

    return i32OpRet;
}


int
BtrCore_BTRegisterMedia (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    enBTDeviceType  aenBTDevType,
    enBTMediaType   aenBTMediaType,
    const char*     apBtUUID,
    void*           apBtMediaCapabilities,
    int             apBtMediaCapabilitiesSize,
    int             abBtMediaDelayReportEnable
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    const char*     lpBtMediaEpObjPath;
    char            lBtMediaCodec;

    (void)lpBtMediaEpObjPath;

    if (!apstBtIfceHdl)
        return -1;


    switch (aenBTMediaType) {
    case enBTMediaTypePCM:
        lBtMediaCodec = BT_MEDIA_CODEC_PCM;
        break;
    case enBTMediaTypeSBC:
        lBtMediaCodec = BT_MEDIA_CODEC_SBC;
        break;
    case enBTMediaTypeMP3:
        lBtMediaCodec = BT_MEDIA_CODEC_MPEG12;
        break;
    case enBTMediaTypeAAC:
        lBtMediaCodec = BT_MEDIA_CODEC_MPEG24;
        break;
    case enBTMediaTypeUnknown:
    default:
        lBtMediaCodec = BT_MEDIA_CODEC_SBC;
        break;
    }


    switch (aenBTDevType) {
    case enBTDevAudioSink:
        if (lBtMediaCodec == BT_MEDIA_CODEC_SBC) {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SOURCE_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG12) {
            lpBtMediaEpObjPath = BT_MEDIA_MP3_A2DP_SOURCE_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG24) {
            lpBtMediaEpObjPath = BT_MEDIA_AAC_A2DP_SOURCE_ENDPOINT;
        }
        else {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SOURCE_ENDPOINT;
        }
        break;
    case enBTDevAudioSource:
        if (lBtMediaCodec == BT_MEDIA_CODEC_SBC) {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SINK_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG12) {
            lpBtMediaEpObjPath = BT_MEDIA_MP3_A2DP_SINK_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG24) {
            lpBtMediaEpObjPath = BT_MEDIA_AAC_A2DP_SINK_ENDPOINT;
        }
        else {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SINK_ENDPOINT;
        }
        break;
    case enBTDevHFPHeadset:
        if (lBtMediaCodec == BT_MEDIA_CODEC_PCM) {
            lpBtMediaEpObjPath = BT_MEDIA_PCM_HFP_AG_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_SBC) {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_HFP_AG_ENDPOINT;
        }
        else {
            lpBtMediaEpObjPath = BT_MEDIA_PCM_HFP_AG_ENDPOINT;
        }
        break;
    case enBTDevHFPAudioGateway:
        lpBtMediaEpObjPath = BT_MEDIA_PCM_HFP_HS_ENDPOINT;
        break;
    case enBTDevUnknown:
    default:
        lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SOURCE_ENDPOINT;
        break;
    }


    return 0;
}

int BtrCore_BTGetBatteryLevel (
    void*        apstBtIfceHdl,
    const char*  lpcBTRCoreBTDevicePath,
    unsigned char * pDeviceBatteryLevel
)
{
    BTRCORELOG_ERROR("Function not yet implemented\n");
    *pDeviceBatteryLevel = 0;
    return -1;
}
int
BtrCore_BTUnRegisterMedia (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    enBTDeviceType  aenBTDevType,
    enBTMediaType   aenBTMediaType
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    const char*     lpBtMediaEpObjPath;
    int             lBtMediaCodec;

    (void)lpBtMediaEpObjPath;

    if (!apstBtIfceHdl)
        return -1;


    switch (aenBTMediaType) {
    case enBTMediaTypePCM:
        lBtMediaCodec = BT_MEDIA_CODEC_PCM;
        break;
    case enBTMediaTypeSBC:
        lBtMediaCodec = BT_MEDIA_CODEC_SBC;
        break;
    case enBTMediaTypeMP3:
        lBtMediaCodec = BT_MEDIA_CODEC_MPEG12;
        break;
    case enBTMediaTypeAAC:
        lBtMediaCodec = BT_MEDIA_CODEC_MPEG24;
        break;
    case enBTMediaTypeUnknown:
    default:
        lBtMediaCodec = BT_MEDIA_CODEC_SBC;
        break;
    }


    switch (aenBTDevType) {
    case enBTDevAudioSink:
        if (lBtMediaCodec == BT_MEDIA_CODEC_SBC) {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SOURCE_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG12) {
            lpBtMediaEpObjPath = BT_MEDIA_MP3_A2DP_SOURCE_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG24) {
            lpBtMediaEpObjPath = BT_MEDIA_AAC_A2DP_SOURCE_ENDPOINT;
        }
        else {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SOURCE_ENDPOINT;
        }
        break;
    case enBTDevAudioSource:
        if (lBtMediaCodec == BT_MEDIA_CODEC_SBC) {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SINK_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG12) {
            lpBtMediaEpObjPath = BT_MEDIA_MP3_A2DP_SINK_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_MPEG24) {
            lpBtMediaEpObjPath = BT_MEDIA_AAC_A2DP_SINK_ENDPOINT;
        }
        else {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SINK_ENDPOINT;
        }
        break;
    case enBTDevHFPHeadset:
        if (lBtMediaCodec == BT_MEDIA_CODEC_PCM) {
            lpBtMediaEpObjPath = BT_MEDIA_PCM_HFP_AG_ENDPOINT;
        }
        else if (lBtMediaCodec == BT_MEDIA_CODEC_SBC) {
            lpBtMediaEpObjPath = BT_MEDIA_SBC_HFP_AG_ENDPOINT;
        }
        else {
            lpBtMediaEpObjPath = BT_MEDIA_PCM_HFP_AG_ENDPOINT;
        }
        break;
    case enBTDevHFPAudioGateway:
        lpBtMediaEpObjPath = BT_MEDIA_PCM_HFP_HS_ENDPOINT;
        break;
    case enBTDevUnknown:
    default:
        lpBtMediaEpObjPath = BT_MEDIA_SBC_A2DP_SOURCE_ENDPOINT;
        break;
    }


    return 0;
}


int
BtrCore_BTAcquireDevDataPath (
    void*           apstBtIfceHdl,
    char*           apcDevTransportPath,
    int*            dataPathFd,
    int*            dataReadMTU,
    int*            dataWriteMTU
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    return 0;
}


int
BtrCore_BTReleaseDevDataPath (
    void*           apstBtIfceHdl,
    char*           apcDevTransportPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    return 0;
}


int
BtrCore_BTSetDevDataAckTimeout (
    void*           apstBtIfceHdl,
    unsigned int    aui32AckTOutms
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    FILE*           lHciDataAck = NULL;
    int             i32OpRet    = -1;
    char            lcpHciDataWriteAckTOutIp[BT_MAX_STR_LEN/2] = {'\0'};
    char            lcpHciDataWriteAckTOutOp[BT_MAX_STR_LEN] = {'\0'};

    if (!apstBtIfceHdl)
        return -1;

    (void)pstlhBtIfce;

    snprintf(lcpHciDataWriteAckTOutIp, BT_MAX_STR_LEN/2, "hcitool -i hci0 cmd 0x03 0x0028 0x0C 0x00 0x%02x 0x00", aui32AckTOutms);
    BTRCORELOG_INFO ("lcpHciDataWriteAckTOutIp: %s\n", lcpHciDataWriteAckTOutIp);

    lHciDataAck = popen(lcpHciDataWriteAckTOutIp, "r");
    if ((lHciDataAck == NULL)) {
        BTRCORELOG_ERROR ("Failed to run lcpHciDataWriteAckTOutIp command\n");
    }
    else {
        if (fgets(lcpHciDataWriteAckTOutOp, sizeof(lcpHciDataWriteAckTOutOp)-1, lHciDataAck) == NULL) {
            BTRCORELOG_ERROR ("Failed to Output of lcpHciDataWriteAckTOutIp\n");
        }
        else {
            BTRCORELOG_WARN ("Output of lcpHciDataWriteAckTOutIp =  %s\n", lcpHciDataWriteAckTOutOp);
            if (strstr(lcpHciDataWriteAckTOutOp, "HCI Command: ogf 0x03, ocf 0x0028, plen 4")) {
                i32OpRet = 0;
            }
        }

        pclose(lHciDataAck);
    }

    return i32OpRet;
}


/////////////////////////////////////////////////////         AVRCP Functions         ////////////////////////////////////////////////////
/* Get Player Object Path on Remote BT Device*/
char*
BtrCore_BTGetMediaPlayerPath (
    void*          apstBtIfceHdl,
    const char*    apBtDevPath
) {
    stBtIfceHdl*   pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    char*          playerObjectPath = NULL;
    bool           isConnected      = false;

    (void)pstlhBtIfce;
    (void)isConnected;

    if (!apstBtIfceHdl || !apBtDevPath) {
        BTRCORELOG_ERROR ("Invalid args!!!");
        return NULL;
    }


    return playerObjectPath;
}



/* Control Media on Remote BT Device*/
int
BtrCore_BTDevMediaControl (
    void*               apstBtIfceHdl,
    const char*         apMediaPlayerPath,
    enBTMediaControlCmd aenBTMediaOper
) {
    stBtIfceHdl*        pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    char                mediaOper[16]  = "\0";
    char*               mediaProp      = 0;
    char*               propValue      = 0;

    (void)pstlhBtIfce;
    (void)mediaOper;
    (void)mediaProp;
    (void)propValue;

    if (!apstBtIfceHdl || !apMediaPlayerPath) {
        BTRCORELOG_ERROR ("Invalid args!!!");
        return -1;
    }


    switch (aenBTMediaOper) {
    case enBTMediaCtrlPlay:
        STRCPY_S(mediaOper, sizeof(mediaOper), "Play");
        break;
    case enBTMediaCtrlPause:
        STRCPY_S(mediaOper, sizeof(mediaOper), "Pause");
        break;
    case enBTMediaCtrlStop:
        STRCPY_S(mediaOper, sizeof(mediaOper), "Stop");
        break;
    case enBTMediaCtrlNext:
        STRCPY_S(mediaOper, sizeof(mediaOper), "Next");
        break;
    case enBTMediaCtrlPrevious:
        STRCPY_S(mediaOper, sizeof(mediaOper), "Previous");
        break;
    case enBTMediaCtrlFastForward:
        STRCPY_S(mediaOper, sizeof(mediaOper), "FastForward");
        break;
    case enBTMediaCtrlRewind:
        STRCPY_S(mediaOper, sizeof(mediaOper), "Rewind");
        break;
    case enBTMediaCtrlVolumeUp:
        STRCPY_S(mediaOper, sizeof(mediaOper), "VolumeUp");
        break;
    case enBTMediaCtrlVolumeDown:
        STRCPY_S(mediaOper, sizeof(mediaOper), "VolumeDown");
        break;
    case enBTMediaCtrlEqlzrOff:
        mediaProp = "Equalizer";
        propValue = "off";
        break;
    case enBTMediaCtrlEqlzrOn:
        mediaProp = "Equalizer";
        propValue = "on";
        break;
    case enBTMediaCtrlShflOff:
        mediaProp = "Shuffle";
        propValue = "off";
        break;
    case enBTMediaCtrlShflAllTracks:
        mediaProp = "Shuffle";
        propValue = "alltracks";
        break;
    case enBTMediaCtrlShflGroup:
        mediaProp = "Shuffle";
        propValue = "group";
        break;
    case enBTMediaCtrlRptOff:
        mediaProp = "Repeat";
        propValue = "off";
        break;
    case enBTMediaCtrlRptSingleTrack:
        mediaProp = "Repeat";
        propValue = "singletrack";
        break;
    case enBTMediaCtrlRptAllTracks:
        mediaProp = "Repeat";
        propValue = "alltracks";
        break;
    case enBTMediaCtrlRptGroup:
        mediaProp = "Repeat";
        propValue = "group";
        break;
    default:
        break;
    }



    return 0;
}

int
BtrCore_BTGetTransportState (
    void*           apstBtIfceHdl,
    const char*     apBtDataPath,
    void*           pState
) { 
    stBtIfceHdl*    pstlhBtIfce = NULL;

    (void)pstlhBtIfce;

    if (!apstBtIfceHdl) {
        BTRCORELOG_ERROR ("Inavalid args - BtIfceHdl : %p\n", apstBtIfceHdl);
        return -1;
    }

    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    /* switch() */
    return 0;
}

/* Get Media Player Property on Remote BT Device*/
int
BtrCore_BTGetMediaPlayerProperty (
    void*           apstBtIfceHdl,
    const char*     apBtMediaPlayerPath,
    const char*     mediaProperty,
    void*           mediaPropertyValue
) {
    stBtIfceHdl*    pstlhBtIfce = NULL;

    (void)pstlhBtIfce;

    if (!apstBtIfceHdl) {
        BTRCORELOG_ERROR ("Inavalid args - BtIfceHdl : %p\n", apstBtIfceHdl);
        return -1;
    }

    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    /* switch() */
    return 0;
}


int
BtrCore_BTGetMediaFolderNumberOfItems (
    void*           apstBtIfceHdl,
    const char*     apBtMediaPlayerPath,
    unsigned int*   aui32NumberOfItems
) {
    stBtIfceHdl*    pstlhBtIfce = NULL;

    (void)pstlhBtIfce;

    if (!apstBtIfceHdl) {
        BTRCORELOG_ERROR ("Inavalid args - BtIfceHdl : %p\n", apstBtIfceHdl);
        return -1;
    }

    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    return 0;
}


/* Set Media Property on Remote BT Device (Equalizer, Repeat, Shuffle, Scan, Status)*/
int
BtrCore_BTSetMediaProperty (
    void*           apstBtIfceHdl,
    const char*     apBtAdapterPath,
    char*           mediaProperty,
    char*           pValue
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    return 0;
}

/* Get Track information and place them in an array (Title, Artists, Album, number of tracks, tracknumber, duration, Genre)*/
// TODO : write a api that gets any properties irrespective of the objects' interfaces
int
BtrCore_BTGetTrackInformation (
    void*               apstBtIfceHdl,
    const char*         apBtmediaObjectPath,
    stBTMediaTrackInfo* apstBTMediaTrackInfo
) {
    stBtIfceHdl*        pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    char*               pcProperty  = "Track";

    (void)pstlhBtIfce;
    (void)pcProperty;

    return 0;
}

tBTMediaItemId
BtrCore_BTGetCommonParentMediaItemId (
        tBTMediaItemId      aBTcurrMediaItemId,
        tBTMediaItemId      aBTdestMediaItemId
        ) {
    tBTMediaItemId  currItemId = 0;
    tBTMediaItemId  destItemId = 0;
    tBTMediaItemId  comnItemId = 0;
    short           powerIdx   = 1;

    /* logic to reverse the digits. Ex: n=1234, val=4321 */
    while (destItemId = destItemId*10 + aBTdestMediaItemId/powerIdx%10, aBTdestMediaItemId/(powerIdx*=10));
    powerIdx = 1;
    /* logic to reverse the digits. Ex: n=1234, val=4321 */
    while (currItemId = currItemId*10 + aBTcurrMediaItemId/powerIdx%10, aBTcurrMediaItemId/(powerIdx*=10));
    powerIdx = 1;
    /* logic to find mediaID portion in common between 2 IDs. Ex: ID_1 - 1345, ID_2 - 132, common ID - 13 (parent ID) */
    while (comnItemId = comnItemId*10 + currItemId*10/powerIdx%10, powerIdx*=10, (currItemId*10/powerIdx || destItemId*10/powerIdx) && destItemId%powerIdx == currItemId%powerIdx);

    return comnItemId;
}

int
BtrCore_BTChangeMediaFolder (
    void*               apstBtIfceHdl,
    const char*         apBtmediaPlayerObjectPath,
    const char*         apBtdestMediaFolderPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    return 0;
}


int
BtrCore_BTSelectMediaFolderItems (
    void*               apstBtIfceHdl,
    const char*         apBtMediaPlayerObjectPath,
    // to change the diff filter args into a single filterList
    unsigned int        apBtMediaFolderStartIndex,
    unsigned int        apBtMediaFolderEndIndex,
    const char*         apBtMediaFilter,
    int                 apBtMediaFilterSize
) {
    stBtIfceHdl*     pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    return 0;
}


int
BtrCore_BTPlayMediaTrackItem (
    void*               apstBtIfceHdl,
    const char*         apBtMediaItemObjectPath
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    return 0;
}

#if 0
int
BtrCore_BTAddTrackItemToPlayList (
        void*               apstBtIfceHdl,
        const char*         apBtMediaItemObjectPath
        ) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    (void)pstlhBtIfce;

    return 0;
}
#endif

static bleGattInfo gLstGattInfo;

typedef struct _gattInfo
{
    char uuid[256];
    char path[256];
} gattInfo;

gattInfo gGattInfo[20];
// search for the char obj path
static char*
btrCore_BTLEGetGattPath(const char *uuid)
{
    if (strcmp(uuid, ""))
    {
        for (int idx = 0; idx < 20; idx++)
        {
            if (!strcmp(uuid, gGattInfo[idx].uuid))
            {
                BTRCORELOG_INFO("Char Path: %s\n",gGattInfo[idx].path);
                return gGattInfo[idx].path;
            }
        }
    }
    return NULL;
}

static stBtIfceHdl* gpstlhBtIfce = NULL;
/* Function to read/write characteristic/descriptor value to/from Bluez */
static int
btrCore_BTLEGattOps(
    BluezGattCharacteristic1 *object,
    stBtIfceHdl *apstlhBtIfce,
    enBTOpIfceType aenIfceType,
    enBTLeGattOp aenGattOp)
{
    int i32OpRet = -1;
    char propertyValue[BT_MAX_GATT_OP_DATA_LEN] = {'\0'};
    const char *lpUuid = bluez_gatt_characteristic1_get_uuid(object);
    const char *lpcPath = btrCore_BTLEGetGattPath(lpUuid);

    if(!lpcPath) return -1;
    //todo better way for the path, may put into the GattCharSerObj

    stBtIfceHdl*     pstlhBtIfce = apstlhBtIfce;
    stBTDeviceInfo *pstBTDeviceInfo = NULL;

    if (pstlhBtIfce != NULL)
    {
        pstBTDeviceInfo = &pstlhBtIfce->lstBTDeviceInfo;
    }
    else
    {
        BTRCORELOG_INFO("User data is empty \n");
        return -1;
    }
    
    memset(pstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));
    i32OpRet = btrCore_BTGetDeviceInfo(apstlhBtIfce, pstBTDeviceInfo, lpcPath);

    switch (aenGattOp)
    {
#if 0
    // todo for the write callback, leave stub here for now. 
    case enBTLeGattOpWriteValue: {
        BTRCORELOG_INFO("enBTLeGattOpWriteValue\n");
        
        stBTDeviceInfo  lstBTDeviceInfo;
        btrCore_BTGetDeviceInfo(apstlhBtIfce->pDBusConn, &lstBTDeviceInfo, objectPath);
        BTRCORELOG_DEBUG("device address is %s\n", lstBTDeviceInfo.pcAddress);
        BTRCORELOG_DEBUG("Received data is %s with length %lu\n", propertyValue, (unsigned long)strlen(propertyValue));

        /* Update data to the LE layer */
        if (apstlhBtIfce->fpcBTLeGattPath) {
            apstlhBtIfce->fpcBTLeGattPath(aenIfceType, aenGattOp, path, lstBTDeviceInfo.pcAddress, enBTDevStPropChanged, propertyValue, apstlhBtIfce->pcBLePathUserData);
        }
    }
        break;
#endif
    case enBTLeGattOpReadValue:
    {
        BTRCORELOG_INFO("enBTLeGattOpReadValue\n");

        /* Update data to the LE layer, it's only for event notify when working as GATT server */
        if (apstlhBtIfce->fpcBTLeGattPath) {
            apstlhBtIfce->fpcBTLeGattPath(aenIfceType, aenGattOp, lpcPath, pstBTDeviceInfo->pcAddress, enBTDevStPropChanged, propertyValue, apstlhBtIfce->pcBLePathUserData);
            BTRCORELOG_DEBUG("device address is %s and Property value is %s \n", pstBTDeviceInfo->pcAddress, propertyValue);
        }
    }
    break;
    case enBTLeGattOpStartNotify:
    {
        strncpy(propertyValue, lpUuid, BT_MAX_STR_LEN - 1);
        BTRCORELOG_INFO("enBTLeGattOpStartNotify for UUID : %s.\n", propertyValue);
        /* Post to LE layer */
        if (apstlhBtIfce->fpcBTLeGattPath)
        {
            apstlhBtIfce->fpcBTLeGattPath(aenIfceType, aenGattOp, lpcPath, pstBTDeviceInfo->pcAddress, enBTDevStPropChanged, propertyValue, apstlhBtIfce->pcBLePathUserData);
        }
        BTRCORELOG_INFO("Property value is %s\n", propertyValue);
    }
    break;
    case enBTLeGattOpStopNotify:
    {
        strncpy(propertyValue, lpUuid, BT_MAX_STR_LEN - 1);
        BTRCORELOG_INFO("enBTLeGattOpStopNotify for UUID : %s.\n", propertyValue);
        /* Post to LE layer */
        if (apstlhBtIfce->fpcBTLeGattPath)
        {
            apstlhBtIfce->fpcBTLeGattPath(aenIfceType, aenGattOp, lpcPath, pstBTDeviceInfo->pcAddress, enBTDevStPropChanged, propertyValue, apstlhBtIfce->pcBLePathUserData);
        }
    }
    break;
    default:
        break;
    }

    return i32OpRet;
}
static ble_adv_info_t bleAdvInfo;
static char pstBackupAdName[BT_MAX_STR_LEN];

int
BtrCore_BTRegisterLeAdvertisement (
    void*               apstBtIfceHdl,
    const char*         apBtAdapter,
    stBTLeCustomAdv*    apstBtAdvInfo
) {
    if ((NULL == apstBtIfceHdl) || (NULL == apBtAdapter)) {
        return -1;
    }
    FILE * fp = NULL;

    stBTLeCustomAdv* pstlhBtAdvInfo = apstBtAdvInfo;
    BTRCORELOG_INFO("Registering advertisement\n");
    memset(&bleAdvInfo, 0, sizeof(bleAdvInfo));
    if (!g_strcmp0(pstlhBtAdvInfo->BeaconName, "")){
        //if no beacon name given, try and use Product class to replace it
#ifdef LIBSYSWRAPPER_BUILD
        fp = v_secure_popen("r", "dmcli eRT getv Device.DeviceInfo.ProductClass | grep value | awk '{print $5}'");
#else
        fp = popen("dmcli eRT getv Device.DeviceInfo.ProductClass | grep value | awk '{print $5}'", "r");
#endif
        if (fp){
            fgets(pstBackupAdName, sizeof(pstBackupAdName) - 1, fp);
            BTRCORELOG_DEBUG("Name = %s\n", pstBackupAdName);
            BTRCORELOG_WARN("No Beacon Name set, use default name %s\n", pstBackupAdName);
            STRCPY_S((char *) bleAdvInfo.localName, sizeof(bleAdvInfo.localName), pstBackupAdName);
#ifdef LIBSYSWRAPPER_BUILD
            v_secure_pclose(fp);
#else
            pclose(fp);
#endif
        }
        else
        {
            BTRCORELOG_ERROR("Could not default beacon name to Product Class\n");
        }
        
    }
    else
        bleAdvInfo.localName = pstlhBtAdvInfo->BeaconName;

    bleAdvInfo.bTxPower = pstlhBtAdvInfo->bTxPower;
    bleAdvInfo.advType = pstlhBtAdvInfo->AdvertisementType;
    bleAdvInfo.manufacturer.id = pstlhBtAdvInfo->ManfData.ManfID;
    bleAdvInfo.manufacturer.data.len = pstlhBtAdvInfo->ManfData.lenManfData;
    char const* plServiceUuid[BT_MAX_NUM_GATT_SERVICE+1] = {NULL};
    int i=0;
    while((i < BT_MAX_NUM_GATT_SERVICE) && (pstlhBtAdvInfo->ServiceUUID[i][0] != 0)){
        plServiceUuid[i] = pstlhBtAdvInfo->ServiceUUID[i];
        BTRCORELOG_DEBUG("Adv/Beacon Service UUID: %s\n", pstlhBtAdvInfo->ServiceUUID[i]);
        i++;
    }
    bleAdvInfo.serviceUuids = &plServiceUuid[0];
    // the manf data size is reffed from the bluez stack example which is data[25]
    memcpy(&bleAdvInfo.manufacturer.data.data, &pstlhBtAdvInfo->ManfData.data, 24);

    // hard codede value so far. todo
    bleAdvInfo.discoverable = true;
    bleAdvInfo.timeoutSecs = 64805; // 18 hrs 5s, the second is in the middle of the activation checking timer
    bleAdvInfo.appearance = 0x0087;
    // the product id is already in the manf data
    // bleAdvInfo.productId = 0x0301;
    BTRCORELOG_INFO("Adv/Beacon Name: %s\n", bleAdvInfo.localName);
    BTRCORELOG_INFO("Adv/Beacon tx power: %d\n", bleAdvInfo.bTxPower);
    BTRCORELOG_INFO("Adv/Beacon type: %s\n", bleAdvInfo.advType);

    bleAdvInit(&bleAdvInfo);

    return 0;
}

static gboolean
BtrCore_BTLeGattCharReadCb(BluezGattCharacteristic1 *object,
                           GDBusMethodInvocation *invocation,
                           GVariant *options)
{
    GVariant *value;
    value = bluez_gatt_characteristic1_get_value(object);
    BTRCORELOG_INFO("Getting value from reading properties: %s\n", g_variant_get_bytestring(value));
    bluez_gatt_characteristic1_complete_read_value(object, invocation, value);
    btrCore_BTLEGattOps(object, gpstlhBtIfce, enBTGattCharacteristic, enBTLeGattOpReadValue);
    return TRUE;
}

// write value callback stub if it needs later, update the value to the char object
#if 1
static gboolean
BtrCore_BTLeGattCharWriteCb(BluezGattCharacteristic1 *object,
                            GDBusMethodInvocation *invocation,
                            GVariant *arg_value,
                            GVariant *arg_options)
{
    gboolean gbool = false;
    g_info("setting value to : %s", g_variant_print(arg_value, gbool));
    // callback to update LE layer data, todo
    // if(cb == success)
    bluez_gatt_characteristic1_complete_write_value(object, invocation);
    // else error ro write value
    return TRUE;
}
#endif

static gboolean
BtrCore_BTOnNotifyStart(BluezGattCharacteristic1 *object,
                        GDBusMethodInvocation *invocation)
{
    btrCore_BTLEGattOps(object, gpstlhBtIfce, enBTGattCharacteristic, enBTLeGattOpStartNotify);

    return TRUE;
}

static gboolean
BtrCore_BTOnNotifyStop(BluezGattCharacteristic1 *object,
                       GDBusMethodInvocation *invocation)
{
    btrCore_BTLEGattOps(object, gpstlhBtIfce, enBTGattCharacteristic, enBTLeGattOpStopNotify);

    return TRUE;
}

static const char *lGattFlags[BT_MAX_UUID_STR_LEN] =
    {"read", "write", "encrypt-read", "encrypt-write", "encrypt-authenticated-read",
     "encrypt-authenticated-write", "secure-read", "secure-write", "notify", "indicate",
     "broadcast", "write-without-response", "authenticated-signed-writes", "reliable-write",
     "writable-auxiliaries"};

// return number of flags
static uint8_t
BtrCore_BTParseGattCharFlag(gatt_characteristic_desc_t *gattChar, const unsigned short aflag)
{
    uint8_t index = 0;
    uint8_t charFlagsIdx = 0;

    // check if the flag is zero/empty, set to default flag "read"
    if (aflag == 0)
    {
        BTRCORELOG_WARN("No FLAG setting! Set to default flag: read\n");
        gattChar->flags[0] = "read";
        charFlagsIdx++;
    }
    else
    {
        // the flag coming from uplayer, parse the aflag to corresponding char values
        // 0x101 e.g, it stands for "read" and "notify"
        while (index < BT_MAX_NUM_GATT_CHAR_FLAGS)
        {
            if ((aflag & (1 << index)) != 0)
            {
                gattChar->flags[charFlagsIdx] = lGattFlags[index];
                charFlagsIdx++;
            }
            index++;
        }
    }
    // todo, check the size of charFlagsIdx which is maximum 7, and 8th is null.
    gattChar->flags[charFlagsIdx] = NULL;
    return charFlagsIdx;
}

static void
BtrCore_BTSetGattCharCb(gatt_characteristic_desc_t *gattChar, const uint8_t nFlags)
{
    for (int flagIdx = 0; flagIdx < nFlags; flagIdx++)
    {
        const char *flag = gattChar->flags[flagIdx];

        if (!strcmp("read", flag) || !strcmp("secure-read", flag) || !strcmp("encrypt-read", flag))
        {
            gattChar->on_read = &BtrCore_BTLeGattCharReadCb;
        }
        if (!strcmp("write", flag) || !strcmp("secure-write", flag) || !strcmp("encrypt-write", flag))
        {
            gattChar->on_write = &BtrCore_BTLeGattCharWriteCb;
        }
        if (!strcmp("notify", flag))
        {
            gattChar->on_notify_start = &BtrCore_BTOnNotifyStart;
            gattChar->on_notify_stop = &BtrCore_BTOnNotifyStop;
            gattChar->notifying = TRUE;
        }
    }
}

int
BtrCore_BTRegisterLeGatt (
    void *apstBtIfceHdl,
    const char *apBtAdapter,
    stBTLeGattInfo *apstBTLeGattInfo
) {
    stBtIfceHdl *pstlhBtIfce = (stBtIfceHdl *)apstBtIfceHdl;
    BTRCORELOG_INFO("Register le gatt\n");
    memset(&gLstGattInfo, 0, sizeof(gLstGattInfo));
    stBTLeGattInfo *lpstBTLeGattInfo = apstBTLeGattInfo;
    BTRCORELOG_INFO("service number : %d\n", lpstBTLeGattInfo->nNumGattServices);
    gLstGattInfo.nNumGattServices = lpstBTLeGattInfo->nNumGattServices;
    uint gattInfoIdx = 0;

    for (int serviceIndex = 0; serviceIndex < gLstGattInfo.nNumGattServices; serviceIndex++)
    {
        stBTLeGattService *lpstBTLeGattService = &lpstBTLeGattInfo->astBTRGattService[serviceIndex];
        BTRCORELOG_INFO("servicePath : %s\n", lpstBTLeGattService->servicePath);
        gLstGattInfo.gattService[serviceIndex].service_def.name = lpstBTLeGattService->servicePath;
        gLstGattInfo.gattService[serviceIndex].service_def.uuid = lpstBTLeGattService->serviceUuid;
        gLstGattInfo.gattService[serviceIndex].service_def.is_primary = lpstBTLeGattService->serviceType;
        gLstGattInfo.gattService[serviceIndex].ui16NumberOfGattChar = lpstBTLeGattService->ui16NumberOfGattChar;

        for (uint8_t index = 0; index < lpstBTLeGattService->ui16NumberOfGattChar; index++)
        {
            stBTLeGattChar *lpstBTLeGattChar = &lpstBTLeGattService->astBTRGattChar[index];
            gLstGattInfo.gattService[serviceIndex].listof_chars[index].name = lpstBTLeGattService->astBTRGattChar[index].charPath;
            gLstGattInfo.gattService[serviceIndex].listof_chars[index].uuid = lpstBTLeGattService->astBTRGattChar[index].charUuid;
            memcpy(gGattInfo[gattInfoIdx].path, lpstBTLeGattService->astBTRGattChar[index].charPath, BT_MAX_STR_LEN - 1);
            memcpy(gGattInfo[gattInfoIdx].uuid, lpstBTLeGattService->astBTRGattChar[index].charUuid, BT_MAX_UUID_STR_LEN - 1);
            gattInfoIdx++;
            uint8_t nFlags = BtrCore_BTParseGattCharFlag(&gLstGattInfo.gattService[serviceIndex].listof_chars[index], lpstBTLeGattService->astBTRGattChar[index].charFlags);
            BtrCore_BTSetGattCharCb(&gLstGattInfo.gattService[serviceIndex].listof_chars[index], nFlags);

            gLstGattInfo.gattService[serviceIndex].listof_chars[index].value = lpstBTLeGattService->astBTRGattChar[index].value;

            BTRCORELOG_INFO("char value: %s\n", lpstBTLeGattService->astBTRGattChar[index].value);

            for (int DescIndex = 0; DescIndex < lpstBTLeGattChar->ui16NumberOfGattDesc; DescIndex++)
            {
                // stBTLeGattDesc *lpstBTLeGattDesc = &lpstBTLeGattChar->atBTRGattDesc[DescIndex];
            }
        }
    }

    bleGattServiceInit(&gLstGattInfo);
    bleStartAdv(apBtAdapter, &leAdvIfaceCb, pstlhBtIfce);

    return 0;
}

int
BtrCore_BTUnRegisterLeGatt (
    void *apstBtIfceHdl,
    const char *apBtAdapter
) {
    stBtIfceHdl *pstlhBtIfce = (stBtIfceHdl *)apstBtIfceHdl;
    BTRCORELOG_INFO("UnRegister le gatt\n");

    if (!pstlhBtIfce)
        return -1;

    BTRCORELOG_INFO("Stopping Advertisement ......\n");
    bleStopAdv(apBtAdapter, pstlhBtIfce);
    // todo, check the return of stop
    BTRCORELOG_INFO("stopAdv ......success\n");
    return 0;
}

int BtrCore_BTReleaseAdvertisement(
    void *apstBtIfceHdl,
    const char *apBtAdapter)
{
    BTRCORELOG_INFO("Release Advertisement\n");

    bleAdvRelease();
    return 0;
}

int
BtrCore_BTBatteryWriteOTAControl (
   void*        apstBtIfceHdl,
   const char*  apBtGattPath,
   const char*  uuid,
   int          value
) {
    stBtIfceHdl*   pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    BluezGattCharacteristic1* lDBusGattCharProxy = NULL;
    int  index = 0;

    BTRCORELOG_INFO("In BtrCore_BTBatteryWriteOTAControl\n");

    while (index < pstlhBtIfce->GattCharDevCount) {
        if (strcmp(pstlhBtIfce->GattCharDevPath[index],apBtGattPath) == 0) {
            break;
         }
         index++;
    }

    lDBusGattCharProxy = pstlhBtIfce->GattCharProxyList[index];

    BTRCORELOG_INFO("Stored the OTA control Path...\n");
    strncpy(pstlhBtIfce->BatteryOTAControlPath,apBtGattPath,BT_MAX_STR_LEN - 1);

    if (value == 0) {
        BTRCORELOG_INFO("In Start OTA \n");
        GVariantBuilder b;
        guint value[] = { 0 };
        g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&b, "{sv}", "offset", g_variant_new_uint16(0));
        g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("request"));
        GVariant *dict = g_variant_builder_end(&b);

        bluez_gatt_characteristic1_call_write_value(lDBusGattCharProxy,
                    g_variant_new_from_data(G_VARIANT_TYPE("ay"), value, 1, TRUE, NULL, NULL),
                    dict,
                    NULL,
                    (GAsyncReadyCallback)btrCore_BTGattCharWriteValueCb,
                    pstlhBtIfce);
    } else if (value == 3) {
        BTRCORELOG_INFO("In end OTA\n");
        GVariantBuilder b;
        guint value[] = { 3 };
        g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&b, "{sv}", "offset", g_variant_new_uint16(0));
        g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("request"));
        GVariant *dict = g_variant_builder_end(&b);

        bluez_gatt_characteristic1_call_write_value(lDBusGattCharProxy,
                    g_variant_new_from_data(G_VARIANT_TYPE("ay"), value, 1, TRUE, NULL, NULL),
                    dict,
                    NULL,
                    (GAsyncReadyCallback)btrCore_BTGattCharWriteValueCb,
                    pstlhBtIfce);
    } else {
        BTRCORELOG_INFO("Wrong value for OTA control \n");
        return -1;
    }

    return 0;
}

int
BtrCore_BTBatterySetLED (
    void*           apstBtIfceHdl,
    const char*     apBtGattPath,
    const char*     uuid
) {
    stBtIfceHdl*              pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    BluezGattCharacteristic1* lDBusGattCharProxy = NULL;
    int  index = 0;

    BTRCORELOG_INFO("In BtrCore_BTBatterySetLED\n");
    while (index < pstlhBtIfce->GattCharDevCount) {
        if (strcmp(pstlhBtIfce->GattCharDevPath[index],apBtGattPath) == 0) {
            break;
        }
        index++;
    }

    lDBusGattCharProxy = pstlhBtIfce->GattCharProxyList[index];
    BTRCORELOG_INFO("setting XBB led to green");

    guint8 value[] = { 0xfa, 0x64, 0x93, 0xa4, 0x97, 0xb5, 0x62, 0xd3 };

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&builder, "{sv}", "offset", g_variant_new_uint16(0));
    g_variant_builder_add(&builder, "{sv}", "type", g_variant_new_string("request"));

    bluez_gatt_characteristic1_call_write_value(lDBusGattCharProxy,
                     g_variant_new_from_data(G_VARIANT_TYPE("ay"), value, 8, TRUE, NULL, NULL),
                     g_variant_builder_end(&builder),
                     NULL,
                     (GAsyncReadyCallback)btrCore_BTGattCharWriteValueCb,
                     pstlhBtIfce);
   return 0;
}

static gint BtrCore_BTFirmwareUpdate(stBtIfceHdl*  pstlhBtIfce)
{
    gsize fw_transfer_bytes_read;
    GByteArray *fw_transfer_buffer;
 
    BTRCORELOG_INFO("In BtrCore_BTFirmwareUpdate \n");
    fw_transfer_buffer = g_byte_array_sized_new(200);
    fw_transfer_bytes_read = fread(fw_transfer_buffer->data, 1, 200, pstlhBtIfce->BatteryFirmFilep);

    if (fw_transfer_bytes_read > 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&builder, "{sv}", "offset", g_variant_new_uint16(0));
        g_variant_builder_add(&builder, "{sv}", "type", g_variant_new_string("request"));

        bluez_gatt_characteristic1_call_write_value(pstlhBtIfce->BatteryDevProxy,
                         g_variant_new_from_data(G_VARIANT_TYPE("ay"), fw_transfer_buffer->data, fw_transfer_bytes_read, TRUE, NULL, NULL),
                         g_variant_builder_end(&builder),
                         NULL,
                         (GAsyncReadyCallback)btrCore_BTFirmwareUpdateCb,
                         pstlhBtIfce);
    } else {
        int ret = 0;
        BTRCORELOG_INFO(" >>>> Firmware upgrade completed <<<<\n");
        ret = BtrCore_BTBatteryWriteOTAControl(pstlhBtIfce,pstlhBtIfce->BatteryOTAControlPath,"f7bf3564-fb6d-4e53-88a4-5e37e0326063",3);
        if (ret == 0) {
            BTRCORELOG_INFO("End OTA Control success\n");
        } else {
            BTRCORELOG_INFO("End OTA Control failure\n");
        }
    }
    return G_SOURCE_REMOVE;
}

static void
btrCore_BTFirmwareUpdateCb (
    BluezGattCharacteristic1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;

    result = bluez_gatt_characteristic1_call_write_value_finish(proxy,res,&error);

    BTRCORELOG_INFO("In Data write callback\n");
    if (result == TRUE && error == NULL) {
        BTRCORELOG_INFO("Bytes written\n");
        g_timeout_add(40,G_SOURCE_FUNC(BtrCore_BTFirmwareUpdate),pstlhBtIfce);
    } else {
        BTRCORELOG_INFO("Failed to do firmware upgrade\n");
    }
}

int
BtrCore_BTBatteryOTADataTransfer (
   void*        apstBtIfceHdl,
   const char*  apBtGattPath,
   const char*  uuid,
   char*        FileName
) {
    stBtIfceHdl*              pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    int  index = 0,ret;
    char FilePath[BT_MAX_STR_LEN] = "/etc/";

    while (index < pstlhBtIfce->GattCharDevCount) {
        if (strcmp(pstlhBtIfce->GattCharDevPath[index],apBtGattPath) == 0) {
            break;
        }
        index++;
    }
 
    pstlhBtIfce->BatteryDevProxy = pstlhBtIfce->GattCharProxyList[index];
    strcat(FilePath,FileName);

    pstlhBtIfce->BatteryFirmFilep = fopen(FilePath, "rb");

    if (!pstlhBtIfce->BatteryFirmFilep) {
        BTRCORELOG_ERROR("Failed to open file for firmware upgrade\n");
        return -1;
    }

    ret = BtrCore_BTFirmwareUpdate(pstlhBtIfce);

    if (ret == 0) {
        BTRCORELOG_INFO(" starting firmware upgrade was successfull \n");
    } else {
        BTRCORELOG_INFO(" Initiating Firmware upgrade failed \n");
    }

    return 0;
}

int
BtrCore_BTPerformLeGattOp (
    void*           apstBtIfceHdl,
    const char*     apBtGattPath,
    enBTOpIfceType  aenBTOpIfceType,
    enBTLeGattOp    aenBTLeGattOp,
    char*           apLeGatOparg1,
    char*           apLeGatOparg2,
    char*           rpLeOpRes
) {
    stBtIfceHdl*              pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    BluezGattCharacteristic1* lDBusGattCharProxy = NULL;
    BluezGattDescriptor1*     lDBusGattDesProxy = NULL;
    int                       index = 0;
    GError*                   error = NULL;
    gboolean                  result;
    GVariant*                 lOutObject = NULL;

    if (aenBTOpIfceType == enBTGattCharacteristic) {

        while (index < pstlhBtIfce->GattCharDevCount) {
            if (strcmp(pstlhBtIfce->GattCharDevPath[index],apBtGattPath) == 0) {
                break;
            }
            index++;
        }

        lDBusGattCharProxy = pstlhBtIfce->GattCharProxyList[index];

        if (aenBTLeGattOp == enBTLeGattOpStartNotify) {

            bluez_gatt_characteristic1_call_start_notify(lDBusGattCharProxy,
                    NULL,
                    (GAsyncReadyCallback)btrCore_BTGattCharStartNotifyCb,
                    pstlhBtIfce);

        }
        else if (aenBTLeGattOp == enBTLeGattOpReadValue) {

            GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
            g_variant_builder_add(b, "{sv}", "org.bluez.GattCharacteristic1", g_variant_new_string("le"));
            GVariant *dict = g_variant_builder_end(b);
            g_variant_builder_unref(b);

            result = bluez_gatt_characteristic1_call_read_value_sync(lDBusGattCharProxy,dict,&lOutObject,NULL,&error);

            if (result == TRUE) {
                BTRCORELOG_INFO("Performed read value operation on %s \n",apBtGattPath);
            }
            else {
                BTRCORELOG_INFO("Failed to perform read value operation on %s error - %s \n",apBtGattPath,error->message);
            }

            if (lOutObject != NULL) {
                const guint8 *array = NULL;
                gsize n_elements;
                int i;
                char byteValue[BT_MAX_STR_LEN] = "\0";
                char hexValue[] = "0123456789abcdef";
                unsigned short u16idx = 0;

                array = g_variant_get_fixed_array (lOutObject,&n_elements,sizeof(guint8));

                for (i=0; i<n_elements ;i++)
                {
                    byteValue[u16idx++] = hexValue[array[i] >> 4];
                    byteValue[u16idx++] = hexValue[array[i] &  0x0F];
                }
                byteValue[u16idx] = '\0';
                memcpy (rpLeOpRes, byteValue, BT_MAX_STR_LEN-1);

                g_variant_unref(lOutObject);
                BTRCORELOG_DEBUG ("rpLeOpRes : %s\n", rpLeOpRes);
            }
        }
        else if (aenBTLeGattOp == enBTLeGattOpStopNotify) {

            bluez_gatt_characteristic1_call_stop_notify(lDBusGattCharProxy,
                    NULL,
                    (GAsyncReadyCallback)btrCore_BTGattCharStopNotifyCb,
                    pstlhBtIfce);
        }
        else if (aenBTLeGattOp == enBTLeGattOpWriteValue && apLeGatOparg2) {
            GVariantBuilder *builder = NULL;
            GVariant *value = NULL;

            builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));

            BTRCORELOG_INFO ("apLeGatOparg2 : %s \n", apLeGatOparg2);
            unsigned char lpcLeWriteByteData[BT_MAX_STR_LEN];
            int lWriteByteDataLen  = 0;
            unsigned short u16idx  = 0;
            unsigned char u8val    = 0;
            unsigned char *laptr   = (unsigned char*)apLeGatOparg2;

            memset (lpcLeWriteByteData, 0, BT_MAX_STR_LEN);

            while (*laptr) {
                // Below logic is to map literals in a string to Hex values. Ex: hexOf_a = 'a' - 87 | hexOf_B = 'B' - 55 | hexOf_0 = '0' - 48
                // TODO : May be we should check apLeGatOparg2 in upper layer, if it has only [a..f] | [A..F] | [0..9]
                u8val = *laptr - ((*laptr <= 'f' && *laptr >= 'a')? ('a'-10) : (*laptr <= 'F' && *laptr >= 'A')? ('A'-10) : '0');

                if (u16idx % 2) {
                    lpcLeWriteByteData[u16idx++/2] |= u8val;
                }
                else {
                    lpcLeWriteByteData[u16idx++/2]  = u8val << 4;
                }

                laptr++;
            }

            lpcLeWriteByteData[u16idx] = '\0';
            lWriteByteDataLen          = u16idx/2;

            for(int i = 0; i < lWriteByteDataLen; i++ ) {
                BTRCORELOG_INFO ("lpcLeWriteByteData[%d] : %02x\n", i, lpcLeWriteByteData[i]);
                g_variant_builder_add(builder,"y",lpcLeWriteByteData[i]);
            }

            value = g_variant_new("ay",builder);
            g_variant_builder_unref(builder);

            GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
            g_variant_builder_add(b, "{sv}", "org.bluez.GattCharacteristic1", g_variant_new_string("le"));
            GVariant *dict = g_variant_builder_end(b);
            g_variant_builder_unref(b);

            bluez_gatt_characteristic1_call_write_value(lDBusGattCharProxy,
                    value,
                    dict,
                    NULL,
                    (GAsyncReadyCallback)btrCore_BTGattCharWriteValueCb,
                    pstlhBtIfce);
            g_free(value);
        }
    }
    else if (aenBTOpIfceType == enBTGattDescriptor) {

        while (index < pstlhBtIfce->GattDesDevCount) {
            if (strcmp(pstlhBtIfce->GattDesDevPath[index],apBtGattPath) == 0) {
                break;
            }
            index++;
        }

        lDBusGattDesProxy = pstlhBtIfce->GattDesProxyList[index];

        if (aenBTLeGattOp == enBTLeGattOpReadValue) {
            GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
            g_variant_builder_add(b, "{sv}", "org.bluez.GattDescriptor1", g_variant_new_string("le"));
            GVariant *dict = g_variant_builder_end(b);
            g_variant_builder_unref(b);

            result = bluez_gatt_descriptor1_call_read_value_sync(lDBusGattDesProxy,dict,&lOutObject,NULL,&error);

            if (result == TRUE) {
                BTRCORELOG_INFO("Performed read value operation on %s \n",apBtGattPath);
            }
            else {
                BTRCORELOG_INFO("Failed to perform read value operation on %s error %s\n",apBtGattPath,error->message);
            }

            if (lOutObject != NULL) {
                const guint8 *array = NULL;
                gsize n_elements;
                array = g_variant_get_fixed_array (lOutObject,&n_elements,sizeof(guint8));
                int i;
                char byteValue[BT_MAX_STR_LEN] = "\0";
                char hexValue[] = "0123456789abcdef";
                unsigned short u16idx = 0;

                for (i=0; i<n_elements ;i++)
                {
                    byteValue[u16idx++] = hexValue[array[i] >> 4];
                    byteValue[u16idx++] = hexValue[array[i] &  0x0F];
                }
                byteValue[u16idx] = '\0';
                memcpy (rpLeOpRes, byteValue, BT_MAX_STR_LEN-1);

                g_variant_unref(lOutObject);
                BTRCORELOG_DEBUG ("rpLeOpRes : %s\n", rpLeOpRes);
            }
        }
        else if (aenBTLeGattOp == enBTLeGattOpWriteValue && apLeGatOparg2) {
            GVariantBuilder *builder = NULL;
            GVariant *value = NULL;

            builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));

            BTRCORELOG_INFO ("apLeGatOparg2 : %s \n", apLeGatOparg2);
            unsigned char lpcLeWriteByteData[BT_MAX_STR_LEN] = "\0";
            int lWriteByteDataLen  = 0;
            unsigned short u16idx  = 0;
            unsigned char u8val    = 0;
            unsigned char *laptr   = (unsigned char*)apLeGatOparg2;

            memset (lpcLeWriteByteData, 0, BT_MAX_STR_LEN);

            while (*laptr) {
                // Below logic is to map literals in a string to Hex values. Ex: hexOf_a = 'a' - 87 | hexOf_B = 'B' - 55 | hexOf_0 = '0' - 48
                // TODO : May be we should check apLeGatOparg2 in upper layer, if it has only [a..f] | [A..F] | [0..9]
                u8val = *laptr - ((*laptr <= 'f' && *laptr >= 'a')? ('a'-10) : (*laptr <= 'F' && *laptr >= 'A')? ('A'-10) : '0');

                if (u16idx % 2) {
                    lpcLeWriteByteData[u16idx++/2] |= u8val;
                }
                else {
                    lpcLeWriteByteData[u16idx++/2]  = u8val << 4;
                }

                laptr++;
            }

            lpcLeWriteByteData[u16idx] = '\0';
            lWriteByteDataLen          = u16idx/2;

            for(int i = 0; i < lWriteByteDataLen; i++ ) {
                BTRCORELOG_INFO ("lpcLeWriteByteData[%d] : %02x\n", i, lpcLeWriteByteData[i]);
                g_variant_builder_add(builder,"y",lpcLeWriteByteData[i]);
            }

            value = g_variant_new("ay",builder);
            g_variant_builder_unref(builder);

            GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
            g_variant_builder_add(b, "{sv}", "org.bluez.GattDescriptor1", g_variant_new_string("le"));
            GVariant *dict = g_variant_builder_end(b);
            g_variant_builder_unref(b);

            bluez_gatt_descriptor1_call_write_value(lDBusGattDesProxy,
                    value,
                    dict,
                    NULL,
                    (GAsyncReadyCallback)btrCore_BTGattDesWriteValueCb,
                    pstlhBtIfce);
            g_free(value);
        }

    }
    return 0;
}

int
BtrCore_BTSendReceiveMessages (
    void*           apstBtIfceHdl
) {
    if (!apstBtIfceHdl) {
        return -1;
    }

    if (gpvMainLoop == NULL) {
        return -1;
    }

    return 0;
}

int
BtrCore_BTGetBluetoothVersion (
    char* version
) {
    char output[BT_MAX_HCICONFIG_OUTPUT_SIZE] = {0};
    static char sBluetoothVersion[BT_MAX_BLUETOOTH_VERSION] = {0}; // Bluetooth version will be 4.9, 5.0, 5.1, 5.2, 5.3
    FILE* fp;

    int length = 0;

    if (version == NULL) {
        BTRCORELOG_ERROR("Invalid version pointer\n");
        return -1;
    }

    length = strlen(sBluetoothVersion);

    if (!length) {
        const char* version_start;
        size_t output_length;

        // Execute hciconfig -a command and capture its output
        fp = popen("hciconfig -a", "r");
        if (fp == NULL) {
            BTRCORELOG_ERROR("Failed to execute hciconfig\n");
            return -1;
        }

        output_length = fread(output, sizeof(char), BT_MAX_HCICONFIG_OUTPUT_SIZE - 1, fp);
        output[output_length] = '\0';
        pclose(fp);

        version_start = strstr(output, "HCI Version: ");

        if (version_start != NULL) {
            version_start += strlen("HCI Version: ");
            strncpy(sBluetoothVersion, version_start, BT_MAX_BLUETOOTH_VERSION - 1);
            BTRCORELOG_DEBUG("Parsed bluetooth version:%s\n",sBluetoothVersion);
        }
        else {
            BTRCORELOG_ERROR("Failed to get the Bluetooth version\n");
            return -1;
        }
    } else {
        BTRCORELOG_DEBUG("Already retrieved the BT version:%s, lenghth:%d\n",sBluetoothVersion,length);
    }

    length = strlen(sBluetoothVersion);
    strncpy(version, sBluetoothVersion, length);

    BTRCORELOG_INFO("Bluetooth HCI version:%s\n",version);

    return 0;
}

// Outgoing callbacks Registration Interfaces
int
BtrCore_BTRegisterAdapterStatusUpdateCb (
    void*                                   apstBtIfceHdl,
    fPtr_BtrCore_BTAdapterStatusUpdateCb    afpcBAdapterStatusUpdate,
    void*                                   apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !afpcBAdapterStatusUpdate)
        return -1;


    pstlhBtIfce->fpcBAdapterStatusUpdate    = afpcBAdapterStatusUpdate;
    pstlhBtIfce->pcBAdapterStatusUserData   = apUserData;

    return 0;
}


int
BtrCore_BTRegisterDevStatusUpdateCb (
    void*                               apstBtIfceHdl,
    fPtr_BtrCore_BTDevStatusUpdateCb    afpcBDevStatusUpdate,
    void*                               apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !afpcBDevStatusUpdate)
        return -1;


    pstlhBtIfce->fpcBDevStatusUpdate    = afpcBDevStatusUpdate;
    pstlhBtIfce->pcBDevStatusUserData   = apUserData;

    return 0;
}


int
BtrCore_BTRegisterMediaStatusUpdateCb (
    void*                               apstBtIfceHdl,
    fPtr_BtrCore_BTMediaStatusUpdateCb  afpcBMediaStatusUpdate,
    void*                               apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !afpcBMediaStatusUpdate)
        return -1;


    pstlhBtIfce->fpcBMediaStatusUpdate    = afpcBMediaStatusUpdate;
    pstlhBtIfce->pcBMediaStatusUserData   = apUserData;

    return 0;
}

int
BtrCore_BTRegisterConnIntimationCb (
    void*                       apstBtIfceHdl,
    fPtr_BtrCore_BTConnIntimCb  afpcBConnIntim,
    void*                       apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !afpcBConnIntim)
        return -1;


    pstlhBtIfce->fpcBConnectionIntimation   = afpcBConnIntim;
    pstlhBtIfce->pcBConnIntimUserData       = apUserData;

    return 0;
}


int
BtrCore_BTRegisterConnAuthCb (
    void*                       apstBtIfceHdl,
    fPtr_BtrCore_BTConnAuthCb   afpcBConnAuth,
    void*                       apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !afpcBConnAuth)
        return -1;


    pstlhBtIfce->fpcBConnectionAuthentication   = afpcBConnAuth;
    pstlhBtIfce->pcBConnAuthUserData            = apUserData;

    return 0;
}


int
BtrCore_BTRegisterNegotiateMediaCb (
    void*                           apstBtIfceHdl,
    const char*                     apBtAdapter,
    fPtr_BtrCore_BTNegotiateMediaCb afpcBNegotiateMedia,
    void*                           apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !apBtAdapter || !afpcBNegotiateMedia)
        return -1;


    pstlhBtIfce->fpcBNegotiateMedia     = afpcBNegotiateMedia;
    pstlhBtIfce->pcBNegMediaUserData    = apUserData;

    return 0;
}


int
BtrCore_BTRegisterTransportPathMediaCb (
    void*                               apstBtIfceHdl,
    const char*                         apBtAdapter,
    fPtr_BtrCore_BTTransportPathMediaCb afpcBTransportPathMedia,
    void*                               apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !apBtAdapter || !afpcBTransportPathMedia)
        return -1;


    pstlhBtIfce->fpcBTransportPathMedia     = afpcBTransportPathMedia;
    pstlhBtIfce->pcBTransPathMediaUserData  = apUserData;

    return 0;
}


int
BtrCore_BTRegisterMediaPlayerPathCb (
    void*                               apstBtIfceHdl,
    const char*                         apBtAdapter,
    fPtr_BtrCore_BTMediaPlayerPathCb    afpcBTMediaPlayerPath,
    void*                               apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !apBtAdapter || !afpcBTMediaPlayerPath)
        return -1;


    pstlhBtIfce->fpcBTMediaPlayerPath       = afpcBTMediaPlayerPath;
    pstlhBtIfce->pcBMediaPlayerPathUserData = apUserData;

    return 0;
}


int
BtrCore_BTRegisterMediaBrowserUpdateCb (
    void*                               apstBtIfceHdl,
    fPtr_BtrCore_BTMediaBrowserPathCb   afpcBTMediaBrowserPath,
    void*                               apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !afpcBTMediaBrowserPath)
        return -1;


    pstlhBtIfce->fpcBTMediaBrowserPath       = afpcBTMediaBrowserPath;
    pstlhBtIfce->pcBMediaBrowserPathUserData = apUserData;

    return 0;
}


int
BtrCore_BTRegisterLEGattInfoCb (
    void*                           apstBtIfceHdl,
    const char*                     apBtAdapter,
    fPtr_BtrCore_BTLeGattPathCb     afpcBtLeGattPath,
    void*                           apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl || !apBtAdapter || !afpcBtLeGattPath)
        return -1;


    pstlhBtIfce->fpcBTLeGattPath    = afpcBtLeGattPath;
    pstlhBtIfce->pcBLePathUserData  = apUserData;
    gpstlhBtIfce = pstlhBtIfce;
    return 0;
}


int
BtrCore_BTRegisterLEAdvInfoCb (
    void*                                apstBtIfceHdl,
    const char*                          apBtAdapter,
    fPtr_BtrCore_BTLeAdvertisementCb     afpcBtLeAdvPath,
    void*                                apUserData
) {
    stBtIfceHdl*    pstlhBtIfce = NULL;

    if (!apstBtIfceHdl || !apBtAdapter || !afpcBtLeAdvPath) {
        return -1;
    }

    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    pstlhBtIfce->fpcBTLeAdvPath = afpcBtLeAdvPath;
    pstlhBtIfce->pcBLeAdvUserData = apUserData;

    return 0;
}


/* Incoming Callbacks */
static void
btrCore_bluezSignalAdatperChangedCb (
    BluezAdapter1 *proxy,
    GParamSpec *spec,
    gpointer userdata
) {
    stBtIfceHdl*     pstlhBtIfce = (stBtIfceHdl*)userdata;
    int              i32OpRet = -1;
    stBTAdapterInfo* pstBTAdapterInfo = NULL;
    const char* path = NULL;

    if (pstlhBtIfce != NULL) {
        pstBTAdapterInfo = &pstlhBtIfce->lstBTAdapterInfo;
    }
    else {
        BTRCORELOG_INFO("User data was empty\n");
        return;
    }

    BTRCORELOG_INFO ("Property Changed! : %s\n", BT_DBUS_BLUEZ_ADAPTER_PATH);

    path = g_dbus_proxy_get_object_path((GDBusProxy *)proxy);
    if (path != NULL) {
        memcpy (pstBTAdapterInfo->pcPath, path, strlen(path) < BT_MAX_DEV_PATH_LEN ? strlen(path) : BT_MAX_DEV_PATH_LEN - 1);
    }
    else {
        BTRCORELOG_INFO ("Not able to get the object path\n");
    }

    i32OpRet = btrCore_BTParseAdapter(proxy,pstBTAdapterInfo);
    if (spec != NULL) {
        if (strcmp (g_param_spec_get_name(spec),"Discovering") == 0) {
            BTRCORELOG_DEBUG ("%s received event <%s>\n", BT_DBUS_BLUEZ_ADAPTER_PATH, g_param_spec_get_name(spec));
            pstlhBtIfce->ui32IsAdapterDiscovering = pstBTAdapterInfo->bDiscovering;
            if (pstBTAdapterInfo->bDiscovering) {
                BTRCORELOG_INFO ("Adapter Started Discovering | %d\n", pstlhBtIfce->ui32IsAdapterDiscovering);
            }
            else {
                BTRCORELOG_INFO ("Adapter Stopped Discovering | %d\n", pstlhBtIfce->ui32IsAdapterDiscovering);
            }
        }
    }
    else {
        BTRCORELOG_INFO("Empty paramspec received \n");
        return;
    }

    if (!i32OpRet) {
        if (pstlhBtIfce->fpcBAdapterStatusUpdate) {
            if(pstlhBtIfce->fpcBAdapterStatusUpdate(enBTAdPropDiscoveryStatus, pstBTAdapterInfo, pstlhBtIfce->pcBAdapterStatusUserData)) {
            }
        }
    }
}

static void
btrCore_bluezSignalDeviceChangedCb (
    BluezDevice1 *proxy,
    GParamSpec *spec,
    gpointer userdata
) {
    stBTDeviceInfo*  pstBTDeviceInfo    = NULL;
    stBtIfceHdl*     pstlhBtIfce = (stBtIfceHdl*)userdata;
    int              i32OpRet    = -1;
    const char*            path = NULL;
    int settingRSSItoZero = 0;
    int bPairingEvent = 0;
    int bPaired = 0;
    int bSrvResolvedEvent = 0; //TODO: Bad way to do this. Live with it for now
    int bSrvResolved = 0;
    int bConnectEvent = 0; //TODO: Bad way to do this. Live with it for now
    int bConnected = 0;
    int bRssiEvent = 0; //TODO: Bad way to do this. Live with it for now
    short i16RSSI = 0;


    if (pstlhBtIfce != NULL) {
        pstBTDeviceInfo  = &pstlhBtIfce->lstBTDeviceInfo;
    }
    else {
        BTRCORELOG_INFO("User data is empty\n");
        return;
    }
    memset(pstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));

    path = g_dbus_proxy_get_object_path((GDBusProxy *)proxy);
    if (path != NULL) {
        memcpy(pstBTDeviceInfo, path, (strlen(path) < BT_MAX_DEV_PATH_LEN) ? strlen(path) : BT_MAX_DEV_PATH_LEN - 1 );
    }
    else {
        BTRCORELOG_INFO("Not able to parse the object path \n");
        return;
    }

    i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce,pstBTDeviceInfo, path);

    BTRCORELOG_INFO ("Property Changed! : %s \n",path);
    BTRCORELOG_DEBUG ("%s received event <%s>\n", BT_DBUS_BLUEZ_DEVICE_PATH, g_param_spec_get_name(spec));

    if (spec != NULL) {
        if (strcmp (g_param_spec_get_name(spec),"paired") == 0) {
            bPairingEvent = 1;
            bPaired = bluez_device1_get_paired(proxy);
            BTRCORELOG_INFO ("bPaired = %d\n", bPaired);
        }
        else if (strcmp (g_param_spec_get_name(spec), "connected") == 0) {
            bConnectEvent = 1;
            bConnected = bluez_device1_get_connected(proxy);
            BTRCORELOG_INFO ("bConnected = %d\n", bConnected);
        }
        else if (strcmp (g_param_spec_get_name(spec), "services-resolved") == 0) {
            bSrvResolvedEvent = 1;
            bSrvResolved = bluez_device1_get_services_resolved(proxy);
            BTRCORELOG_INFO ("bServicesResolved = %d\n", bSrvResolved);
        }
        else if (strcmp (g_param_spec_get_name(spec), "RSSI") == 0) {
            pstBTDeviceInfo->i32RSSI = i16RSSI;
            bRssiEvent = 1;
            BTRCORELOG_DEBUG ("Event - bi32Rssi = %d\n", pstBTDeviceInfo->i32RSSI);
        }
    }

    if (!i32OpRet) {
        enBTDeviceState lenBtDevState = enBTDevStUnknown;
        enBTDeviceType  lenBTDevType  = btrCore_BTMapDevClasstoDevType(pstBTDeviceInfo->ui32Class);

        if (bPairingEvent) {
            const char* value = NULL;
            BTRCORELOG_INFO ("Parsing Property Changed event figured that its pairing change..\n");
            if (bPaired) {
                value = "paired";
            }
            else {
                value = "unpaired";
            }
            memcpy(pstBTDeviceInfo->pcDevicePrevState, pstlhBtIfce->pcDeviceCurrState, BT_MAX_STR_LEN - 1);
            strncpy(pstBTDeviceInfo->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);
            strncpy(pstlhBtIfce->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);

            lenBtDevState = enBTDevStPropChanged;
            if (pstlhBtIfce->fpcBDevStatusUpdate) {
                if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, lenBtDevState, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                }
            }

            gchar **uuid = NULL;
            int count = 0,i;
            gboolean IsBattery = FALSE;

            uuid = bluez_device1_dup_uuids(proxy);

            while (uuid && (uuid[count] != NULL)) {
                if (!strcmp(uuid[count],BT_UUID_GATT_BATTERY_SPECIFIC_1) || !strcmp(uuid[count],BT_UUID_GATT_BATTERY_SPECIFIC_2)) {
                    IsBattery = TRUE;
                }
                count++;
            }
            g_strfreev(uuid);

            if (bPaired && IsBattery && pstBTDeviceInfo->bConnected) {
                for (i=0; i<pstlhBtIfce->GattCharNotifyDevCount ;i++) {
                    if (pstlhBtIfce->fpcBTLeGattPath) {
                        pstlhBtIfce->fpcBTLeGattPath(enBTGattCharacteristic, enBTLeGattOpUnknown, pstlhBtIfce->GattCharNotifyPath[i], pstBTDeviceInfo->pcAddress, enBTDevStPropChanged, NULL, pstlhBtIfce->pcBLePathUserData);
                    }
                    sleep(1);
                }
            }
        }
        /* Added the check bConnectEvent since the connected event was received 
         * before the paired event on pair/Connect with XBB.
         */
        else if (pstBTDeviceInfo->bPaired || bConnectEvent || (!bConnectEvent && bSrvResolvedEvent && bSrvResolved)) {
            if (settingRSSItoZero || bRssiEvent) {
                if (pstlhBtIfce->ui32IsAdapterDiscovering || pstBTDeviceInfo->bConnected) {
                    lenBtDevState = enBTDevStRSSIUpdate;
                }
            }
            else {
                if (pstBTDeviceInfo->bConnected) {
                    const char* value = "connected";

                    memcpy(pstBTDeviceInfo->pcDevicePrevState, pstlhBtIfce->pcDeviceCurrState, BT_MAX_STR_LEN - 1);
                    strncpy(pstBTDeviceInfo->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);
                    strncpy(pstlhBtIfce->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);

                    lenBtDevState = enBTDevStPropChanged;
                }
                else if (!pstBTDeviceInfo->bConnected) {
                    const char* value = "disconnected";
                    memcpy(pstBTDeviceInfo->pcDevicePrevState, pstlhBtIfce->pcDeviceCurrState, BT_MAX_STR_LEN - 1);
                    strncpy(pstBTDeviceInfo->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);
                    strncpy(pstlhBtIfce->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);

                    lenBtDevState = enBTDevStPropChanged;

                    if (enBTDevAudioSink == lenBTDevType && pstlhBtIfce->ui32DevLost) {
                        lenBtDevState = enBTDevStLost;
                    }
                }
                pstlhBtIfce->ui32DevLost = 0;
            }

            if (enBTDevAudioSource != lenBTDevType || strcmp(pstlhBtIfce->pcDeviceCurrState, "connected")) {
                if (pstlhBtIfce->fpcBDevStatusUpdate) {
                    if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, lenBtDevState, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                    }
                }
                if (bSrvResolvedEvent) {
                    if (pstlhBtIfce->fpcBTLeGattPath) {
                        pstlhBtIfce->fpcBTLeGattPath(enBTDevice, enBTLeGattOpUnknown, path, pstBTDeviceInfo->pcAddress, enBTDevStPropChanged, NULL, pstlhBtIfce->pcBLePathUserData);
                    }
               }
            }
            else if ((enBTDevAudioSource == lenBTDevType) && bSrvResolvedEvent && !strncmp(pstBTDeviceInfo->pcDevicePrevState, "connected",sizeof("connected")) && !strcmp(pstBTDeviceInfo->pcDeviceCurrState, "connected")) {
                if (pstlhBtIfce->fpcBDevStatusUpdate) {
                    if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, lenBtDevState, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                    }
                }
            }
        }
        else if (!pstBTDeviceInfo->bPaired && !pstBTDeviceInfo->bConnected &&
                strncmp(pstlhBtIfce->pcLeDeviceAddress, pstBTDeviceInfo->pcAddress,
                    ((BT_MAX_STR_LEN > strlen(pstBTDeviceInfo->pcAddress))?strlen(pstBTDeviceInfo->pcAddress):BT_MAX_STR_LEN))) {

            if (pstlhBtIfce->ui32IsAdapterDiscovering && settingRSSItoZero) {
                lenBtDevState = enBTDevStRSSIUpdate;
            }
            else {
                lenBtDevState = enBTDevStFound;
            }

            if (pstlhBtIfce->fpcBDevStatusUpdate) {
                if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, lenBtDevState, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                }
            }
        }
        else if (lenBTDevType == enBTDevUnknown) { //TODO: Have to figure out a way to identify it as a LE device

            if (bConnectEvent) {
                if (pstBTDeviceInfo->bConnected) {
                    const char* value = "connected";

                    memcpy(pstBTDeviceInfo->pcDevicePrevState, pstlhBtIfce->pcLeDeviceCurrState, BT_MAX_STR_LEN - 1);
                    strncpy(pstBTDeviceInfo->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);
                    strncpy(pstlhBtIfce->pcLeDeviceCurrState, value, BT_MAX_STR_LEN - 1);
                    memcpy(pstlhBtIfce->pcLeDeviceAddress, pstBTDeviceInfo->pcAddress, BT_MAX_STR_LEN - 1);
                }
                else if (!pstBTDeviceInfo->bConnected) {
                    const char* value = "disconnected";

                    memcpy(pstBTDeviceInfo->pcDevicePrevState, pstlhBtIfce->pcLeDeviceCurrState, BT_MAX_STR_LEN - 1);
                    strncpy(pstBTDeviceInfo->pcDeviceCurrState, value, BT_MAX_STR_LEN - 1);
                    strncpy(pstlhBtIfce->pcLeDeviceCurrState, value, BT_MAX_STR_LEN - 1);
                    strncpy(pstlhBtIfce->pcLeDeviceAddress, "none", BT_MAX_STR_LEN - 1);
                }

                lenBTDevType  = enBTDevLE;
                lenBtDevState = enBTDevStPropChanged;

                if (pstlhBtIfce->fpcBDevStatusUpdate) {
                    if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, lenBtDevState, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                    }
                }
            }
            if (bSrvResolvedEvent) {
                if (pstlhBtIfce->fpcBTLeGattPath) {
                    pstlhBtIfce->fpcBTLeGattPath(enBTDevice, enBTLeGattOpUnknown, path, pstBTDeviceInfo->pcAddress, enBTDevStPropChanged, NULL, pstlhBtIfce->pcBLePathUserData);
                }
            }
            else if (!bConnectEvent) {
                lenBTDevType  = enBTDevLE;
                lenBtDevState = enBTDevStPropChanged;

                if (pstlhBtIfce->fpcBDevStatusUpdate) {
                    if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, lenBtDevState, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                    }
                }
            }
        }
    }
}

static void
btrCore_bluezDeviceDisappearedCb (
    DBusObjectManager *proxy,
    const gchar *object,
    GDBusInterfaceInfo *value,
    void *userdata
) {
    stBTDeviceInfo*  pstBTDeviceInfo    = NULL;
    stBtIfceHdl*     pstlhBtIfce = (stBtIfceHdl*)userdata;
    gulong rm_handler_id = 0;
    int i = 0,j = 0;
    gboolean DeviceFound = FALSE;

    if (pstlhBtIfce != NULL) {
        pstBTDeviceInfo  = &pstlhBtIfce->lstBTDeviceInfo;
    }
    else {
        BTRCORELOG_INFO("User data was not received \n");
        return;
    }

    memset(pstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));

    BTRCORELOG_INFO ("InterfacesRemoved : Interface %s\n",object ? object : NULL);
    BTRCORELOG_INFO ("Interface Name - %s\n",value->name);

    if (!object || g_strstr_len(object,-1,"player") || g_strstr_len(object,-1,"fd") || g_strstr_len(object,-1,"sep"))
        return;

    /* Since the interface name was not received as a part of callback
     * Checking the interface name based on the object deleted
     */

    if (g_strstr_len(object,-1,"des")) {
        BTRCORELOG_INFO ("InterfacesRemoved : %s\n", BT_DBUS_BLUEZ_GATT_DESCRIPTOR_PATH);
        char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};
        unsigned int ui32DeviceIfceLen = strstr(object, "/service") - object;
        memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);

        if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
            strncpy(apcDeviceIfce, object, ui32DeviceIfceLen);

            BluezGattDescriptor1 *rm_proxy = NULL;
            for (i=0 ;i < pstlhBtIfce->GattDesDevCount; i++) {
                rm_proxy = pstlhBtIfce->GattDesProxyList[i];
                rm_handler_id = pstlhBtIfce->GattDesSignalHandlerID[i];
                if (strncmp (pstlhBtIfce->GattDesDevPath[i],object,BT_MAX_DEV_PATH_LEN) == 0) {
                    DeviceFound = TRUE;
                    for (j=i ;j<pstlhBtIfce->GattDesDevCount; j++) {
                        pstlhBtIfce->GattDesProxyList[j] = pstlhBtIfce->GattDesProxyList[j+1];
                        pstlhBtIfce->GattDesSignalHandlerID[j] = pstlhBtIfce->GattDesSignalHandlerID[j+1];
                        strncpy(pstlhBtIfce->GattDesDevPath[j], pstlhBtIfce->GattDesDevPath[j+1], BT_MAX_DEV_PATH_LEN);
                    }
                    break;
                }
            }

            if (DeviceFound != TRUE) {
               return;
            }

            pstlhBtIfce->GattDesDevCount--;
            memset(pstlhBtIfce->GattDesDevPath[j],'\0',sizeof(pstlhBtIfce->GattCharDevPath[j]));
            pstlhBtIfce->GattDesProxyList[j] = NULL;
            pstlhBtIfce->GattDesSignalHandlerID[j] = 0;

            if (rm_proxy != NULL && rm_handler_id != 0) {
                g_signal_handler_disconnect(rm_proxy,rm_handler_id);
                BTRCORELOG_INFO ("SignalDisconnected for object : %s\n", object);
                g_object_unref(rm_proxy);
                rm_proxy = NULL;
            }

            if (pstlhBtIfce->fpcBTLeGattPath) {
                pstlhBtIfce->fpcBTLeGattPath(enBTGattDescriptor, enBTLeGattOpUnknown, object, pstBTDeviceInfo->pcAddress, enBTDevStLost, NULL, pstlhBtIfce->pcBLePathUserData);
            }
            memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);
        }
    }
    else if (g_strstr_len(object,-1,"char")) {
        BTRCORELOG_INFO ("InterfacesRemoved : %s\n", BT_DBUS_BLUEZ_GATT_CHAR_PATH);
        char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};
        unsigned int ui32DeviceIfceLen = strstr(object, "/service") - object;

        memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);
        if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
            strncpy(apcDeviceIfce, object, ui32DeviceIfceLen);

            BluezGattCharacteristic1 *rm_proxy = NULL;
            for (i=0 ;i < pstlhBtIfce->GattCharDevCount; i++) {
                rm_proxy = pstlhBtIfce->GattCharProxyList[i];
                rm_handler_id = pstlhBtIfce->GattCharSignalHandlerID[i];
                if (strncmp (pstlhBtIfce->GattCharDevPath[i],object,BT_MAX_DEV_PATH_LEN) == 0) {
                    DeviceFound = TRUE;
                    for (j=i ;j<pstlhBtIfce->GattCharDevCount; j++) {
                        pstlhBtIfce->GattCharProxyList[j] = pstlhBtIfce->GattCharProxyList[j+1];
                        pstlhBtIfce->GattCharSignalHandlerID[j] = pstlhBtIfce->GattCharSignalHandlerID[j+1];
                        strncpy(pstlhBtIfce->GattCharDevPath[j], pstlhBtIfce->GattCharDevPath[j+1], BT_MAX_DEV_PATH_LEN);
                    }
                    break;
                }
            }

            if (DeviceFound != TRUE) {
               return;
            }

            pstlhBtIfce->GattCharDevCount--;
            memset(pstlhBtIfce->GattCharDevPath[j],'\0',sizeof(pstlhBtIfce->GattCharDevPath[j]));
            pstlhBtIfce->GattCharProxyList[j] = NULL;
            pstlhBtIfce->GattCharSignalHandlerID[j] = 0;

            if (rm_proxy != NULL && rm_handler_id != 0) {
                g_signal_handler_disconnect(rm_proxy,rm_handler_id);
                BTRCORELOG_INFO ("SignalDisconnected for object : %s\n", object);
                g_object_unref(rm_proxy);
                rm_proxy = NULL;
            }


            if (pstlhBtIfce->fpcBTLeGattPath) {
                pstlhBtIfce->fpcBTLeGattPath(enBTGattCharacteristic, enBTLeGattOpUnknown, object, pstBTDeviceInfo->pcAddress, enBTDevStLost, NULL, pstlhBtIfce->pcBLePathUserData);
            }
            memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);
        }
    }
    else if (g_strstr_len(object,-1,"ser")) {
        BTRCORELOG_INFO ("InterfacesRemoved : %s\n", BT_DBUS_BLUEZ_GATT_SERVICE_PATH);
        char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};
        unsigned int ui32DeviceIfceLen = strstr(object, "/service") - object;

        if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
            strncpy(apcDeviceIfce, object, ui32DeviceIfceLen);
            BluezGattService1 *rm_proxy = NULL;
            for (i=0 ;i < pstlhBtIfce->GattSerDevCount; i++) {
                rm_proxy = pstlhBtIfce->GattSerProxyList[i];
                rm_handler_id = pstlhBtIfce->GattSerSignalHandlerID[i];
                if (strncmp (pstlhBtIfce->GattSerDevPath[i],object,BT_MAX_DEV_PATH_LEN) == 0) {
                    DeviceFound = TRUE;
                    for (j=i ;j<pstlhBtIfce->GattSerDevCount; j++) {
                        pstlhBtIfce->GattSerProxyList[j] = pstlhBtIfce->GattSerProxyList[j+1];
                        pstlhBtIfce->GattSerSignalHandlerID[j] = pstlhBtIfce->GattSerSignalHandlerID[j+1];
                        strncpy(pstlhBtIfce->GattSerDevPath[j], pstlhBtIfce->GattSerDevPath[j+1], BT_MAX_DEV_PATH_LEN);
                    }
                    break;
                }
            }

            if (DeviceFound != TRUE) {
                return;
            }

            pstlhBtIfce->GattSerDevCount--;
            memset(pstlhBtIfce->GattSerDevPath[j],'\0',sizeof(pstlhBtIfce->GattSerDevPath[j]));
            pstlhBtIfce->GattSerProxyList[j] = NULL;
            pstlhBtIfce->GattSerSignalHandlerID[j] = 0;

            if (rm_proxy != NULL && rm_handler_id != 0) {
                g_signal_handler_disconnect(rm_proxy,rm_handler_id);
                BTRCORELOG_INFO ("SignalDisconnected for object : %s\n", object);
                g_object_unref(rm_proxy);
                rm_proxy = NULL;
            }

            if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
                strncpy(apcDeviceIfce, object, ui32DeviceIfceLen);

                if (pstlhBtIfce->fpcBTLeGattPath) {
                    pstlhBtIfce->fpcBTLeGattPath(enBTGattService, enBTLeGattOpUnknown, object, pstBTDeviceInfo->pcAddress, enBTDevStLost, NULL, pstlhBtIfce->pcBLePathUserData);
                }
            }
            memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);
        }
    }
    else if (g_strstr_len(object,-1,"dev")) {
        BTRCORELOG_INFO ("InterfacesRemoved : %s\n", BT_DBUS_BLUEZ_DEVICE_PATH);

        BluezDevice1 *rm_proxy = NULL;
        for (i=0 ;i < pstlhBtIfce->DevCount; i++) {
            rm_proxy = pstlhBtIfce->DevProxyList[i];
            rm_handler_id = pstlhBtIfce->DevSignalHandlerID[i];
            if (strncmp (pstlhBtIfce->DevPathInfo[i],object,BT_MAX_DEV_PATH_LEN) == 0) {
                DeviceFound = TRUE;
                /* Interfaces removed signal should be received from bluez on unpairing of device.
                 * Added the below check to handle the case where the interfaces removed signal is received
                 * from bluez on device disconnection.
                 */
                if (1 == bluez_device1_get_paired(pstlhBtIfce->DevProxyList[i])) {
                    BTRCORELOG_INFO("Interfaces removed signal received for battery on paired state\n");
                    enBTDeviceType  lenBTDevType = enBTDevUnknown;
                    if (pstlhBtIfce->fpcBDevStatusUpdate) {
                        btrCore_BTGetDevAddressFromDevPath(object, pstBTDeviceInfo->pcAddress);
                        if (pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, enBTDevStLost, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                        }
                    }
                    return;
                }
                for (j=i ;j<pstlhBtIfce->DevCount; j++) {
                    pstlhBtIfce->DevProxyList[j] = pstlhBtIfce->DevProxyList[j+1];
                    pstlhBtIfce->DevSignalHandlerID[j] = pstlhBtIfce->DevSignalHandlerID[j+1];
                    strncpy(pstlhBtIfce->DevPathInfo[j], pstlhBtIfce->DevPathInfo[j+1], BT_MAX_DEV_PATH_LEN);
                }
                break;
            }
        }

        if (DeviceFound != TRUE) {
            return;
        }

        pstlhBtIfce->DevCount--;
        memset(pstlhBtIfce->DevPathInfo[j],'\0',sizeof(pstlhBtIfce->DevPathInfo[j]));
        pstlhBtIfce->DevProxyList[j] = NULL;
        pstlhBtIfce->DevSignalHandlerID[j] = 0;

        if (rm_proxy != NULL && rm_handler_id != 0) {
            g_signal_handler_disconnect(rm_proxy,rm_handler_id);
            BTRCORELOG_INFO ("SignalDisconnected for object : %s\n", object);
            g_object_unref(rm_proxy);
            rm_proxy = NULL;
        }

        enBTDeviceType  lenBTDevType = enBTDevUnknown;
        if (pstlhBtIfce->fpcBDevStatusUpdate) {
            btrCore_BTGetDevAddressFromDevPath(object, pstBTDeviceInfo->pcAddress);
            if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, enBTDevStLost, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
            }
        }

    }
    else if (g_strstr_len(object,-1,"hci")) {
        BTRCORELOG_INFO ("InterfacesRemoved : %s Object : %s\n", BT_DBUS_BLUEZ_ADAPTER_PATH,object);
    }

}

static void
btrCore_bluezDeviceAppearedCb (
    DBusObjectManager *proxy,
    const gchar *object,
    GVariant *value,
    gpointer userdata
) {
    const char *interface_name = NULL;
    stBTDeviceInfo*  pstBTDeviceInfo = NULL;
    GVariant *properties = NULL;
    int              i32OpRet = -1,count = 0;
    stBtIfceHdl*     pstlhBtIfce = (stBtIfceHdl*)userdata;
    GVariantIter i;
    gboolean DevFound = FALSE;

    if (pstlhBtIfce != NULL) {
        pstBTDeviceInfo  = &pstlhBtIfce->lstBTDeviceInfo;
    }
    else {
        BTRCORELOG_INFO("User data is empty \n");
        return;
    }
    memset(pstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));

    BTRCORELOG_INFO ("InterfacesAdded : Interface %s\n",object ? object : NULL);

    /* currently not handled for the objects with player/fd/sep. we will handle it in future
     * if necessary */
    if (!object || g_strstr_len(object,-1,"player") || g_strstr_len(object,-1,"fd") || g_strstr_len(object,-1,"sep"))
        return;

    g_variant_iter_init(&i,value);
    while (g_variant_iter_next(&i,"{&s@a{sv}}",&interface_name,&properties)) {
        if (!strcmp(interface_name, BT_DBUS_BLUEZ_ADAPTER_PATH)) {
            BTRCORELOG_INFO ("InterfacesAdded : %s\n", BT_DBUS_BLUEZ_ADAPTER_PATH);
            BTRCORELOG_INFO ("Adapter object added : %s\n",object);
            /* TODO Currently implementation for handling hci0 adapter only.
             * In future we will add a logic to store the object path of
             * multiple adapters.
             */
        }
        else if (!strcmp(interface_name, BT_DBUS_BLUEZ_DEVICE_PATH) || !strcmp(interface_name, BT_DBUS_BLUEZ_BATTERY_PATH)) {
            if (strcmp(interface_name, BT_DBUS_BLUEZ_DEVICE_PATH) == 0) {
                BTRCORELOG_INFO ("InterfacesAdded : %s\n", BT_DBUS_BLUEZ_DEVICE_PATH);
            } else {
                BTRCORELOG_INFO ("InterfacesAdded : %s\n", BT_DBUS_BLUEZ_BATTERY_PATH);
            }

            if (pstlhBtIfce->DevCount >= BT_MAX_DEVICES) {
                return;
            }

            for (count=0 ;count < pstlhBtIfce->DevCount; count++) {
                if (strcmp(pstlhBtIfce->DevPathInfo[count],object) == 0 && pstlhBtIfce->DevProxyList[count] != NULL) {
                    DevFound = TRUE;
                }
            }

            if (DevFound != TRUE) {
                strncpy(pstlhBtIfce->DevPathInfo[pstlhBtIfce->DevCount], object, BT_MAX_DEV_PATH_LEN - 1);
                btrCore_BTDevSignalInit(pstlhBtIfce);
            }

            i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce,pstBTDeviceInfo,object);
            pstlhBtIfce->DevCount++;
            if (!i32OpRet) {
                enBTDeviceState lenBtDevState = enBTDevStUnknown;
                enBTDeviceType  lenBTDevType  = btrCore_BTMapDevClasstoDevType(pstBTDeviceInfo->ui32Class);

                if ((!pstBTDeviceInfo->bPaired && !pstBTDeviceInfo->bConnected) ||
                        ((enBTDevUnknown == lenBTDevType) && (TRUE == pstBTDeviceInfo->bConnected)) ) {
                     lenBtDevState = enBTDevStFound;

                     if (pstlhBtIfce->fpcBDevStatusUpdate) {
                         if(pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, lenBtDevState, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData)) {
                         }
                     }
                }
            }
        }
#ifdef GATT_CLIENT
        else if (!strcmp(interface_name, BT_DBUS_BLUEZ_GATT_SERVICE_PATH)) {
            BTRCORELOG_INFO ("InterfacesAdded : %s\n", BT_DBUS_BLUEZ_GATT_SERVICE_PATH);
            char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};
            unsigned int ui32DeviceIfceLen = strstr(object, "/service") - object;

            if (pstlhBtIfce->GattSerDevCount >= BT_MAX_NUM_GATT_SERVICE) {
                BTRCORELOG_INFO("Maximum GATT service limit exceeded ...\n");
                return;
            }

            if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
                strncpy(apcDeviceIfce, object, ui32DeviceIfceLen);

                memset(pstlhBtIfce->GattSerDevPath[pstlhBtIfce->GattSerDevCount],'\0',BT_MAX_STR_LEN);
                strncpy(pstlhBtIfce->GattSerDevPath[pstlhBtIfce->GattSerDevCount], object, BT_MAX_STR_LEN - 1);
                btrCore_BTGattSerSignalInit(pstlhBtIfce);
                i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce, pstBTDeviceInfo, apcDeviceIfce);
                /* Stub function to print the gatt service properties.
                 * For debugging purpose we will remove it in future.
                 */
                //btrCore_BTGetGattProperty(pstlhBtIfce,object,interface_name);
                pstlhBtIfce->GattSerDevCount++;
                if (!i32OpRet) {
                    if (pstlhBtIfce->fpcBTLeGattPath) {
                        pstlhBtIfce->fpcBTLeGattPath(enBTGattService, enBTLeGattOpUnknown, object, pstBTDeviceInfo->pcAddress, enBTDevStFound, NULL, pstlhBtIfce->pcBLePathUserData);
                    }
                }
                memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);
            }
        }
        else if (!strcmp(interface_name, BT_DBUS_BLUEZ_GATT_CHAR_PATH)) {
            BTRCORELOG_INFO ("InterfacesAdded : %s\n", BT_DBUS_BLUEZ_GATT_CHAR_PATH);
            char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};
            unsigned int ui32DeviceIfceLen = strstr(object, "/service") - object;

            if (pstlhBtIfce->GattCharDevCount >= BT_MAX_NUM_GATT_CHAR) {
                BTRCORELOG_INFO("Maximum GATT characteristics limit exceeded ...\n");
                return;
            }

            if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
                strncpy(apcDeviceIfce, object, ui32DeviceIfceLen);
                memset(pstlhBtIfce->GattCharDevPath[pstlhBtIfce->GattCharDevCount],'\0',BT_MAX_STR_LEN);
                strncpy(pstlhBtIfce->GattCharDevPath[pstlhBtIfce->GattCharDevCount], object, BT_MAX_STR_LEN - 1);
                btrCore_BTGattCharSignalInit(pstlhBtIfce);
                i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce, pstBTDeviceInfo, apcDeviceIfce);
                /* Stub function to print the gatt characteristic properties.
                 * For debugging purpose we will remove it in future.
                 */
                //btrCore_BTGetGattProperty(pstlhBtIfce,object,interface_name);
                BluezGattCharacteristic1 *lDBusGattCharProxy = pstlhBtIfce->GattCharProxyList[pstlhBtIfce->GattCharDevCount];
                pstlhBtIfce->GattCharDevCount++;
                const gchar * const* arg = NULL;
                arg = bluez_gatt_characteristic1_get_flags(lDBusGattCharProxy);
                if (arg != NULL) {
                   while (*arg != NULL) {
                       BTRCORELOG_DEBUG("> Characteristic - flag - %s <\n",*arg);
                       if (!strcmp(*arg,"notify")) {
                           memset(pstlhBtIfce->GattCharNotifyPath[pstlhBtIfce->GattCharNotifyDevCount],'\0',BT_MAX_STR_LEN);
                           strncpy(pstlhBtIfce->GattCharNotifyPath[pstlhBtIfce->GattCharNotifyDevCount],object,BT_MAX_STR_LEN - 1);
                           pstlhBtIfce->GattCharNotifyDevCount++;
                           break;
                       }
                   arg++;
                   }
                }

                if (!i32OpRet) {
                    if (pstlhBtIfce->fpcBTLeGattPath) {
                        pstlhBtIfce->fpcBTLeGattPath(enBTGattCharacteristic, enBTLeGattOpUnknown, object, pstBTDeviceInfo->pcAddress, enBTDevStFound, NULL, pstlhBtIfce->pcBLePathUserData);
                    }
                }
                memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);
            }
        }
        else if (!strcmp(interface_name, BT_DBUS_BLUEZ_GATT_DESCRIPTOR_PATH)) {
            BTRCORELOG_INFO ("InterfacesAdded : %s\n", BT_DBUS_BLUEZ_GATT_DESCRIPTOR_PATH);
            char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};
            unsigned int ui32DeviceIfceLen = strstr(object, "/service") - object;

            if (pstlhBtIfce->GattDesDevCount >= BT_MAX_NUM_GATT_DESC) {
                BTRCORELOG_INFO("Maximum GATT descriptor limit exceeded ...\n");
                return;
            }

            if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
                strncpy(apcDeviceIfce, object, ui32DeviceIfceLen);

                memset(pstlhBtIfce->GattDesDevPath[pstlhBtIfce->GattDesDevCount],'\0',BT_MAX_STR_LEN);
                strncpy(pstlhBtIfce->GattDesDevPath[pstlhBtIfce->GattDesDevCount], object, BT_MAX_STR_LEN-1);
                btrCore_BTGattDesSignalInit(pstlhBtIfce);
                i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce, pstBTDeviceInfo, apcDeviceIfce);
                /* Stub function to print the gatt descriptor properties.
                 * For debugging purpose we will remove it in future.
                 */
                //btrCore_BTGetGattProperty(pstlhBtIfce,object,interface_name);
                pstlhBtIfce->GattDesDevCount++;
                if (!i32OpRet) {
                    if (pstlhBtIfce->fpcBTLeGattPath) {
                        pstlhBtIfce->fpcBTLeGattPath(enBTGattDescriptor, enBTLeGattOpUnknown, object, pstBTDeviceInfo->pcAddress, enBTDevStFound, NULL, pstlhBtIfce->pcBLePathUserData);
                    }
                }
                memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);
            }
        }
#endif
        g_variant_unref(properties);
    }
    g_variant_unref(value);
}
#ifdef GATT_CLIENT
/* Added the signal handler for gatt descriptor property changes
 * by monitoring the notify signal.
 * TODO : Print the changed property and action for changed
 * property.
 */
static void
btrCore_BTGattDesPropertyChangedCb (
    BluezGattDescriptor1 *proxy,
    GParamSpec *spec,
    gpointer userdata
) {
    GVariant *lOutObjects = NULL;
    const guint8 *array = NULL;
    gsize n_elements;
    int i;
    char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};

    if (spec != NULL) {
        strncpy(apcDeviceIfce, g_dbus_proxy_get_object_path((GDBusProxy *)proxy), BT_MAX_STR_LEN - 1);
        BTRCORELOG_INFO ("Property Changed! : %s\n",g_param_spec_get_name(spec));
    }

    BTRCORELOG_INFO ("Property Changed on interface  %s\n", BT_DBUS_BLUEZ_GATT_DESCRIPTOR_PATH);
    BTRCORELOG_INFO ("Descriptor object path - %s\n",apcDeviceIfce);

    if (spec != NULL) {
        if (strcmp(g_param_spec_get_name(spec),"value") == 0) {
            lOutObjects = bluez_gatt_descriptor1_get_value(proxy);
            if(lOutObjects != NULL) {
                array = g_variant_get_fixed_array (lOutObjects,&n_elements,sizeof(guint8));
                BTRCORELOG_DEBUG("Value =0x ");
                for (i=0; i<n_elements ;i++)
                    g_print("%02x",array[i]);
                g_print("\n");

                g_variant_unref(lOutObjects);
            }
        }
    }
    memset(apcDeviceIfce,'\0',strlen(apcDeviceIfce) - 1);
}

/* Added the signal handler for gatt service property changes
 * by monitoring the notify signal.
 * TODO : Print the changed property and action for changed
 * property.
 */
static void
btrCore_BTGattSerPropertyChangedCb (
    BluezGattService1 *proxy,
    GParamSpec *spec,
    gpointer userdata
) {
    BTRCORELOG_INFO ("Property Changed on interface : %s\n", BT_DBUS_BLUEZ_GATT_SERVICE_PATH);
    BTRCORELOG_INFO ("Property Changed! : %s\n",g_param_spec_get_name(spec));
    if (spec!=NULL) {
        BTRCORELOG_INFO ("Service object path : %s\n",g_dbus_proxy_get_object_path((GDBusProxy *)proxy));
    }
}

/* Added the signal handler for gatt characteristic property changes
 * by monitoring the notify signal.
 * TODO : Print the changed property and action for changed
 * property.
 */
static void
btrCore_BTGattCharPropertyChangedCb (
    BluezGattCharacteristic1 *proxy,
    GParamSpec *spec,
    gpointer userdata
) {
    GVariant *lOutObjects = NULL;
    const guint8 *array = NULL;
    gsize n_elements;
    int i;
    char apcDeviceIfce[BT_MAX_STR_LEN] = {'\0'};
    const char* pCharIface  = NULL;
    unsigned int ui32DeviceIfceLen = 0;
    int i32OpRet = -1;
    stBTDeviceInfo*  pstBTDeviceInfo    = NULL;
    stBtIfceHdl*     pstlhBtIfce = (stBtIfceHdl*)userdata;


    BTRCORELOG_INFO ("Property Changed on interface : %s\n", BT_DBUS_BLUEZ_GATT_CHAR_PATH);
    BTRCORELOG_INFO ("Property Changed !- %s\n",g_param_spec_get_name(spec));

    if (pstlhBtIfce != NULL) {
        pstBTDeviceInfo  = &pstlhBtIfce->lstBTDeviceInfo;
    }
    else {
        BTRCORELOG_INFO("User data is empty \n");
        return;
    }

    if (spec != NULL) {
        BTRCORELOG_INFO ("Characteristic object path - %s\n",g_dbus_proxy_get_object_path((GDBusProxy *)proxy));
        pCharIface = g_dbus_proxy_get_object_path((GDBusProxy *)proxy);
    }

    if(pCharIface != NULL) {
       ui32DeviceIfceLen = strstr(pCharIface, "/service") - pCharIface;
    }

    if ((ui32DeviceIfceLen > 0) && (ui32DeviceIfceLen < (BT_MAX_STR_LEN - 1))) {
        strncpy(apcDeviceIfce, pCharIface, ui32DeviceIfceLen);
    }

    i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce, pstBTDeviceInfo, apcDeviceIfce);

    if (!i32OpRet) {
        if (pstlhBtIfce->fpcBTLeGattPath) {
            pstlhBtIfce->fpcBTLeGattPath(enBTGattCharacteristic, enBTLeGattOpUnknown, pCharIface, pstBTDeviceInfo->pcAddress, enBTDevStPropChanged, NULL, pstlhBtIfce->pcBLePathUserData);
        }
    }
    memset(apcDeviceIfce,'\0',ui32DeviceIfceLen);

    if (spec != NULL) {
        if (strcmp (g_param_spec_get_name(spec),"notifying") == 0) {
            BTRCORELOG_INFO ("Notifying = %d\n",bluez_gatt_characteristic1_get_notifying(proxy));
        }
        else if (strcmp(g_param_spec_get_name(spec),"value") == 0) {
            lOutObjects = bluez_gatt_characteristic1_get_value(proxy);
            if(lOutObjects != NULL) {
                array = g_variant_get_fixed_array (lOutObjects,&n_elements,sizeof(guint8));
                BTRCORELOG_INFO("Value = 0x");
                for (i=0; i<n_elements ;i++)
                    g_print("%02x",array[i]);
                g_print("\n");

                g_variant_unref(lOutObjects);
            }

        }
    }
}
#endif
static gpointer
btrCore_g_main_loop_Task (
    gpointer appvMainLoop
) {
    GMainLoop* pMainLoop = (GMainLoop*)appvMainLoop;
    if (!pMainLoop) {
        BTRCORELOG_INFO ("GMainLoop Error - In arguments Exiting\n");
        return NULL;
    }

    BTRCORELOG_INFO ("GMainLoop Running - %p - %p\n", pMainLoop, appvMainLoop);
    g_main_loop_run (pMainLoop);

    return appvMainLoop;
}

static void
btrCore_BTStartDiscoveryCb (
    BluezAdapter1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_adapter1_call_start_discovery_finish(proxy,res,&error);

    if (result == 1  && error == NULL) {
        BTRCORELOG_INFO("Discovery started successfully\n");
    }
    else {
        //This is telemetry log. If we change this marker name, need to change and configure the telemetry marker in xconf server.
        telemetry_event_d("BT_ERR_DiscStartFail", 1);
        BTRCORELOG_INFO("Discovery start failed - %s\n", error ? error->message : "Unknown error");
    }
}

static void
btrCore_BTStopDiscoveryCb (
    BluezAdapter1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_adapter1_call_stop_discovery_finish(proxy,res,&error);

    if (result == 1  && error == NULL) {
        BTRCORELOG_INFO("Discovery stopped successfully\n");
    }
    else {
        //This is telemetry log. If we change this marker name, need to change and configure the telemetry marker in xconf server.
        telemetry_event_d("BT_ERR_DiscStopFail", 1);
        BTRCORELOG_INFO("Discovery stop failed - %s\n", error ? error->message : "Unknown error");
    }
}

static void
btrCore_BTConnectDeviceCb (
    BluezDevice1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_device1_call_connect_finish(proxy,res,&error);

    if (result == 1  && error == NULL) {
        BTRCORELOG_INFO("Connected succesfully\n");
    }
    else {
        BTRCORELOG_INFO("Connection failed - %s\n",error->message);
    }
}

static void
btrCore_BTDisconnectDeviceCb (
    BluezDevice1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_device1_call_disconnect_finish(proxy,res,&error);

    if (result == 1  && error == NULL) {
        BTRCORELOG_INFO("Disconnected succesfully\n");
    }
    else {
        BTRCORELOG_INFO("Disconnect failed - %s\n",error->message);
    }
}

static void
btrCore_BTRemoveDeviceCb (
    BluezAdapter1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_adapter1_call_remove_device_finish(proxy,res,&error);

    if (result == 1  && error == NULL) {
        BTRCORELOG_INFO("Device removed\n");
    }
    else {
        BTRCORELOG_INFO("Failed to remove device - %s\n",error->message);
    }
}

static void
btrCore_BTSetDiscoveryFilterCb (
    BluezAdapter1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_adapter1_call_set_discovery_filter_finish(proxy,res,&error);

    if (result == TRUE && error == NULL) {
        BTRCORELOG_INFO("Succesfully set the discovery filter for LE devices\n");
    }
    else {
        BTRCORELOG_INFO("Setting discovery filter for LE devices failed - %s\n",error->message);
    }
}

static void
btrCore_BTGattCharStartNotifyCb (
    BluezGattCharacteristic1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_gatt_characteristic1_call_start_notify_finish(proxy,res,&error);

    if (result == TRUE && error == NULL) {
        BTRCORELOG_INFO("Performed start notify operation\n");
    }
    else {
        BTRCORELOG_INFO("Failed to perform start notify operation - %s\n",error->message);
    }
}

static void
btrCore_BTGattCharStopNotifyCb (
    BluezGattCharacteristic1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_gatt_characteristic1_call_stop_notify_finish(proxy,res,&error);

    if (result == TRUE && error == NULL) {
        BTRCORELOG_INFO("Performed stop notify operation \n");
    }
    else {
        BTRCORELOG_INFO("Failed to perform stop notify operation - %s\n",error->message);
    }
}

static void
btrCore_BTGattCharWriteValueCb (
    BluezGattCharacteristic1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_gatt_characteristic1_call_write_value_finish(proxy,res,&error);

    if (result == TRUE && error == NULL) {
        BTRCORELOG_INFO("Performed write value operation on gatt charateristic \n");
    }
    else {
        BTRCORELOG_INFO("Failed to perform write value operation on gatt characteristic - %s\n",error->message);
    }
}

static void
btrCore_BTGattDesWriteValueCb (
    BluezGattDescriptor1 *proxy,
    GAsyncResult *res,
    gpointer data
) {
    GError *error = NULL;
    gboolean result;
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)data;
    (void)pstlhBtIfce;

    result = bluez_gatt_descriptor1_call_write_value_finish(proxy,res,&error);

    if (result == TRUE && error == NULL) {
        BTRCORELOG_INFO("Performed write value operation on gatt descriptor\n");
    }
    else {
        BTRCORELOG_INFO("Failed to perform write value operation on gatt descriptor - %s\n",error->message);
    }

}

static int
btrCore_BTReqPasskeyCB (
    const gchar *lpcPath,
    void *apvUserData
) {
    stBtIfceHdl *pstlhBtIfce = (stBtIfceHdl *)apvUserData;
    unsigned int ui32PassCode = 0;

    if (pstlhBtIfce->pcBTOutPassCode)
    {
        BTRCORELOG_INFO("Pass code request for device %s\n", lpcPath);
        ui32PassCode = strtoul(pstlhBtIfce->pcBTOutPassCode, NULL, 10);
    }
    return ui32PassCode;
}

static int
btrCore_BTReqAutherizationCB (
    const gchar *lpcPath,
    void *apvUserData
) {
    stBtIfceHdl *pstlhBtIfce = (stBtIfceHdl *)apvUserData;
    int i32OpRet = -1;
    int yesNo = 0;
    enBTDeviceType lenBTDevType = enBTDevUnknown;
    stBTDeviceInfo *pstBTDeviceInfo = &pstlhBtIfce->lstBTDeviceInfo;

    memset(pstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));
    i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce, pstBTDeviceInfo, lpcPath);

    if (lpcPath)
    {
        i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce, pstBTDeviceInfo, lpcPath);
        lenBTDevType = btrCore_BTMapDevClasstoDevType(pstBTDeviceInfo->ui32Class);

        if (pstlhBtIfce->fpcBConnectionAuthentication)
        {
            BTRCORELOG_INFO("calling ConnAuth cb for %s - OpRet = %d\n", lpcPath, i32OpRet);
            yesNo = pstlhBtIfce->fpcBConnectionAuthentication(lenBTDevType, pstBTDeviceInfo, pstlhBtIfce->pcBConnAuthUserData);
        }
    }

    if (enBTDevAudioSource == lenBTDevType && yesNo)
    {
        strncpy(pstBTDeviceInfo->pcDeviceCurrState, "connected", BT_MAX_STR_LEN - 1);

        if (pstlhBtIfce->fpcBDevStatusUpdate)
        {
            pstlhBtIfce->fpcBDevStatusUpdate(lenBTDevType, enBTDevStPropChanged, pstBTDeviceInfo, pstlhBtIfce->pcBDevStatusUserData);
        }
    }

    return yesNo;
}

static int
btrCore_BTReqConfirmationCB (
    const gchar *lpcPath,
    guint ui32PassCode,
    void *apvUserData
) {
    stBtIfceHdl *pstlhBtIfce = (stBtIfceHdl *)apvUserData;
    int yesNo = 0;
    int i32OpRet = -1;
    enBTDeviceType lenBTDevType = enBTDevUnknown;
    stBTDeviceInfo *pstBTDeviceInfo = &pstlhBtIfce->lstBTDeviceInfo;
    memset(pstBTDeviceInfo, 0, sizeof(stBTDeviceInfo));

    BTRCORELOG_INFO("btrCore_BTAgentRequestConfirmation: PASS Code for %s is %6d\n", lpcPath, ui32PassCode);

    if (lpcPath)
    {
        i32OpRet = btrCore_BTGetDeviceInfo(pstlhBtIfce, pstBTDeviceInfo, lpcPath);
        lenBTDevType = btrCore_BTMapDevClasstoDevType(pstBTDeviceInfo->ui32Class);

        /* Set the ucIsReqConfirmation as 1; as we expect confirmation from the user */
        if (pstlhBtIfce->fpcBConnectionIntimation)
        {
            BTRCORELOG_INFO("calling ConnIntimation cb for %s - OpRet = %d\n", lpcPath, i32OpRet);
            yesNo = pstlhBtIfce->fpcBConnectionIntimation(lenBTDevType, pstBTDeviceInfo, ui32PassCode, 1, pstlhBtIfce->pcBConnIntimUserData);
        }
    }
    pstlhBtIfce->ui32cBConnAuthPassKey = ui32PassCode;

    return yesNo;
}

static void btrCore_BTAdvReleaseCB(const char *apBtAdapter, void *apvUserData){
    stBtIfceHdl *pstlhBtIfce = (stBtIfceHdl *)apvUserData;
    stBTAdapterInfo *pstBTAdapterInfo = NULL;

    if (pstlhBtIfce != NULL)
    {
        pstBTAdapterInfo = &pstlhBtIfce->lstBTAdapterInfo;
    }
    else {
        BTRCORELOG_INFO("User data was empty\n");
        return;
    }

    BTRCORELOG_INFO("Advertisement released CB.\n");
    if (pstlhBtIfce->fpcBAdapterStatusUpdate)
    {
        if (!pstlhBtIfce->fpcBAdapterStatusUpdate(enBTAdPropAdvTimeout, pstBTAdapterInfo, pstlhBtIfce->pcBAdapterStatusUserData))
        {
            BTRCORELOG_INFO("Adv released informed.\n");
        }
    }
}
