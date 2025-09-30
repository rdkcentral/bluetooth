#ifndef UNIT_TEST
#define UNIT_TEST
#endif
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


/* Local Headers */
#include "btrCore.h"
#include "btrCore_service.h"

#include "mock_btrCore_avMedia.h"

#include "mock_btrCore_le.h"


#include "mock_btrCore_bt_ifce.h"

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


void test_BTRCore_Init_NULL()
{
    BTRCore_Init(NULL);

}

void test_BTRCore_Init_ValidParameters(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterAdapterStatusUpdateCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterDevStatusUpdateCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterConnIntimationCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterConnAuthCb_IgnoreAndReturn(0);
    BTRCore_AVMedia_RegisterMediaStatusUpdateCb_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_LE_RegisterStatusUpdateCb_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterMediaStatusUpdateCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterNegotiateMediaCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterTransportPathMediaCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterMediaPlayerPathCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterMediaBrowserUpdateCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterLEAdvInfoCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterLEGattInfoCb_IgnoreAndReturn(0);
    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(0);
    BtrCore_BTReleaseAdapterPath_IgnoreAndReturn(0);
    BtrCore_BTReleaseAgentPath_IgnoreAndReturn(0);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(0);

    // Act
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    TEST_ASSERT_NOT_NULL(hBTRCore);

    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}    


void test_BTRCore_Init_should_ReturnFailure_when_InitializationFails(void)
{
    tBTRCoreHandle hBTRCore;
    stBTRCoreHdl handle_data;
    enBTRCoreRet expected_result = enBTRCoreInitFailure;
    enBTRCoreRet actual_result;

    BtrCore_BTInitGetConnection_ExpectAndReturn(NULL);

    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(NULL);
    
    actual_result = BTRCore_Init(&hBTRCore);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_RegisterAgent_should_RegisterBluetoothCoreAgentSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore =  malloc(sizeof(stBTRCoreHdl));
     //Assuming these are your actual adapter and agent paths in your code 
    hBTRCore->curAdapterPath = "/path/to/adapter";
    hBTRCore->agentPath = "/path/to/agent";

    int iBTRCapMode = 1;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    // Expectations for BtrCore_BTRegisterAgent
    BtrCore_BTRegisterAgent_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, hBTRCore->agentPath, "DisplayYesNo",0);
    
    actual_result = BTRCore_RegisterAgent(hBTRCore, iBTRCapMode);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_RegisterAgent_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    int iBTRCapMode = 1;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_RegisterAgent(hBTRCore, iBTRCapMode);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_RegisterAgent_should_ReturnFailure_when_BtrCore_BTRegisterAgentFails(void)
{
    stBTRCoreHdl* hBTRCore =  malloc(sizeof(stBTRCoreHdl));
    // Assuming these are your actual adapter and agent paths in your code
    hBTRCore->curAdapterPath = "/path/to/adapter";
    hBTRCore->agentPath = "/path/to/agent";

    int iBTRCapMode = 1;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;


    // BtrCore_BTRegisterAgent is expected to be called once and return -1 (failure)
    BtrCore_BTRegisterAgent_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, hBTRCore->agentPath, "DisplayYesNo",-1);
    
    actual_result = BTRCore_RegisterAgent(hBTRCore, iBTRCapMode);
    printf("Actual result: %d\n", actual_result);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_UnregisterAgent_should_UnregisterBluetoothCoreAgentSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
     //Assuming these are your actual adapter and agent paths in your code
    hBTRCore->curAdapterPath = "/path/to/adapter";
    hBTRCore->agentPath = "/path/to/agent";

    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    // Expectations for BtrCore_BTUnregisterAgent
    BtrCore_BTUnregisterAgent_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, hBTRCore->agentPath, 0);
    
    actual_result = BTRCore_UnregisterAgent(hBTRCore);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_UnregisterAgent_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_UnregisterAgent(hBTRCore);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_UnregisterAgent_should_ReturnFailure_when_BtrCore_BTUnregisterAgentFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    // Assuming these are your actual adapter and agent paths in your code
    hBTRCore->curAdapterPath = "/path/to/adapter";
    hBTRCore->agentPath = "/path/to/agent";

    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    // BtrCore_BTUnregisterAgent is expected to be called once and return -1 (failure)
    BtrCore_BTUnregisterAgent_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, hBTRCore->agentPath, -1);
    
    actual_result = BTRCore_UnregisterAgent(hBTRCore);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_GetListOfAdapters_should_GetBluetoothCoreAdaptersSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
     // Assuming these are your actual adapter and agent paths in your code 
    hBTRCore->curAdapterPath = "/path/to/adapter";
    hBTRCore->agentPath = "/path/to/agent";

    stBTRCoreListAdapters listAdapters;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;
    int numberOfAdapters = 1;

    // Initial data setup  
    hBTRCore->numOfAdapters = numberOfAdapters;
    hBTRCore->adapterPath[0] = strdup("/path/to/adapter");
    hBTRCore->adapterAddr[0] = strdup("adapter_address");

    // Expectations for BtrCore_BTGetAdapterList
    BtrCore_BTGetAdapterList_ExpectAndReturn(hBTRCore->connHdl, &hBTRCore->numOfAdapters, hBTRCore->adapterPath, 0); // Assuming 0 is success

    // Expectations for BtrCore_BTGetProp with ignoring all arguments
    BtrCore_BTGetProp_IgnoreAndReturn(0);  // Assuming 0 is success

    actual_result = BTRCore_GetListOfAdapters(hBTRCore, &listAdapters);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(numberOfAdapters, listAdapters.number_of_adapters);
    for (int i = 0; i < numberOfAdapters; ++i) {
        TEST_ASSERT_EQUAL_STRING(hBTRCore->adapterAddr[i], listAdapters.adapterAddr[i]);
    }

    free(hBTRCore);
}

void test_BTRCore_GetListOfAdapters_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreListAdapters listAdapters;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetListOfAdapters(hBTRCore, &listAdapters);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_GetListOfAdapters_should_ReturnInvalidArg_when_listAdaptersIsNULL(void)
{
    tBTRCoreHandle hBTRCore = malloc(sizeof(stBTRCoreHdl));
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetListOfAdapters(hBTRCore, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    free(hBTRCore);
}

void test_BTRCore_GetListOfAdapters_should_ReturnFailure_when_BtrCore_BTGetAdapterListFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    stBTRCoreListAdapters listAdapters;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    // Expectations for BtrCore_BTGetAdapterList
   // BtrCore_BTGetAdapterList_ExpectAnyArgsAndReturn(-1); // Assuming -1 is failure
    BtrCore_BTGetAdapterList_ExpectAndReturn(hBTRCore->connHdl, &hBTRCore->numOfAdapters, hBTRCore->adapterPath,-1);
    actual_result = BTRCore_GetListOfAdapters(hBTRCore, &listAdapters);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    free(hBTRCore);
}


void test_BTRCore_GetAdapters_should_GetBluetoothCoreAdaptersSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore =  malloc(sizeof(stBTRCoreHdl)); 
    stBTRCoreGetAdapters getAdaptors;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;
    int numberOfAdapters = 1;

    // Expectations for BtrCore_BTGetAdapterList
    hBTRCore->numOfAdapters = numberOfAdapters;
    BtrCore_BTGetAdapterList_ExpectAndReturn(hBTRCore->connHdl, &hBTRCore->numOfAdapters, hBTRCore->adapterPath, 0); // Assuming 0 is success

    actual_result = BTRCore_GetAdapters(hBTRCore, &getAdaptors);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(numberOfAdapters, getAdaptors.number_of_adapters);

    free(hBTRCore);
}

void test_BTRCore_GetAdapters_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreGetAdapters getAdaptors;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapters(hBTRCore, &getAdaptors);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_GetAdapters_should_ReturnInvalidArg_when_getAdaptorsIsNULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); // assuming that hBTRCore is malloced elsewhere
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapters(hBTRCore, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    free(hBTRCore);
}


void test_BTRCore_GetAdapter_should_GetBluetoothCoreAdapterSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 

    // Assuming these are your actual adapter path in your code
    hBTRCore->curAdapterPath = "/path/to/adapter"; 

    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    // No Expectations for BtrCore_BTGetAdapterPath because hBTRCore->curAdapterPath already contains a path

    actual_result = BTRCore_GetAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(hBTRCore->curAdapterPath, adapter.pcAdapterPath);

    free(hBTRCore);
}


void test_BTRCore_GetAdapter_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}
void test_BTRCore_GetAdapter_should_ReturnInvalidAdapter_when_BtrCore_BTGetAdapterPathFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreInvalidAdapter;
    enBTRCoreRet actual_result;

    hBTRCore->numOfAdapters = 0;
    hBTRCore->connHdl = NULL; // Or set it to a valid mock value if necessary
    hBTRCore->curAdapterPath = NULL;
    printf("Before calling BTRCore_GetAdapter, adapter.adapter_number: %d, adapter.pcAdapterPath: %p, adapter.pcAdapterDevName: %p\n",
           adapter.adapter_number, adapter.pcAdapterPath, adapter.pcAdapterDevName);
    
    BtrCore_BTGetAdapterPath_ExpectAndReturn((void*)hBTRCore->connHdl,NULL,NULL);
    
    actual_result = BTRCore_GetAdapter(hBTRCore, &adapter);
    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}


void test_BTRCore_GetAdapter_should_GetBluetoothCoreAdapterSuccessfully_when_curAdapterPathIsNull(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    hBTRCore->curAdapterPath = NULL; 

    char newAdapterPath[] = "/new/path/to/adapter"; 
    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;
   
    BtrCore_BTGetAdapterPath_ExpectAndReturn((void*)hBTRCore->connHdl, NULL, newAdapterPath);

    actual_result = BTRCore_GetAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(newAdapterPath, adapter.pcAdapterPath);

    free(hBTRCore);
}

void test_BTRCore_SetAdapter_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    int adapter_number = 1;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapter(hBTRCore, adapter_number);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_SetAdapter_should_UpdateAdapterNumberSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    // As per original code, we assume adapter path ends with adapter number, and allocate 2 extra chars
    hBTRCore->curAdapterPath = malloc(32);
    int adapter_number = 1;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    strcpy(hBTRCore->curAdapterPath, "/path/to/adapter0");

    actual_result = BTRCore_SetAdapter(hBTRCore, adapter_number);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter1", hBTRCore->curAdapterPath);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_SetAdapter_should_ClampInvalidAdapterNumber(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    // As per original code, we assume adapter path ends with adapter number, and allocate 2 extra chars
    hBTRCore->curAdapterPath = malloc(32);
    int adapter_number = 10;  // Invalid adapter number 
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    strcpy(hBTRCore->curAdapterPath, "/path/to/adapter0");

    actual_result = BTRCore_SetAdapter(hBTRCore, adapter_number);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter0", hBTRCore->curAdapterPath);  // Adapter number should be clamped to 0 

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_EnableAdapter_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_EnableAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_EnableAdapter_should_EnableBluetoothCoreAdapterSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    hBTRCore->curAdapterPath = strdup("/path/to/adapter");

    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;
    //int powered = 1;
   // unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_IgnoreAndReturn(0); // Assuming 0 is success

    actual_result = BTRCore_EnableAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_TRUE(adapter.enable);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_EnableAdapter_should_ReturnFailure_when_BtrCore_BTSetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    hBTRCore->curAdapterPath = "/path/to/adapter"; 

    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;
    //int powered = 1;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_IgnoreAndReturn( -1); // Assuming -1 is failure

    actual_result = BTRCore_EnableAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}


void test_BTRCore_DisableAdapter_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_DisableAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_DisableAdapter_should_DisableBluetoothCoreAdapterSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
	hBTRCore->curAdapterPath = strdup("/path/to/adapter"); 
	stBTRCoreAdapter adapter;

    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;
	
    int powered = 0;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropPowered;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &powered, 0); // Assuming 0 is success

    actual_result = BTRCore_DisableAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_FALSE(adapter.enable);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_DisableAdapter_should_ReturnFailure_when_BtrCore_BTSetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    hBTRCore->curAdapterPath = strdup("/path/to/adapter");

    stBTRCoreAdapter adapter;

    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    int powered = 0;
    unBTOpIfceProp prop;

    prop.enBtAdapterProp = enBTAdPropPowered;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &powered, -1); // Assuming -1 indicates failure

    actual_result = BTRCore_DisableAdapter(hBTRCore, &adapter);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_GetAdapterAddr_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    unsigned char adapterIdx = 0;
    char adapterAddr[BD_NAME_LEN];
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterAddr(hBTRCore, adapterIdx, adapterAddr);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_GetAdapterAddr_should_ReturnInvalidArg_when_adapterAddr_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    unsigned char adapterIdx = 0;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterAddr(hBTRCore, adapterIdx, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_GetAdapterAddr_should_GetBluetoothCoreAdapterAddressSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    unsigned char adapterIdx = 0;
    char adapterAddr[BD_NAME_LEN];
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;
    char* expectedAddr = NULL;

    hBTRCore->numOfAdapters = 2;
    hBTRCore->adapterAddr[0] = strdup(" addr0 ");
    hBTRCore->adapterAddr[1] = strdup(" addr1 ");

    expectedAddr = hBTRCore->adapterAddr[adapterIdx];
    actual_result = BTRCore_GetAdapterAddr(hBTRCore, adapterIdx, adapterAddr);
    printf("expectedAddr: %s, adapterAddr: %s\n", expectedAddr, adapterAddr);
    printf("expectedAddr length: %zu\n", strlen(expectedAddr));
    printf("adapterAddr length: %zu\n", strlen(adapterAddr));

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL_STRING(expectedAddr, adapterAddr);

    free(hBTRCore->adapterAddr[0]);
    free(hBTRCore->adapterAddr[1]);
    free(hBTRCore);
}

void test_BTRCore_GetAdapterAddr_should_Fail_when_IndexIsOutOfRange(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    unsigned char adapterIdx = 2; // Out of range index 
    char adapterAddr[BD_NAME_LEN];
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    hBTRCore->numOfAdapters = 2;
    hBTRCore->adapterAddr[0] = strdup("addr0");
    hBTRCore->adapterAddr[1] = strdup("addr1");

    actual_result = BTRCore_GetAdapterAddr(hBTRCore, adapterIdx, adapterAddr);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->adapterAddr[0]);
    free(hBTRCore->adapterAddr[1]);
    free(hBTRCore);
}


void test_BTRCore_SetAdapterDiscoverable_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    char* adapterPath = "/path/to/adapter";
    unsigned char discoverable = 1;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterDiscoverable(hBTRCore, adapterPath, discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_SetAdapterDiscoverable_should_ReturnInvalidArg_when_adapterPath_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    unsigned char discoverable = 1;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterDiscoverable(hBTRCore, NULL, discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_SetAdapterDiscoverable_should_SetBluetoothCoreAdapterDiscoverableSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    hBTRCore->curAdapterPath = strdup("/path/to/adapter"); 

    const char* adapterPath = hBTRCore->curAdapterPath;
    unsigned char discoverable = 1;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    int isDiscoverable = (int)discoverable;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropDiscoverable;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &isDiscoverable, 0); // Assuming 0 is success

    actual_result = BTRCore_SetAdapterDiscoverable(hBTRCore, adapterPath, discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_SetAdapterDiscoverable_should_ReturnFailure_when_BtrCore_BTSetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    hBTRCore->curAdapterPath = strdup("/path/to/adapter"); 

    const char* adapterPath = hBTRCore->curAdapterPath;
    unsigned char discoverable = 1;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    int isDiscoverable = (int)discoverable;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropDiscoverable;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &isDiscoverable, -1); // Assuming -1 is failure

    actual_result = BTRCore_SetAdapterDiscoverable(hBTRCore, adapterPath, discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_SetAdapterDiscoverableTimeout_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    const char* adapterPath = "/path/to/adapter";
    unsigned short timeout = 10;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterDiscoverableTimeout(hBTRCore, adapterPath, timeout);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_SetAdapterDiscoverableTimeout_should_ReturnInvalidArg_when_adapterPath_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    unsigned short timeout = 10;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterDiscoverableTimeout(hBTRCore, NULL, timeout);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_SetAdapterDiscoverableTimeout_should_SetBluetoothCoreAdapterDiscoverableTimeoutSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    hBTRCore->curAdapterPath = strdup("/path/to/adapter"); 

    const char* adapterPath = hBTRCore->curAdapterPath;
    unsigned short timeout = 10;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    unsigned int givenTimeout = (unsigned int)timeout;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropDiscoverableTimeOut;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &givenTimeout, 0); // Assuming 0 is success

    actual_result = BTRCore_SetAdapterDiscoverableTimeout(hBTRCore, adapterPath, timeout);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_SetAdapterDiscoverableTimeout_should_ReturnFailure_when_BtrCore_BTSetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    hBTRCore->curAdapterPath = strdup("/path/to/adapter");
    
    const char* adapterPath = hBTRCore->curAdapterPath;
    unsigned short timeout = 10;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    unsigned int givenTimeout = (unsigned int)timeout;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropDiscoverableTimeOut;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &givenTimeout, -1); // Assuming -1 is failure

    actual_result = BTRCore_SetAdapterDiscoverableTimeout(hBTRCore, adapterPath, timeout);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}


void test_BTRCore_GetAdapterDiscoverableStatus_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    const char* adapterPath = "/path/to/adapter";
    unsigned char discoverable;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterDiscoverableStatus(hBTRCore, adapterPath, &discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}
void test_BTRCore_GetAdapterDiscoverableStatus_should_ReturnInvalidArg_when_adapterPath_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    unsigned char discoverable;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterDiscoverableStatus(hBTRCore, NULL, &discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_GetAdapterDiscoverableStatus_should_ReturnInvalidArg_when_Discoverable_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    const char* adapterPath = "/path/to/adapter";
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterDiscoverableStatus(hBTRCore, adapterPath, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}
void test_BTRCore_GetAdapterDiscoverableStatus_should_GetBluetoothCoreAdapterDiscoverableStatusSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 

    const char* adapterPath = "/path/to/adapter";
    unsigned char discoverable;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    unsigned char isDiscoverable = 0;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropDiscoverable;

    // Expectations for BtrCore_BTGetProp
    BtrCore_BTGetProp_ExpectAndReturn(hBTRCore->connHdl,adapterPath, enBTAdapter, prop, &isDiscoverable, 0); // Assuming 0 is success

    actual_result = BTRCore_GetAdapterDiscoverableStatus(hBTRCore, adapterPath, &discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL_UINT8(isDiscoverable,discoverable);

    free(hBTRCore);
}


void test_BTRCore_GetAdapterDiscoverableStatus_should_ReturnFailure_when_BtrCore_BTGetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    const char* adapterPath = "/path/to/adapter";
    unsigned char discoverable;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;
 
    unsigned char isDiscoverable = 0;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropDiscoverable;

    // Expectations for BtrCore_BTGetProp
    BtrCore_BTGetProp_ExpectAndReturn(hBTRCore->connHdl, adapterPath, enBTAdapter, prop, &isDiscoverable, -1); // Assuming -1 is failure

    actual_result = BTRCore_GetAdapterDiscoverableStatus(hBTRCore, adapterPath, &discoverable);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_SetAdapterDeviceName_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreAdapter adapter;
    char* deviceName = "DeviceName";
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterDeviceName(hBTRCore, &adapter, deviceName);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}


void test_BTRCore_SetAdapterDeviceName_should_ReturnInvalidArg_when_deviceName_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    stBTRCoreAdapter adapter;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterDeviceName(hBTRCore, &adapter, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}



void test_BTRCore_SetAdapterDeviceName_should_SetBluetoothCoreAdapterDeviceNameSuccessfully(void) {
    // Allocate memory for the BTRCore handle and initialize necessary members
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    TEST_ASSERT_NOT_NULL(hBTRCore);  // Ensure memory allocation succeeded

    hBTRCore->curAdapterPath = strdup("/path/to/adapter"); 
    TEST_ASSERT_NOT_NULL(hBTRCore->curAdapterPath);  // Ensure memory allocation succeeded

    // Initialize the adapter structure and device name
    stBTRCoreAdapter adapter = {0};
    char* deviceName = "DeviceName";
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropName;

    // Expectations for BtrCore_BTSetProp
    // Note: Since BTRCore_SetAdapterDeviceName duplicates the string, use adapter.pcAdapterDevName in the expectation
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &adapter.pcAdapterDevName, 0); // Assuming 0 is success

    // Call the function being tested
    actual_result = BTRCore_SetAdapterDeviceName(hBTRCore, &adapter, deviceName);

    // Validate the results
    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL_STRING(deviceName, adapter.pcAdapterDevName);

    // Clean up allocated memory
    free(adapter.pcAdapterDevName);
    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

// Mock function
int mock_BtrCore_BTSetProp2(void *apBtConn,const char* pAdapterPath,enBTDeviceType enBTDeviceType,unBTOpIfceProp prop,const void* pValue) {
    return 0; // Simulate failure
}
int _mockgetprop(void *apBtConn, const char *apcOpIfcePath, enBTOpIfceType aenBtOpIfceType, unBTOpIfceProp aunBtOpIfceProp, void *apvVal)
{
 strncpy(apvVal ,"hello",strlen("hello"));
    
    return 0;
}


int _mockgetsupported
 (
    void*                   apBtConn,
    const char*             apBtAdapter,
    stBTPairedDeviceInfo*   pairedDevices
){
    //stBTPairedDeviceInfo pairedDevices = {0};
 
    pairedDevices->numberOfDevices = 2;
 
    // Initialize first device
    strcpy(pairedDevices->devicePath[0], "/path/to/device1");
    pairedDevices->deviceInfo[0].bPaired = 1;
    pairedDevices->deviceInfo[0].bConnected = 1;
    pairedDevices->deviceInfo[0].bTrusted = 1;
    pairedDevices->deviceInfo[0].bBlocked = 0;
    pairedDevices->deviceInfo[0].bServicesResolved = 1;
    pairedDevices->deviceInfo[0].ui16Vendor = 1234;
    pairedDevices->deviceInfo[0].ui16VendorSource = 1;
    pairedDevices->deviceInfo[0].ui16Product = 5678;
    pairedDevices->deviceInfo[0].ui16Version = 1;
    pairedDevices->deviceInfo[0].ui32Class = 0x1F00;
    pairedDevices->deviceInfo[0].i32RSSI = -40;
    pairedDevices->deviceInfo[0].ui16Appearance = 0;
    pairedDevices->deviceInfo[0].i16TxPower = 0;
    pairedDevices->deviceInfo[0].bLegacyPairing = 0;
    strcpy(pairedDevices->deviceInfo[0].pcModalias, "modalias1");
    strcpy(pairedDevices->deviceInfo[0].pcAdapter, "adapter1");
    strcpy(pairedDevices->deviceInfo[0].pcName, "Device1");
    strcpy(pairedDevices->deviceInfo[0].pcAddress, "00:11:22:33:44:55");
    strcpy(pairedDevices->deviceInfo[0].pcAlias, "Alias1");
    strcpy(pairedDevices->deviceInfo[0].pcIcon, "icon1");
    strcpy(pairedDevices->deviceInfo[0].pcDevicePrevState, "prevState1");
    strcpy(pairedDevices->deviceInfo[0].pcDeviceCurrState, "currState1");
    strcpy(pairedDevices->deviceInfo[0].pcDevicePath, "/path/to/device1");
    strcpy(pairedDevices->deviceInfo[0].aUUIDs[0], "uuid1");
    strcpy(pairedDevices->deviceInfo[0].saServices[0].pcUUIDs, "serviceUUID1");
    pairedDevices->deviceInfo[0].saServices[0].len = 0;
 
    // Initialize second device
    strcpy(pairedDevices->devicePath[1], "/path/to/device2");
    pairedDevices->deviceInfo[1].bPaired = 1;
    pairedDevices->deviceInfo[1].bConnected = 0;
    pairedDevices->deviceInfo[1].bTrusted = 0;
    pairedDevices->deviceInfo[1].bBlocked = 0;
    pairedDevices->deviceInfo[1].bServicesResolved = 0;
    pairedDevices->deviceInfo[1].ui16Vendor = 4321;
    pairedDevices->deviceInfo[1].ui16VendorSource = 2;
    pairedDevices->deviceInfo[1].ui16Product = 8765;
    pairedDevices->deviceInfo[1].ui16Version = 2;
    pairedDevices->deviceInfo[1].ui32Class = 0x2F00;
    pairedDevices->deviceInfo[1].i32RSSI = -50;
    pairedDevices->deviceInfo[1].ui16Appearance = 1;
    pairedDevices->deviceInfo[1].i16TxPower = 1;
    pairedDevices->deviceInfo[1].bLegacyPairing = 1;
    strcpy(pairedDevices->deviceInfo[1].pcModalias, "modalias2");
    strcpy(pairedDevices->deviceInfo[1].pcAdapter, "adapter2");
    strcpy(pairedDevices->deviceInfo[1].pcName, "Device2");
    strcpy(pairedDevices->deviceInfo[1].pcAddress, "66:77:88:99:AA:BB");
    strcpy(pairedDevices->deviceInfo[1].pcAlias, "Alias2");
    strcpy(pairedDevices->deviceInfo[1].pcIcon, "icon2");
    strcpy(pairedDevices->deviceInfo[1].pcDevicePrevState, "prevState2");
    strcpy(pairedDevices->deviceInfo[1].pcDeviceCurrState, "currState2");
    strcpy(pairedDevices->deviceInfo[1].pcDevicePath, "/path/to/device2");
    strcpy(pairedDevices->deviceInfo[1].aUUIDs[0], "uuid2");
    strcpy(pairedDevices->deviceInfo[1].saServices[0].pcUUIDs, "serviceUUID2");
    pairedDevices->deviceInfo[1].saServices[0].len = 0;



    for (int i = 0; i < pairedDevices->numberOfDevices; i++) {
        printf("Device %d Path: %s, Address: %s\n", 
               i, pairedDevices->devicePath[i], pairedDevices->deviceInfo[i].pcAddress);
    }
 
    return 0;
 
}

int
mock_BTDiscoverDevice (
    void*                           apstBtIfceHdl,
    const char*                     apcDevPath,
    stBTDeviceSupportedServiceList* pProfileList
) {
    
    pProfileList->numberOfService = 2;
    if(pProfileList->numberOfService == 2)
    return 0;
    else 
    return  -1;
}

void test_BTRCore_SetAdapterDeviceName_should_ReturnFailure_when_BtrCore_BTSetPropFails(void) {
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    TEST_ASSERT_NOT_NULL(hBTRCore);  // Ensure memory allocation succeeded

    hBTRCore->curAdapterPath = strdup("/path/to/adapter"); 
    TEST_ASSERT_NOT_NULL(hBTRCore->curAdapterPath);  // Ensure memory allocation succeeded

    // Initialize the adapter structure and device name
    stBTRCoreAdapter adapter = {0};
    char* deviceName = "DeviceName";
    enBTRCoreRet expected_result = enBTRCoreFailure; // Expect failure
    enBTRCoreRet actual_result;

    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropName;

    // Expectations for BtrCore_BTSetProp
   // BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp2);

     BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &adapter.pcAdapterDevName, -1); // Assuming 0 is success


    // Call the function being tested
    actual_result = BTRCore_SetAdapterDeviceName(hBTRCore, &adapter, deviceName);

    // Validate the results
    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL_STRING(deviceName, adapter.pcAdapterDevName);

    // Clean up allocated memory
    free(adapter.pcAdapterDevName);
    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
} 
void test_BTRCore_GetAdapterName_should_GetBluetoothCoreAdapterNameSuccessfully(void) {
    // Allocate memory for the BTRCore handle and check if it's not null
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    TEST_ASSERT_NOT_NULL(hBTRCore);  // Ensure memory allocation succeeded

    const char* adapterPath = "/path/to/adapter";
    char adapterName[BD_NAME_LEN] = {0};  // Initialize with zeros
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

     
    char name[BD_NAME_LEN] = "hello";
    unsigned int nameSize = strlen(name) < BD_NAME_LEN ? strlen(name) : BD_NAME_LEN - 1;

    unBTOpIfceProp prop;
    prop.enBtAdapterProp = enBTAdPropName;

    // Expectations for BtrCore_BTGetProp
    // Pass 'name' correctly to simulate the behavior as the real function will modify this value.
    BtrCore_BTGetProp_StubWithCallback(_mockgetprop);
   //BtrCore_BTGetProp_ExpectAndReturn(hBTRCore->connHdl, adapterPath, enBTAdapter, prop, name , 0);
   
   //BtrCore_BTGetProp_IgnoreAndReturn(enBTRCoreSuccess);
   
    // Call the function being tested
    actual_result = BTRCore_GetAdapterName(hBTRCore, adapterPath, adapterName);

    // Validate the results
    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL_MEMORY(name, adapterName, nameSize);
    
    free(hBTRCore);
}




void test_BTRCore_GetAdapterName_should_ReturnFailure_when_BtrCore_BTGetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    const char* adapterPath = "/path/to/adapter";
    char adapterName[BD_NAME_LEN];
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    char name[BD_NAME_LEN + 1]; // +1 for null terminator
    memset(name, 0, sizeof(name)); // Initialize to zero
   // char name[BD_NAME_LEN] = "Name";
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropName;

    // Expectations for BtrCore_BTGetProp
    BtrCore_BTGetProp_ExpectAndReturn(hBTRCore->connHdl, adapterPath, enBTAdapter, prop, name, -1); // Assuming -1 is failure

    actual_result = BTRCore_GetAdapterName(hBTRCore, adapterPath, adapterName);
    //printf("Expected result: %d\n", expected_result);
    //printf("Actual result:   %d\n", actual_result);
    
    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}


void test_BTRCore_SetAdapterPower_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    const char* adapterPath = "/path/to/adapter";
    unsigned char powerStatus = 1;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterPower(hBTRCore, adapterPath, powerStatus);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_SetAdapterPower_should_ReturnInvalidArg_when_adapterPath_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    unsigned char powerStatus = 1;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_SetAdapterPower(hBTRCore, NULL, powerStatus);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_SetAdapterPower_should_SetBluetoothCoreAdapterPowerSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    hBTRCore->curAdapterPath = strdup("/path/to/adapter"); 

    const char* adapterPath = hBTRCore->curAdapterPath;
    unsigned char powerStatus = 1;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    int power = (int)powerStatus;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropPowered;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &power, 0); // Assuming 0 is success

    actual_result = BTRCore_SetAdapterPower(hBTRCore, adapterPath, powerStatus);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_SetAdapterPower_should_ReturnFailure_when_BtrCore_BTSetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    hBTRCore->curAdapterPath = strdup("/path/to/adapter");
    
    const char* adapterPath = hBTRCore->curAdapterPath;
    unsigned char powerStatus = 1;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    int power = (int)powerStatus;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropPowered;

    // Expectations for BtrCore_BTSetProp
    BtrCore_BTSetProp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, enBTAdapter, prop, &power, -1); // Assuming -1 is failure

    actual_result = BTRCore_SetAdapterPower(hBTRCore, adapterPath, powerStatus);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}


void test_BTRCore_GetAdapterPower_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    const char* adapterPath = "/path/to/adapter";
    unsigned char powerStatus;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterPower(hBTRCore, adapterPath, &powerStatus);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_GetAdapterPower_should_ReturnInvalidArg_when_adapterPath_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    unsigned char powerStatus;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterPower(hBTRCore, NULL, &powerStatus);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_GetAdapterPower_should_ReturnInvalidArg_when_powerStatus_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    const char* adapterPath = "/path/to/adapter";
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetAdapterPower(hBTRCore, adapterPath, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}

void test_BTRCore_GetAdapterPower_should_GetBluetoothCoreAdapterPowerSuccessfully(void) {
    // Allocate memory for the BTRCore handle and check if it's not null
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    TEST_ASSERT_NOT_NULL(hBTRCore);  // Ensure memory allocation succeeded

    const char* adapterPath = "/path/to/adapter";
    unsigned char powerStatus;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    int power = 0;  
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropPowered;

    // Expectations for BtrCore_BTGetProp
    // Note: The actual function will pass the address of powerStatus, so we should use that in the expectation
    BtrCore_BTGetProp_ExpectAndReturn(hBTRCore->connHdl, adapterPath, enBTAdapter, prop, &powerStatus, 0); // Assuming 0 is success

    // Call the function being tested
    actual_result = BTRCore_GetAdapterPower(hBTRCore, adapterPath, &powerStatus);

    // Validate the results
    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(power, (int)powerStatus);

    // Clean up allocated memory
    free(hBTRCore);
}


void test_BTRCore_GetAdapterPower_should_ReturnFailure_when_BtrCore_BTGetPropFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    const char* adapterPath = "/path/to/adapter";
    unsigned char powerStatus;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    int power = 0;
    unBTOpIfceProp prop;

    // Initialize the union type with the appropriate value
    prop.enBtAdapterProp = enBTAdPropPowered;

    // Expectations for BtrCore_BTGetProp
    BtrCore_BTGetProp_ExpectAndReturn(hBTRCore->connHdl, adapterPath, enBTAdapter, prop, &power, -1);
    //BtrCore_BTGetProp_ExpectAndReturn(hBTRCore->connHdl, adapterPath, enBTAdapter, prop, &power, -1); // Assuming -1 is failure

    actual_result = BTRCore_GetAdapterPower(hBTRCore, adapterPath, &powerStatus);
    printf("test_BTRCore_GetAdapterPower: power: %d, powerStatus: %d\n", power, powerStatus);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}


void test_BTRCore_GetVersionInfo_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    char version[BTRCORE_STR_LEN];
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetVersionInfo(hBTRCore, version);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_GetVersionInfo_should_ReturnInvalidArg_when_version_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetVersionInfo(hBTRCore, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}



void test_BTRCore_GetVersionInfo_should_ReturnFailure_when_BtrCore_BTGetIfceNameVersionFails(void) {
    // Allocate memory for the BTRCore handle and check if it's not null
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    TEST_ASSERT_NOT_NULL(hBTRCore);  // Ensure memory allocation succeeded

    // Prepare buffers for the version and interface name
    char version[BTRCORE_STR_LEN];
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    // Prepare the expected interface name and version
    char ifceName[BTRCORE_STR_LEN] = "lBtIfceName";
    char btVersion[BTRCORE_STR_LEN] = "lBtVersion";

    // Expectation: We expect BtrCore_BTGetIfceNameVersion to be called and fail (-1 return value)
    // Note: The buffers for the interface name and version passed to the mock should be initialized as empty strings
    BtrCore_BTGetIfceNameVersion_ExpectAndReturn(hBTRCore->connHdl, "", "", -1);

    // Call the function being tested
    actual_result = BTRCore_GetVersionInfo(hBTRCore, version);
    
    // Validate the result
    TEST_ASSERT_EQUAL(expected_result, actual_result);

    // Clean up allocated memory
    free(hBTRCore);
}

void test_BTRCore_StartDiscovery_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    const char* adapterPath = "/path/to/adapter";
    enBTRCoreDeviceType deviceType = enBTRCoreLE;
    unsigned int discDuration = 5;

    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_StartDiscovery(hBTRCore, adapterPath, deviceType, discDuration);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_StartDiscovery_should_ReturnInvalidArg_when_AdapterPathIsNULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    enBTRCoreDeviceType deviceType = enBTRCoreLE;
    unsigned int discDuration = 5;

    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_StartDiscovery(hBTRCore, NULL, deviceType, discDuration);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}


void test_BTRCore_StopDiscovery_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    const char* adapterPath = "/path/to/adapter";
    enBTRCoreDeviceType enType = enBTRCoreLE;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_StopDiscovery(hBTRCore, adapterPath, enType);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_StopDiscovery_should_ReturnInvalidArg_when_adapterPath_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    enBTRCoreDeviceType enType = enBTRCoreLE;
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_StopDiscovery(hBTRCore, NULL, enType);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
} 


void test_BTRCore_GetSupportedServices_NullPointers(void) {
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreSupportedServiceList profileList;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;
     actual_result=BTRCore_GetSupportedServices(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(expected_result,actual_result);
}



void test_BTRCore_GetSupportedServices_NullHandle(void) {
    stBTRCoreSupportedServiceList profileList;
    enBTRCoreRet ret = BTRCore_GetSupportedServices(NULL, 1, &profileList);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_GetSupportedServices_NullProfileList(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    enBTRCoreRet ret = BTRCore_GetSupportedServices(hBTRCore, 1, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_GetSupportedServices_InvalidDevId(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    stBTRCoreSupportedServiceList profileList;
    enBTRCoreRet ret = BTRCore_GetSupportedServices(hBTRCore, 0, &profileList);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_GetSupportedServices_Success(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    stBTRCoreSupportedServiceList profileList;
    stBTRCoreHdl coreHandle;
    stBTDeviceSupportedServiceList mockProfileList;
 
    coreHandle.connHdl = (void*)1;
    coreHandle.curAdapterPath = "/org/bluez/hci0";
//  hBTRCore->curAdapterPath = "/test/path";
    mockProfileList.numberOfService = 2;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
   // btrCore_GetDeviceInfoKnown_ExpectAndReturn(&coreHandle, 1, enBTRCoreUnknown, enBTRCoreSuccess);
   BtrCore_BTDiscoverDeviceServices_StubWithCallback(mock_BTDiscoverDevice);
   // BtrCore_BTDiscoverDeviceServices_ExpectAndReturn(coreHandle.connHdl,coreHandle.curAdapterPath, &mockProfileList, 0);
 
    enBTRCoreRet ret = BTRCore_GetSupportedServices(&coreHandle, 1, &profileList);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(2, profileList.numberOfService);
}

void test_BTRCore_GetSupportedServices_DiscoveryFailure(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    stBTRCoreSupportedServiceList profileList;
    stBTRCoreHdl coreHandle;

    coreHandle.connHdl = (void*)1;
    coreHandle.curAdapterPath = "/org/bluez/hci0";
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
     BtrCore_BTDiscoverDeviceServices_StubWithCallback(mock_BTDiscoverDevice);
     //btrCore_GetDeviceInfoKnown_ExpectAndReturn(&coreHandle, 1, enBTRCoreUnknown, enBTRCoreSuccess);
    // BtrCore_BTDiscoverDeviceServices_ExpectAndReturn(coreHandle.connHdl, coreHandle.curAdapterPath, NULL, -1);

    enBTRCoreRet ret = BTRCore_GetSupportedServices(&coreHandle, -1, &profileList);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

int mock_BTIsDeviceConnectable (
        void*                           apstBtIfceHdl,
        const char*                     apcDevPath,
        enBTDeviceType                  aenBtDeviceType,
        enBTDeviceClass                 aenBtDeviceClass,
        enBTDeviceType*                 apenBtDeviceType,
        stBTRCoreBTDevice*              apstFoundDevice
) 
{
    if (!apstBtIfceHdl || !apcDevPath || !apenBtDeviceType || !apstFoundDevice) {
        return -1;
    }
    // Simulate the behavior of the original function
    // For example, we can assume that if the device path contains "valid", it returns 1 (match found)
    if (strstr(apcDevPath, "valid") != NULL) {
        return 1;
    }
    // Otherwise, return 0 (no match found)
    return 0;
}
int mock_BTRCore_AVMedia_GetElementTrackInfo(
    tBTRCoreAVMediaHdl avMediaHdl,
    const char* pDeviceAddress,
    tBTRCoreMediaElementId aBtrMediaElementId,
    stBTRCoreAVMediaTrackInfo* apstBTMediaTrackInfo
)
 {
    apstBTMediaTrackInfo->ui32TrackNumber = 1;
    strncpy(apstBTMediaTrackInfo->pcTitle, "Test Track", sizeof(apstBTMediaTrackInfo->pcTitle) - 1);
    apstBTMediaTrackInfo->pcTitle[sizeof(apstBTMediaTrackInfo->pcTitle) - 1] = '\0';
        return enBTRCoreSuccess;
 }


int _mock_BTGetPairedDeviceInfo1(
    void* apBtConn, const char* apBtAdapter, stBTPairedDeviceInfo* pairedDevices
) {
    pairedDevices->numberOfDevices = 1;
    pairedDevices->deviceInfo[0].bPaired = 1;
    strcpy(pairedDevices->deviceInfo[0].pcAddress, "00:11:22:33:44:55");
    return 0;
}
int _mock_BTGetPairedDeviceInfo(
    void* apBtConn, const char* apBtAdapter, stBTPairedDeviceInfo* pairedDevices
) {
    pairedDevices->numberOfDevices = 1;
    pairedDevices->deviceInfo[0].bPaired = 1;
    strcpy(pairedDevices->deviceInfo[0].pcAddress, "00:11:22:33:44:55");

    return 0;
}
 

void test_BTRCore_IsDeviceConnectable_NULL_hBTRCore(void) {
    enBTRCoreRet ret = BTRCore_IsDeviceConnectable(NULL, 0);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_IsDeviceConnectable_numOfPairedDevices_zero(void) {
    stBTRCoreHdl hBTRCore;
    hBTRCore.numOfPairedDevices = 0;
    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_IsDeviceConnectable(&hBTRCore, 0);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_IsDeviceConnectable_aBTRCoreDevId_out_of_range(void) {
    stBTRCoreHdl hBTRCore;
    hBTRCore.numOfPairedDevices = 1;
    enBTRCoreRet ret = BTRCore_IsDeviceConnectable(&hBTRCore, BTRCORE_MAX_NUM_BT_DEVICES + 1);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

void test_BTRCore_IsDeviceConnectable_pDeviceMac_NULL(void) {
    stBTRCoreHdl hBTRCore;
    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.stKnownDevicesArr[0].pcDeviceAddress[0] = '\0';
    enBTRCoreRet ret = BTRCore_IsDeviceConnectable(&hBTRCore, 0);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

void test_BTRCore_IsDeviceConnectable_pDeviceMac_empty(void) {
    stBTRCoreHdl hBTRCore;
    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.stKnownDevicesArr[0].pcDeviceAddress[0] = "";
    BtrCore_BTIsDeviceConnectable_IgnoreAndReturn(1);
    enBTRCoreRet ret = BTRCore_IsDeviceConnectable(&hBTRCore, 0);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_IsDeviceConnectable_BTIsDeviceConnectable_non_zero(void) {
    stBTRCoreHdl hBTRCore;
    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.stKnownDevicesArr[0].pcDeviceAddress[0] = "00:11:22:33:44:55";
    BtrCore_BTIsDeviceConnectable_IgnoreAndReturn(1);
  //  BtrCore_BTIsDeviceConnectable_ExpectAndReturn(NULL, "00:11:22:33:44:55", 1);
   // BtrCore_BTIsDeviceConnectable_StubWithCallback(mock_BTIsDeviceConnectable);
    enBTRCoreRet ret = BTRCore_IsDeviceConnectable(&hBTRCore, 0);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_IsDeviceConnectable_BTIsDeviceConnectable_zero(void) {
    stBTRCoreHdl hBTRCore;
    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.stKnownDevicesArr[0].pcDeviceAddress[0] = "00:11:22:33:44:55";
    BtrCore_BTIsDeviceConnectable_IgnoreAndReturn(0);
   // BtrCore_BTIsDeviceConnectable_ExpectAndReturn(NULL, "00:11:22:33:44:55", 0);
    enBTRCoreRet ret = BTRCore_IsDeviceConnectable(&hBTRCore, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_ConnectDevice_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_ConnectDevice(NULL, 0, enBTRCoreSpeakers);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}
void test_BTRCore_ConnectDevice_InvalidDeviceId(void) {
    // Allocate memory for hBTRCore and initialize its members
    tBTRCoreHandle hBTRCore = malloc(sizeof(stBTRCoreHdl));
    if (hBTRCore == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    enBTRCoreRet ret;

    // Stub the BtrCore_BTGetPairedDeviceInfo function
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);

    // Call the function under test
    ret = BTRCore_ConnectDevice(hBTRCore, BTRCORE_MAX_NUM_BT_DEVICES, enBTRCoreSpeakers);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);

    // Free the allocated memory
    free(hBTRCore);
}

void test_BTRCore_SetDeviceDataAckTimeout_Success(void) {
    // Allocate memory for the handle
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    if (hBTRCore == NULL) {
        TEST_FAIL_MESSAGE("Failed to allocate memory for hBTRCore");
        return;
    }

    // Initialize the structure to avoid undefined behavior
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    unsigned int aui32AckTOutms = 1000;

    // Set up expectations
    BtrCore_BTSetDevDataAckTimeout_IgnoreAndReturn( 0);

    // Call the function under test
    enBTRCoreRet result = BTRCore_SetDeviceDataAckTimeout(hBTRCore, aui32AckTOutms);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    // Clean up
    free(hBTRCore);
}

void test_BTRCore_SetDeviceDataAckTimeout_NullHandle(void) {
    unsigned int aui32AckTOutms = 1000;

    enBTRCoreRet result = BTRCore_SetDeviceDataAckTimeout(NULL, aui32AckTOutms);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, result);
}


void test_BTRCore_SetDeviceDataAckTimeout_Failure(void) {
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    unsigned int aui32AckTOutms = 1000;
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));
    BtrCore_BTSetDevDataAckTimeout_IgnoreAndReturn(-1);

    enBTRCoreRet result = BTRCore_SetDeviceDataAckTimeout(hBTRCore, aui32AckTOutms);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}



void test_BTRCore_MediaControl_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_MediaControl(NULL, 0, enBTDevUnknown, enBTRCoreMediaCtrlPlay, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}




void test_BTRCore_MediaControl_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice device;
    const char* deviceAddress = "/path/to/device1";

   
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&device, 0, sizeof(stBTRCoreBTDevice));


    device.bDeviceConnected = FALSE;
    device.pcDeviceName[0] = "Device1";

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    BTRCore_AVMedia_MediaControl_ExpectAndReturn(hBTRCore.avMediaHdl, deviceAddress, enBTRCoreMediaCtrlPlay, eBTRCoreAVMediaFlowUnknown, NULL, "Device1", enBTRCoreFailure);

    // Call the function under test
    enBTRCoreRet ret = BTRCore_MediaControl(&hBTRCore, 0, enBTDevUnknown, enBTRCoreMediaCtrlPlay, NULL);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}






void test_BTRCore_MediaControl_Play_Success(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice ;
    
    const char* deviceAddress = "/path/to/device1";
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    knownDevice.bDeviceConnected = TRUE, 
    knownDevice.pcDeviceName[0] = "Device1";


    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    BTRCore_AVMedia_MediaControl_ExpectAndReturn(hBTRCore.avMediaHdl, deviceAddress, enBTRCoreAVMediaCtrlPlay, eBTRCoreAVMediaFlowOut, NULL, "Device1", enBTRCoreSuccess);

    enBTRCoreRet result = BTRCore_MediaControl(&hBTRCore, 0, enBTDevAudioSink, enBTRCoreMediaCtrlPlay, NULL);

    // Verify that it returns success
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}


void test_BTRCore_MediaControl_PauseCommand(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice ;
    const char* deviceAddress = "/path/to/device1";
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    knownDevice.bDeviceConnected = TRUE, 
    knownDevice.pcDeviceName[0] = "Device1";
   

   
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    BTRCore_AVMedia_MediaControl_ExpectAndReturn(NULL, deviceAddress, enBTRCoreAVMediaCtrlPause, eBTRCoreAVMediaFlowUnknown, NULL, "Device1", enBTRCoreSuccess);
 
    enBTRCoreRet ret = BTRCore_MediaControl(&hBTRCore, 0, enBTDevUnknown, enBTRCoreMediaCtrlPause, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}



void test_BTRCore_MediaControl_StopCommand(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice ;
    const char* deviceAddress = "/path/to/device1";
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    knownDevice.bDeviceConnected = TRUE, 
    knownDevice.pcDeviceName[0] = "Device1";
   
   // btrCore_GetDeviceInfoKnown_ExpectAndReturn(&hBTRCore, 0, enBTDevUnknown, NULL, NULL, NULL, NULL, NULL, NULL, NULL, enBTRCoreSuccess);
     BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
     BTRCore_AVMedia_MediaControl_ExpectAndReturn(NULL, deviceAddress, enBTRCoreAVMediaCtrlStop, eBTRCoreAVMediaFlowUnknown, NULL, "Device1", enBTRCoreSuccess);
    //BTRCore_AVMedia_SendMediaControl_ExpectAndReturn(NULL, eBTRCoreAVMediaCtrlStop, eBTRCoreAVMediaFlowUnknown, NULL, enBTRCoreSuccess);

    enBTRCoreRet ret = BTRCore_MediaControl(&hBTRCore, 0, enBTDevUnknown, enBTRCoreMediaCtrlStop, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_MediaControl_UnknownCommand(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice ;
    const char* deviceAddress = "/path/to/device1";
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    knownDevice.bDeviceConnected = TRUE, 
    knownDevice.pcDeviceName[0] = "Device1";

   // btrCore_GetDeviceInfoKnown_ExpectAndReturn(&hBTRCore, 0, enBTDevUnknown, NULL, NULL, NULL, NULL, NULL, NULL, NULL, enBTRCoreSuccess);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    enBTRCoreRet ret = BTRCore_MediaControl(&hBTRCore, 0, enBTDevUnknown, enBTRCoreMediaCtrlUnknown, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_GetMediaTrackInfo_hBTRCore_NULL(void) {
    enBTRCoreRet ret;
    ret = BTRCore_GetMediaTrackInfo(NULL, 0, 0, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_GetMediaTrackInfo_GetDeviceInfoKnown_Fails(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret;

   
    stBTRCoreMediaTrackInfo trackInfo;
    stBTRCoreBTDevice knownDevice;
    const char* deviceAddress = "/path/to/device1";
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

     knownDevice.bDeviceConnected = FALSE;

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
   // btrCore_GetDeviceInfoKnown_ExpectAndReturn(&hBTRCore, devId, devType, NULL, enBTRCoreFailure);
    BTRCore_AVMedia_GetTrackInfo_ExpectAndReturn(hBTRCore.avMediaHdl, deviceAddress, (stBTRCoreAVMediaTrackInfo*)&trackInfo, enBTRCoreFailure);
    ret = BTRCore_GetMediaTrackInfo(&hBTRCore, 0, enBTDevAudioSink, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetMediaTrackInfo_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret;
  
 // stBTRCoreMediaTrackInfo trackInfo;
    stBTRCoreBTDevice knownDevice ;
    const char* deviceAddress = "/path/to/device1";
     //const char* deviceAddress = NULL;
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    knownDevice.bDeviceConnected = FALSE;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    BTRCore_AVMedia_GetTrackInfo_ExpectAndReturn(NULL, deviceAddress,NULL, enBTRCoreFailure);
    //knownDevice.bDeviceConnected = FALSE;
    ret = BTRCore_GetMediaTrackInfo(&hBTRCore, 0, enBTDevUnknown, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}




void test_BTRCore_GetMediaTrackInfo_AVMedia_GetTrackInfo_Fails(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret;
    tBTRCoreDevId aBTRCoreDevId;
    stBTRCoreMediaTrackInfo trackInfo;
    stBTRCoreBTDevice knownDevice ;

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&trackInfo, 0, sizeof(stBTRCoreMediaTrackInfo));
    memset(&aBTRCoreDevId,0,sizeof(tBTRCoreDevId));
    const char* deviceAddress = "/path/to/device1";
    knownDevice.bDeviceConnected = TRUE ;
    //knownDevice.pcDeviceName[0] = "Device1";
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    BTRCore_AVMedia_GetTrackInfo_ExpectAndReturn(hBTRCore.avMediaHdl,deviceAddress, (stBTRCoreAVMediaTrackInfo*)&trackInfo, enBTRCoreFailure);

    ret = BTRCore_GetMediaTrackInfo(&hBTRCore, aBTRCoreDevId,  enBTDevUnknown, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_GetMediaTrackInfo_Success(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret;

    stBTRCoreMediaTrackInfo trackInfo;
    stBTRCoreBTDevice knownDevice;
    tBTRCoreDevId aBTRCoreDevId;
    const char* deviceAddress = "/path/to/device1";

    // Initialize the structures to zero
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&trackInfo, 0, sizeof(stBTRCoreMediaTrackInfo));
    memset(&aBTRCoreDevId,0,sizeof(tBTRCoreDevId));

    // Simulate the device as connected
    knownDevice.bDeviceConnected = TRUE;

    // Mock the function btrCore_GetDeviceInfoKnown to return a connected device with a valid address
   // btrCore_GetDeviceInfoKnown_ExpectAndReturn(&hBTRCore, 0, enBTDevAudioSink, NULL, &knownDevice, NULL, &deviceAddress, enBTRCoreSuccess);

    // Mock the function BTRCore_AVMedia_GetTrackInfo to successfully return track information
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    BTRCore_AVMedia_GetTrackInfo_ExpectAndReturn(hBTRCore.avMediaHdl, deviceAddress, (stBTRCoreAVMediaTrackInfo*)&trackInfo, enBTRCoreSuccess);

    // Call the function under test
    ret = BTRCore_GetMediaTrackInfo(&hBTRCore, aBTRCoreDevId, enBTDevAudioSink, &trackInfo);

    // Assert that the function returns enBTRCoreSuccess
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_GetMediaElementTrackInfo_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_GetMediaElementTrackInfo(NULL, 0, enBTDevUnknown, NULL,NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized,ret);
}
void test_BTRCore_GetMediaElementTrackInfo_NullDeviceInfo(void) {
    stBTRCoreHdl hBTRCore;
  //  btrCore_GetDeviceInfoKnown_ExpectAnyArgsAndReturn(enBTRCoreFailure);
    BTRCore_AVMedia_GetElementTrackInfo_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_GetMediaElementTrackInfo(&hBTRCore, 0,0, enBTDevUnknown, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}
void test_BTRCore_GetMediaElementTrackInfo_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice device;
    device.bDeviceConnected = FALSE ;
  //  btrCore_GetDeviceInfoKnown_ExpectAnyArgsAndReturn(enBTRCoreSuccess);
   //  btrCore_GetDeviceInfoKnown_ReturnThruPtr_pstKnownDevice(&device);
    BTRCore_AVMedia_GetElementTrackInfo_IgnoreAndReturn(1);
    enBTRCoreRet ret = BTRCore_GetMediaElementTrackInfo(&hBTRCore, 0, enBTDevUnknown,NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


 

void test_BTRCore_GetMediaPositionInfo_hBTRCore_NULL(void) {
    stBTRCoreMediaPositionInfo mediaPositionInfo;
    enBTRCoreRet ret = BTRCore_GetMediaPositionInfo(NULL, 0, 0, &mediaPositionInfo);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_GetMediaPositionInfo_btrCore_GetDeviceInfoKnown_Fails(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreMediaPositionInfo mediaPositionInfo;
    stBTRCoreBTDevice knownDevice ;
 //   knownDevice.bDeviceConnected = FALSE ;
    const char* deviceAddress = "/path/to/device1";
    enBTRCoreRet ret = BTRCore_GetMediaPositionInfo(&hBTRCore, deviceAddress, 0, &mediaPositionInfo);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

void test_BTRCore_GetMediaPositionInfo_AVMedia_GetPositionInfo_Fails(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreMediaPositionInfo mediaPositionInfo;
    tBTRCoreDevId aBTRCoreDevId;
    stBTRCoreBTDevice device ;
    stBTRCoreBTDevice knownDevice;
    knownDevice.bDeviceConnected = TRUE ;

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    BTRCore_AVMedia_GetPositionInfo_IgnoreAndReturn(enBTRCoreFailure);

    enBTRCoreRet ret = BTRCore_GetMediaPositionInfo(&hBTRCore, 0, 0, &mediaPositionInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}



void test_BTRCore_GetMediaPositionInfo_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreMediaPositionInfo mediaPositionInfo;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    enBTDeviceType devType =  enBTRCoreSpeakers;

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&mediaPositionInfo, 0, sizeof(stBTRCoreMediaPositionInfo));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Simulate the behavior of btrCore_GetDeviceInfoKnown
    knownDevice.bDeviceConnected = FALSE;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    hBTRCore.stKnownDevicesArr[0] = knownDevice;
    hBTRCore.numOfPairedDevices = 1;

    // Mock the BTRCore_AVMedia_GetPositionInfo function
    BTRCore_AVMedia_GetPositionInfo_IgnoreAndReturn(enBTRCoreSuccess);

    // Mock the BtrCore_BTGetPairedDeviceInfo function
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);

    // Call the function under test
    enBTRCoreRet ret = BTRCore_GetMediaPositionInfo(&hBTRCore, 0, 0, &mediaPositionInfo);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetMediaElementTrackInfo_Success(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreMediaTrackInfo trackInfo;
    tBTRCoreDevId aBTRCoreDevId = 12345; // Initialize with a valid device ID
    tBTRCoreMediaElementId aBtrMediaElementId = 1; // Initialize with a valid media element ID
    stBTRCoreBTDevice device;
    const char* deviceAddress = "00:11:22:33:44:55";
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers; // Initialize with a valid device type

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&device, 0, sizeof(stBTRCoreBTDevice));
    memset(&trackInfo, 0, sizeof(stBTRCoreMediaTrackInfo));

    // Set the connection status of the known device
    device.bDeviceConnected = TRUE;
    device.tDeviceId = aBTRCoreDevId;
    strncpy(device.pcDevicePath, deviceAddress, sizeof(device.pcDevicePath) - 1);
    device.pcDevicePath[sizeof(device.pcDevicePath) - 1] = '\0';
    strncpy(device.pcDeviceName, "Test Device", sizeof(device.pcDeviceName) - 1);
    device.pcDeviceName[sizeof(device.pcDeviceName) - 1] = '\0';

    // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = device;
    hBTRCore.stKnownDevicesArr[0] = device;

    // Mock the LE handle
    hBTRCore.leHdl = (void*)0x123456;  // Mock the leHdl to simulate a valid LE handle

 
    BTRCore_AVMedia_GetElementTrackInfo_StubWithCallback(mock_BTRCore_AVMedia_GetElementTrackInfo);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);

    // Call the function under test
    enBTRCoreRet ret = BTRCore_GetMediaElementTrackInfo(&hBTRCore, aBTRCoreDevId, aenBTRCoreDevType, aBtrMediaElementId, &trackInfo);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(1, trackInfo.ui32TrackNumber);
    TEST_ASSERT_EQUAL_STRING("Test Track", trackInfo.pcTitle);
}

void test_BTRCore_GetMediaProperty_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_GetMediaProperty(NULL, 0, 0, "propertyKey", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_GetMediaProperty_Success(void) {
    stBTRCoreHdl btrCoreHandle;
    tBTRCoreDevId aBTRCoreDevId;
    enBTDeviceType lenBTDeviceType;
    stBTRCoreBTDevice knownDevice ;
  
    const char* mediaPropertyKey;
    void*   mediaPropertyValue;
    knownDevice.bDeviceConnected = TRUE;
    memset(&btrCoreHandle, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
 //   memset(&mediaPositionInfo, 0, sizeof(stBTRCoreMediaPositionInfo));
    memset(&aBTRCoreDevId,0,sizeof(tBTRCoreDevId));
   // btrCoreHandle.knownDevices[0] = knownDevice;

   // btrCore_GetDeviceInfoKnown_ExpectAndReturn(&btrCoreHandle, 0, 0, &knownDevice, NULL, NULL, enBTRCoreSuccess);
    BTRCore_AVMedia_GetMediaProperty_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
    enBTRCoreRet ret = BTRCore_GetMediaProperty(&btrCoreHandle, 0, 0, "propertyKey", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

int mock_BtrCore_BTGetPairedDeviceInfo(void* apstBtIfceHdl, const char* apcDevPath, stBTRCoreBTDevice* apstFoundDevice) {
    if (!apstBtIfceHdl || !apcDevPath || !apstFoundDevice) {
        return -1;
    }
    if (strstr(apcDevPath, "valid") != NULL) {
        apstFoundDevice->enDeviceType= enBTRCoreUnknown;
        return 0;
    }
    return -1;
}
// Mock functions
int mock_BTRCore_AVMedia_IsMediaElementNotPlayable(
    tBTRCoreAVMediaHdl avMediaHdl,
    const char* pDeviceAddress,
    tBTRCoreMediaElementId aBtrMediaElementId,
    char* isPlayable
) {
    *isPlayable = 0; // Simulate that the media element is not playable
    return enBTRCoreFailure;
}
int  mock_BTRCore_AVMedia_IsMediaElementPlayable(
    tBTRCoreAVMediaHdl avMediaHdl,
    const char* pDeviceAddress,
    tBTRCoreMediaElementId aBtrMediaElementId,
    char* isPlayable
) {
    *isPlayable = 1; // Simulate that the media element is playable
    return enBTRCoreSuccess;
}

int mock_BTRCore_AVMedia_SelectTrack(
    tBTRCoreAVMediaHdl avMediaHdl,
    const char* pDeviceAddress,
    tBTRCoreMediaElementId aBtrMediaElementId
) {
    return enBTRCoreSuccess; // Simulate successful track selection
}

void test_BTRCore_GetMediaProperty_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    tBTRCoreDevId aBTRCoreDevId;
    stBTRCoreBTDevice device;
    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&aBTRCoreDevId, 0, sizeof( tBTRCoreDevId));
    memset(&device, 0, sizeof(stBTRCoreBTDevice));

    devStateInfo.eDeviceCurrState = enBTRCoreDevStDisconnected;
    device.bDeviceConnected = FALSE ;
    // Set the device state as disconnected
    //devStateInfo.eDeviceCurrState = enBTRCoreDevStDisconnected;

    // Assuming stScannedDevStInfoArr is an array, set the first element to the device state info
    hBTRCore.stScannedDevStInfoArr[0] = devStateInfo;
    hBTRCore.avMediaHdl = (void*)0x80; 

    // Define the media property key and value
    const char* mediaPropertyKey = "someMediaPropertyKey";
    void* mediaPropertyValue = NULL; // Assuming you want to retrieve the value
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback( mock_BtrCore_BTGetPairedDeviceInfo);
    enBTRCoreRet ret = BTRCore_GetMediaProperty(&hBTRCore, aBTRCoreDevId, enBTRCoreUnknown, mediaPropertyKey, mediaPropertyValue);
    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SelectMediaElement_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_SelectMediaElement(NULL, 1, 1, enBTDevAudioSink, enBTRCoreMedETypeTrack);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}


void test_BTRCore_SelectMediaElement_InvalidMediaElementId(void) {
    stBTRCoreHdl hBTRCore;
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
     
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback( mock_BtrCore_BTGetPairedDeviceInfo); 
    enBTRCoreRet ret = BTRCore_SelectMediaElement(&hBTRCore, 1, -1, enBTDevAudioSink, enBTRCoreMedETypeTrack);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SelectMediaElement_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreMediaElementInfoList mediaElementListInfo;
    stBTRCoreBTDevice knownDevice;
    tBTRCoreDevId aBTRCoreDevId = 12345;
    tBTRCoreMediaElementId aBtrMediaElementId = 1;
    const char* deviceAddress = "00:11:22:33:44:55";
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers;
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack;

    // Initialize structures
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&mediaElementListInfo, 0, sizeof(stBTRCoreMediaElementInfoList));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set the device as not connected
    knownDevice.bDeviceConnected = FALSE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // knownDevice.tDeviceId = 1;
    strncpy(knownDevice.pcDevicePath,"Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // btrCore_GetDeviceInfoKnown_StubWithCallback(_mock_btrCore_GetDeviceInfoKnown);
   // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    // Call the function
    enBTRCoreRet result = BTRCore_SelectMediaElement(&hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aenBTRCoreDevType, aenBTRCoreMedElementType);

    // Verify the result is failure since the device is not connected
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SelectMediaElement_DeviceConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreMediaElementInfoList mediaElementListInfo;
    stBTRCoreBTDevice knownDevice;
    tBTRCoreDevId aBTRCoreDevId = 12345;
    tBTRCoreMediaElementId aBtrMediaElementId = 1;
    const char* deviceAddress = "00:11:22:33:44:55";
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers;
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack;

    // Initialize structures
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&mediaElementListInfo, 0, sizeof(stBTRCoreMediaElementInfoList));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set the device as not connected
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    // hBTRCore.stKnownDevicesArr[0] = knownDevice;

    // Mock btrCore_GetDeviceInfoKnown to return success and device info

    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // knownDevice.tDeviceId = 1;
    strncpy(knownDevice.pcDevicePath,"Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    BTRCore_AVMedia_IsMediaElementPlayable_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_AVMedia_ChangeBrowserLocation_IgnoreAndReturn(enBTRCoreSuccess);

    // Call the function
    enBTRCoreRet result = BTRCore_SelectMediaElement(&hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aenBTRCoreDevType, aenBTRCoreMedElementType);

    // Verify the result is failure since the device is not connected
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}

void test_BTRCore_SelectMediaElement_DifferentMediaElementTypes(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreMediaElementInfoList mediaElementListInfo;
    stBTRCoreBTDevice knownDevice;
    tBTRCoreDevId aBTRCoreDevId = 12345;
    tBTRCoreMediaElementId aBtrMediaElementId = 1;
    const char* deviceAddress = "00:11:22:33:44:55";
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers;
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack;
    enBTRCoreRet result;

    // Initialize structures
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&mediaElementListInfo, 0, sizeof(stBTRCoreMediaElementInfoList));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set the device as not connected
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    // Mock btrCore_GetDeviceInfoKnown to return success and device info

    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // knownDevice.tDeviceId = 1;
    strncpy(knownDevice.pcDevicePath,"Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // btrCore_GetDeviceInfoKnown_StubWithCallback(_mock_btrCore_GetDeviceInfoKnown);
   // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    BTRCore_AVMedia_IsMediaElementPlayable_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_AVMedia_ChangeBrowserLocation_IgnoreAndReturn(enBTRCoreSuccess);
    result = BTRCore_SelectMediaElement(&hBTRCore, 0,0, enBTDevAudioSink, enBTRCoreMedETypeAlbum);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    result = BTRCore_SelectMediaElement(&hBTRCore, 0, 0, enBTDevAudioSink, enBTRCoreMedETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    result = BTRCore_SelectMediaElement(&hBTRCore, 0, 0, enBTDevAudioSink, enBTRCoreMedETypeGenre);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
	
    result = BTRCore_SelectMediaElement(&hBTRCore, 0, 0, enBTDevAudioSink, enBTRCoreMedETypeCompilation);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    result = BTRCore_SelectMediaElement(&hBTRCore, 0, 0, enBTDevAudioSink, enBTRCoreMedETypePlayList);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    result = BTRCore_SelectMediaElement(&hBTRCore, 0, 0, enBTDevAudioSink, enBTRCoreMedETypeTrackList);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    result = BTRCore_SelectMediaElement(&hBTRCore, 0, 0, enBTDevAudioSink, enBTRCoreMedETypeTrack);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result); 
}
void test_BTRCore_GetMediaElementList_ValidInputs(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreMediaElementInfoList mediaElementListInfo;
    tBTRCoreDevId aBTRCoreDevId = 12345; // Initialize with a valid device ID
    const char* deviceAddress = "00:11:22:33:44:55";
    tBTRCoreMediaElementId aBtrMediaElementId = 1; // Initialize with a valid media element ID
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers; // Initialize with a valid device type
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack;  // Media element type (Track)
    unsigned short aui16BtrMedElementStartIdx = 0; // Start index
    unsigned short aui16BtrMedElementEndIdx = 10;  // End index
    

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&mediaElementListInfo, 0, sizeof(stBTRCoreMediaElementInfoList));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // knownDevice.tDeviceId = 1;
    strncpy(knownDevice.pcDevicePath,"Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

      // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    hBTRCore.leHdl = (void*)0x123456;  
    // Mock necessary functions
    BTRCore_AVMedia_SelectMediaBrowserElements_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_AVMedia_GetMediaElementList_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);

    // Call the function under test
    enBTRCoreRet result = BTRCore_GetMediaElementList(&hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aui16BtrMedElementStartIdx, aui16BtrMedElementEndIdx, aenBTRCoreDevType, aenBTRCoreMedElementType, &mediaElementListInfo);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}
void test_BTRCore_GetMediaElementList_InvalidHandle(void) {
    tBTRCoreHandle hBTRCore = NULL;
    tBTRCoreDevId aBTRCoreDevId = 12345;
    tBTRCoreMediaElementId aBtrMediaElementId = 1;
    unsigned short aui16BtrMedElementStartIdx = 0;
    unsigned short aui16BtrMedElementEndIdx = 10;
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreUnknown;
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack;
    stBTRCoreMediaElementInfoList apstMediaElementListInfo;

    enBTRCoreRet result = BTRCore_GetMediaElementList(hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aui16BtrMedElementStartIdx, aui16BtrMedElementEndIdx, aenBTRCoreDevType, aenBTRCoreMedElementType, &apstMediaElementListInfo);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, result);
}

void test_BTRCore_GetMediaElementList_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreMediaElementInfoList mediaElementListInfo;
    stBTRCoreDevStateInfo devStateInfo;
    tBTRCoreDevId aBTRCoreDevId = 12345;
    tBTRCoreMediaElementId aBtrMediaElementId = 1; // Initialize with a valid media element ID
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers; // Initialize with a valid device type
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack;  // Media element type (Track)  

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&mediaElementListInfo, 0, sizeof(stBTRCoreMediaElementInfoList));
     memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = FALSE; // Device is not connected
    knownDevice.tDeviceId = aBTRCoreDevId;
    const char* deviceAddress = "00:11:22:33:44:55";


    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // knownDevice.tDeviceId = 1;
    strncpy(knownDevice.pcDevicePath,"Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

    // Set the number of scanned devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;
	
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);

    // Call the function
    enBTRCoreRet result = BTRCore_GetMediaElementList(&hBTRCore, 12345, 1, 0, 10, enBTRCoreSpeakers, enBTRCoreMedETypeTrack, &mediaElementListInfo);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}



void test_BTRCore_SetMediaElementActive_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_SetMediaElementActive(NULL, 1, 1, enBTDevAudioSink, enBTRCoreMedETypeTrack);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_SetMediaElementActive_InvalidMediaElementId(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    tBTRCoreDevId aBTRCoreDevId = 12345;
    tBTRCoreMediaElementId aBtrMediaElementId = 99999; // Invalid media element ID
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers; // Valid device type

    // Initialize structures
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set device as connected and other necessary fields
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDevicePath, "Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;
    // Mock the necessary functions
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);
    BTRCore_AVMedia_IsMediaElementPlayable_IgnoreAndReturn(enBTRCoreFailure);

    // Call the function with an invalid media element ID
    enBTRCoreRet result = BTRCore_SetMediaElementActive(&hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aenBTRCoreDevType,aenBTRCoreDevType);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}
void test_BTRCore_SetMediaElementActive_MediaElementNotPlayable(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreBTDevice knownDevice;
    tBTRCoreDevId aBTRCoreDevId = 12345; // Initialize with a valid device ID
    const char* deviceAddress = "00:11:22:33:44:55";
    tBTRCoreMediaElementId aBtrMediaElementId = 1; // Initialize with a valid media element ID
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers; // Initialize with a valid device type
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack; // Media element type (Track)

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, "Test Device", sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';

    // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    hBTRCore.leHdl = (void*)0x123456; // Mock the leHdl to simulate a valid LE handle

    // Mock the necessary functions
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);
    BTRCore_AVMedia_IsMediaElementPlayable_StubWithCallback(mock_BTRCore_AVMedia_IsMediaElementNotPlayable);

    // Call the function under test
    enBTRCoreRet ret = BTRCore_SetMediaElementActive(&hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aenBTRCoreDevType, aenBTRCoreMedElementType);

    // Assert the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetMediaElementActive_Success(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreMediaElementInfoList mediaElementListInfo;
    tBTRCoreDevId aBTRCoreDevId = 12345; // Initialize with a valid device ID
    const char* deviceAddress = "00:11:22:33:44:55";
    tBTRCoreMediaElementId aBtrMediaElementId = 1; // Initialize with a valid media element ID
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers; // Initialize with a valid device type
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack;  // Media element type (Track)


    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&mediaElementListInfo, 0, sizeof(stBTRCoreMediaElementInfoList));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
   // knownDevice.tDeviceId = 1;
    strncpy(knownDevice.pcDevicePath,"Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

      // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

     hBTRCore.leHdl = (void*)0x123456;  

    // Mock the necessary functions
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);
    BTRCore_AVMedia_IsMediaElementPlayable_StubWithCallback(mock_BTRCore_AVMedia_IsMediaElementPlayable);
    BTRCore_AVMedia_SelectTrack_StubWithCallback(mock_BTRCore_AVMedia_SelectTrack);

    // Call the function under test
    enBTRCoreRet ret = BTRCore_SetMediaElementActive(&hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aenBTRCoreDevType, aenBTRCoreMedElementType);

    // Assert the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_SetMediaElementActive_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreMediaElementInfoList mediaElementListInfo;
    tBTRCoreDevId aBTRCoreDevId = 12345; // Valid device ID
    tBTRCoreMediaElementId aBtrMediaElementId = 1; // Valid media element ID
    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreSpeakers; // Valid device type
    eBTRCoreMedElementType aenBTRCoreMedElementType = enBTRCoreMedETypeTrack; // Media element type (Track)
     const char* deviceAddress = "00:11:22:33:44:55";

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&mediaElementListInfo, 0, sizeof(stBTRCoreMediaElementInfoList));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = FALSE; // Device is not connected
    knownDevice.tDeviceId = aBTRCoreDevId;

    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
 
    strncpy(knownDevice.pcDevicePath,"Test Device", sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

    
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
  
    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;
    // Mock necessary functions (no need to mock them for this case)
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo1);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_SetMediaElementActive(&hBTRCore, aBTRCoreDevId, aBtrMediaElementId, aenBTRCoreDevType, aenBTRCoreMedElementType);

}

void test_BTRCore_GetLEProperty_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_GetLEProperty(NULL, 0, "UUID", enBTRCoreLePropGUUID, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_GetLEProperty_NullUUID(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret = BTRCore_GetLEProperty(&hBTRCore, 0, NULL, enBTRCoreLePropGUUID, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_GetLEProperty_InvalidDeviceId(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret = BTRCore_GetLEProperty(&hBTRCore, BTRCORE_MAX_NUM_BT_DEVICES + 1, "UUID", enBTRCoreLePropGUUID, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

int mock_BTRCore_LE_GetGattProperty(
    tBTRCoreLeHdl leHdl,
    tBTRCoreDevId tDeviceId,
    const char* pcBTRCoreLEUuid,
    enBTRCoreLEGattProp enBTRCoreLEGattProp,
    void* apvBTRCorePropValue
) {
    // Simulate successful retrieval of GATT property
    if (enBTRCoreLEGattProp == enBTRCoreLEGPropUUID) {
        strncpy((char*)apvBTRCorePropValue, "TestUUID", 8);
    }
    return enBTRCoreSuccess;
}

int mock_BtrCore_LE_PerformGattOp(
    tBTRCoreLeHdl leHdl,
    tBTRCoreDevId tDeviceId,
    const char* pcBTRCoreLEUuid,
    enBTRCoreLEGattOp enBTRCoreLEGattOp,
    char* apLeOpArg,
    char* rpLeOpRes
) {
    // Simulate successful GATT operation
    if (enBTRCoreLEGattOp == enBTRCoreLEGOpReadValue) {
        strncpy(rpLeOpRes, "ReadValue", 9);
    }
    return enBTRCoreSuccess;
}


void test_BTRCore_GetLEProperty_Success(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreBTDevice knownDevice;
    tBTRCoreDevId aBTRCoreDevId = 12345; // Initialize with a valid device ID
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* apcBTRCoreLEUuid = "TestUUID";
    enBTRCoreLeProp aenBTRCoreLeProp = enBTRCoreLePropGUUID;
    char apvBTRCorePropValue[256]; // Buffer to store the property value

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(apvBTRCorePropValue, 0, sizeof(apvBTRCorePropValue));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, "Test Device", sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';

    // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    hBTRCore.leHdl = (void*)0x123456; // Mock the leHdl to simulate a valid LE handle

    // Mock the necessary functions
    BTRCore_LE_GetGattProperty_StubWithCallback(mock_BTRCore_LE_GetGattProperty);

    // Call the function under test
    enBTRCoreRet result = BTRCore_GetLEProperty(&hBTRCore, aBTRCoreDevId, apcBTRCoreLEUuid, aenBTRCoreLeProp, apvBTRCorePropValue);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    TEST_ASSERT_EQUAL_STRING("TestUUID", apvBTRCorePropValue);
}


void test_BTRCore_GetLEProperty_FailedPropertyRetrieval(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* apcBTRCoreLEUuid = "TestUUID";
    enBTRCoreLeProp aenBTRCoreLeProp = enBTRCoreLePropUnknown; // Unknown property type
    void* apvBTRCorePropValue = NULL; // No buffer for property value

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.bFound = TRUE;
    knownDevice.tDeviceId = 12345;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, "Test Device", sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';

    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.stScannedDevicesArr[0] = knownDevice;

    hBTRCore.leHdl = (void*)0x123456; // Mock the leHdl to simulate a valid LE handle

    // Mock the necessary functions to simulate failure
    BTRCore_LE_GetGattProperty_StubWithCallback(mock_BTRCore_LE_GetGattProperty);

    // Call the function
    enBTRCoreRet result = BTRCore_GetLEProperty(&hBTRCore, knownDevice.tDeviceId, apcBTRCoreLEUuid, aenBTRCoreLeProp, apvBTRCorePropValue);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}


void test_BTRCore_GetLEProperty_DeviceNotFound(void) {
    stBTRCoreHdl hBTRCore;
    const char* apcBTRCoreLEUuid = "TestUUID";
    enBTRCoreLeProp aenBTRCoreLeProp = enBTRCoreLePropGUUID;
    void* apvBTRCorePropValue = NULL;

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    hBTRCore.numOfScannedDevices = 0; // No devices scanned

    // Call the function
    enBTRCoreRet result = BTRCore_GetLEProperty(&hBTRCore, 12345, apcBTRCoreLEUuid, aenBTRCoreLeProp, apvBTRCorePropValue);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, result);
}


void test_BTRCore_BatteryWriteOTAControl_NullArguments(void) {
    enBTRCoreRet ret;

    ret = BTRCore_BatteryWriteOTAControl(NULL, 0, NULL, 0);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_BatteryWriteOTAControl(NULL, 0, "some_uuid", 0);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_BatteryWriteOTAControl((tBTRCoreHandle)1, 0, NULL, 0);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_BatteryWriteOTAControl_ValidArguments(void) {
    stBTRCoreHdl coreHandle;
    stBTRCoreBTDevice knownDevice;
    coreHandle.numOfPairedDevices = 1;
    knownDevice.bDeviceConnected = TRUE;

   // mock_btrCore_bt_ifce_ExpectAndReturn(&coreHandle, 0, "some_uuid", 0, enBTRCoreSuccess);
    BtrCore_LE_BatteryWriteOTAControl_IgnoreAndReturn(enBTRCoreSuccess);
    enBTRCoreRet ret = BTRCore_BatteryWriteOTAControl(&coreHandle, 0, "some_uuid", 0);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_BTRCore_BatteryWriteOTAControl_NoPairedDevices(void) {
    stBTRCoreHdl coreHandle;
    coreHandle.numOfPairedDevices = 0;
    BtrCore_LE_BatteryWriteOTAControl_IgnoreAndReturn(enBTRCoreSuccess);
    
    enBTRCoreRet ret = BTRCore_BatteryWriteOTAControl(&coreHandle, 0, "some_uuid", 0);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_BatteryWriteOTAControl_DeviceNotConnected(void) {
    stBTRCoreHdl coreHandle;
    stBTRCoreBTDevice knownDevice;

    // Initialize the structures to avoid undefined behavior
    memset(&coreHandle, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    // Set the number of paired devices and the connection status of the known device
    coreHandle.numOfPairedDevices = 1;
    knownDevice.bDeviceConnected = FALSE;

    // Assign the known device to the core handle's known devices array
    coreHandle.stKnownDevicesArr[0] = knownDevice;

    // Mock the dependency
    BtrCore_LE_BatteryWriteOTAControl_IgnoreAndReturn(enBTRCoreFailure);

    // Call the function under test
    enBTRCoreRet ret = BTRCore_BatteryWriteOTAControl(&coreHandle, 0, "some_uuid", 0);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_BatterySetLED_NullHandle(void) {
    enBTRCoreRet result = BTRCore_BatterySetLED(NULL, 0, "valid_uuid");
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, result);
}

void test_BTRCore_BatterySetLED_NullUUID(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet result = BTRCore_BatterySetLED(&hBTRCore, 0, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, result);
}

void test_BTRCore_BatterySetLED_NoPairedDevices(void) {
    stBTRCoreHdl hBTRCore = { .numOfPairedDevices = 0 };
    //BtrCore_LE_BatterySetLED_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_LE_BatterySetLED_ExpectAndReturn(hBTRCore.leHdl, 0, "valid_uuid", enBTRCoreSuccess);
    enBTRCoreRet result = BTRCore_BatterySetLED(&hBTRCore, 0, "valid_uuid");
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_BatterySetLED_ValidInputs(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    const char* apBtUuid = "UUID1234";

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    // Set the number of paired devices and the connection status of the known device
    hBTRCore.numOfPairedDevices = 1;
    knownDevice.bDeviceConnected = TRUE;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    // Initialize leHdl, which is likely expected by BtrCore_LE_BatterySetLED
    hBTRCore.leHdl = (void*)0x123456;  // Mock the leHdl to simulate a valid LE handle

    // Mock the BtrCore_LE_BatterySetLED function with the correct handle and UUID
    BtrCore_LE_BatterySetLED_ExpectAndReturn(hBTRCore.leHdl, 0, "valid_uuid", enBTRCoreSuccess);

    // Call the function under test
    enBTRCoreRet result = BTRCore_BatterySetLED(&hBTRCore, 0, "valid_uuid");

    // Verify the result
    TEST_ASSERT_EQUAL(0, result);
}

void test_BTRCore_BatteryOTADataTransfer_Null_hBTRCore(void) {
    enBTRCoreRet ret = BTRCore_BatteryOTADataTransfer(NULL, 0, "someUuid", "someFile");
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_BatteryOTADataTransfer_Null_apBtUuid(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1; // Mock handle
    enBTRCoreRet ret = BTRCore_BatteryOTADataTransfer(hBTRCore, 0, NULL, "someFile");
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}
void test_BTRCore_BatteryOTADataTransfer_ValidInputs(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1; // Mocked handle
    tBTRCoreDevId aBTRCoreDevId = 0; // Example device ID
    const char* apBtUuid = "123e4567-e89b-12d3-a456-426614174000"; // Example UUID
    char* fileName = "example_file.bin"; // Example file
    stBTRCoreHdl mockedCoreHdl;
    memset(&mockedCoreHdl, 0, sizeof(stBTRCoreHdl)); 
    mockedCoreHdl.numOfPairedDevices = 1;
    mockedCoreHdl.stKnownDevicesArr[0].tDeviceId = 0;
    mockedCoreHdl.leHdl = (void*)0x1234; // Mocking the leHdl


    // Expect and mock the call to the BtrCore_LE_BatteryOTATransfer
    BtrCore_LE_BatteryOTATransfer_ExpectAndReturn( mockedCoreHdl.leHdl, aBTRCoreDevId, apBtUuid, fileName,enBTRCoreSuccess);

    // Call the function
    enBTRCoreRet result = BTRCore_BatteryOTADataTransfer(&mockedCoreHdl, aBTRCoreDevId, apBtUuid, fileName);

    // Check the result
    TEST_ASSERT_EQUAL(0, result);
}

void test_BTRCore_BatteryOTADataTransfer_DeviceNotFound(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    tBTRCoreDevId aBTRCoreDevId = 99; // Device ID not in list
    const char* apBtUuid = "123e4567-e89b-12d3-a456-426614174000";
    char* fileName = "example_file.bin";

    // Mock stBTRCoreHdl structure
    stBTRCoreHdl mockedCoreHdl;
    mockedCoreHdl.numOfPairedDevices = 1;
    mockedCoreHdl.stKnownDevicesArr[0].tDeviceId = 1;
    mockedCoreHdl.leHdl = (void*)0x1234;
    BtrCore_LE_BatteryOTATransfer_IgnoreAndReturn(enBTRCoreSuccess);
    // Call the function with a device ID not in the list
    enBTRCoreRet result = BTRCore_BatteryOTADataTransfer(&mockedCoreHdl, aBTRCoreDevId, apBtUuid, fileName);

    // Check that it returns the correct error
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}


void test_BTRCore_PerformLEOp_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_PerformLEOp(NULL, 0, "UUID", enBTRCoreLeOpGReadValue, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_PerformLEOp_NullUUID(void) {
    tBTRCoreHandle handle = (tBTRCoreHandle)1;
    enBTRCoreRet ret = BTRCore_PerformLEOp(handle, 0, NULL, enBTRCoreLeOpGReadValue, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_PerformLEOp_ValidHandleUUID_ReadValue(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreBTDevice knownDevice;
    tBTRCoreDevId aBTRCoreDevId = 12345; // Initialize with a valid device ID
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* apBtUuid = "TestUUID";
    enBTRCoreLeOp aenBTRCoreLeOp = enBTRCoreLeOpGReadValue;
    char apLeOpArg[256] = {0}; // Argument for the LE operation
    char rpLeOpRes[256] = {0}; // Buffer to store the operation result

    // Initialize the structures to avoid undefined behavior
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(apLeOpArg, 0, sizeof(apLeOpArg));
    memset(rpLeOpRes, 0, sizeof(rpLeOpRes));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = TRUE;
    knownDevice.bFound = TRUE;   
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, "Test Device", sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';

    // Set the number of scanned devices and paired devices
    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.numOfPairedDevices = 1;

    // Assign the known device to the core handle's known devices array
    hBTRCore.stScannedDevicesArr[0] = knownDevice;
    hBTRCore.stKnownDevicesArr[0] = knownDevice;

    hBTRCore.leHdl = (void*)0x123456; // Mock the leHdl to simulate a valid LE handle

    // Mock the necessary functions
    BtrCore_LE_PerformGattOp_StubWithCallback(mock_BtrCore_LE_PerformGattOp);

    // Call the function under test
    enBTRCoreRet result = BTRCore_PerformLEOp(&hBTRCore, aBTRCoreDevId, apBtUuid, aenBTRCoreLeOp, apLeOpArg, rpLeOpRes);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    TEST_ASSERT_EQUAL_STRING("ReadValue", rpLeOpRes);
}


void test_BTRCore_PerformLEOp_DeviceNotFound(void) {
    stBTRCoreHdl hBTRCore;
    const char* apBtUuid = "TestUUID";
    enBTRCoreLeOp aenBTRCoreLeOp = enBTRCoreLeOpGReadValue;
    char apLeOpArg[256] = {0};
    char rpLeOpRes[256] = {0};

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    hBTRCore.numOfScannedDevices = 0; // No devices scanned

    // Call the function
    enBTRCoreRet result = BTRCore_PerformLEOp(&hBTRCore, 12345, apBtUuid, aenBTRCoreLeOp, apLeOpArg, rpLeOpRes);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, result);
}


void test_BTRCore_PerformLEOp_Success_ReadValue(void) {
    // Initialize variables
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    tBTRCoreDevId aBTRCoreDevId = 12345;
    const char* apBtUuid = "UUID1234"; // Example UUID
    enBTRCoreLeOp aenBTRCoreLeOp = enBTRCoreLeOpGReadValue;
    char apLeOpArg[256] = {0};
    char rpLeOpRes[256] = {0};
    enBTRCoreRet ret;

    // Initialize scanned devices
    hBTRCore->numOfScannedDevices = 1;
    hBTRCore->stScannedDevicesArr[0].tDeviceId = aBTRCoreDevId;
    hBTRCore->stScannedDevicesArr[0].bFound = 1;
    hBTRCore->stScannedDevicesArr[0].bDeviceConnected = 1;
    strncpy(hBTRCore->stScannedDevicesArr[0].pcDeviceName, "Test Device", sizeof(hBTRCore->stScannedDevicesArr[0].pcDeviceName) - 1);
    hBTRCore->stScannedDevicesArr[0].pcDeviceName[sizeof(hBTRCore->stScannedDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Initialize paired devices
    hBTRCore->numOfPairedDevices = 1;
    hBTRCore->stKnownDevicesArr[0].tDeviceId = aBTRCoreDevId;
    hBTRCore->stKnownDevicesArr[0].bFound = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = 1;
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "Test Device", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName) - 1);
    hBTRCore->stKnownDevicesArr[0].pcDeviceName[sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Debug prints to verify setup
   
    BtrCore_LE_PerformGattOp_IgnoreAndReturn(enBTRCoreSuccess);

    // Call the function under test
    ret = BTRCore_PerformLEOp((tBTRCoreHandle)hBTRCore, aBTRCoreDevId, apBtUuid, aenBTRCoreLeOp, apLeOpArg, rpLeOpRes);

    // Check that the return value is enBTRCoreSuccess
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Clean up allocated memory
    free(hBTRCore);
}

void test_BTRCore_PerformLEOp_Failure_UnknownOperation(void) {
    // Allocate memory for the BTRCore handle and initialize it
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    tBTRCoreDevId aBTRCoreDevId = 12345;  // Device ID to test
    const char* apBtUuid = "UUID1234";
    enBTRCoreLeOp aenBTRCoreLeOp = enBTRCoreLeOpUnknown;  // Invalid operation
    char apLeOpArg[10] = {0};
    char rpLeOpRes[10] = {0};

    // Initialize device in scanned devices array
    hBTRCore->numOfScannedDevices = 1;
    hBTRCore->stScannedDevicesArr[0].tDeviceId = aBTRCoreDevId;
    hBTRCore->stScannedDevicesArr[0].bFound = 1;  // Ensure it's found
    hBTRCore->stScannedDevicesArr[0].bDeviceConnected = 1;  // Ensure it's connected

    // Copy the device name into the array
    strncpy(hBTRCore->stScannedDevicesArr[0].pcDeviceName, "Test Device", sizeof(hBTRCore->stScannedDevicesArr[0].pcDeviceName) - 1);
    hBTRCore->stScannedDevicesArr[0].pcDeviceName[sizeof(hBTRCore->stScannedDevicesArr[0].pcDeviceName) - 1] = '\0'; // Ensure null-termination

    // Since operation is unknown, it should return failure
    enBTRCoreRet result = BTRCore_PerformLEOp((tBTRCoreHandle)hBTRCore, aBTRCoreDevId, apBtUuid, aenBTRCoreLeOp, apLeOpArg, rpLeOpRes);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);

    // Clean up allocated memory
    free(hBTRCore);
}


void test_BTRCore_PerformLEOp_DeviceNotConnected(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* apBtUuid = "TestUUID";
    enBTRCoreLeOp aenBTRCoreLeOp = enBTRCoreLeOpGReadValue;
    char apLeOpArg[256] = {0};
    char rpLeOpRes[256] = {0};

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = FALSE; // Set not connected
    knownDevice.bFound = TRUE;             // Mark as found
    knownDevice.tDeviceId = 12345;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, "Test Device", sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';

    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.stScannedDevicesArr[0] = knownDevice;

    // Call the function
    enBTRCoreRet result = BTRCore_PerformLEOp(&hBTRCore, knownDevice.tDeviceId, apBtUuid, aenBTRCoreLeOp, apLeOpArg, rpLeOpRes);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, result);
}
void test_BTRCore_PerformLEOp_SuccessfulWriteValue(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* apBtUuid = "TestUUID";
    enBTRCoreLeOp aenBTRCoreLeOp = enBTRCoreLeOpGWriteValue;
    char apLeOpArg[256] = "DataToWrite"; // Argument for the write operation
    char rpLeOpRes[256] = {0};

    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    // Initialize knownDevice fields
    knownDevice.bDeviceConnected = TRUE;  // Set connected status
    knownDevice.bFound = TRUE;             // Mark as found
    knownDevice.tDeviceId = 12345;
    strncpy(knownDevice.pcDevicePath, deviceAddress, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, "Test Device", sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';

    hBTRCore.numOfScannedDevices = 1;
    hBTRCore.stScannedDevicesArr[0] = knownDevice;

    hBTRCore.leHdl = (void*)0x123456; // Mock the leHdl to simulate a valid LE handle

    // Mock the necessary functions
    BtrCore_LE_PerformGattOp_StubWithCallback(mock_BtrCore_LE_PerformGattOp);

    // Call the function
    enBTRCoreRet result = BTRCore_PerformLEOp(&hBTRCore, knownDevice.tDeviceId, apBtUuid, aenBTRCoreLeOp, apLeOpArg, rpLeOpRes);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}




void test_BTRCore_GetListOfScannedDevices_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreScannedDevicesCount listOfScannedDevices;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetListOfScannedDevices(hBTRCore, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}


void test_BTRCore_GetListOfScannedDevices_should_ReturnInvalidArg_when_listOfScannedDevices_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetListOfScannedDevices(hBTRCore, NULL);
    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}




void test_BTRCore_GetListOfScannedDevices_should_GetScannedDevicesSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    stBTRCoreScannedDevicesCount listOfScannedDevices;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;
    
    // Assuming these are your actual device parameters in your code 
    
    hBTRCore->stScannedDevicesArr[0].bFound = TRUE;
    hBTRCore->stScannedDevicesArr[0].tDeviceId = 123;
    strncpy(hBTRCore->stScannedDevicesArr[0].pcDeviceName, "device0", sizeof("device0"));
    strncpy(hBTRCore->stScannedDevicesArr[0].pcDeviceAddress, "address0", sizeof("address0"));
    hBTRCore->stScannedDevicesArr[0].i32RSSI = -50;

    hBTRCore->stScannedDevicesArr[1].bFound = TRUE;
    hBTRCore->stScannedDevicesArr[1].tDeviceId = 456;
    strncpy(hBTRCore->stScannedDevicesArr[1].pcDeviceName, "device1", sizeof("device1"));
    strncpy(hBTRCore->stScannedDevicesArr[1].pcDeviceAddress, "address1", sizeof("address1"));
    hBTRCore->stScannedDevicesArr[1].i32RSSI = -70;

    actual_result = BTRCore_GetListOfScannedDevices(hBTRCore, &listOfScannedDevices);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(2, listOfScannedDevices.numberOfDevices);

    // comparing values 
    
    TEST_ASSERT_EQUAL_MEMORY(&hBTRCore->stScannedDevicesArr[0], &listOfScannedDevices.devices[0], sizeof(stBTRCoreBTDevice));
    TEST_ASSERT_EQUAL_MEMORY(&hBTRCore->stScannedDevicesArr[1], &listOfScannedDevices.devices[1], sizeof(stBTRCoreBTDevice));
    
    free(hBTRCore);
}

void test_BTRCore_PairDevice_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    tBTRCoreDevId deviceID = 1;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_PairDevice(hBTRCore, deviceID);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_PairDevice_should_ReturnDeviceNotFound_when_DeviceIdNotFound(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    tBTRCoreDevId deviceID = 10;  // Assuming this ID doesn't exist 
    enBTRCoreRet expected_result = enBTRCoreDeviceNotFound;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_PairDevice(hBTRCore, deviceID);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}



void test_BTRCore_PairDevice_should_PairBluetoothCoreDeviceSuccessfully(void) {
    // Allocate and initialize the BTRCore handle
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    TEST_ASSERT_NOT_NULL(hBTRCore); // Ensure allocation was successful

    tBTRCoreDevId aBTRCoreDevId = 0;   // Assuming this is the device ID to pair
    enBTRCoreRet ret;

    // Initialize the scanned device details
    stBTRCoreBTDevice* scannedDevice = &(((stBTRCoreHdl*)hBTRCore)->stScannedDevicesArr[0]);
    scannedDevice->bFound = TRUE;
    scannedDevice->tDeviceId = aBTRCoreDevId;
    strcpy(scannedDevice->pcDeviceAddress, "address");  // Set the device address correctly

    // Mock the necessary function call with the correct number of arguments
    BtrCore_BTPerformAdapterOp_ExpectAndReturn(
        ((stBTRCoreHdl*)hBTRCore)->connHdl,           // connHdl
        ((stBTRCoreHdl*)hBTRCore)->curAdapterPath,    // adapterPath
        ((stBTRCoreHdl*)hBTRCore)->agentPath,         // agentPath
        "address",                                    // devAddress
        enBTAdpOpCreatePairedDev,                     // operation
        0                                             // return value (success)
    );

    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(enBTRCoreSuccess);
    

    // Call the function under test
    ret = BTRCore_PairDevice(hBTRCore, aBTRCoreDevId);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Clean up
    free(hBTRCore);
}

void test_BTRCore_UnPairDevice_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    tBTRCoreDevId deviceID = 1;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_UnPairDevice(hBTRCore, deviceID);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}


void test_BTRCore_UnPairDevice_DeviceNotFound(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    tBTRCoreDevId aBTRCoreDevId = 1;
    enBTRCoreRet ret;

    stBTRCoreHdl* pBTRCore = (stBTRCoreHdl*)hBTRCore;  // Cast the handle to the correct structure type

    pBTRCore->numOfPairedDevices = 1;
    pBTRCore->stScannedDevicesArr[0].tDeviceId = 2; // Use the correct member

    ret = BTRCore_UnPairDevice(hBTRCore, aBTRCoreDevId);

    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);

    free(pBTRCore);
}

void test_BTRCore_GetListOfPairedDevices_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCorePairedDevicesCount listOfDevices;
    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetListOfPairedDevices(hBTRCore, &listOfDevices);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}


void test_BTRCore_GetListOfPairedDevices_should_ReturnInvalidArg_when_listOfDevices_is_NULL(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    enBTRCoreRet expected_result = enBTRCoreInvalidArg;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_GetListOfPairedDevices(hBTRCore, NULL);

    TEST_ASSERT_EQUAL(expected_result, actual_result);

    free(hBTRCore);
}


int
_mock_BTGetPairedDeviceInfo3 (
    void*                   apBtConn,
    const char*             apBtAdapter,
    stBTPairedDeviceInfo*   pairedDevices
){
    //stBTPairedDeviceInfo pairedDevices = {0};

    memset(pairedDevices, 0, sizeof(stBTPairedDeviceInfo));
    pairedDevices->numberOfDevices = 2;   

    // Initialize first device
    strcpy(pairedDevices->devicePath[0], "/path/to/device1");
    pairedDevices->deviceInfo[0].bPaired = 1;
    pairedDevices->deviceInfo[0].bConnected = 1;
    pairedDevices->deviceInfo[0].bTrusted = 1;
    pairedDevices->deviceInfo[0].bBlocked = 0;
    pairedDevices->deviceInfo[0].bServicesResolved = 1;
    pairedDevices->deviceInfo[0].ui16Vendor = 1234;
    pairedDevices->deviceInfo[0].ui16VendorSource = 1;
    pairedDevices->deviceInfo[0].ui16Product = 5678;
    pairedDevices->deviceInfo[0].ui16Version = 1;
    pairedDevices->deviceInfo[0].ui32Class = 0x1F00;
    pairedDevices->deviceInfo[0].i32RSSI = -40;
    pairedDevices->deviceInfo[0].ui16Appearance = 0;
    pairedDevices->deviceInfo[0].i16TxPower = 0;
    pairedDevices->deviceInfo[0].bLegacyPairing = 0;
    strcpy(pairedDevices->deviceInfo[0].pcModalias, "modalias1");
    strcpy(pairedDevices->deviceInfo[0].pcAdapter, "adapter1");
    strcpy(pairedDevices->deviceInfo[0].pcName, "Device1");
    strcpy(pairedDevices->deviceInfo[0].pcAddress, "00:11:22:33:44:55");
    strcpy(pairedDevices->deviceInfo[0].pcAlias, "Alias1");
    strcpy(pairedDevices->deviceInfo[0].pcIcon, "icon1");
    strcpy(pairedDevices->deviceInfo[0].pcDevicePrevState, "prevState1");
    strcpy(pairedDevices->deviceInfo[0].pcDeviceCurrState, "currState1");
    strcpy(pairedDevices->deviceInfo[0].pcDevicePath, "/path/to/device1");
    strcpy(pairedDevices->deviceInfo[0].aUUIDs[0], "uuid1");
    strcpy(pairedDevices->deviceInfo[0].saServices[0].pcUUIDs, "serviceUUID1");
    pairedDevices->deviceInfo[0].saServices[0].len = 0;

    // Initialize second device
    strcpy(pairedDevices->devicePath[1], "/path/to/device2");
    pairedDevices->deviceInfo[1].bPaired = 1;
    pairedDevices->deviceInfo[1].bConnected = 0;
    pairedDevices->deviceInfo[1].bTrusted = 0;
    pairedDevices->deviceInfo[1].bBlocked = 0;
    pairedDevices->deviceInfo[1].bServicesResolved = 0;
    pairedDevices->deviceInfo[1].ui16Vendor = 4321;
    pairedDevices->deviceInfo[1].ui16VendorSource = 2;
    pairedDevices->deviceInfo[1].ui16Product = 8765;
    pairedDevices->deviceInfo[1].ui16Version = 2;
    pairedDevices->deviceInfo[1].ui32Class = 0x2F00;
    pairedDevices->deviceInfo[1].i32RSSI = -50;
    pairedDevices->deviceInfo[1].ui16Appearance = 1;
    pairedDevices->deviceInfo[1].i16TxPower = 1;
    pairedDevices->deviceInfo[1].bLegacyPairing = 1;
    strcpy(pairedDevices->deviceInfo[1].pcModalias, "modalias2");
    strcpy(pairedDevices->deviceInfo[1].pcAdapter, "adapter2");
    strcpy(pairedDevices->deviceInfo[1].pcName, "Device2");
    strcpy(pairedDevices->deviceInfo[1].pcAddress, "66:77:88:99:AA:BB");
    strcpy(pairedDevices->deviceInfo[1].pcAlias, "Alias2");
    strcpy(pairedDevices->deviceInfo[1].pcIcon, "icon2");
    strcpy(pairedDevices->deviceInfo[1].pcDevicePrevState, "prevState2");
    strcpy(pairedDevices->deviceInfo[1].pcDeviceCurrState, "currState2");
    strcpy(pairedDevices->deviceInfo[1].pcDevicePath, "/path/to/device2");
    strcpy(pairedDevices->deviceInfo[1].aUUIDs[0], "uuid2");
    strcpy(pairedDevices->deviceInfo[1].saServices[0].pcUUIDs, "serviceUUID2");
    pairedDevices->deviceInfo[1].saServices[0].len = 0;

    
    return 0;

}
 
void test_BTRCore_GetListOfPairedDevices_should_GetPairedDevicesSuccessfully(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    stBTRCorePairedDevicesCount listOfDevices;
    enBTRCoreRet expected_result = enBTRCoreSuccess;
    enBTRCoreRet actual_result;

    // Initialize hBTRCore to zero
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));
    hBTRCore->connHdl = 1; // Example value, adjust as needed
    hBTRCore->curAdapterPath = "/test/path";

    // Sample details for 1st Known Device 
    hBTRCore->stKnownDevicesArr[0].tDeviceId = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = TRUE;  // Assuming this means paired
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "device0", sizeof("device0"));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress, "address0", sizeof("address0"));
    
    hBTRCore->numOfPairedDevices = 0;

    // Sample details for 2nd Known Device 
    hBTRCore->stKnownDevicesArr[1].tDeviceId = 2;
    hBTRCore->stKnownDevicesArr[1].bDeviceConnected = TRUE;  // Assuming this means paired
    strncpy(hBTRCore->stKnownDevicesArr[1].pcDeviceName, "device1", sizeof("device1"));
    strncpy(hBTRCore->stKnownDevicesArr[1].pcDeviceAddress, "address1", sizeof("address1"));
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo3);
    actual_result = BTRCore_GetListOfPairedDevices(hBTRCore, &listOfDevices);   
    // Print debugging information
    printf("actual_result: %d expected_result: %d\n", actual_result, expected_result);
    printf("numberOfDevices: %d\n", listOfDevices.numberOfDevices);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(2, listOfDevices.numberOfDevices);

    // Comparing values
    TEST_ASSERT_EQUAL_MEMORY(&hBTRCore->stKnownDevicesArr[0], &listOfDevices.devices[0], sizeof(stBTRCoreBTDevice));
    TEST_ASSERT_EQUAL_MEMORY(&hBTRCore->stKnownDevicesArr[1], &listOfDevices.devices[1], sizeof(stBTRCoreBTDevice));

    free(hBTRCore);
}

void test_BTRCore_GetListOfPairedDevices_should_ReturnFailure_when_PopulateListOfPairedDevicesFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl)); 
    stBTRCorePairedDevicesCount listOfDevices;
    enBTRCoreRet expected_result = enBTRCoreFailure;
    enBTRCoreRet actual_result;

    // Initialize the hBTRCore structure
    hBTRCore->numOfPairedDevices = 0;
    
    // Set valid or expected test values
    hBTRCore->connHdl = 1; // Example value, adjust as needed
    hBTRCore->curAdapterPath = "/test/path"; // Example value, adjust as needed

    // Initialize listOfDevices
    memset(&listOfDevices, 0, sizeof(listOfDevices));

    // Mock the expected function call to simulate failure
    BtrCore_BTGetPairedDeviceInfo_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, &listOfDevices, -1); // Assuming -1 indicates failure

    actual_result = BTRCore_GetListOfPairedDevices(hBTRCore, &listOfDevices);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
    TEST_ASSERT_EQUAL(0, listOfDevices.numberOfDevices);

    free(hBTRCore);
}
void test_BTRCore_FindDevice_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreDevId deviceId = 1;
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, BTRCore_FindDevice(NULL, deviceId));
}

void test_BTRCore_FindDevice_should_ReturnFailure_when_BtrCore_BTPerformAdapterOpFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    tBTRCoreDevId deviceId = 1;  // Assuming this device exists

    // Initialize some scanned device for the tests
    strncpy(hBTRCore->stScannedDevicesArr[deviceId].pcDeviceAddress, "00:11:22:33:FF:EE", sizeof(hBTRCore->stScannedDevicesArr[deviceId].pcDeviceAddress) - 1);
    hBTRCore->stScannedDevicesArr[deviceId].pcDeviceAddress[sizeof(hBTRCore->stScannedDevicesArr[deviceId].pcDeviceAddress) - 1] = '\0'; // Ensure null-termination

    // Expect BtrCore_BTPerformAdapterOp to be called with these parameters
    // and make it return failure (-1)
    BtrCore_BTPerformAdapterOp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, hBTRCore->agentPath, "00:11:22:33:FF:EE", enBTAdpOpFindPairedDev, -1);
    
    // Check that BTRCore_FindDevice actually returns failure
    TEST_ASSERT_EQUAL(enBTRCoreFailure, BTRCore_FindDevice(hBTRCore, deviceId));

    free(hBTRCore);
}

void test_BTRCore_FindDevice_should_ReturnSuccess_when_BtrCore_BTPerformAdapterOpSucceeds(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    tBTRCoreDevId deviceId = 1;  // Assuming this device exists

    // Initialize some scanned device for the tests
    // Ensure pcDeviceAddress is large enough for the address string and null terminator
    strcpy(hBTRCore->stScannedDevicesArr[deviceId].pcDeviceAddress, "00:11:22:33:FF:EE");

    // Expect BtrCore_BTPerformAdapterOp to be called with these parameters
    // and make it return success (0)
    BtrCore_BTPerformAdapterOp_ExpectAndReturn(hBTRCore->connHdl, hBTRCore->curAdapterPath, hBTRCore->agentPath, "00:11:22:33:FF:EE", enBTAdpOpFindPairedDev, 0);

    // Check that BTRCore_FindDevice actually returns success
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, BTRCore_FindDevice(hBTRCore, deviceId));

    free(hBTRCore);
}

void test_BTRCore_FindService_should_ReturnNotInitialized_when_HandleIsNULL(void)
{
    tBTRCoreHandle hBTRCore = NULL;
    tBTRCoreDevId deviceId = 1;
    const char* UUID = "1234";
    char XMLdata[100];
    int found;

    enBTRCoreRet expected_result = enBTRCoreNotInitialized;
    enBTRCoreRet actual_result;

    actual_result = BTRCore_FindService(hBTRCore, deviceId, UUID, XMLdata, &found);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}


void test_BTRCore_FindService_should_ReturnFailure_when_BtrCore_BTFindServiceSupportedFails(void)
{
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    tBTRCoreDevId deviceId = 1;  // Assuming this device exists 
    const char* UUID = "1234";
    char XMLdata[100];
    int found;

   // btrCore_GetDeviceInfoKnown_ExpectAndReturn(hBTRCore, deviceId, enBTRCoreUnknown, NULL, NULL, NULL, "device-addr", enBTRCoreSuccess);
    //BtrCore_BTFindServiceSupported_ExpectAndReturn(NULL, "device-addr", UUID, XMLdata, -1);  // Assuming BtrCore_BTFindServiceSupported fails

    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(enBTRCoreSuccess);

    TEST_ASSERT_EQUAL(enBTRCoreFailure, BTRCore_FindService(hBTRCore, deviceId, UUID, XMLdata, &found));

    free(hBTRCore);
}

int _mock_BTFindServiceSupported
 (
    void*           apstBtIfceHdl,
    const char*     apcDevPath,
    const char*     apcSearchString,
    char*           apcDataString
) {
    // Assume the service is found
    return 1;
}

void test__BTRCore_FindServiceshould_ReturnSuccess_when_ServiceExists(void)
{
    // Initialize 
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    tBTRCoreDevId deviceId = 1;  // Assuming this device exists 
    const char* UUID = "1234";
    char XMLdata[100];
    int found;
    hBTRCore->connHdl = 1; // Example value, adjust as needed
    hBTRCore->curAdapterPath = "/test/path";

    // Set up the expected return values for the function calls inside BTRCore_FindService 
    hBTRCore->numOfPairedDevices = 0;
    // For the btrCore_GetDeviceInfoKnown call 
    enBTDeviceType devType = enBTDevUnknown;
    stBTRCoreBTDevice deviceInfo = {0};
    stBTRCoreDevStateInfo devStInfo = {0};
    const char* deviceAddress = "00:11:22:33:44:55";  // replace with actual device address you expect 
    //BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(0);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo3);
    BtrCore_BTFindServiceSupported_StubWithCallback(_mock_BTFindServiceSupported);
    // Call the function under test and verify the results 
    enBTRCoreRet actual_result = BTRCore_FindService(hBTRCore, deviceId, UUID, XMLdata, &found);

    printf("actual_result: %d\n", actual_result);
    printf("found: %d\n", found);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, actual_result);
    TEST_ASSERT_EQUAL(1, found);  // found should be set to 1 as service is found 

    free(hBTRCore);
}


//need to add vismal's code 2 here






//From this Tharun 2


void test_BTRCore_GetDeviceConnected_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_GetDeviceConnected(NULL, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

int
_mock_BTGetPairedDeviceInfo4 (
    void*                   apBtConn,
    const char*             apBtAdapter,
    stBTPairedDeviceInfo*   pairedDevices
){
    //stBTPairedDeviceInfo pairedDevices = {0};

    memset(pairedDevices, 0, sizeof(stBTPairedDeviceInfo));
    pairedDevices->numberOfDevices = 1;
    // Initialize first device
    strcpy(pairedDevices->devicePath[0], "/path/to/device1");
    pairedDevices->deviceInfo[0].bPaired = 1;
    pairedDevices->deviceInfo[0].bConnected = 1;
    pairedDevices->deviceInfo[0].bTrusted = 1;
    pairedDevices->deviceInfo[0].bBlocked = 0;
    pairedDevices->deviceInfo[0].bServicesResolved = 1;
    pairedDevices->deviceInfo[0].ui16Vendor = 1234;
    pairedDevices->deviceInfo[0].ui16VendorSource = 1;
    pairedDevices->deviceInfo[0].ui16Product = 5678;
    pairedDevices->deviceInfo[0].ui16Version = 1;
    pairedDevices->deviceInfo[0].ui32Class = 0x1F00;
    pairedDevices->deviceInfo[0].i32RSSI = -40;
    pairedDevices->deviceInfo[0].ui16Appearance = 0;
    pairedDevices->deviceInfo[0].i16TxPower = 0;
    pairedDevices->deviceInfo[0].bLegacyPairing = 0;
    strcpy(pairedDevices->deviceInfo[0].pcModalias, "modalias1");
    strcpy(pairedDevices->deviceInfo[0].pcAdapter, "adapter1");
    strcpy(pairedDevices->deviceInfo[0].pcName, "Device1");
    strcpy(pairedDevices->deviceInfo[0].pcAddress, "00:11:22:33:44:55");
    strcpy(pairedDevices->deviceInfo[0].pcAlias, "Alias1");
    strcpy(pairedDevices->deviceInfo[0].pcIcon, "icon1");
    strcpy(pairedDevices->deviceInfo[0].pcDevicePrevState, "prevState1");
    strcpy(pairedDevices->deviceInfo[0].pcDeviceCurrState, "currState1");
    strcpy(pairedDevices->deviceInfo[0].pcDevicePath, "/path/to/device1");
    strcpy(pairedDevices->deviceInfo[0].aUUIDs[0], "uuid1");
    strcpy(pairedDevices->deviceInfo[0].saServices[0].pcUUIDs, "serviceUUID1");
    pairedDevices->deviceInfo[0].saServices[0].len = 0;

    //hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStConnected;
}
enBTRCoreRet mock_BTRCore_StatusCb (stBTRCoreDevStatusCBInfo* apstDevStatusCbInfo, void* apvUserData)
{
    return enBTRCoreSuccess;
}
enBTRCoreRet mock_BTRCore_MediaStatusCb (stBTRCoreMediaStatusCBInfo* apstMediaStatusCbInfo, void* apvUserData)
{
    return enBTRCoreSuccess;
}

void test_BTRCore_GetDeviceConnected_DeviceConnected(void) {
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    //stBTRCoreDevStateInfo devStateInfo = { .eDeviceCurrState = enBTRCoreDevStConnected };
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));
    
    // Initialize the structure members
    hBTRCore->connHdl = (void*)1; 
    hBTRCore->curAdapterPath = "/test/path";

    // Sample details for 1st Known Device 
    hBTRCore->stKnownDevicesArr[0].tDeviceId = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = TRUE;  // Assuming this means connected
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "device0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress, "address0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress));
    
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(hBTRCore->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
    hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStConnected;
    hBTRCore->numOfPairedDevices = 1; 
    // Mocking and stubbing
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo4);
    enBTRCoreRet ret = BTRCore_GetDeviceConnected(hBTRCore, 0, enBTRCore_DC_Unknown);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_GetDeviceConnected_NotConnected(void) {
    stBTRCoreHdl mockHandle;
    stBTRCoreDevStateInfo mockDevStateInfo;
    mockDevStateInfo.eDeviceCurrState = enBTRCoreDevStDisconnected;
    
    // Assign the mock device state info to the corresponding entry in the scanned device state info array
    mockHandle.stScannedDevStInfoArr[0] = mockDevStateInfo;

    // Set up the expectation for the btrCore_GetDeviceInfo function if needed
    // btrCore_GetDeviceInfo_ExpectAndReturn(&mockHandle, 0, enBTRCoreUnknown, &mockDevStateInfo, enBTRCoreSuccess);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo4);
    // Call the function being tested
    enBTRCoreRet ret = BTRCore_GetDeviceConnected(&mockHandle, 0, enBTRCoreUnknown);

    // Assert that the function returned the expected result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetDeviceConnected_DeviceInfoFailure(void) {
    stBTRCoreHdl mockHandle;

    //btrCore_GetDeviceInfo_ExpectAndReturn(&mockHandle, 0, enBTRCoreUnknown, NULL, enBTRCoreFailure);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo3);

    enBTRCoreRet ret = BTRCore_GetDeviceConnected(&mockHandle, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_GetDeviceDisconnected_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_GetDeviceDisconnected(NULL, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_GetDeviceDisconnected_DeviceInfoSuccess_Disconnected(void) {
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));

    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    // Initialize the structure members
    hBTRCore->connHdl = (void*)1; 
    hBTRCore->curAdapterPath = "/test/path";
    
    // Sample details for 1st Known Device 
    hBTRCore->stKnownDevicesArr[0].tDeviceId = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = TRUE;  // Assuming this means connected
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "device0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress, "address0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress));
        
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(hBTRCore->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
    hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStDisconnected;

    hBTRCore->numOfPairedDevices = 1; 

    //btrCore_GetDeviceInfo_ExpectAndReturn(&hBTRCore, 0, enBTRCoreUnknown, enBTRCoreSuccess);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo3);

    enBTRCoreRet ret = BTRCore_GetDeviceDisconnected(hBTRCore, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}




void test_BTRCore_GetDeviceDisconnected_NotDisconnected(void) {
    stBTRCoreHdl mockHandle;
    stBTRCoreDevStateInfo mockDevStateInfo;
    mockDevStateInfo.eDeviceCurrState = enBTRCoreDevStConnected;
    mockHandle.stScannedDevStInfoArr[0] = mockDevStateInfo;
    //btrCore_GetDeviceInfo_ExpectAndReturn(&mockHandle, 0, enBTRCoreUnknown, &mockDevStateInfo, enBTRCoreSuccess);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo3);

    enBTRCoreRet ret = BTRCore_GetDeviceDisconnected(&mockHandle, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_GetDeviceDisconnected_DeviceInfoFailure(void) {
    stBTRCoreHdl mockHandle;

    //btrCore_GetDeviceInfo_ExpectAndReturn(&mockHandle, 0, enBTRCoreUnknown, NULL, enBTRCoreFailure);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo3);

    enBTRCoreRet ret = BTRCore_GetDeviceDisconnected(&mockHandle, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_GetDeviceTypeClass_NullHandle(void) {
    enBTRCoreRet ret;
    enBTRCoreDeviceType devType;
    enBTRCoreDeviceClass devClass;

    ret = BTRCore_GetDeviceTypeClass(NULL, 0, &devType, &devClass);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}


void test_BTRCore_GetDeviceTypeClass_NullDeviceType(void) {
    enBTRCoreRet ret;
    stBTRCoreHdl hBTRCore;
    enBTRCoreDeviceClass devClass;

    ret = BTRCore_GetDeviceTypeClass(&hBTRCore, 0, NULL, &devClass);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_GetDeviceTypeClass_NullDeviceClass(void) {
    enBTRCoreRet ret;
    stBTRCoreHdl hBTRCore;
    enBTRCoreDeviceType devType;

    ret = BTRCore_GetDeviceTypeClass(&hBTRCore, 0, &devType, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_GetDeviceTypeClass_NoPairedDevices(void) {
    enBTRCoreRet ret;
    stBTRCoreHdl hBTRCore = { .numOfPairedDevices = 0 };
    enBTRCoreDeviceType devType;
    enBTRCoreDeviceClass devClass;

    ret = BTRCore_GetDeviceTypeClass(&hBTRCore, 0, &devType, &devClass);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_GetDeviceTypeClass_NoScannedDevices(void) {
    enBTRCoreRet ret;
    stBTRCoreHdl hBTRCore = { .numOfPairedDevices = 1, .numOfScannedDevices = 0 };
    enBTRCoreDeviceType devType;
    enBTRCoreDeviceClass devClass;

    // Use a device ID that is not present in the stKnownDevicesArr array
    tBTRCoreDevId nonExistentDeviceId = 999; // Assuming this ID is out of range

    ret = BTRCore_GetDeviceTypeClass(&hBTRCore, nonExistentDeviceId, &devType, &devClass);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}



void test_BTRCore_GetDeviceTypeClass_ValidInput(void) {
    enBTRCoreRet ret;
    stBTRCoreHdl hBTRCore = { .numOfPairedDevices = 1, .numOfScannedDevices = 1 };
    enBTRCoreDeviceType devType;
    enBTRCoreDeviceClass devClass;
    stBTRCoreBTDevice device = { .enDeviceType = enBTDevAudioSink };

    hBTRCore.stKnownDevicesArr[0] = device;

    ret = BTRCore_GetDeviceTypeClass(&hBTRCore, 0, &devType, &devClass);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(enBTDevAudioSink, devClass);
    
}


void test_BTRCore_GetDeviceMediaInfo_NULL_Handle(void) {
    stBTRCoreDevMediaInfo mediaInfo;
    enBTRCoreRet ret = BTRCore_GetDeviceMediaInfo(NULL, 0, enBTRCoreUnknown, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}


void test_BTRCore_GetDeviceMediaInfo_NULL_MediaInfo(void) {
    tBTRCoreHandle handle = (tBTRCoreHandle)1;
    enBTRCoreRet ret = BTRCore_GetDeviceMediaInfo(handle, 0, enBTRCoreUnknown, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}



void test_BTRCore_GetDeviceMediaInfo_ValidInputs(void) {
    stBTRCoreDevMediaInfo mediaInfo;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreAVMediaInfo avMediaInfo;
    
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    if (!hBTRCore) {
        BTRCORELOG_ERROR("Memory allocation failed for hBTRCore\n");
        return;
    }
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    // Initialize mediaInfo and allocate memory for the nested pstBtrCoreDevMCodecInfo
    memset(&mediaInfo, 0, sizeof(stBTRCoreDevMediaInfo));
    mediaInfo.pstBtrCoreDevMCodecInfo = malloc(sizeof(stBTRCoreDevMediaSbcInfo)); // Adjust size based on actual use case
    
    memset(mediaInfo.pstBtrCoreDevMCodecInfo, 0, sizeof(stBTRCoreDevMediaSbcInfo));
    
    // Initialize the structure members
    hBTRCore->connHdl = (void*)1; 
    hBTRCore->curAdapterPath = "/test/path";
    
    // Sample details for the 1st Known Device 
    hBTRCore->stKnownDevicesArr[0].tDeviceId = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = TRUE;  // Assuming this means connected
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "device0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress, "address0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(hBTRCore->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
    hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStConnected;

    hBTRCore->numOfPairedDevices = 1; 

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo4);

    // Mocking BTRCore_AVMedia_GetCurMediaInfo
    BTRCore_AVMedia_GetCurMediaInfo_IgnoreAndReturn(enBTRCoreSuccess);

    enBTRCoreRet ret = BTRCore_GetDeviceMediaInfo(hBTRCore, 0, enBTRCoreUnknown, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Clean up
    free(mediaInfo.pstBtrCoreDevMCodecInfo);
    free(hBTRCore);
}


//from this tharun3

void test_BTRCore_StartAdvertisement_hBTRCore_NULL(void) {
    enBTRCoreRet ret = BTRCore_StartAdvertisement(NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}


void test_BTRCore_StartAdvertisement_hBTRCore_Not_NULL(void) {
    stBTRCoreHdl mockBTRCoreHandle;
    
    // Initialize the mockBTRCoreHandle with valid test data or NULL values
    mockBTRCoreHandle.leHdl = NULL;
    mockBTRCoreHandle.connHdl = NULL;
    mockBTRCoreHandle.curAdapterPath = NULL;

    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&mockBTRCoreHandle; // Valid mock handle

    BTRCore_LE_StartAdvertisement_IgnoreAndReturn(enBTRCoreSuccess);

    enBTRCoreRet ret = BTRCore_StartAdvertisement(hBTRCore);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_StopAdvertisement_NullInput(void) {
    enBTRCoreRet ret = BTRCore_StopAdvertisement(NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_StopAdvertisement_ValidInput(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    enBTRCoreRet ret;

    // Mock the necessary functions if any
    // Example: mock_function_ExpectAndReturn(...);

    BTRCore_LE_StopAdvertisement_IgnoreAndReturn(enBTRCoreSuccess);

    ret = BTRCore_StopAdvertisement(hBTRCore);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    free(hBTRCore);
}



void test_BTRCore_ReleaseAdvertisement_hBTRCore_NULL(void) {
    enBTRCoreRet ret = BTRCore_ReleaseAdvertisement(NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}


void test_BTRCore_ReleaseAdvertisement_hBTRCore_Not_NULL(void) {
    stBTRCoreHdl btrCoreHandle;
    enBTRCoreRet ret;

    BTRCore_LE_ReleaseAdvertisement_IgnoreAndReturn(enBTRCoreSuccess); 

    // Initialize the handle with some values if needed
    ret = BTRCore_ReleaseAdvertisement(&btrCoreHandle);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_GetPropertyValue_NullHandle(void) {
    char uuid[] = "some-uuid";
    char value[256];
    enBTRCoreRet ret = BTRCore_GetPropertyValue(NULL, uuid, value, enBTRCoreLePropGValue);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}



void test_BTRCore_GetPropertyValue_NullUUID(void) {
    tBTRCoreHandle handle = (tBTRCoreHandle)1;
    char value[256];
    enBTRCoreRet ret = BTRCore_GetPropertyValue(handle, NULL, value, enBTRCoreLePropGValue);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}


void test_BTRCore_GetPropertyValue_NullValue(void) {
    tBTRCoreHandle handle = (tBTRCoreHandle)1;
    char uuid[] = "some-uuid";
    enBTRCoreRet ret = BTRCore_GetPropertyValue(handle, uuid, NULL, enBTRCoreLePropGValue);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}



void test_BTRCore_GetPropertyValue_ValidInputs(void) {
    tBTRCoreHandle handle = (tBTRCoreHandle)1;
    char uuid[] = "some-uuid";
    char value[256];
    enBTRCoreRet ret;

    // Mock the behavior of BTRCore_LE_GetPropertyValue
    //BTRCore_LE_GetPropertyValue_ExpectAndReturn(handle, uuid, value, enBTRCoreLePropGValue, enBTRCoreSuccess);

    ret = BTRCore_GetPropertyValue(handle, uuid, value, enBTRCoreLePropGValue);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);

    //  There is no test function for Success case for the BTRCore_GetPropertyValue function
}
    
void test_BTRCore_GetPropertyValue_InvalidProperty(void) {
    tBTRCoreHandle handle = (tBTRCoreHandle)1;
    char uuid[] = "some-uuid";
    char value[256];
    enBTRCoreRet ret;

    // Mock the behavior of BTRCore_LE_GetPropertyValue
    //BTRCore_LE_GetPropertyValue_ExpectAndReturn(handle, uuid, value, enBTRCoreLePropUnknown, enBTRCoreFailure);

    ret = BTRCore_GetPropertyValue(handle, uuid, value, enBTRCoreLePropUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetAdvertisementInfo_ValidInputs(void) {
    // Allocate memory for the structure and initialize it
    stBTRCoreHdl stBTRCoreInstance = {0}; // Initialize to zero
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&stBTRCoreInstance;

    // Mock input values
    char advtType[] = "Type";
    char advtBeaconName[] = "BeaconName";

    // Mock the BTRCore_LE_SetAdvertisementInfo function to return success
    BTRCore_LE_SetAdvertisementInfo_ExpectAndReturn(stBTRCoreInstance.leHdl, advtType, advtBeaconName, enBTRCoreSuccess);

    // Call the function under test
    enBTRCoreRet result = BTRCore_SetAdvertisementInfo(hBTRCore, advtType, advtBeaconName);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}

void test_BTRCore_SetAdvertisementInfo_NullHandle(void) {
    char advtType[] = "Type";
    char advtBeaconName[] = "BeaconName";

    enBTRCoreRet result = BTRCore_SetAdvertisementInfo(NULL, advtType, advtBeaconName);

    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetAdvertisementInfo_NullAdvtType(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1; // Mock handle
    char advtBeaconName[] = "BeaconName";

    enBTRCoreRet result = BTRCore_SetAdvertisementInfo(hBTRCore, NULL, advtBeaconName);

    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetAdvertisementInfo_NullAdvtBeaconName(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1; // Mock handle
    char advtType[] = "Type";

    enBTRCoreRet result = BTRCore_SetAdvertisementInfo(hBTRCore, advtType, NULL);

    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetServiceUUIDs_NULL_hBTRCore(void) {
    enBTRCoreRet ret = BTRCore_SetServiceUUIDs(NULL, "someUUID");
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetServiceUUIDs_NULL_aUUID(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret = BTRCore_SetServiceUUIDs(&hBTRCore, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetServiceUUIDs_ValidInputs(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret;

    // Mock any necessary functions here
    // Example: mock_function_ExpectAndReturn(...);

    BTRCore_LE_SetServiceUUIDs_IgnoreAndReturn(enBTRCoreSuccess);

    ret = BTRCore_SetServiceUUIDs(&hBTRCore, "someUUID");
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_SetManufacturerData_NullHandle(void) {
    unsigned char deviceDetails[] = {0x01, 0x02, 0x03};
    enBTRCoreRet ret = BTRCore_SetManufacturerData(NULL, 1234, deviceDetails, 3);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetManufacturerData_NullDeviceDetails(void) {
    tBTRCoreHandle handle = (tBTRCoreHandle)1;
    enBTRCoreRet ret = BTRCore_SetManufacturerData(handle, 1234, NULL, 3);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetManufacturerData_ValidInput(void) {
    stBTRCoreHdl stHandle = {0};  // Create a valid stBTRCoreHdl structure
    stHandle.leHdl = (tBTRCoreLeHdl)1234;  // Assign a valid leHdl or mock it if necessary
    tBTRCoreHandle handle = &stHandle;  // Set the handle to point to the structure
    unsigned char deviceDetails[] = {0x01, 0x02, 0x03};
    enBTRCoreRet ret;

    // Mock the expected behavior of the dependencies
    BTRCore_LE_SetManufacturerData_ExpectAndReturn(stHandle.leHdl, 1234, deviceDetails, 3, enBTRCoreSuccess);

    ret = BTRCore_SetManufacturerData(handle, 1234, deviceDetails, 3);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_SetManufacturerData_InvalidLength(void) {
    stBTRCoreHdl mockHandle;  // Create a mock stBTRCoreHdl structure
    tBTRCoreHandle handle = &mockHandle;  // Assign its address to handle
    unsigned char deviceDetails[] = {0x01, 0x02, 0x03};
    enBTRCoreRet ret;

    // Initialize mockHandle as needed
    mockHandle.leHdl = NULL;  // Or assign an appropriate value if needed

    // Mock the expected behavior of the dependencies
    BTRCore_LE_SetManufacturerData_ExpectAndReturn(mockHandle.leHdl, 1234, deviceDetails, -1, enBTRCoreFailure);

    ret = BTRCore_SetManufacturerData(handle, 1234, deviceDetails, -1);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetEnableTxPower_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_SetEnableTxPower(NULL, TRUE);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetEnableTxPower_ValidHandle_True(void) {
    stBTRCoreHdl mockHandle;
    enBTRCoreRet ret;

    // Mock any required functions here

    BTRCore_LE_SetEnableTxPower_IgnoreAndReturn(enBTRCoreSuccess);  

    ret = BTRCore_SetEnableTxPower(&mockHandle, TRUE);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_SetEnableTxPower_ValidHandle_False(void) {
    stBTRCoreHdl mockHandle;
    enBTRCoreRet ret;

    // Mock any required functions here
    BTRCore_LE_SetEnableTxPower_IgnoreAndReturn(enBTRCoreFailure);

    ret = BTRCore_SetEnableTxPower(&mockHandle, FALSE);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetServiceInfo_ValidInputs(void) {
    stBTRCoreHdl mockBTRCoreHdl = {0};  // Initialize the structure with zeros
    mockBTRCoreHdl.curAdapterPath = "/org/bluez/hci0";  // Example initialization
    mockBTRCoreHdl.curAdapterAddr = "00:11:22:33:44:55";  // Example initialization
    mockBTRCoreHdl.leHdl = (tBTRCoreLeHdl)1;  // Mocking the LE handle

    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&mockBTRCoreHdl;  // Use the address of the mock structure
    char* aUUID = "1234";
    BOOLEAN aServiceType = TRUE;

    BTRCore_LE_AddGattServiceInfo_IgnoreAndReturn(enBTRCoreSuccess);

    enBTRCoreRet result = BTRCore_SetServiceInfo(hBTRCore, aUUID, aServiceType);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}

void test_BTRCore_SetServiceInfo_NullHandle(void) {
    char* aUUID = "1234";
    BOOLEAN aServiceType = TRUE;

    enBTRCoreRet result = BTRCore_SetServiceInfo(NULL, aUUID, aServiceType);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetServiceInfo_NullUUID(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1; // Mock handle
    BOOLEAN aServiceType = TRUE;

    enBTRCoreRet result = BTRCore_SetServiceInfo(hBTRCore, NULL, aServiceType);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetServiceInfo_NullHandleAndUUID(void) {
    BOOLEAN aServiceType = TRUE;

    enBTRCoreRet result = BTRCore_SetServiceInfo(NULL, NULL, aServiceType);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_RemoveServiceInfo_NullHandle(void) {
    enBTRCoreRet result = BTRCore_RemoveServiceInfo(NULL, "validUUID");
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_RemoveServiceInfo_NullUUID(void) {
    tBTRCoreHandle validHandle = (tBTRCoreHandle)1; // Mock valid handle
    enBTRCoreRet result = BTRCore_RemoveServiceInfo(validHandle, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_RemoveServiceInfo_ValidInputs(void) {
    // Initialize a valid stBTRCoreHdl structure
    stBTRCoreHdl mockHdl;
    memset(&mockHdl, 0, sizeof(mockHdl));  // Initialize the structure to zero
    mockHdl.leHdl = (tBTRCoreLeHdl)1; // Mock valid LE handle
    mockHdl.curAdapterPath = "mockAdapterPath";

    // Set up the function to be called by BTRCore_RemoveServiceInfo
    // You may need to use a mocking framework to properly mock this function
    // For example, using CMock or another mocking library
    BTRCore_LE_RemoveGattServiceInfo_IgnoreAndReturn(enBTRCoreSuccess);

    char* validUUID = "validUUID";

    enBTRCoreRet result = BTRCore_RemoveServiceInfo((tBTRCoreHandle)&mockHdl, validUUID);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}

void test_BTRCore_SetGattInfo_ValidInputs(void) {
    // Create a mock BTRCoreHdl structure
    stBTRCoreHdl mockBTRCoreHdl = {0};
    mockBTRCoreHdl.leHdl = (tBTRCoreLeHdl)1;  // Set a dummy valid handle
    mockBTRCoreHdl.curAdapterPath = "mockAdapterPath";
    mockBTRCoreHdl.curAdapterAddr = "mockAdapterAddr";

    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&mockBTRCoreHdl;
    char parentUUID[] = "parentUUID";
    char UUID[] = "UUID";
    unsigned short flags = 0;
    char value[] = "value";
    enBTRCoreLeProp element = enBTRCoreLePropGUUID;

    // Mock the BTRCore_LE_SetPropertyValue function call if necessary
    BTRCore_LE_SetPropertyValue_ExpectAndReturn(hBTRCore, UUID, value, element, enBTRCoreSuccess);
    
    BTRCore_LE_AddGattDescInfo_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_AddGattCharInfo_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_SetPropertyValue_IgnoreAndReturn(enBTRCoreSuccess);  

    // Call the function under test
    enBTRCoreRet result = BTRCore_SetGattInfo(hBTRCore, parentUUID, UUID, flags, value, element);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
}

void test_BTRCore_SetGattInfo_NullHandle(void) {
    char parentUUID[] = "parentUUID";
    char UUID[] = "UUID";
    unsigned short flags = 0;
    char value[] = "value";
    enBTRCoreLeProp element = enBTRCoreLePropGUUID;

    enBTRCoreRet result = BTRCore_SetGattInfo(NULL, parentUUID, UUID, flags, value, element);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetGattInfo_NullParentUUID(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    char UUID[] = "UUID";
    unsigned short flags = 0;
    char value[] = "value";
    enBTRCoreLeProp element = enBTRCoreLePropGUUID;

    enBTRCoreRet result = BTRCore_SetGattInfo(hBTRCore, NULL, UUID, flags, value, element);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetGattInfo_NullUUID(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    char parentUUID[] = "parentUUID";
    unsigned short flags = 0;
    char value[] = "value";
    enBTRCoreLeProp element = enBTRCoreLePropGUUID;

    enBTRCoreRet result = BTRCore_SetGattInfo(hBTRCore, parentUUID, NULL, flags, value, element);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetGattInfo_NullValue(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    char parentUUID[] = "parentUUID";
    char UUID[] = "UUID";
    unsigned short flags = 0;
    enBTRCoreLeProp element = enBTRCoreLePropGUUID;

    enBTRCoreRet result = BTRCore_SetGattInfo(hBTRCore, parentUUID, UUID, flags, NULL, element);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}

void test_BTRCore_SetPropertyValue_hBTRCore_NULL(void) {
    enBTRCoreRet ret = BTRCore_SetPropertyValue(NULL, "UUID", "Value", enBTRCoreLePropGValue);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_SetPropertyValue_aValue_NULL(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)1;
    enBTRCoreRet ret = BTRCore_SetPropertyValue(hBTRCore, "UUID", NULL, enBTRCoreLePropGValue);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_SetPropertyValue_Success(void) {
    stBTRCoreHdl mockHdl = {0};  // Initialize mock handle
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&mockHdl;  // Use pointer to mock handle
    
    // Initialize mock handle values
    mockHdl.leHdl = (void*)1;  // Set to a dummy value for the test

    // Set up mock expectations
    mock_btrCore_le_Init();
    BTRCore_LE_SetPropertyValue_ExpectAndReturn(mockHdl.leHdl, "UUID", "Value", enBTRCoreLePropGValue, 1);
    
    // Call function under test
    enBTRCoreRet ret = BTRCore_SetPropertyValue(hBTRCore, "UUID", "Value", enBTRCoreLePropGValue);

    // Assert expected result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Verify mocks
    mock_btrCore_le_Verify();
    mock_btrCore_le_Destroy();
}
void test_BTRCore_SetPropertyValue_Failure(void) {
    stBTRCoreHdl stHandle;
    tBTRCoreHandle hBTRCore = &stHandle;  // Point to valid memory
    stHandle.leHdl = NULL;  // Ensure this is properly initialized, e.g., NULL or a mock handle

    mock_btrCore_le_Init();
    
    // You may need to mock BTRCore_LE_SetPropertyValue if it's expected to fail
    BTRCore_LE_SetPropertyValue_ExpectAndReturn(stHandle.leHdl, "UUID", "Value", enBTRCoreLePropGValue, 0);
    
    enBTRCoreRet ret = BTRCore_SetPropertyValue(hBTRCore, "UUID", "Value", enBTRCoreLePropGValue);
    
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
    
    mock_btrCore_le_Verify();
    mock_btrCore_le_Destroy();
}

void test_BTRCore_RegisterDiscoveryCb_Success(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    fPtr_BTRCore_DeviceDiscCb afpcBBTRCoreDeviceDisc = (fPtr_BTRCore_DeviceDiscCb)0x1234;
    void* apUserData = (void*)0x5678;

    ((stBTRCoreHdl*)hBTRCore)->fpcBBTRCoreDeviceDisc = NULL;

    enBTRCoreRet result = BTRCore_RegisterDiscoveryCb(hBTRCore, afpcBBTRCoreDeviceDisc, apUserData);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    TEST_ASSERT_EQUAL(afpcBBTRCoreDeviceDisc, ((stBTRCoreHdl*)hBTRCore)->fpcBBTRCoreDeviceDisc);

    free(hBTRCore);
}

void test_BTRCore_RegisterDiscoveryCb_NullHandle(void) {
    fPtr_BTRCore_DeviceDiscCb afpcBBTRCoreDeviceDisc = (fPtr_BTRCore_DeviceDiscCb)0x1234;
    void* apUserData = (void*)0x5678;

    enBTRCoreRet result = BTRCore_RegisterDiscoveryCb(NULL, afpcBBTRCoreDeviceDisc, apUserData);

    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, result);
}

void test_BTRCore_RegisterDiscoveryCb_NullCallback(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    void* apUserData = (void*)0x5678;

    ((stBTRCoreHdl*)hBTRCore)->fpcBBTRCoreDeviceDisc = NULL;

    enBTRCoreRet result = BTRCore_RegisterDiscoveryCb(hBTRCore, NULL, apUserData);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    free(hBTRCore);
}

void test_BTRCore_RegisterDiscoveryCb_AlreadySet(void) {
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    fPtr_BTRCore_DeviceDiscCb afpcBBTRCoreDeviceDisc = (fPtr_BTRCore_DeviceDiscCb)0x1234;
    void* apUserData = (void*)0x5678;

    ((stBTRCoreHdl*)hBTRCore)->fpcBBTRCoreDeviceDisc = (fPtr_BTRCore_DeviceDiscCb)0x9999;

    enBTRCoreRet result = BTRCore_RegisterDiscoveryCb(hBTRCore, afpcBBTRCoreDeviceDisc, apUserData);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    free(hBTRCore);
}

void test_BTRCore_RegisterStatusCb_hBTRCore_NULL(void) {
    enBTRCoreRet result = BTRCore_RegisterStatusCb(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, result);
}

void test_BTRCore_RegisterStatusCb_hBTRCore_Not_NULL_fpcBBTRCoreStatus_NULL(void) {
    stBTRCoreHdl* hBTRCore = malloc(sizeof(hBTRCore));
    hBTRCore->fpcBBTRCoreStatus = NULL;

    enBTRCoreRet result = BTRCore_RegisterStatusCb(hBTRCore, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    //TEST_ASSERT_NULL(hBTRCore->fpcBBTRCoreStatus);
}

void test_BTRCore_RegisterStatusCb_hBTRCore_Not_NULL_fpcBBTRCoreStatus_Not_NULL(void) {
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    fPtr_BTRCore_StatusCb mockStatusCb = mock_BTRCore_StatusCb;

    hBTRCore->fpcBBTRCoreStatus = NULL;

    enBTRCoreRet result = BTRCore_RegisterStatusCb(hBTRCore, mockStatusCb, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    TEST_ASSERT_EQUAL(mockStatusCb, hBTRCore->fpcBBTRCoreStatus);
}

void test_BTRCore_RegisterMediaStatusCb_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_RegisterMediaStatusCb(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_RegisterMediaStatusCb_ValidHandle_ValidCallback(void) {
    
    stBTRCoreHdl* hBTRCore = malloc (sizeof(stBTRCoreHdl));
    fPtr_BTRCore_MediaStatusCb callback = mock_BTRCore_MediaStatusCb;

    hBTRCore->fpcBBTRCoreMediaStatus = NULL;
    // Call the function to register the callback
    enBTRCoreRet ret = BTRCore_RegisterMediaStatusCb(hBTRCore, mock_BTRCore_MediaStatusCb, NULL);

    // Check the return value
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    free (hBTRCore);

}

void test_BTRCore_RegisterConnectionIntimationCb_ValidInputs(void) {
    stBTRCoreHdl stBTRCoreHandle; // Declare an instance of stBTRCoreHdl
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&stBTRCoreHandle; // Use the address of the instance as the handle
    fPtr_BTRCore_ConnIntimCb afpcBBTRCoreConnIntim = (fPtr_BTRCore_ConnIntimCb)0x5678;
    void* apUserData = (void*)0x9ABC;

    // Initialize the structure to avoid any garbage values
    memset(&stBTRCoreHandle, 0, sizeof(stBTRCoreHandle));

    // Call the function
    enBTRCoreRet ret = BTRCore_RegisterConnectionIntimationCb(hBTRCore, afpcBBTRCoreConnIntim, apUserData);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Optional: Verify if the callback was registered correctly
    TEST_ASSERT_EQUAL_PTR(afpcBBTRCoreConnIntim, stBTRCoreHandle.fpcBBTRCoreConnIntim);
    TEST_ASSERT_EQUAL_PTR(apUserData, stBTRCoreHandle.pvcBConnIntimUserData);
}

void test_BTRCore_RegisterConnectionIntimationCb_NullHandle(void) {
    fPtr_BTRCore_ConnIntimCb afpcBBTRCoreConnIntim = (fPtr_BTRCore_ConnIntimCb)0x5678;
    void* apUserData = (void*)0x9ABC;

    enBTRCoreRet ret = BTRCore_RegisterConnectionIntimationCb(NULL, afpcBBTRCoreConnIntim, apUserData);

    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_RegisterConnectionIntimationCb_NullCallback(void) {
    stBTRCoreHdl stBTRCoreHandle = {0};  // Allocate and zero-initialize the structure
    tBTRCoreHandle hBTRCore = &stBTRCoreHandle;  // Use a valid handle
    void* apUserData = (void*)0x9ABC;

    enBTRCoreRet ret = BTRCore_RegisterConnectionIntimationCb(hBTRCore, NULL, apUserData);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_RegisterConnectionIntimationCb_NullUserData(void) {
    stBTRCoreHdl stBTRCore;
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&stBTRCore;  // Use the address of stBTRCore as the handle
    fPtr_BTRCore_ConnIntimCb afpcBBTRCoreConnIntim = (fPtr_BTRCore_ConnIntimCb)0x5678;

    // Initialize the callback pointers to NULL to simulate the first-time registration
    stBTRCore.fpcBBTRCoreConnIntim = NULL;
    stBTRCore.pvcBConnIntimUserData = NULL;

    enBTRCoreRet ret = BTRCore_RegisterConnectionIntimationCb(hBTRCore, afpcBBTRCoreConnIntim, NULL);

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_RegisterConnectionIntimationCb_AlreadySet(void) {
    stBTRCoreHdl stBTRCoreHandle = {0};  // Initialize the structure to zero
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)&stBTRCoreHandle;  // Use the address of the structure as the handle
    fPtr_BTRCore_ConnIntimCb afpcBBTRCoreConnIntim = (fPtr_BTRCore_ConnIntimCb)0x5678;
    void* apUserData = (void*)0x9ABC;

    // Initialize the structure with valid values
    stBTRCoreHandle.fpcBBTRCoreConnIntim = (fPtr_BTRCore_ConnIntimCb)0x9999;
    stBTRCoreHandle.pvcBConnIntimUserData = (void*)0x1234;

    // Call the function to register the callback
    enBTRCoreRet ret = BTRCore_RegisterConnectionIntimationCb(hBTRCore, afpcBBTRCoreConnIntim, apUserData);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Verify that the callback was not overwritten
    TEST_ASSERT_EQUAL_PTR((fPtr_BTRCore_ConnIntimCb)0x9999, stBTRCoreHandle.fpcBBTRCoreConnIntim);
    TEST_ASSERT_EQUAL_PTR((void*)0x1234, stBTRCoreHandle.pvcBConnIntimUserData);
}
void test_BTRCore_RegisterConnectionAuthenticationCb_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_RegisterConnectionAuthenticationCb(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_RegisterConnectionAuthenticationCb_NullCallback(void) {
    stBTRCoreHdl hBTRCore;
    enBTRCoreRet ret = BTRCore_RegisterConnectionAuthenticationCb(&hBTRCore, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_RegisterConnectionAuthenticationCb_Success(void) {
    stBTRCoreHdl hBTRCore;
    fPtr_BTRCore_ConnAuthCb callback = (fPtr_BTRCore_ConnAuthCb)0x1234;
    enBTRCoreRet ret = BTRCore_RegisterConnectionAuthenticationCb(&hBTRCore, callback, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}
///////////////////////////////////////////////////////////////
int
mock_BtrCore_BTSetProp (
    void*               apstBtIfceHdl,
    const char*         apcBtOpIfcePath,
    enBTOpIfceType      aenBtOpIfceType,
    unBTOpIfceProp      aunBtOpIfceProp,
    void*               apvVal
) {
       
    return 0;
}
int
mock_BtrCore_BTSetProp_fail (
    void*               apstBtIfceHdl,
    const char*         apcBtOpIfcePath,
    enBTOpIfceType      aenBtOpIfceType,
    unBTOpIfceProp      aunBtOpIfceProp,
    void*               apvVal
) {
       
    return 1;
}

int
mock_BtrCore_BTGetIfceNameVersion (
    void* apstBtIfceHdl,
    char* apBtOutIfceName,
    char* apBtOutVersion
) {
   return 0;
}

int
mock_BtrCore_BTPerformAdapterOp (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath,
    const char*     apcDevPath,
    enBTAdapterOp   aenBTAdpOp
) {

    return 0;
}
int
mock_BtrCore_BTPerformAdapterOp_fail (
    void*           apstBtIfceHdl,
    const char*     apBtAdapter,
    const char*     apBtAgentPath,
    const char*     apcDevPath,
    enBTAdapterOp   aenBTAdpOp
) {

    return 0;
}

int
mock_BtrCore_BTGetProp (
    void*               apstBtIfceHdl,
    const char*         apcBtOpIfcePath,
    enBTOpIfceType      aenBtOpIfceType,
    unBTOpIfceProp      aunBtOpIfceProp,
    void*               apvVal
) {
       
    return 0;
}
int
mock_BtrCore_BTGetProp_fail (
    void*               apstBtIfceHdl,
    const char*         apcBtOpIfcePath,
    enBTOpIfceType      aenBtOpIfceType,
    unBTOpIfceProp      aunBtOpIfceProp,
    void*               apvVal
) {
       
    return 1;
}
int mock_BtrCore_BTGetPairedDeviceInfo_fail(void* apstBtIfceHdl, const char* apcDevPath, stBTRCoreBTDevice* apstFoundDevice) {

    return -1;
}
int mock_btrCore_GetScannedDeviceAddress(stBTRCoreHdl* pstlhBTRCore, tBTRCoreDevId aBTRCoreDevId) {
    return 1; // Simulate that the device is found in the scanned list
}

enBTRCoreRet mock_btrCore_RemoveDeviceFromScannedDevicesArr(stBTRCoreHdl* pstlhBTRCore, tBTRCoreDevId aBTRCoreDevId, stBTRCoreBTDevice* pstScannedDevice) {
    pstScannedDevice->tDeviceId = aBTRCoreDevId; // Simulate successful removal
    return enBTRCoreSuccess;
}

int mock_BtrCore_BTGetBatteryLevel(void* connHdl, const char* devicePath, unsigned char* batteryLevel) {
    *batteryLevel = 75; // Simulate a battery level of 75%
    return 0; // Simulate success
}

int mock_BtrCore_BTGetBatteryLevel_fail(void* connHdl, const char* devicePath, unsigned char* batteryLevel) {
    return -1; // Simulate failure
}


void test_BTRCore_SetAdapterName_NotInitialized(void) {
    enBTRCoreRet ret;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_SetAdapterName(NULL, "/org/bluez/hci0", "TestAdapter");
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_SetAdapterName_InvalidArguments(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Test case for null adapter path
    ret = BTRCore_SetAdapterName(&btrCoreHdl, NULL, "TestAdapter");
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    // Test case for null adapter name
    ret = BTRCore_SetAdapterName(&btrCoreHdl, "/org/bluez/hci0", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_SetAdapterName_SetPropFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";

    // Mock the BtrCore_BTSetProp function to return failure
   
    BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp_fail);

    // Test case for BtrCore_BTSetProp failure
    ret = BTRCore_SetAdapterName(&btrCoreHdl, "/org/bluez/hci0", "TestAdapter");
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SetAdapterName_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";

    // Mock the BtrCore_BTSetProp function to return success
   BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp);

    // Test case for successful adapter name set
    ret = BTRCore_SetAdapterName(&btrCoreHdl, "/org/bluez/hci0", "TestAdapter");
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_GetAdapterName_NotInitialized(void) {
    enBTRCoreRet ret;
    char adapterName[BD_NAME_LEN + 1] = {'\0'};

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_GetAdapterName(NULL, "/org/bluez/hci0", adapterName);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_GetAdapterName_InvalidArguments(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    char adapterName[BD_NAME_LEN + 1] = {'\0'};

    // Test case for null adapter path
    ret = BTRCore_GetAdapterName(&btrCoreHdl, NULL, adapterName);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    // Test case for null adapter name
    ret = BTRCore_GetAdapterName(&btrCoreHdl, "/org/bluez/hci0", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_GetVersionInfo_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    char btVersion[BTRCORE_STR_LEN] = {'\0'};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTGetIfceNameVersion function to return success and provide interface name and version
    BtrCore_BTGetIfceNameVersion_StubWithCallback( mock_BtrCore_BTGetIfceNameVersion);

    // Call the function with valid parameters and expect success
    ret = BTRCore_GetVersionInfo(&btrCoreHdl, btVersion);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_StartDiscovery_NotInitialized(void) {
    enBTRCoreRet ret;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_StartDiscovery(NULL, "/org/bluez/hci0", enBTRCoreSpeakers, 10);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_StartDiscovery_InvalidArguments(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Test case for null adapter path
    ret = BTRCore_StartDiscovery(&btrCoreHdl, NULL, enBTRCoreSpeakers, 10);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_StartDiscovery_LEDiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStartLEDiscovery function to return failure
    BtrCore_BTStartLEDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for LE discovery failure
    ret = BTRCore_StartDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreLE, 10);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StartDiscovery_LEStopDiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStartLEDiscovery function to return success
    BtrCore_BTStartLEDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 0);

    // Mock the BtrCore_BTStopLEDiscovery function to return failure
    BtrCore_BTStopLEDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for LE stop discovery failure
    ret = BTRCore_StartDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreLE, 10);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StartDiscovery_ClassicDiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStartClassicDiscovery function to return failure
    BtrCore_BTStartClassicDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for Classic discovery failure
    ret = BTRCore_StartDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreSpeakers, 10);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StartDiscovery_ClassicStopDiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStartClassicDiscovery function to return success
    BtrCore_BTStartClassicDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 0);

    // Mock the BtrCore_BTStopClassicDiscovery function to return failure
    BtrCore_BTStopClassicDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for Classic stop discovery failure
    ret = BTRCore_StartDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreSpeakers, 10);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StartDiscovery_DiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStartDiscovery function to return failure
    BtrCore_BTStartDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for discovery failure
    ret = BTRCore_StartDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreUnknown, 10);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StartDiscovery_StopDiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStartDiscovery function to return success
    BtrCore_BTStartDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 0);

    // Mock the BtrCore_BTStopDiscovery function to return failure
    BtrCore_BTStopDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for stop discovery failure
    ret = BTRCore_StartDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreUnknown, 10);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StartDiscovery_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStartDiscovery function to return success
    BtrCore_BTStartDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 0);

    // Mock the BtrCore_BTStopDiscovery function to return success
    BtrCore_BTStopDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 0);

    // Test case for successful discovery
    ret = BTRCore_StartDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreUnknown, 10);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_StopDiscovery_LEDiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStopLEDiscovery function to return failure
    BtrCore_BTStopLEDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for LE stop discovery failure
    ret = BTRCore_StopDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreLE);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StopDiscovery_ClassicDiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStopClassicDiscovery function to return failure
    BtrCore_BTStopClassicDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for Classic stop discovery failure
    ret = BTRCore_StopDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreSpeakers);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StopDiscovery_DiscoveryFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStopDiscovery function to return failure
    BtrCore_BTStopDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 1);

    // Test case for stop discovery failure
    ret = BTRCore_StopDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreDiscoveryFailure, ret);
}

void test_BTRCore_StopDiscovery_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTStopDiscovery function to return success
    BtrCore_BTStopDiscovery_ExpectAndReturn(btrCoreHdl.connHdl, "/org/bluez/hci0", btrCoreHdl.agentPath, 0);

    // Test case for successful stop discovery
    ret = BTRCore_StopDiscovery(&btrCoreHdl, "/org/bluez/hci0", enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_PairDevice_NotInitialized(void) {
    enBTRCoreRet ret;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_PairDevice(NULL, 0);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_PairDevice_DeviceNotFound(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Test case for device not found
    ret = BTRCore_PairDevice(&btrCoreHdl, BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES + 1);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

void test_BTRCore_PairDevice_SetPropPairableModeFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    stBTRCoreBTDevice scannedDevice = {0};

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";
    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    strcpy(btrCoreHdl.stScannedDevicesArr[0].pcDeviceAddress,"00:11:22:33:44:55");
    btrCoreHdl.stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_HID_Keyboard;

    // Mock the BtrCore_BTSetProp function to return failure
    BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp_fail);

    // Test case for setting adapter to pairable mode failure
    ret = BTRCore_PairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_PairDevice_GetPropPairableModeFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    stBTRCoreBTDevice scannedDevice = {0};

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";
    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    strcpy(btrCoreHdl.stScannedDevicesArr[0].pcDeviceAddress,"00:11:22:33:44:55");
    btrCoreHdl.stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_Loudspeaker;

    // Mock the BtrCore_BTGetProp function to return failure
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp_fail);

    // Test case for getting adapter pairable mode failure
    ret = BTRCore_PairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_PairDevice_SetPropPairableModeSuccess(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    stBTRCoreBTDevice scannedDevice = {0};

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";
    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    strcpy(btrCoreHdl.stScannedDevicesArr[0].pcDeviceAddress,"00:11:22:33:44:55");
    btrCoreHdl.stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_Loudspeaker;

    // Mock the BtrCore_BTGetProp function to return success and PairableMode to 0
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp);
    BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp);
    BtrCore_BTPerformAdapterOp_StubWithCallback(mock_BtrCore_BTPerformAdapterOp);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);

    // Test case for setting adapter to pairable mode success
    ret = BTRCore_PairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_PairDevice_PerformAdapterOpFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    stBTRCoreBTDevice scannedDevice = {0};

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";
    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    strcpy(btrCoreHdl.stScannedDevicesArr[0].pcDeviceAddress,"00:11:22:33:44:55");
    btrCoreHdl.stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_HID_Keyboard;

    // Mock the BtrCore_BTSetProp function to return success
    BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp);

    // Mock the BtrCore_BTPerformAdapterOp function to return failure
    BtrCore_BTPerformAdapterOp_StubWithCallback(mock_BtrCore_BTPerformAdapterOp_fail);
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo_fail);

    // Test case for performing adapter operation failure
    ret = BTRCore_PairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_PairDevice_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    stBTRCoreBTDevice scannedDevice = {0};

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";
    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    strcpy(btrCoreHdl.stScannedDevicesArr[0].pcDeviceAddress,"00:11:22:33:44:55");
    btrCoreHdl.stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_HID_Keyboard;

    // Mock the BtrCore_BTSetProp function to return success
    BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp);

    // Mock the BtrCore_BTPerformAdapterOp function to return success
    BtrCore_BTPerformAdapterOp_StubWithCallback(mock_BtrCore_BTPerformAdapterOp);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    // Test case for successful pairing
    ret = BTRCore_PairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_PairDevice_DeviceFoundInLoop(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    stBTRCoreBTDevice scannedDevice = {0};

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    btrCoreHdl.curAdapterPath = "/org/bluez/hci0";
    btrCoreHdl.numOfScannedDevices = 1;
    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    btrCoreHdl.stScannedDevicesArr[0].tDeviceId = 1;
    strcpy(btrCoreHdl.stScannedDevicesArr[0].pcDeviceAddress,"00:11:22:33:44:55");
    btrCoreHdl.stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_HID_Keyboard;

    // Mock the BtrCore_BTSetProp function to return success
    BtrCore_BTSetProp_StubWithCallback(mock_BtrCore_BTSetProp);

    // Mock the BtrCore_BTPerformAdapterOp function to return success
    BtrCore_BTPerformAdapterOp_StubWithCallback(mock_BtrCore_BTPerformAdapterOp);

    // Test case for device found in loop
    ret = BTRCore_PairDevice(&btrCoreHdl, 1);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

void test_BTRCore_GetDeviceBatteryLevel_NotInitialized(void) {
    enBTRCoreRet ret;
    unsigned char batteryLevel;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_GetDeviceBatteryLevel(NULL, 0, enBTRCoreUnknown, &batteryLevel);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_GetDeviceBatteryLevel_InvalidArg(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Test case for null battery level pointer
    ret = BTRCore_GetDeviceBatteryLevel(&btrCoreHdl, 0, enBTRCoreUnknown, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_GetDeviceBatteryLevel_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    unsigned char batteryLevel;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    // Test case for getting device info failure
    ret = BTRCore_GetDeviceBatteryLevel(&btrCoreHdl, 0, enBTRCoreUnknown, &batteryLevel);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetDeviceBatteryLevel_GetBatteryLevelFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    unsigned char batteryLevel;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    // Mock the BtrCore_BTGetBatteryLevel function to return failure
    BtrCore_BTGetBatteryLevel_StubWithCallback(mock_BtrCore_BTGetBatteryLevel_fail);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    // Test case for getting battery level failure
    ret = BTRCore_GetDeviceBatteryLevel(&btrCoreHdl, 0, enBTRCoreUnknown, &batteryLevel);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetDeviceBatteryLevel_Success(void) {
 
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    unsigned char batteryLevel;
    stBTRCoreBTDevice btDevice = {0};
    stBTRCoreDevStateInfo devStateInfo = {0};
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";

    btrCoreHdl.numOfPairedDevices=1;
    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress , "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath , devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName ,deviceName);
    BtrCore_BTGetBatteryLevel_StubWithCallback(mock_BtrCore_BTGetBatteryLevel);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);

    // Test case for successful battery level retrieval
    ret = BTRCore_GetDeviceBatteryLevel(&btrCoreHdl, 0, enBTRCoreUnknown, &batteryLevel);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}
void test_BTRCore_GetDeviceBatteryLevel_GetBatteryLevel_fail(void) {
 
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    unsigned char batteryLevel;
    stBTRCoreBTDevice btDevice = {0};
    stBTRCoreDevStateInfo devStateInfo = {0};
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";

    btrCoreHdl.numOfPairedDevices=1;
    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress , "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath , devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName ,deviceName);
    BtrCore_BTGetBatteryLevel_StubWithCallback(mock_BtrCore_BTGetBatteryLevel_fail);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);

    // Test case for successful battery level retrieval
    ret = BTRCore_GetDeviceBatteryLevel(&btrCoreHdl, 0, enBTRCoreUnknown, &batteryLevel);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

int mock_BtrCore_BTConnectDevice(void* connHdl, const char* devicePath, enBTDeviceType deviceType) {
    return 0; // Simulate success
}

int mock_BtrCore_BTConnectDevice_fail(void* connHdl, const char* devicePath, enBTDeviceType deviceType) {
    return -1; // Simulate failure
}

int mock_BtrCore_BTDisableEnhancedRetransmissionMode(void* connHdl) {
    return 0; // Simulate success
}

int mock_BtrCore_BTDisableEnhancedRetransmissionMode_fail(void* connHdl) {
    return -1; // Simulate failure
}

int mock_BtrCore_BTDisconnectDevice(void* connHdl, const char* devicePath, enBTDeviceType deviceType) {
    return 0; // Simulate success
}

int mock_BtrCore_BTDisconnectDevice_fail(void* connHdl, const char* devicePath, enBTDeviceType deviceType) {
    return -1; // Simulate failure
}

int mock_BtrCore_BTEnableEnhancedRetransmissionMode(void* connHdl) {
    return 0; // Simulate success
}

int mock_BtrCore_BTEnableEnhancedRetransmissionMode_fail(void* connHdl) {
    return -1; // Simulate failure
}

void test_BTRCore_ConnectDevice_NotInitialized(void) {
    enBTRCoreRet ret;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_ConnectDevice(NULL, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_ConnectDevice_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo_fail);
    // Test case for getting device info failure
    ret = BTRCore_ConnectDevice(&btrCoreHdl, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_ConnectDevice_ConnectDeviceFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    // Mock the BtrCore_BTDisableEnhancedRetransmissionMode function to return success
    BtrCore_BTDisableEnhancedRetransmissionMode_StubWithCallback(mock_BtrCore_BTDisableEnhancedRetransmissionMode);

    // Mock the BtrCore_BTConnectDevice function to return failure
    BtrCore_BTConnectDevice_StubWithCallback(mock_BtrCore_BTConnectDevice_fail);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo_fail);

    // Test case for connect device failure
    ret = BTRCore_ConnectDevice(&btrCoreHdl, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_ConnectDevice_DisableERTMFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    // Mock the BtrCore_BTDisableEnhancedRetransmissionMode function to return failure
    BtrCore_BTDisableEnhancedRetransmissionMode_StubWithCallback(mock_BtrCore_BTDisableEnhancedRetransmissionMode_fail);

    // Mock the BtrCore_BTConnectDevice function to return success
    BtrCore_BTConnectDevice_StubWithCallback(mock_BtrCore_BTConnectDevice);

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo_fail);
    // Test case for disabling ERTM failure
    ret = BTRCore_ConnectDevice(&btrCoreHdl, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetDeviceConnected_HIDDevice(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreBTDevice knownDevice;
    stBTPairedDeviceInfo pairedDeviceInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    tBTRCoreDevId aBTRCoreDevId = 0;
    char lcDevHdlArr[13] = {'\0'};
    hBTRCore.numOfPairedDevices = 2;

    lcDevHdlArr[0]  = deviceAddress[0];
    lcDevHdlArr[1]  = deviceAddress[1];
    lcDevHdlArr[2]  = deviceAddress[3];
    lcDevHdlArr[3]  = deviceAddress[4];
    lcDevHdlArr[4]  = deviceAddress[6];
    lcDevHdlArr[5]  = deviceAddress[7];
    lcDevHdlArr[6]  = deviceAddress[9];
    lcDevHdlArr[7]  = deviceAddress[10];
    lcDevHdlArr[8]  = deviceAddress[12];
    lcDevHdlArr[9]  = deviceAddress[13];
    lcDevHdlArr[10] = deviceAddress[15];
    lcDevHdlArr[11] = deviceAddress[16];

    aBTRCoreDevId = (tBTRCoreDevId) strtoll(lcDevHdlArr, NULL, 16);

    enBTRCoreDeviceType aenBTRCoreDevType = enBTRCoreHID;

    // Initialize the structures
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&pairedDeviceInfo, 0, sizeof(stBTPairedDeviceInfo));

    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

    hBTRCore.stKnownDevicesArr[0] = knownDevice;
    hBTRCore.numOfScannedDevices = 1;

    // Set up the paired device info
    pairedDeviceInfo.numberOfDevices = 1;
    strncpy(pairedDeviceInfo.deviceInfo[0].pcAddress, deviceAddress, sizeof(pairedDeviceInfo.deviceInfo[0].pcAddress) - 1);
    pairedDeviceInfo.deviceInfo[0].pcAddress[sizeof(pairedDeviceInfo.deviceInfo[0].pcAddress) - 1] = '\0';
    pairedDeviceInfo.deviceInfo[0].bConnected = 1;

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_GetDeviceConnected(&hBTRCore, aBTRCoreDevId, aenBTRCoreDevType);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}
void test_BTRCore_AcquireDeviceDataPath_NotInitialized(void) {
    enBTRCoreRet ret;
    int dataPath, dataReadMTU, dataWriteMTU;
    unsigned int delay;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_AcquireDeviceDataPath(NULL, 0, enBTRCoreUnknown, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_AcquireDeviceDataPath_InvalidArg(void) {
    stBTRCoreHdl btrCoreHdl;
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    // Test case for invalid arguments
    enBTRCoreRet ret = BTRCore_AcquireDeviceDataPath(&btrCoreHdl, 0, enBTRCoreUnknown, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AcquireDeviceDataPath_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl;
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    int dataPath, dataReadMTU, dataWriteMTU;
    unsigned int delay;

    // Initialize the BTRCore handle with invalid device info
    btrCoreHdl.numOfPairedDevices = 0;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    // Test case for getting device info failure
    enBTRCoreRet ret = BTRCore_AcquireDeviceDataPath(&btrCoreHdl, 0, enBTRCoreUnknown, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_ReleaseDeviceDataPath_NotInitialized(void) {
    enBTRCoreRet ret;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_ReleaseDeviceDataPath(NULL, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_ReleaseDeviceDataPath_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl;
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    // Initialize the BTRCore handle with invalid device info
    btrCoreHdl.numOfPairedDevices = 0;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);

    // Test case for getting device info failure
    enBTRCoreRet ret = BTRCore_ReleaseDeviceDataPath(&btrCoreHdl, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AcquireDeviceDataPath_Failure(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    int dataPath, dataReadMTU, dataWriteMTU;
    unsigned int delay;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    // knownDevice.stDevStateInfo = devStateInfo;

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    BTRCore_AVMedia_AcquireDataPath_IgnoreAndReturn(enBTRCoreFailure);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_AcquireDeviceDataPath(&btrCoreHdl, 0, enBTRCoreUnknown, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
  
}

void test_BTRCore_AcquireDeviceDataPath_Success(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    int dataPath, dataReadMTU, dataWriteMTU;
    unsigned int delay;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    BTRCore_AVMedia_AcquireDataPath_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_AVMedia_ReleaseDataPath_IgnoreAndReturn(enBTRCoreSuccess);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_AcquireDeviceDataPath(&btrCoreHdl, 0, enBTRCoreUnknown, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
  
}
void test_BTRCore_ReleaseDeviceDataPath_Failure(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    BTRCore_AVMedia_AcquireDataPath_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_AVMedia_ReleaseDataPath_IgnoreAndReturn(enBTRCoreFailure);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_ReleaseDeviceDataPath(&btrCoreHdl, 0, enBTRCoreUnknown);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_ReleaseDeviceDataPath_Success(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    // knownDevice.stDevStateInfo = devStateInfo;

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    BTRCore_AVMedia_AcquireDataPath_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_AVMedia_ReleaseDataPath_IgnoreAndReturn(enBTRCoreSuccess);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_ReleaseDeviceDataPath(&btrCoreHdl, 0, enBTRCoreUnknown);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_MediaControl_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl;
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    // Initialize the BTRCore handle with invalid device info
    btrCoreHdl.numOfPairedDevices = 0;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo_fail);
    // Test case for getting device info failure
    enBTRCoreRet ret = BTRCore_MediaControl(&btrCoreHdl, 0, enBTRCoreUnknown, enBTRCoreMediaCtrlPlay, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_MediaControl_DeviceNotConnected1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = FALSE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;

    // Test case for device not connected
    enBTRCoreRet ret = BTRCore_MediaControl(&btrCoreHdl, 0, enBTRCoreUnknown, enBTRCoreMediaCtrlPlay, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_MediaControl_Success(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = TRUE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, deviceName, sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';


    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    BTRCore_AVMedia_MediaControl_IgnoreAndReturn(enBTRCoreSuccess);
    // Test case for successful media control
    enBTRCoreRet ret = BTRCore_MediaControl(&btrCoreHdl, 0, enBTRCoreUnknown, enBTRCoreMediaCtrlPlay, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_MediaControl_AllSwitchCases(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";
    enBTRCoreDeviceType deviceType = enBTRCoreUnknown;
    enBTRCoreMediaCtrl mediaCtrl;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = TRUE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, deviceName, sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';
   

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    
    BTRCore_AVMedia_MediaControl_IgnoreAndReturn(enBTRCoreSuccess);
    // Test all switch cases for device type
    for (deviceType = enBTDevAudioSink; deviceType <= enBTDevUnknown; deviceType++) {
        enBTRCoreRet ret = BTRCore_MediaControl(&btrCoreHdl, 0, deviceType, enBTRCoreMediaCtrlPlay, NULL);
        TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    }

    // Test all switch cases for media control
    for (mediaCtrl = enBTRCoreMediaCtrlPlay; mediaCtrl <= enBTRCoreMediaCtrlRptGroup; mediaCtrl++) {
        enBTRCoreRet ret = BTRCore_MediaControl(&btrCoreHdl, 0, enBTRCoreUnknown, mediaCtrl, NULL);
        TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    }
}

void test_BTRCore_GetMediaTrackInfo_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreMediaTrackInfo mediaTrackInfo;
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    // Initialize the BTRCore handle with invalid device info
    btrCoreHdl.numOfPairedDevices = 0;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo_fail);
    // Test case for getting device info failure
    enBTRCoreRet ret = BTRCore_GetMediaTrackInfo(&btrCoreHdl, 0, enBTRCoreUnknown, &mediaTrackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetMediaTrackInfo_DeviceNotConnected1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreMediaTrackInfo mediaTrackInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = FALSE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    // knownDevice.stDevStateInfo = devStateInfo;

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;

    // Test case for device not connected
    enBTRCoreRet ret = BTRCore_GetMediaTrackInfo(&btrCoreHdl, 0, enBTRCoreUnknown, &mediaTrackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetMediaElementTrackInfo_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreMediaTrackInfo mediaTrackInfo;
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    // Initialize the BTRCore handle with invalid device info
    btrCoreHdl.numOfPairedDevices = 0;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);

    // Test case for getting device info failure
    enBTRCoreRet ret = BTRCore_GetMediaElementTrackInfo(&btrCoreHdl, 0, enBTRCoreUnknown, 0, &mediaTrackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetMediaElementTrackInfo_DeviceNotConnected1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    stBTRCoreMediaTrackInfo mediaTrackInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = FALSE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    // knownDevice.stDevStateInfo = devStateInfo;

    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo_fail);

    // Test case for device not connected
    enBTRCoreRet ret = BTRCore_GetMediaElementTrackInfo(&btrCoreHdl, 0, enBTRCoreUnknown, 0, &mediaTrackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}
void test_BTRCore_GetMediaProperty_DeviceNotConnected1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* mediaPropertyKey = "testKey";
    char mediaPropertyValue[256];
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = FALSE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';


    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;

    // Test case for device not connected
    enBTRCoreRet ret = BTRCore_GetMediaProperty(&btrCoreHdl, 0, enBTRCoreUnknown, mediaPropertyKey, mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetMediaProperty_GetMediaPropertyFailure(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* mediaPropertyKey = "testKey";
    char mediaPropertyValue[256];
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = TRUE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    
    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;
    BTRCore_AVMedia_GetMediaProperty_IgnoreAndReturn(enBTRCoreFailure);
    // Test case for getting media property failure
    enBTRCoreRet ret = BTRCore_GetMediaProperty(&btrCoreHdl, 0, enBTRCoreUnknown, mediaPropertyKey, mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}
void test_BTRCore_SelectMediaElement_PlayableState(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";
    char isPlayable = 1;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = TRUE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, deviceName, sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';
    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;

    BTRCore_AVMedia_IsMediaElementPlayable_IgnoreAndReturn(enBTRCoreFailure);

    // Test case for media element being playable
    enBTRCoreRet ret = BTRCore_SelectMediaElement(&btrCoreHdl, 0, 0, enBTRCoreUnknown, enBTRCoreMedETypeTrack);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_SelectMediaElement_NotPlayableState(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";
    char isPlayable = 0;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the known device
    knownDevice.tDeviceId = 0;
    knownDevice.bDeviceConnected = TRUE;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(knownDevice.pcDevicePath, devicePath, sizeof(knownDevice.pcDevicePath) - 1);
    knownDevice.pcDevicePath[sizeof(knownDevice.pcDevicePath) - 1] = '\0';
    strncpy(knownDevice.pcDeviceName, deviceName, sizeof(knownDevice.pcDeviceName) - 1);
    knownDevice.pcDeviceName[sizeof(knownDevice.pcDeviceName) - 1] = '\0';
   
    btrCoreHdl.stKnownDevicesArr[0] = knownDevice;
    btrCoreHdl.numOfPairedDevices = 1;

    BTRCore_AVMedia_IsMediaElementPlayable_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_AVMedia_ChangeBrowserLocation_IgnoreAndReturn(enBTRCoreFailure);

    // Test case for media element not being playable
    enBTRCoreRet ret = BTRCore_SelectMediaElement(&btrCoreHdl, 0, 0, enBTRCoreUnknown, enBTRCoreMedETypeTrack);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_GetLEProperty_AllCases(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice scannedDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";
    char propValue[256];
    enBTRCoreRet ret;
    const char* uuid = "UUID";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&scannedDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    // Set up the scanned device
    scannedDevice.tDeviceId = 0;
    scannedDevice.bDeviceConnected = TRUE;
    strncpy(scannedDevice.pcDeviceAddress, deviceAddress, sizeof(scannedDevice.pcDeviceAddress) - 1);
    scannedDevice.pcDeviceAddress[sizeof(scannedDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(scannedDevice.pcDevicePath, devicePath, sizeof(scannedDevice.pcDevicePath) - 1);
    scannedDevice.pcDevicePath[sizeof(scannedDevice.pcDevicePath) - 1] = '\0';
    strncpy(scannedDevice.pcDeviceName, deviceName, sizeof(scannedDevice.pcDeviceName) - 1);
    scannedDevice.pcDeviceName[sizeof(scannedDevice.pcDeviceName) - 1] = '\0';

    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    btrCoreHdl.numOfScannedDevices = 1;

    // Mock the btrCore_GetDeviceInfo function to return success
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);

    // Iterate over all possible values of the aenBTRCoreLeProp enumeration
    for (enBTRCoreLeProp prop = enBTRCoreLePropGUUID; prop <= enBTRCoreLePropGChar; prop++) {
        ret = BTRCore_GetLEProperty(&btrCoreHdl, 0, uuid, prop, propValue);
        TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
    }

    // Test the default case where aenBTRCoreLeProp is enBTRCoreLePropUnknown
    ret = BTRCore_GetLEProperty(&btrCoreHdl, 0, uuid, enBTRCoreLePropUnknown, propValue);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
}

void test_btrCore_BTDeviceStatusUpdateCb_AllCases2(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    enBTDeviceType deviceType;
    enBTDeviceState deviceState;
    int ret;

    // Initialize the structures
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    btrCoreHdl.numOfPairedDevices=2;
    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; 

//     // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer  
//     // Test case for enBTDevStScanInProgress
    deviceState = enBTDevStScanInProgress;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);


    // Test case for enBTDevStPairingRequest
    deviceState = enBTDevStPairingRequest;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for enBTDevStPairingInProgress
    deviceState = enBTDevStPairingInProgress;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for enBTDevStPaired
    deviceState = enBTDevStPaired;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for enBTDevStUnPaired
    deviceState = enBTDevStUnPaired;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for enBTDevStConnectInProgress
    deviceState = enBTDevStConnectInProgress;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for enBTDevStConnected
    deviceState = enBTDevStConnected;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for enBTDevStDisconnected
    deviceState = enBTDevStDisconnected;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for enBTDevStUnknown
    deviceState = enBTDevStUnknown;
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, deviceState, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

}
void test_btrCore_BTDeviceStatusUpdateCb_InvalidArguments(void) {
    int ret;

    // Test case for invalid arguments
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStUnknown, NULL, NULL);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for invalid arguments passed!\n");
}
void test_btrCore_BTDeviceStatusUpdateCb_DeviceCreated(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Test case for enBTDevStCreated
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStCreated, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device created passed!\n");
}
void test_btrCore_BTDeviceStatusUpdateCb_DeviceFound(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class
     // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Test case for enBTDevStFound
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStFound, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device found passed!\n");
}

void test_btrCore_BTDeviceStatusUpdateCb_DeviceLost(void) {
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    stBTDeviceInfo* deviceInfo= (stBTDeviceInfo*)malloc(sizeof(stBTDeviceInfo));;
    int ret;

    // Set up the known device
    btrCoreHdl->numOfPairedDevices = 1;
    btrCoreHdl->stKnownDevicesArr[0].tDeviceId = 73588229205;
    strncpy(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55", sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1);
    btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress[sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1] = '\0';

    // Set up the device info
    deviceInfo->ui32Class = 0x1F00; // Example class
    strcpy(deviceInfo->pcAddress, "00:11:22:33:44:55");
    deviceInfo->pcAddress[sizeof(deviceInfo->pcAddress) - 1] = '\0';
    strncpy(deviceInfo->pcName, "Test Device", sizeof(deviceInfo->pcName) - 1);
    deviceInfo->pcName[sizeof(deviceInfo->pcName) - 1] = '\0';
    strncpy(deviceInfo->pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", sizeof(deviceInfo->pcDevicePath) - 1);
    deviceInfo->pcDevicePath[sizeof(deviceInfo->pcDevicePath) - 1] = '\0';

    // Initialize the queue out task pointer
    btrCoreHdl->pGAQueueOutTask = NULL; // Mock pointer
    

    // Test case for enBTDevStLost
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStLost, deviceInfo, btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device lost passed!\n");
    free(btrCoreHdl);
    free(deviceInfo);
}


void test_btrCore_BTDeviceStatusUpdateCb_enBTDevStPropChanged(void) {
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    stBTDeviceInfo* deviceInfo= (stBTDeviceInfo*)malloc(sizeof(stBTDeviceInfo));;
    int ret;

    // Set up the known device
    btrCoreHdl->numOfPairedDevices = 1;
    btrCoreHdl->stKnownDevicesArr[0].tDeviceId = 73588229205;
    strncpy(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55", sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1);
    btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress[sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1] = '\0';

    // Set up the device info
    deviceInfo->ui32Class = 0x1F00; // Example class
    strcpy(deviceInfo->pcAddress, "00:11:22:33:44:55");
    deviceInfo->pcAddress[sizeof(deviceInfo->pcAddress) - 1] = '\0';
    strncpy(deviceInfo->pcName, "Test Device", sizeof(deviceInfo->pcName) - 1);
    deviceInfo->pcName[sizeof(deviceInfo->pcName) - 1] = '\0';
    strncpy(deviceInfo->pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", sizeof(deviceInfo->pcDevicePath) - 1);
    deviceInfo->pcDevicePath[sizeof(deviceInfo->pcDevicePath) - 1] = '\0';

    // Initialize the queue out task pointer
    btrCoreHdl->pGAQueueOutTask = NULL; // Mock pointer
    

    // Test case for enBTDevStLost
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStPropChanged, deviceInfo, btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device lost passed!\n");
    free(btrCoreHdl);
    free(deviceInfo);
}


void test_btrCore_BTDeviceStatusUpdateCb_DeviceRSSIUpdate(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Test case for enBTDevStRSSIUpdate
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStRSSIUpdate, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device RSSI update passed!\n");
}

void test_btrCore_BTDeviceConnectionIntimationCb_BasicInitialization(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Call the function
    ret = btrCore_BTDeviceConnectionIntimationCb(enBTRCoreMobileAudioIn, &deviceInfo, 123456, 1, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for basic initialization passed!\n");
}
void test_btrCore_BTDeviceConnectionIntimationCb_Speakers(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the known device
    btrCoreHdl.numOfPairedDevices = 1;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId = 1;
    strncpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, "Known Device", sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1);
    btrCoreHdl.stKnownDevicesArr[0].pcDeviceName[sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class
    strncpy(deviceInfo.pcAddress, "00:11:22:33:44:55", sizeof(deviceInfo.pcAddress) - 1);
    deviceInfo.pcAddress[sizeof(deviceInfo.pcAddress) - 1] = '\0';
    strncpy(deviceInfo.pcName, "Test Device", sizeof(deviceInfo.pcName) - 1);
    deviceInfo.pcName[sizeof(deviceInfo.pcName) - 1] = '\0';
    strncpy(deviceInfo.pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", sizeof(deviceInfo.pcDevicePath) - 1);
    deviceInfo.pcDevicePath[sizeof(deviceInfo.pcDevicePath) - 1] = '\0';

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Call the function with Speakers device type
    ret = btrCore_BTDeviceConnectionIntimationCb(enBTDevAudioSink, &deviceInfo, 123456, 1, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for Speakers device type passed!\n");
}
void test_btrCore_BTDeviceConnectionIntimationCb_HID(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Call the function with HID device type
    ret = btrCore_BTDeviceConnectionIntimationCb(enBTDevAudioSource, &deviceInfo, 123456, 1, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for HID device type passed!\n");
}

void test_btrCore_BTDeviceAuthenticationCb_BasicInitialization(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x400u; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Call the function
    ret = btrCore_BTDeviceAuthenticationCb(enBTDevUnknown, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for basic initialization passed!\n");
}
enBTRCoreRet myConnAuthCallback(stBTRCoreConnCBInfo* apstConnCbInfo, int* api32ConnInAuthResp, void* apvUserData) {
    // Implement your callback logic here
    *api32ConnInAuthResp = 1; // Example response
    return enBTRCoreSuccess;
}
void test_btrCore_BTDeviceAuthenticationCb_Speakers(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the known device
    btrCoreHdl.numOfPairedDevices = 1;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId = 1;
   // strncpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, "Known Device", sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1);
    btrCoreHdl.stKnownDevicesArr[0].pcDeviceName[sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Set up the device info
    deviceInfo.ui32Class = 0x400000u; // Example class
  //  strncpy(deviceInfo.pcAddress, "00:11:22:33:44:55", sizeof(deviceInfo.pcAddress) - 1);
    deviceInfo.pcAddress[sizeof(deviceInfo.pcAddress) - 1] = '\0';
  //  strncpy(deviceInfo.pcName, "Test Device", sizeof(deviceInfo.pcName) - 1);
    deviceInfo.pcName[sizeof(deviceInfo.pcName) - 1] = '\0';
   // strncpy(deviceInfo.pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", sizeof(deviceInfo.pcDevicePath) - 1);
    deviceInfo.pcDevicePath[sizeof(deviceInfo.pcDevicePath) - 1] = '\0';

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Call the function with Speakers device type
    ret = btrCore_BTDeviceAuthenticationCb(enBTDevUnknown, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for Speakers device type passed!\n");
}
void test_btrCore_BTDeviceAuthenticationCb_HID(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x200000u; // Example class
  //  strncpy(deviceInfo.pcName, "Xbox Controller", sizeof(deviceInfo.pcName) - 1);
    deviceInfo.pcName[sizeof(deviceInfo.pcName) - 1] = '\0';

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Call the function with HID device type
    ret = btrCore_BTDeviceAuthenticationCb(enBTDevUnknown, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for HID device type passed!\n");
}
void test_btrCore_BTDeviceAuthenticationCb_MobileAudioIn(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x80000u; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer
    btrCoreHdl.fpcBBTRCoreConnAuth =myConnAuthCallback;
    // Call the function with Mobile Audio In device type
    ret = btrCore_BTDeviceAuthenticationCb(enBTDevAudioSource, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(1, ret);

    printf("Test case for Mobile Audio In device type passed!\n");
}

void test_btrCore_BTDeviceAuthenticationCb_speakers(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x40000u; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer
    btrCoreHdl.fpcBBTRCoreConnAuth =myConnAuthCallback;
    // Call the function with Mobile Audio In device type
    ret = btrCore_BTDeviceAuthenticationCb(enBTDevHID, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(1, ret);

    printf("Test case for Mobile Audio In device type passed!\n");
}
void test_btrCore_BTDeviceConnectionIntimationCb_Speakers1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the known device
    btrCoreHdl.numOfPairedDevices = 1;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId = 1;
    strncpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, "Known Device", sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1);
    btrCoreHdl.stKnownDevicesArr[0].pcDeviceName[sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Set up the callback
    btrCoreHdl.fpcBBTRCoreConnAuth = myConnAuthCallback;

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Define a set of class IDs to test
    unsigned int classIds[] = {
        enBTRCore_DC_Tile,
        enBTRCore_DC_HID_Keyboard,
        enBTRCore_DC_HID_Mouse,
        enBTRCore_DC_HID_MouseKeyBoard,
        enBTRCore_DC_HID_AudioRemote,
        enBTRCore_DC_HID_Joystick,
        enBTRCore_DC_HID_GamePad,
        // Add other class IDs as needed
    };

    // Iterate over the class IDs and test each one
    for (size_t i = 0; i < sizeof(classIds) / sizeof(classIds[0]); i++) {
        // Set up the device info with the current class ID
        deviceInfo.ui32Class = classIds[i];
        strncpy(deviceInfo.pcAddress, "00:11:22:33:44:55", sizeof(deviceInfo.pcAddress) - 1);
        deviceInfo.pcAddress[sizeof(deviceInfo.pcAddress) - 1] = '\0';
        strncpy(deviceInfo.pcName, "Test Device", sizeof(deviceInfo.pcName) - 1);
        deviceInfo.pcName[sizeof(deviceInfo.pcName) - 1] = '\0';
        strncpy(deviceInfo.pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", sizeof(deviceInfo.pcDevicePath) - 1);
        deviceInfo.pcDevicePath[sizeof(deviceInfo.pcDevicePath) - 1] = '\0';

        // Call the function with Speakers device type
        ret = btrCore_BTDeviceConnectionIntimationCb(enBTDevAudioSink, &deviceInfo, 123456, 1, &btrCoreHdl);
        TEST_ASSERT_EQUAL(0, ret);

        printf("Test case for class ID 0x%x passed!\n", classIds[i]);
    }
}
void test_BTRCore_DisconnectDevice_NotInitialized(void) {
    enBTRCoreRet ret;

    // Test case for uninitialized BTRCore handle
    ret = BTRCore_DisconnectDevice(NULL, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}

void test_BTRCore_DisconnectDevice_GetDeviceInfoFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    // Test case for getting device info failure
    ret = BTRCore_DisconnectDevice(&btrCoreHdl, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_DisconnectDevice_DisconnectDeviceFailure(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    // Mock the BtrCore_BTDisconnectDevice function to return failure
    BtrCore_BTDisconnectDevice_StubWithCallback(mock_BtrCore_BTDisconnectDevice_fail);
    
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    // Test case for disconnect device failure
    ret = BTRCore_DisconnectDevice(&btrCoreHdl, 0, enBTRCoreUnknown);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_DisconnectDevice_FAIL(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Xbox Controller";

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    btrCoreHdl.numOfPairedDevices = 1;
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath, devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, deviceName);

    // Mock the BtrCore_BTDisconnectDevice function to return success
    BtrCore_BTDisconnectDevice_StubWithCallback(mock_BtrCore_BTDisconnectDevice_fail);

    ret = BTRCore_DisconnectDevice(&btrCoreHdl, 0, enBTRCoreUnknown);
    
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_DisconnectDevice_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Xbox Controller";

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    btrCoreHdl.numOfPairedDevices = 1;
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath, devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, deviceName);

    // Mock the BtrCore_BTDisconnectDevice function to return success
    BtrCore_BTDisconnectDevice_StubWithCallback(mock_BtrCore_BTDisconnectDevice);

    // Mock the BtrCore_BTEnableEnhancedRetransmissionMode function to return success
    BtrCore_BTEnableEnhancedRetransmissionMode_StubWithCallback(mock_BtrCore_BTEnableEnhancedRetransmissionMode_fail);

    // Test case for successful disconnection
    ret = BTRCore_DisconnectDevice(&btrCoreHdl, 0, enBTDevHID);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_newBatteryLevelDevice_InvalidArg(void) {
    enBTRCoreRet ret;

    // Test case for invalid argument (NULL handle)
    ret = BTRCore_newBatteryLevelDevice(NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}
void test_btrCore_BTDeviceStatusUpdateCb_For_Loop(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));
    unsigned int classIds[] = {
        enBTRCore_DC_Tile,
        enBTRCore_DC_HID_Keyboard,
        enBTRCore_DC_HID_Mouse,
        enBTRCore_DC_HID_MouseKeyBoard,
        enBTRCore_DC_HID_AudioRemote,
        enBTRCore_DC_HID_Joystick,
        enBTRCore_DC_HID_GamePad,
        // Add other class IDs as needed
    };

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer
    for (size_t i = 0; i < sizeof(classIds) / sizeof(classIds[0]); i++) {
        // Set up the device info with the current class ID
        deviceInfo.ui32Class = classIds[i];
      
        // Call the function with Speakers device type
        ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStPropChanged, &deviceInfo, &btrCoreHdl);
        TEST_ASSERT_EQUAL(0, ret);

    }
}

void test_btrCore_BTDeviceAuthenticationCb_BasicInitialization_loop1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;
    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x400u; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Set up the callback
    btrCoreHdl.fpcBBTRCoreConnAuth = myConnAuthCallback;

    // Set up the paired devices
    btrCoreHdl.numOfPairedDevices = 1;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId = 1;
    strncpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, "Known Device", sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1);
    btrCoreHdl.stKnownDevicesArr[0].pcDeviceName[sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Call the function
    ret = btrCore_BTDeviceAuthenticationCb(enBTDevAudioSource, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(1, ret);

    printf("Test case for basic initialization passed!\n");
}
void test_btrCore_BTDeviceAuthenticationCb_BasicInitialization_loop2(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Set up the callback
    btrCoreHdl.fpcBBTRCoreConnAuth = myConnAuthCallback;

    // Set up the paired devices
    btrCoreHdl.numOfPairedDevices = 1;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId = 1;
    strncpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, "Xbox", sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1);
    btrCoreHdl.stKnownDevicesArr[0].pcDeviceName[sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Define a set of class IDs to test
    unsigned int classIds[] = {
        0x40000u,  // Rendering Class of Service
        0x80000u,  // Capturing Service
        0x200000u, // Audio Class of Service
        0x400000u  // Telephony Class of Service
    };

    // Loop through each class ID and call the function
    for (int i = 0; i < sizeof(classIds) / sizeof(classIds[0]); i++) {
        deviceInfo.ui32Class = classIds[i];
        ret = btrCore_BTDeviceAuthenticationCb(enBTDevHFPHeadset, &deviceInfo, &btrCoreHdl);
        TEST_ASSERT_EQUAL(1, ret);
        printf("Test case for class ID 0x%x passed!\n", classIds[i]);
    }
}

void test_btrCore_BTDeviceAuthenticationCb_BasicInitialization_loop3(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    // Set up the callback
    btrCoreHdl.fpcBBTRCoreConnAuth = myConnAuthCallback;

    // Set up the paired devices
    btrCoreHdl.numOfPairedDevices = 1;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId = 1;
    strncpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, "Known Device", sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1);
    btrCoreHdl.stKnownDevicesArr[0].pcDeviceName[sizeof(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName) - 1] = '\0';

    // Define a set of class IDs to test
    unsigned int classIds[] = {
        0x400u,    // Tablet
        0x200u,    // SmartPhone
        0x100u,    // WearableHeadset
        0x40000u,  // Rendering Class of Service
        0x80000u,  // Capturing Service
        0x200000u, // Audio Class of Service
        0x400000u  // Telephony Class of Service
    };

    // Loop through each class ID and call the function
    for (int i = 0; i < sizeof(classIds) / sizeof(classIds[0]); i++) {
        deviceInfo.ui32Class = classIds[i];
        ret = btrCore_BTDeviceAuthenticationCb(enBTDevAudioSink, &deviceInfo, &btrCoreHdl);
        TEST_ASSERT_EQUAL(1, ret);
        printf("Test case for class ID 0x%x passed!\n", classIds[i]);
    }
}

void test_btrCore_BTMediaStatusUpdateCb(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreAVMediaStatusUpdate mediaStatusUpdate;
    char* btdevAddr = "00:11:22:33:44:55";
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&mediaStatusUpdate, 0, sizeof(stBTRCoreAVMediaStatusUpdate));
    btrCoreHdl.numOfPairedDevices=2;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId=73588229205;
    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask =g_async_queue_new();//(pGAQueueOutTask*)malloc(pGAQueueOutTask); // Mock pointer
    mediaStatusUpdate.eAVMediaState = eBTRCoreAVMediaTrkStPaused;
        // Call the function with the current media state
    ret = btrCore_BTMediaStatusUpdateCb(&mediaStatusUpdate, btdevAddr, &btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Define a set of media states to test
    eBTRCoreAVMediaStatusUpdate mediaStates[] = {
        eBTRCoreAVMediaTrkStStarted,
        eBTRCoreAVMediaTrkStPlaying,
        eBTRCoreAVMediaTrkStPaused,
        eBTRCoreAVMediaTrkStStopped,
        eBTRCoreAVMediaTrkStChanged,
        eBTRCoreAVMediaTrkPosition,
        eBTRCoreAVMediaPlaybackEnded,
        eBTRCoreAVMediaPlyrName,
        eBTRCoreAVMediaPlyrEqlzrStOff,
        eBTRCoreAVMediaPlyrEqlzrStOn,
        eBTRCoreAVMediaPlyrShflStOff,
        eBTRCoreAVMediaPlyrShflStAllTracks,
        eBTRCoreAVMediaPlyrShflStGroup,
        eBTRCoreAVMediaPlyrRptStOff,
        eBTRCoreAVMediaPlyrRptStSingleTrack,
        eBTRCoreAVMediaPlyrRptStAllTracks,
        eBTRCoreAVMediaPlyrRptStGroup,
        eBTRCoreAVMediaPlyrVolume,
        eBTRCoreAVMediaPlyrDelay,
        eBTRCoreAVMediaElementAdded,
        eBTRCoreAVMediaElementRemoved
    };

    // Iterate over the media states and test each one
    for (size_t i = 0; i < sizeof(mediaStates) / sizeof(mediaStates[0]); i++) {
        // Set up the media status update with the current media state
        mediaStatusUpdate.eAVMediaState = mediaStates[i];
        // Call the function with the current media state
        ret = btrCore_BTMediaStatusUpdateCb(&mediaStatusUpdate, btdevAddr, &btrCoreHdl);
        TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

        printf("Test case for media state %d passed!\n", mediaStates[i]);
    }
    // 73588229205
}
void test_btrCore_BTMediaStatusUpdateCb_NULL(void) {
    enBTRCoreRet ret;
    ret = btrCore_BTMediaStatusUpdateCb(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_btrCore_BTLeStatusUpdateCb(void) {
    stBTRCoreHdl btrCoreHdl=btrCoreHdl;
    stBTRCoreLeGattInfo leGattInfo;
    char* btDevAddr = "00:11:22:33:44:55";
    enBTRCoreRet ret;

   
    char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&leGattInfo, 0, sizeof(stBTRCoreLeGattInfo));

    // Set up the LE GATT info
    leGattInfo.enLeOper = enBTRCoreLEGOpReadValue;
    // Set up the scanned device
    btrCoreHdl.numOfScannedDevices = 1;
    btrCoreHdl.numOfPairedDevices=2;
    btrCoreHdl.stScannedDevicesArr[0].tDeviceId=73588229205;
    btrCoreHdl.stScannedDevicesArr[0].ui32DevClassBtSpec = 0x40000;
    btrCoreHdl.stScannedDevicesArr[0].ui16DevAppearanceBleSpec = 0x1234;
    btrCoreHdl.stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_XBB;
    btrCoreHdl.stScannedDevicesArr[0].stDeviceProfile.numberOfService = 1;
    btrCoreHdl.stScannedDevicesArr[0].stDeviceProfile.profile[0].uuid_value = strtol(BTR_CORE_GATT_XBB_1, NULL, 16);

    // Initialize the scanned device state info array
    btrCoreHdl.stScannedDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStOpReady;
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath,devicePath);
    btrCoreHdl.fpcBBTRCoreStatus=myConnAuthCallback;

    // Call the function
    ret = btrCore_BTLeStatusUpdateCb(&leGattInfo, btDevAddr, &btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreDeviceNotFound, ret);
    //need to do proper Initializations

    
}
void test_btrCore_BTDeviceStatusUpdateCb_DeviceLost_fail(void) {
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    stBTDeviceInfo* deviceInfo= (stBTDeviceInfo*)malloc(sizeof(stBTDeviceInfo));;
    int ret;

    // Set up the known device
    // btrCoreHdl->numOfPairedDevices = 0;
    btrCoreHdl->numOfScannedDevices=2;
    btrCoreHdl->stScannedDevicesArr[0].tDeviceId=73588229205;
    // btrCoreHdl->stKnownDevicesArr[0].tDeviceId = 73588229205;
    strncpy(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55", sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1);
    btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress[sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1] = '\0';

    // Set up the device info
    deviceInfo->ui32Class = 0x1F00; // Example class
    strcpy(deviceInfo->pcAddress, "00:11:22:33:44:55");
    deviceInfo->pcAddress[sizeof(deviceInfo->pcAddress) - 1] = '\0';
    strncpy(deviceInfo->pcName, "Test Device", sizeof(deviceInfo->pcName) - 1);
    deviceInfo->pcName[sizeof(deviceInfo->pcName) - 1] = '\0';
    strncpy(deviceInfo->pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", sizeof(deviceInfo->pcDevicePath) - 1);
    deviceInfo->pcDevicePath[sizeof(deviceInfo->pcDevicePath) - 1] = '\0';

    // Initialize the queue out task pointer
    btrCoreHdl->pGAQueueOutTask = NULL; // Mock pointer
    // Test case for enBTDevStLost
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStLost, deviceInfo, btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device lost passed!\n");
    free(btrCoreHdl);
    free(deviceInfo);
}

void test_btrCore_BTDeviceStatusUpdateCb_enBTDevStPropChanged1(void) {
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    stBTDeviceInfo* deviceInfo= (stBTDeviceInfo*)malloc(sizeof(stBTDeviceInfo));;
    int ret;

    // Set up the known device
  //  btrCoreHdl->numOfPairedDevices = 1;

    btrCoreHdl->numOfScannedDevices=2;
    btrCoreHdl->stScannedDevicesArr[0].tDeviceId=73588229205;
    strncpy(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55", sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1);
    btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress[sizeof(btrCoreHdl->stKnownDevicesArr[0].pcDeviceAddress) - 1] = '\0';

    // Set up the device info
    deviceInfo->ui32Class = 0x1F00; // Example class
    strcpy(deviceInfo->pcAddress, "00:11:22:33:44:55");
    deviceInfo->pcAddress[sizeof(deviceInfo->pcAddress) - 1] = '\0';
    strncpy(deviceInfo->pcName, "Test Device", sizeof(deviceInfo->pcName) - 1);
    deviceInfo->pcName[sizeof(deviceInfo->pcName) - 1] = '\0';
    strncpy(deviceInfo->pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", sizeof(deviceInfo->pcDevicePath) - 1);
    deviceInfo->pcDevicePath[sizeof(deviceInfo->pcDevicePath) - 1] = '\0';

    // Initialize the queue out task pointer
    btrCoreHdl->pGAQueueOutTask = NULL; // Mock pointer
    

    // Test case for enBTDevStLost
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevUnknown, enBTDevStPropChanged, deviceInfo, btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device lost passed!\n");
    free(btrCoreHdl);
    free(deviceInfo);
}

void test_btrCore_BTDeviceStatusUpdateCb_DeviceFound1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class
    // Test case for enBTDevStFound
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevAudioSink, enBTDevStFound, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for device found passed!\n");
}
void test_btrCore_BTLeStatusUpdateCb_InvalidArg(void){
    int ret;
    ret = btrCore_BTLeStatusUpdateCb(0, NULL, NULL);
    TEST_ASSERT_EQUAL(7, ret);
}
void test_btrCore_BTAdapterStatusUpdateCb_InvalidArguments(void) {
    int ret;

    // Test case for invalid arguments
    ret = btrCore_BTAdapterStatusUpdateCb(enBTAdPropUnknown, NULL, NULL);
    TEST_ASSERT_EQUAL(-1, ret);

    printf("Test case for invalid arguments passed!\n");
}

void test_btrCore_BTAdapterStatusUpdateCb_NonCurrentAdapterPath(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTAdapterInfo adapterInfo;
    int ret;
    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&adapterInfo, 0, sizeof(stBTAdapterInfo));
    ret = btrCore_BTAdapterStatusUpdateCb(enBTAdPropUnknown, &adapterInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(-1, ret);

}

void test_BTRCore_newBatteryLevelDevice_break(void) {
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    // Test case when batteryLevelThread is NULL
    btrCoreHdl->batteryLevelThread = NULL;
    btrCoreHdl->batteryLevelThreadExit=1;
    enBTRCoreRet ret = BTRCore_newBatteryLevelDevice((tBTRCoreHandle)btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_NOT_NULL(btrCoreHdl->batteryLevelThread);
    g_thread_unref(btrCoreHdl->batteryLevelThread);
    free(btrCoreHdl);
}
void test_BTRCore_newBatteryLevelDevice(void) {
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    // Test case when batteryLevelThread is NULL
    btrCoreHdl->batteryLevelThread = NULL;
    //btrCoreHdl->batteryLevelThreadExit=1;
    enBTRCoreRet ret = BTRCore_newBatteryLevelDevice((tBTRCoreHandle)btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_NOT_NULL(btrCoreHdl->batteryLevelThread);

    // Test case when batteryLevelThread is not NULL
    ret = BTRCore_newBatteryLevelDevice((tBTRCoreHandle)btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Clean up
    g_thread_unref(btrCoreHdl->batteryLevelThread);
    free(btrCoreHdl);
}


unsigned int sleep(unsigned int seconds) {
    // Do nothing
    return 0;
}

void test_BTRCore_newBatteryLevelDevice1(void) {
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    btrCoreHdl->batteryLevelThreadExit = 0;
    g_mutex_init(&btrCoreHdl->batteryLevelMutex);
    g_cond_init(&btrCoreHdl->batteryLevelCond);

    // Mock the function to update battery levels
   // btrCore_updateBatteryLevelsForConnectedDevices = mock_btrCore_updateBatteryLevelsForConnectedDevices;

    // Test case when batteryLevelThread is NULL
    btrCoreHdl->batteryLevelThread = NULL;
    enBTRCoreRet ret = BTRCore_newBatteryLevelDevice((tBTRCoreHandle)btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    // Test case when batteryLevelThread is not NULL
    ret = BTRCore_newBatteryLevelDevice((tBTRCoreHandle)btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Clean up
    // g_thread_unref(btrCoreHdl->batteryLevelThread);
    g_mutex_clear(&btrCoreHdl->batteryLevelMutex);
    g_cond_clear(&btrCoreHdl->batteryLevelCond);
    free(btrCoreHdl);
}

int mock_BtrCore_BTGetPairedDeviceInfo1(void* apstBtIfceHdl, const char* apcDevPath, stBTPairedDeviceInfo* apstFoundDevice) {
    apstFoundDevice->numberOfDevices=2;
    
    // strncpy(apstFoundDevice->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(apstFoundDevice->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
   strncpy( apstFoundDevice->devicePath[0], "/device/path", sizeof(apstFoundDevice->devicePath[0]));  // Initialize device path
   
    return 0;
}
void test_BTRCore_GetDeviceMediaInfo_ValidInputs_Success(void) {
    stBTRCoreDevMediaInfo mediaInfo;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreAVMediaInfo avMediaInfo;
    
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    if (!hBTRCore) {
        BTRCORELOG_ERROR("Memory allocation failed for hBTRCore\n");
        return;
    }
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    // Initialize mediaInfo and allocate memory for the nested pstBtrCoreDevMCodecInfo
    memset(&mediaInfo, 0, sizeof(stBTRCoreDevMediaInfo));
    mediaInfo.pstBtrCoreDevMCodecInfo = malloc(sizeof(stBTRCoreDevMediaSbcInfo)); // Adjust size based on actual use case
    
    memset(mediaInfo.pstBtrCoreDevMCodecInfo, 0, sizeof(stBTRCoreDevMediaSbcInfo));
    
    // Initialize the structure members
    hBTRCore->connHdl = (void*)1; 
    hBTRCore->curAdapterPath = "/test/path";
    
    // Sample details for the 1st Known Device 
    hBTRCore->stKnownDevicesArr[0].tDeviceId = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = TRUE;  // Assuming this means connected
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "device0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress, "address0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(hBTRCore->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
    hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStConnected;

    // hBTRCore->numOfPairedDevices = 1; 
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo1);

    // Mocking BTRCore_AVMedia_GetCurMediaInfo
    BTRCore_AVMedia_GetCurMediaInfo_IgnoreAndReturn(enBTRCoreSuccess);

    enBTRCoreRet ret = BTRCore_GetDeviceMediaInfo(hBTRCore, 0, enBTRCoreUnknown, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Clean up
    free(mediaInfo.pstBtrCoreDevMCodecInfo);
    free(hBTRCore);
}
void test_BTRCore_GetDeviceMediaInfo_ValidInputs_Success1(void) {
    stBTRCoreDevMediaInfo mediaInfo;
    stBTRCoreBTDevice knownDevice;
    stBTRCoreAVMediaInfo avMediaInfo;
    
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    if (!hBTRCore) {
        BTRCORELOG_ERROR("Memory allocation failed for hBTRCore\n");
        return;
    }
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    // Initialize mediaInfo and allocate memory for the nested pstBtrCoreDevMCodecInfo
    memset(&mediaInfo, 0, sizeof(stBTRCoreDevMediaInfo));
    mediaInfo.pstBtrCoreDevMCodecInfo = malloc(sizeof(stBTRCoreDevMediaSbcInfo)); // Adjust size based on actual use case
    
    memset(mediaInfo.pstBtrCoreDevMCodecInfo, 0, sizeof(stBTRCoreDevMediaSbcInfo));
    
    // Initialize the structure members
    hBTRCore->connHdl = (void*)1; 
    hBTRCore->curAdapterPath = "/test/path";
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(hBTRCore->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
    hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStConnected;

    hBTRCore->numOfPairedDevices = 1; 

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo1);

    // Mocking BTRCore_AVMedia_GetCurMediaInfo
    BTRCore_AVMedia_GetCurMediaInfo_IgnoreAndReturn(enBTRCoreFailure);
    enBTDeviceType deviceTypes[] = {
        enBTDevAudioSink,
        enBTDevAudioSource,
        enBTDevHFPHeadset,
        enBTDevHFPAudioGateway,
        enBTDevLE,
        enBTDevUnknown
    };
    int numDeviceTypes = sizeof(deviceTypes) / sizeof(deviceTypes[0]);

    enBTRCoreRet ret = BTRCore_GetDeviceMediaInfo(hBTRCore, 0, enBTRCoreUnknown, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
    
    ret = BTRCore_GetDeviceMediaInfo(hBTRCore, 0, enBTRCoreHeadSet, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);


    ret = BTRCore_GetDeviceMediaInfo(hBTRCore, 0, enBTRCoreHID, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
   
    ret = BTRCore_GetDeviceMediaInfo(hBTRCore, 0, enBTRCorePCAudioIn, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret); 

    ret = BTRCore_GetDeviceMediaInfo(hBTRCore, 0, enBTRCoreLE, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret); 

    // Clean up
    free(mediaInfo.pstBtrCoreDevMCodecInfo);
    free(hBTRCore);
}

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
    char *                                  pDBusConn;

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

    char                                    GattCharDevPath[BT_MAX_NUM_GATT_CHAR][BT_MAX_STR_LEN];
    char                                    GattSerDevPath[BT_MAX_NUM_GATT_SERVICE][BT_MAX_STR_LEN];
    char                                    GattDesDevPath[BT_MAX_NUM_GATT_DESC][BT_MAX_STR_LEN];
    char                                    GattCharNotifyPath[BT_MAX_NOTIFY_CHAR_PATH][BT_MAX_STR_LEN];

    int                                     DevCount;
    int                                     GattCharDevCount;
    int                                     GattSerDevCount;
    int                                     GattDesDevCount;
    int                                     GattCharNotifyDevCount;
    FILE                                    *BatteryFirmFilep;


} stBtIfceHdl;

void* mock_BtrCore_BTInitGetConnection(
    void
) {
    stBtIfceHdl* pstlhBtIfce= NULL;
    pstlhBtIfce = (stBtIfceHdl*)malloc(sizeof(stBtIfceHdl));
    if (!pstlhBtIfce)
        return NULL;

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
    strncpy(pstlhBtIfce->pcDeviceCurrState,   "disconnected", BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcLeDeviceCurrState, "disconnected", BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcLeDeviceAddress,   "none",         BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcMediaCurrState,    "none",         BT_MAX_STR_LEN - 1); 

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

    return (void*)pstlhBtIfce;
}

char*
mock_BtrCore_BTGetAgentPath (
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

char*
mock_BtrCore_BTGetAgentPath_Fail (
    void* apstBtIfceHdl
) {
   
    return NULL;
}

void test_BTRCore_Init_should_ReturnFailure_when_InitializationFails2(void)
{
    tBTRCoreHandle hBTRCore;
    stBTRCoreHdl handle_data;
    enBTRCoreRet expected_result = enBTRCoreInitFailure;
    enBTRCoreRet actual_result;
    
    BtrCore_BTInitGetConnection_StubWithCallback(mock_BtrCore_BTInitGetConnection);

    BtrCore_BTGetAgentPath_IgnoreAndReturn(NULL);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(NULL);
    actual_result = BTRCore_Init(&hBTRCore);

    TEST_ASSERT_EQUAL(expected_result, actual_result);
}

void test_BTRCore_Init_should_ReturnFailure_when_InitializationFails3(void)
{
    tBTRCoreHandle hBTRCore;
    stBTRCoreHdl handle_data;
    enBTRCoreRet expected_result = enBTRCoreInitFailure;
    enBTRCoreRet actual_result;
    
    BtrCore_BTInitGetConnection_StubWithCallback(mock_BtrCore_BTInitGetConnection);


    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTGetAdapterPath_IgnoreAndReturn(NULL);
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(NULL);
    BtrCore_BTReleaseAgentPath_IgnoreAndReturn(NULL);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(NULL);
    actual_result = BTRCore_Init(&hBTRCore);

    TEST_ASSERT_EQUAL(enBTRCoreInitFailure, actual_result);
  
}

void test_BTRCore_Init_ValidParameters4(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");

    BtrCore_BTRegisterAdapterStatusUpdateCb_IgnoreAndReturn(-1);
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreFailure);
    BtrCore_BTReleaseAdapterPath_IgnoreAndReturn(NULL);
    BtrCore_BTReleaseAgentPath_IgnoreAndReturn(NULL);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(-1);
    // Act
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}    

int
mock_BtrCore_BTRegisterConnIntimationCb (
    void*                       apBtConn,
    fPtr_BtrCore_BTConnIntimCb  afpcBConnIntim,
    void*                       apUserData
) {
    return 0;
}

int
mock_BtrCore_BTRegisterConnIntimationCb_fail (
    void*                       apBtConn,
    fPtr_BtrCore_BTConnIntimCb  afpcBConnIntim,
    void*                       apUserData
) {
    
    return -1;
}


void test_BTRCore_Init_ValidParameters5(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp_fail);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreFailure);
    BtrCore_BTReleaseAdapterPath_IgnoreAndReturn(1);
    BtrCore_BTReleaseAgentPath_IgnoreAndReturn(1);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(1);
    // Act
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}    

int
mock_BtrCore_BTRegisterAdapterStatusUpdateCb (
    void*                                   apstBtIfceHdl,
    fPtr_BtrCore_BTAdapterStatusUpdateCb    afpcBAdapterStatusUpdate,
    void*                                   apUserData
) {
    return 0;
}

int
mock_BtrCore_BTRegisterAdapterStatusUpdateCb_Fail (
    void*                                   apstBtIfceHdl,
    fPtr_BtrCore_BTAdapterStatusUpdateCb    afpcBAdapterStatusUpdate,
    void*                                   apUserData
) {
    return 1;
}

int
mock_BtrCore_BTReleaseAdapterPath (
    void*       apstBtIfceHdl,
    const char* apBtAdapter
) {

    return 0;
}


int
mock_BtrCore_BTReleaseAdapterPath_fail (
    void*       apstBtIfceHdl,
    const char* apBtAdapter
) {
        return -1;
}


int
mock_BtrCore_BTReleaseAgentPath (
    void* apstBtIfceHdl
) {
    

    return 0;
}


void test_BTRCore_Init_ValidParameters6(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterAdapterStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb_Fail);
    BtrCore_BTRegisterDevStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb_Fail);
    BtrCore_BTReleaseAdapterPath_StubWithCallback(mock_BtrCore_BTReleaseAdapterPath);
    BtrCore_BTReleaseAgentPath_StubWithCallback(mock_BtrCore_BTReleaseAgentPath);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(0);
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}

void test_BTRCore_Init_ValidParameters7(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterAdapterStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);
    BtrCore_BTRegisterDevStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);

    BtrCore_BTRegisterConnIntimationCb_StubWithCallback(mock_BtrCore_BTRegisterConnIntimationCb_fail);
    BtrCore_BTReleaseAdapterPath_StubWithCallback(mock_BtrCore_BTReleaseAdapterPath);
    BtrCore_BTReleaseAgentPath_StubWithCallback(mock_BtrCore_BTReleaseAgentPath);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(0);
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}

int
mock_BtrCore_BTRegisterConnAuthCb (
    void*                       apBtConn,
    fPtr_BtrCore_BTConnAuthCb   afpcBConnAuth,
    void*                       apUserData
) {
    
    return 0;
}
int
mock_BtrCore_BTRegisterConnAuthCb_fail (
    void*                       apBtConn,
    fPtr_BtrCore_BTConnAuthCb   afpcBConnAuth,
    void*                       apUserData
) {
    
    return -1;
}
void test_BTRCore_Init_ValidParameters8(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterAdapterStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);
    BtrCore_BTRegisterDevStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);

    BtrCore_BTRegisterConnIntimationCb_StubWithCallback(mock_BtrCore_BTRegisterConnIntimationCb);
    BtrCore_BTRegisterConnAuthCb_StubWithCallback(mock_BtrCore_BTRegisterConnAuthCb_fail);
    BtrCore_BTReleaseAdapterPath_StubWithCallback(mock_BtrCore_BTReleaseAdapterPath);
    BtrCore_BTReleaseAgentPath_StubWithCallback(mock_BtrCore_BTReleaseAgentPath);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(0);
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}

void test_BTRCore_Init_ValidParameters9(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterAdapterStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);
    BtrCore_BTRegisterDevStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);

    BtrCore_BTRegisterConnIntimationCb_StubWithCallback(mock_BtrCore_BTRegisterConnIntimationCb);
    BtrCore_BTRegisterConnAuthCb_StubWithCallback(mock_BtrCore_BTRegisterConnAuthCb);
    BTRCore_AVMedia_RegisterMediaStatusUpdateCb_IgnoreAndReturn(1);
    BtrCore_BTReleaseAdapterPath_StubWithCallback(mock_BtrCore_BTReleaseAdapterPath);
    BtrCore_BTReleaseAgentPath_StubWithCallback(mock_BtrCore_BTReleaseAgentPath);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(0);
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}

enBTRCoreRet
mock_BTRCore_LE_RegisterStatusUpdateCb_Fail (
    tBTRCoreLeHdl                       hBTRCoreLe,
    fPtr_BTRCore_LeStatusUpdateCb       afpcBTRCoreLeStatusUpdate,
    void*                               apvBtLeStatusUserData
) {
    

    return enBTRCoreFailure;
}

void test_BTRCore_Init_ValidParameters10(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_IgnoreAndReturn((void *)mockHandle);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterAdapterStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);
    BtrCore_BTRegisterDevStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);

    BtrCore_BTRegisterConnIntimationCb_StubWithCallback(mock_BtrCore_BTRegisterConnIntimationCb);
    BtrCore_BTRegisterConnAuthCb_StubWithCallback(mock_BtrCore_BTRegisterConnAuthCb);
    BTRCore_AVMedia_RegisterMediaStatusUpdateCb_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_LE_RegisterStatusUpdateCb_StubWithCallback(mock_BTRCore_LE_RegisterStatusUpdateCb_Fail);
    BtrCore_BTReleaseAdapterPath_StubWithCallback(mock_BtrCore_BTReleaseAdapterPath);
    BtrCore_BTReleaseAgentPath_StubWithCallback(mock_BtrCore_BTReleaseAgentPath);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(0);
    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}

int
mock_BtrCore_BTRegisterAgent (
    void*       apstBtIfceHdl,
    const char* apBtAdapter,
    const char* apBtAgentPath,
    const char* capabilities
) {
    

    return 0;
}

void test_BTRCore_RegisterAgent(void) {
    stBTRCoreHdl btrCoreHdl;
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle
    BtrCore_BTRegisterAgent_StubWithCallback(mock_BtrCore_BTRegisterAgent);

    enBTRCoreRet ret = BTRCore_RegisterAgent((tBTRCoreHandle)&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    ret = BTRCore_RegisterAgent((tBTRCoreHandle)&btrCoreHdl, 2);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    ret = BTRCore_RegisterAgent((tBTRCoreHandle)&btrCoreHdl, 3);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    ret = BTRCore_RegisterAgent((tBTRCoreHandle)&btrCoreHdl, 4);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
        
    // Test case for invalid handle
    ret = BTRCore_RegisterAgent(NULL, 0);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);
}
// Mock function for BTRCore_SetAdapter
void test_BTRCore_SetAdapter_should_UpdateAdapterNumberSuccessfully1(void) {
    stBTRCoreHdl* hBTRCore = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl)); 
    if (!hBTRCore) {
        BTRCORELOG_ERROR("Memory allocation failed for hBTRCore\n");
        return;
    }
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));

    // Allocate memory for curAdapterPath
    hBTRCore->curAdapterPath = (char*)malloc(32);
    if (!hBTRCore->curAdapterPath) {
        BTRCORELOG_ERROR("Memory allocation failed for curAdapterPath\n");
        free(hBTRCore);
        return;
    }

    // Initialize the adapter path
    strcpy(hBTRCore->curAdapterPath, "/path/to/adapter0");

    // Test case for adapter number 0
    enBTRCoreRet ret = BTRCore_SetAdapter((tBTRCoreHandle)hBTRCore, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter0", hBTRCore->curAdapterPath);

    // Test case for adapter number 1
    ret = BTRCore_SetAdapter((tBTRCoreHandle)hBTRCore, 1);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter1", hBTRCore->curAdapterPath);

    // Test case for adapter number 2
    ret = BTRCore_SetAdapter((tBTRCoreHandle)hBTRCore, 2);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter2", hBTRCore->curAdapterPath);

    // Test case for adapter number 3
    ret = BTRCore_SetAdapter((tBTRCoreHandle)hBTRCore, 3);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter3", hBTRCore->curAdapterPath);

    // Test case for adapter number 4
    ret = BTRCore_SetAdapter((tBTRCoreHandle)hBTRCore, 4);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter4", hBTRCore->curAdapterPath);

    // Test case for adapter number 5
    ret = BTRCore_SetAdapter((tBTRCoreHandle)hBTRCore, 5);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter5", hBTRCore->curAdapterPath);

    // Test case for adapter number 6 (default case)
    ret = BTRCore_SetAdapter((tBTRCoreHandle)hBTRCore, 6);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("/path/to/adapter0", hBTRCore->curAdapterPath);

    // Test case for invalid handle
    ret = BTRCore_SetAdapter(NULL, 0);
    TEST_ASSERT_EQUAL(enBTRCoreNotInitialized, ret);

    // Clean up
    free(hBTRCore->curAdapterPath);
    free(hBTRCore);
}

void test_BTRCore_UNPAIR_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Xbox Controller";

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    btrCoreHdl.numOfPairedDevices = 1;
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath, devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, deviceName);

    // Mock the BtrCore_BTDisconnectDevice function to return success
    BtrCore_BTDisconnectDevice_StubWithCallback(mock_BtrCore_BTDisconnectDevice);

    // Mock the BtrCore_BTEnableEnhancedRetransmissionMode function to return success
    BtrCore_BTEnableEnhancedRetransmissionMode_StubWithCallback(mock_BtrCore_BTEnableEnhancedRetransmissionMode_fail);
    // BtrCore_BTPerformAdapterOp_SttubWithCallback(mock_BtrCore_BTPerformAdapterOp);
    BtrCore_BTPerformAdapterOp_IgnoreAndReturn(0);
    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(0);
    // Test case for successful disconnection
    ret = BTRCore_UnPairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}



void test_BTRCore_UNPAIR_Fail(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Xbox Controller";

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    btrCoreHdl.numOfPairedDevices = 1;
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath, devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, deviceName);

    // Mock the BtrCore_BTDisconnectDevice function to return success
    BtrCore_BTDisconnectDevice_StubWithCallback(mock_BtrCore_BTDisconnectDevice);

    // Mock the BtrCore_BTEnableEnhancedRetransmissionMode function to return success
    BtrCore_BTEnableEnhancedRetransmissionMode_StubWithCallback(mock_BtrCore_BTEnableEnhancedRetransmissionMode_fail);
    // BtrCore_BTPerformAdapterOp_SttubWithCallback(mock_BtrCore_BTPerformAdapterOp);
    BtrCore_BTPerformAdapterOp_IgnoreAndReturn(1);
    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(1);
    // Test case for successful disconnection
    ret = BTRCore_UnPairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCorePairingFailed, ret);
}

void test_BTRCore_ConnectDevice_Success(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Xbox";

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    btrCoreHdl.numOfPairedDevices = 1;
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath, devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, deviceName);

    // Mock the BtrCore_BTDisconnectDevice function to return success
    BtrCore_BTDisconnectDevice_StubWithCallback(mock_BtrCore_BTDisconnectDevice);

    // Mock the BtrCore_BTEnableEnhancedRetransmissionMode function to return success
    BtrCore_BTEnableEnhancedRetransmissionMode_StubWithCallback(mock_BtrCore_BTEnableEnhancedRetransmissionMode_fail);
    // BtrCore_BTPerformAdapterOp_SttubWithCallback(mock_BtrCore_BTPerformAdapterOp);
    BtrCore_BTPerformAdapterOp_IgnoreAndReturn(0);
    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(0);
    BtrCore_BTDisableEnhancedRetransmissionMode_IgnoreAndReturn(1);


    BtrCore_BTConnectDevice_StubWithCallback(mock_BtrCore_BTConnectDevice);
    // Test case for successful disconnection
    ret = BTRCore_ConnectDevice(&btrCoreHdl, 0,enBTDevHID);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_GetDeviceConnected_DeviceConnected1(void) {
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    //stBTRCoreDevStateInfo devStateInfo = { .eDeviceCurrState = enBTRCoreDevStConnected };
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));
    
    // Initialize the structure members
    hBTRCore->connHdl = (void*)1; 
    hBTRCore->curAdapterPath = "/test/path";

    // Sample details for 1st Known Device 
    hBTRCore->stKnownDevicesArr[0].tDeviceId = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = TRUE;  // Assuming this means connected
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "device0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress, "address0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress));
    
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(hBTRCore->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
    hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStUnknown;
    hBTRCore->numOfPairedDevices = 1; 
    // Mocking and stubbing
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo4);
    enBTRCoreRet ret = BTRCore_GetDeviceConnected(hBTRCore, 0, enBTRCoreHID);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

int
_mock_BTGetPairedDeviceInfo_Success (
    void*                   apBtConn,
    const char*             apBtAdapter,
    stBTPairedDeviceInfo*   pairedDevices
){
    //stBTPairedDeviceInfo pairedDevices = {0};
    pairedDevices->numberOfDevices=2;
    pairedDevices->deviceInfo[0].bConnected = 1;
    return 0;
}
void test_BTRCore_GetDeviceConnected_DeviceConnected2(void) {
    stBTRCoreHdl* hBTRCore = malloc(sizeof(stBTRCoreHdl));
    //stBTRCoreDevStateInfo devStateInfo = { .eDeviceCurrState = enBTRCoreDevStConnected };
    memset(hBTRCore, 0, sizeof(stBTRCoreHdl));
    
    // Initialize the structure members
    hBTRCore->connHdl = (void*)1; 
    hBTRCore->curAdapterPath = "/test/path";

    // Sample details for 1st Known Device 
    hBTRCore->stKnownDevicesArr[0].tDeviceId = 1;
    hBTRCore->stKnownDevicesArr[0].bDeviceConnected = TRUE;  // Assuming this means connected
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceName, "device0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceName));
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress, "address0", sizeof(hBTRCore->stKnownDevicesArr[0].pcDeviceAddress));
    
    strncpy(hBTRCore->stKnownDevicesArr[0].pcDevicePath, "/device/path", sizeof(hBTRCore->stKnownDevicesArr[0].pcDevicePath));  // Initialize device path
    hBTRCore->stKnownDevStInfoArr[0].eDeviceCurrState = enBTRCoreDevStUnknown;
    hBTRCore->numOfPairedDevices = 1; 
    // Mocking and stubbing
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo_Success);
    enBTRCoreRet ret = BTRCore_GetDeviceConnected(hBTRCore, 0, enBTRCoreHID);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess ,ret);
}

void test_BTRCore_GetDeviceTypeClass_PairedDevice1(void) {
    stBTRCoreHdl hBTRCore;
    stBTRCoreBTDevice knownDevice;
    enBTRCoreDeviceType deviceType;
    enBTRCoreDeviceClass deviceClass;
    const char* deviceAddress = "00:11:22:33:44:55";
    tBTRCoreDevId aBTRCoreDevId = 0;

    aBTRCoreDevId = 0;
    // Initialize the structures
    memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));

    // Set up the known device
    knownDevice.tDeviceId = aBTRCoreDevId;
    strncpy(knownDevice.pcDeviceAddress, deviceAddress, sizeof(knownDevice.pcDeviceAddress) - 1);
    knownDevice.pcDeviceAddress[sizeof(knownDevice.pcDeviceAddress) - 1] = '\0';
    knownDevice.enDeviceType = enBTRCore_DC_Unknown;

    hBTRCore.stKnownDevicesArr[0] = knownDevice;
    hBTRCore.numOfPairedDevices = 1;
    hBTRCore.numOfScannedDevices=2;
    hBTRCore.stKnownDevicesArr[0].tDeviceId=0;;
    // Call the function under test
    enBTRCoreRet ret = BTRCore_GetDeviceTypeClass(&hBTRCore, aBTRCoreDevId, &deviceType, &deviceClass);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(enBTRCore_DC_Unknown, deviceClass);
    TEST_ASSERT_EQUAL(enBTRCoreUnknown, deviceType);
}

void test_BTRCore_GetLEProperty_AllCases2(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreBTDevice scannedDevice;
    stBTRCoreDevStateInfo devStateInfo;
    const char* deviceAddress = "00:11:22:33:44:55";
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Test Device";
    char propValue[256];
    enBTRCoreRet ret;
    const char* uuid = "UUID";

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&scannedDevice, 0, sizeof(stBTRCoreBTDevice));
    memset(&devStateInfo, 0, sizeof(stBTRCoreDevStateInfo));

    btrCoreHdl.stScannedDevicesArr[0] = scannedDevice;
    // Set up the scanned device
    scannedDevice.tDeviceId = 0;
    scannedDevice.bDeviceConnected = TRUE;
    strncpy(scannedDevice.pcDeviceAddress, deviceAddress, sizeof(scannedDevice.pcDeviceAddress) - 1);
    scannedDevice.pcDeviceAddress[sizeof(scannedDevice.pcDeviceAddress) - 1] = '\0';
    strncpy(scannedDevice.pcDevicePath, devicePath, sizeof(scannedDevice.pcDevicePath) - 1);
    scannedDevice.pcDevicePath[sizeof(scannedDevice.pcDevicePath) - 1] = '\0';
    strncpy(scannedDevice.pcDeviceName, deviceName, sizeof(scannedDevice.pcDeviceName) - 1);
    scannedDevice.pcDeviceName[sizeof(scannedDevice.pcDeviceName) - 1] = '\0';

    btrCoreHdl.numOfScannedDevices = 1;

    btrCoreHdl.stScannedDevicesArr[0].tDeviceId=1;


    // Mock the btrCore_GetDeviceInfo function to return success
    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo);
    BTRCore_LE_GetGattProperty_IgnoreAndReturn(0);

   // Iterate over all possible values of the aenBTRCoreLeProp enumeration
    for (enBTRCoreLeProp prop = enBTRCoreLePropGUUID; prop <= enBTRCoreLePropGChar; prop++) {
        ret = BTRCore_GetLEProperty(&btrCoreHdl, 0, uuid, prop, propValue);
        TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
    }
    ret = BTRCore_GetLEProperty(&btrCoreHdl, 0, uuid, enBTRCoreLePropUnknown, propValue);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void*
mock_BtrCore_BTInitGetConnection1 (
    void
) 
{

    stBtIfceHdl* pstlhBtIfce = (stBtIfceHdl*)malloc(sizeof(stBtIfceHdl));
    if (!pstlhBtIfce)
        return NULL;

   // pstlhBtIfce->pGAQueueOutTask->enBTRCoreTskOp=enBTRCoreTaskOpStop;
    pstlhBtIfce->pDBusConn                      = 1;
    strncpy(pstlhBtIfce->pcDeviceCurrState,   "disconnected", BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcLeDeviceCurrState, "disconnected", BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcLeDeviceAddress,   "none",         BT_MAX_STR_LEN - 1);
    strncpy(pstlhBtIfce->pcMediaCurrState,    "none",         BT_MAX_STR_LEN - 1); 

 
    BtrCore_BTDeInitReleaseConnection((void*)pstlhBtIfce);
    
    return (void*)pstlhBtIfce;
}

int
mock_BtrCore_BTDeInitReleaseConnection (
    void* apstBtIfceHdl
) {
    stBtIfceHdl*    pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;

    if (!apstBtIfceHdl)
        return -1;

   
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


    pstlhBtIfce->pDBusConn = NULL;

    free(pstlhBtIfce);
    pstlhBtIfce = NULL;


    return 0;
}



void test_BTRCore_Init_ValidParameters11(void) {
    // Arrange
    tBTRCoreHandle hBTRCore = NULL;
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));

    BtrCore_BTInitGetConnection_StubWithCallback(mock_BtrCore_BTInitGetConnection1);
    BtrCore_BTDeInitReleaseConnection_StubWithCallback(mock_BtrCore_BTDeInitReleaseConnection);
    BtrCore_BTGetAgentPath_IgnoreAndReturn("Agent Path");
    BtrCore_BTSendReceiveMessages_IgnoreAndReturn(0);
    BtrCore_BTGetAdapterPath_IgnoreAndReturn("Adapter Path");
    BtrCore_BTGetProp_IgnoreAndReturn(0);
    BTRCore_AVMedia_Init_IgnoreAndReturn(enBTRCoreSuccess);

    BTRCore_LE_Init_IgnoreAndReturn(enBTRCoreSuccess);
    BtrCore_BTRegisterAdapterStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);
    BtrCore_BTRegisterDevStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterAdapterStatusUpdateCb);

    BtrCore_BTRegisterConnIntimationCb_StubWithCallback(mock_BtrCore_BTRegisterConnIntimationCb);
    BtrCore_BTRegisterConnAuthCb_StubWithCallback(mock_BtrCore_BTRegisterConnAuthCb);
    BTRCore_AVMedia_RegisterMediaStatusUpdateCb_IgnoreAndReturn(enBTRCoreSuccess);
    BTRCore_LE_RegisterStatusUpdateCb_StubWithCallback(mock_BTRCore_LE_RegisterStatusUpdateCb_Fail);
    BtrCore_BTReleaseAdapterPath_StubWithCallback(mock_BtrCore_BTReleaseAdapterPath);
    BtrCore_BTReleaseAgentPath_StubWithCallback(mock_BtrCore_BTReleaseAgentPath);
    BtrCore_BTDeInitReleaseConnection_IgnoreAndReturn(0);


    enBTRCoreRet result = BTRCore_Init(&hBTRCore);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    // Cleanup
    free(mockHandle);
    BTRCore_DeInit(hBTRCore); // Ensure proper deinitialization
}


void test_btrCore_BTMediaStatusUpdateCb1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTRCoreAVMediaStatusUpdate mediaStatusUpdate;
    char* btdevAddr = "00:11:22:33:44:55";
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&mediaStatusUpdate, 0, sizeof(stBTRCoreAVMediaStatusUpdate));
    btrCoreHdl.numOfPairedDevices=2;
    btrCoreHdl.stKnownDevicesArr[0].tDeviceId=73588229205;
    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask =g_async_queue_new();//(pGAQueueOutTask*)malloc(pGAQueueOutTask); // Mock pointer
    mediaStatusUpdate.eAVMediaState = eBTRCoreAVMediaTrkStPaused;
        // Call the function with the current media state
    ret = btrCore_BTMediaStatusUpdateCb(&mediaStatusUpdate, btdevAddr, &btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Define a set of media states to test
    eBTRCoreAVMElementType mediaStates[] ={
        eBTRCoreAVMETypeUnknown,
        eBTRCoreAVMETypeAlbum,
        eBTRCoreAVMETypeArtist,
        eBTRCoreAVMETypeGenre,
        eBTRCoreAVMETypeCompilation,
        eBTRCoreAVMETypePlayList,
        eBTRCoreAVMETypeTrackList,
        eBTRCoreAVMETypeTrack
    } ;

    // Iterate over the media states and test each one
    for (size_t i = 0; i < sizeof(mediaStates) / sizeof(mediaStates[0]); i++) {
        // Set up the media status update with the current media state
        mediaStatusUpdate.eAVMediaState = eBTRCoreAVMediaElementAdded;
        mediaStatusUpdate.m_mediaElementInfo.eAVMElementType=mediaStates[i];
        // Call the function with the current media state
        ret = btrCore_BTMediaStatusUpdateCb(&mediaStatusUpdate, btdevAddr, &btrCoreHdl);
        TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    }
    // 73588229205
}
void test_btrCore_BTDeviceAuthenticationCb_speakers1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x40000u; // Example class
    strcpy(deviceInfo.aUUIDs[0], "0000110d-0000-1000-8000-00805f9b34fb"); // A2DP Sink UUID
    strcpy(deviceInfo.aUUIDs[1], "");

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer
    btrCoreHdl.fpcBBTRCoreConnAuth =myConnAuthCallback;
    // Call the function with Mobile Audio In device type
    ret = btrCore_BTDeviceAuthenticationCb(enBTDevHID, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(1, ret);

}

void test_btrCore_BTDeviceAuthenticationCb_speakers2(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    const char* uuids[] = {
        "0000110d-0000-1000-8000-00805f9b34fb", // A2DP Sink UUID
        "0000110a-0000-1000-8000-00805f9b34fb", // A2DP Source UUID
        "00001803-0000-1000-8000-00805f9b34fb", // GATT Tile 1 UUID
        "00001804-0000-1000-8000-00805f9b34fb", // GATT Tile 2 UUID
        "00001805-0000-1000-8000-00805f9b34fb", // GATT Tile 3 UUID
        "00001124-0000-1000-8000-00805f9b34fb", // HID 1 UUID
        "00001125-0000-1000-8000-00805f9b34fb", // HID 2 UUID
        "0000180f-0000-1000-8000-00805f9b34fb", // GATT XBB 1 UUID
        "0000180e-0000-1000-8000-00805f9b34fb", // Battery Service XBB 1 UUID
        "0000180d-0000-1000-8000-00805f9b34fb"  // Battery Service XBB 3 UUID
    };
    const enBTRCoreDeviceClass expectedDeviceTypes[] = {
        enBTRCore_DC_Loudspeaker,
        enBTRCore_DC_SmartPhone,
        enBTRCore_DC_Tile,
        enBTRCore_DC_Tile,
        enBTRCore_DC_Tile,
        enBTRCore_DC_HID_Keyboard,
        enBTRCore_DC_HID_Keyboard,
        enBTRCore_DC_XBB,
        enBTRCore_DC_XBB,
        enBTRCore_DC_XBB
    };

    for (int i = 0; i < sizeof(uuids) / sizeof(uuids[0]); i++) {
        // Reset the device info and found device
        memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));
        memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));

        // Set the UUID
        strcpy(deviceInfo.aUUIDs[0], uuids[i]);
        strcpy(deviceInfo.aUUIDs[1], "");

        // Initialize the queue out task pointer
        btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer
        btrCoreHdl.fpcBBTRCoreConnAuth = myConnAuthCallback;
        deviceInfo.saServices[0].len=1;

        // Call the function with Mobile Audio In device type
        ret = btrCore_BTDeviceAuthenticationCb(enBTRCoreAudioAndHID, &deviceInfo, &btrCoreHdl);
        TEST_ASSERT_EQUAL(1, ret);

        // Check the device type
    }
}
void test_BTRCore_UNPAIR_Success1(void) {
    stBTRCoreHdl btrCoreHdl = {0};
    enBTRCoreRet ret;
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Xbox Controller";

    // Initialize the BTRCore handle
    btrCoreHdl.connHdl = (void*)1; // Mock connection handle

    btrCoreHdl.numOfPairedDevices = 1;
    btrCoreHdl.numOfScannedDevices=1;
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55");
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDevicePath, devicePath);
    strcpy(btrCoreHdl.stKnownDevicesArr[0].pcDeviceName, deviceName);

    // Mock the BtrCore_BTDisconnectDevice function to return success
    BtrCore_BTDisconnectDevice_StubWithCallback(mock_BtrCore_BTDisconnectDevice);

    // Mock the BtrCore_BTEnableEnhancedRetransmissionMode function to return success
    BtrCore_BTEnableEnhancedRetransmissionMode_StubWithCallback(mock_BtrCore_BTEnableEnhancedRetransmissionMode_fail);
    // BtrCore_BTPerformAdapterOp_SttubWithCallback(mock_BtrCore_BTPerformAdapterOp);
    BtrCore_BTPerformAdapterOp_IgnoreAndReturn(0);
    BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(0);
    // Test case for successful disconnection
    ret = BTRCore_UnPairDevice(&btrCoreHdl, 0);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}
int mock_BtrCore_BTGetPairedDeviceInfo3(
    void*                   apstBtIfceHdl,
    const char*             apBtAdapter,
    stBTPairedDeviceInfo*   pPairedDeviceInfo)
{   pPairedDeviceInfo->numberOfDevices=1;
    return 0;
}
void test_BTRCore_PairDevice_should_PairBluetoothCoreDeviceSuccessfully1(void) {
    // Allocate and initialize the BTRCore handle
    tBTRCoreHandle hBTRCore = (tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    TEST_ASSERT_NOT_NULL(hBTRCore); // Ensure allocation was successful

    tBTRCoreDevId aBTRCoreDevId = 0;   // Assuming this is the device ID to pair
    enBTRCoreRet ret;

    // Initialize the scanned device details
    stBTRCoreBTDevice* scannedDevice = &(((stBTRCoreHdl*)hBTRCore)->stScannedDevicesArr[0]);
    scannedDevice->bFound = TRUE;
    scannedDevice->tDeviceId = aBTRCoreDevId;
    
    strcpy(scannedDevice->pcDeviceAddress, "address");  // Set the device address correctly
    
  
    // Mock the necessary function call with the correct number of arguments
    BtrCore_BTPerformAdapterOp_ExpectAndReturn(
        ((stBTRCoreHdl*)hBTRCore)->connHdl,           // connHdl
        ((stBTRCoreHdl*)hBTRCore)->curAdapterPath,    // adapterPath
        ((stBTRCoreHdl*)hBTRCore)->agentPath,         // agentPath
        "address",                                    // devAddress
        enBTAdpOpCreatePairedDev,                     // operation
        0                                             // return value (success)
    );
    
    stBTRCoreHdl vvd;
    ((stBTRCoreHdl*)hBTRCore)->numOfPairedDevices=2;

    BtrCore_BTGetPairedDeviceInfo_StubWithCallback(mock_BtrCore_BTGetPairedDeviceInfo3);
    

    // Call the function under test
    ret = BTRCore_PairDevice(hBTRCore, aBTRCoreDevId);

    // Verify the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Clean up
    free(hBTRCore);
}
void test_btrCore_BTDeviceStatusUpdateCb_DeviceFound2(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;
    unsigned int classIds[] = {
    (enBTRCore_DC_WearableHeadset| 0x400),  
    (enBTRCore_DC_HIFIAudioDevice| 0x400), 
    (enBTRCore_DC_Headphones| 0x400),
    (enBTRCore_DC_Tablet| 0x400),           
    (enBTRCore_DC_SmartPhone| 0x400)
    };

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class
     // Initialize the queue out task pointer
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevHFPAudioGateway, enBTDevStFound, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

     // Test case for enBTDevStFound
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevLE, enBTDevStFound, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);
    for(int i=0;i<4;i++){
     
    
    deviceInfo.ui32Class = classIds[i]; 
    // Test case for enBTDevStFound
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevAudioSink, enBTDevStFound, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    }
    for(int i=4;i<6;i++){
     
    
    deviceInfo.ui32Class = classIds[i]; 
    // Test case for enBTDevStFound
    ret = btrCore_BTDeviceStatusUpdateCb(enBTDevAudioSource, enBTDevStFound, &deviceInfo, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    }

    printf("Test case for device found passed!\n");
}

void test_BTRCore_newBatteryLevelDevice2(void) {
    tBTRCoreHandle btrCoreHdl=(tBTRCoreHandle)malloc(sizeof(stBTRCoreHdl));
    memset(btrCoreHdl, 0, sizeof(tBTRCoreHandle));
    ((stBTRCoreHdl*)btrCoreHdl)->batteryLevelThreadExit = 0;
    const char* devicePath = "/org/bluez/hci0/dev_00_11_22_33_44_55";
    const char* deviceName = "Xbox Controller";

    // Initialize the BTRCore handle
    ((stBTRCoreHdl*)btrCoreHdl)->connHdl = (void*)1; // Mock connection handle

    // ((stBTRCoreHdl*)btrCoreHdl)->numOfPairedDevices = 1;
    ((stBTRCoreHdl*)btrCoreHdl)->numOfScannedDevices=1;
    strcpy(((stBTRCoreHdl*)btrCoreHdl)->stKnownDevicesArr[0].pcDeviceAddress, "00:11:22:33:44:55");
    strcpy(((stBTRCoreHdl*)btrCoreHdl)->stKnownDevicesArr[0].pcDevicePath, devicePath);
    strcpy(((stBTRCoreHdl*)btrCoreHdl)->stKnownDevicesArr[0].pcDeviceName, deviceName);
    
    ((stBTRCoreHdl*)btrCoreHdl)->stKnownDevicesArr[0].tDeviceId=1;
  
    ((stBTRCoreHdl*)btrCoreHdl)->stKnownDevicesArr[0].bDeviceConnected = 1;
    ((stBTRCoreHdl*)btrCoreHdl)->stKnownDevicesArr[0].enDeviceType=enBTRCore_DC_HID_Keyboard;
    ((stBTRCoreHdl*)btrCoreHdl)->batteryLevelThread = NULL;
    enBTRCoreRet ret = BTRCore_newBatteryLevelDevice(btrCoreHdl);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    free(btrCoreHdl);
}



void test_btrCore_BTDeviceConnectionIntimationCb_HID1(void) {
    stBTRCoreHdl btrCoreHdl;
    stBTDeviceInfo deviceInfo;
    int ret;

    // Initialize the structures
    memset(&btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    memset(&deviceInfo, 0, sizeof(stBTDeviceInfo));

    // Set up the device info
    deviceInfo.ui32Class = 0x1F00; // Example class

    // Initialize the queue out task pointer
    btrCoreHdl.pGAQueueOutTask = (void*)1; // Mock pointer

    strcpy(deviceInfo.pcAddress,"20:44:41");
    deviceInfo.ui16Appearance=0x0180;

    // Call the function with HID device type
    ret = btrCore_BTDeviceConnectionIntimationCb(enBTDevAudioSource, &deviceInfo, 123456, 1, &btrCoreHdl);
    TEST_ASSERT_EQUAL(0, ret);

    printf("Test case for HID device type passed!\n");
}

void test_btrCore_BTAdapterStatusUpdateCb(void) {
    // Arrange
    stBTRCoreHdl* mockHandle = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(mockHandle, 0, sizeof(stBTRCoreHdl));
    mockHandle->curAdapterPath = "/org/bluez/hci0";

    // Mock data for the test
    stBTAdapterInfo mockAdapterInfo;
    memset(&mockAdapterInfo, 0, sizeof(stBTAdapterInfo));
    strcpy(mockAdapterInfo.pcPath, mockHandle->curAdapterPath);
    mockAdapterInfo.bDiscoverable = TRUE;
    mockAdapterInfo.bDiscovering = TRUE;

    // Act
    int result = btrCore_BTAdapterStatusUpdateCb(enBTAdPropDiscoveryStatus, &mockAdapterInfo, mockHandle);

    // Assert
    TEST_ASSERT_EQUAL(0, result);

    result = btrCore_BTAdapterStatusUpdateCb(enBTAdPropAdvTimeout, &mockAdapterInfo, mockHandle);

    // Assert
    TEST_ASSERT_EQUAL(0, result);

    result = btrCore_BTAdapterStatusUpdateCb(enBTAdPropUnknown, &mockAdapterInfo, mockHandle);

    // Assert
    TEST_ASSERT_EQUAL(0, result);

    // Cleanup
    free(mockHandle);
}
void test_BTRCore_GetDeviceTypeClass(void) {
    // Arrange
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    btrCoreHdl->numOfScannedDevices = 1;
    btrCoreHdl->stScannedDevicesArr[0].tDeviceId = 0;
    btrCoreHdl->stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_Tile;

    tBTRCoreDevId devId = 0; // This should be less than BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES
    enBTRCoreDeviceType devType;
    enBTRCoreDeviceClass devClass;

    // Act
    enBTRCoreRet result = BTRCore_GetDeviceTypeClass((tBTRCoreHandle)btrCoreHdl, devId, &devType, &devClass);

    // Assert
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    // Cleanup
    free(btrCoreHdl);
}
void test_BTRCore_GetDeviceTypeClass_GreaterThanOrEqualToMaxDiscoveredDevices(void) {
    // Arrange
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(btrCoreHdl, 0, sizeof(stBTRCoreHdl));

    btrCoreHdl->numOfScannedDevices = 1;
    btrCoreHdl->stScannedDevicesArr[0].tDeviceId = 123456789; // Some unique ID
    btrCoreHdl->stScannedDevicesArr[0].enDeviceType = enBTRCore_DC_Tile;

    tBTRCoreDevId devId = 123456789; // This should be greater than or equal to BTRCORE_MAX_NUM_BT_DISCOVERED_DEVICES
    enBTRCoreDeviceType devType;
    enBTRCoreDeviceClass devClass;

    // Act
    enBTRCoreRet result = BTRCore_GetDeviceTypeClass((tBTRCoreHandle)btrCoreHdl, devId, &devType, &devClass);

    // Assert
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);

    // Cleanup
    free(btrCoreHdl);
}
void test_btrCore_BTDeviceAuthenticationCb1(void) {
    // Arrange
    stBTRCoreHdl* btrCoreHdl = (stBTRCoreHdl*)malloc(sizeof(stBTRCoreHdl));
    memset(btrCoreHdl, 0, sizeof(stBTRCoreHdl));
    btrCoreHdl->aenDeviceDiscoveryType = enBTRCoreAudioAndHID;

    stBTDeviceInfo btDeviceInfo;
    memset(&btDeviceInfo, 0, sizeof(stBTDeviceInfo));
    btDeviceInfo.ui32Class = 0x00250; // Some class ID
    strncpy(btDeviceInfo.pcName, "Mock Device", BD_NAME_LEN);
    strncpy(btDeviceInfo.pcAddress, "00:11:22:33:44:55", BD_NAME_LEN);
    strncpy(btDeviceInfo.pcDevicePath, "/org/bluez/hci0/dev_00_11_22_33_44_55", BD_NAME_LEN);

    // // Initialize UUIDs to cover all cases
    strcpy(btDeviceInfo.aUUIDs[0],"00001101-0000-1000-8000-00805f9b34fb");
    strcpy(btDeviceInfo.aUUIDs[1],"00001108-0000-1000-8000-00805f9b34fb");
    strcpy(btDeviceInfo.aUUIDs[0], "00001101-0000-1000-8000-00805f9b34fb"); // Serial Port
    strcpy(btDeviceInfo.aUUIDs[1], "00001108-0000-1000-8000-00805f9b34fb"); // Headset
    strcpy(btDeviceInfo.aUUIDs[2], "0000110a-0000-1000-8000-00805f9b34fb"); // Audio Source
    strcpy(btDeviceInfo.aUUIDs[3], "0000110b-0000-1000-8000-00805f9b34fb"); // Audio Sink
    strcpy(btDeviceInfo.aUUIDs[4], "0000110e-0000-1000-8000-00805f9b34fb"); // AV Remote Target
    strcpy(btDeviceInfo.aUUIDs[5], "0000110c-0000-1000-8000-00805f9b34fb"); // Advanced Audio Distribution
    strcpy(btDeviceInfo.aUUIDs[6], "0000110f-0000-1000-8000-00805f9b34fb"); // AV Remote Control
    strcpy(btDeviceInfo.aUUIDs[7], "0000110d-0000-1000-8000-00805f9b34fb"); // AV Remote
    strcpy(btDeviceInfo.aUUIDs[8], "00001112-0000-1000-8000-00805f9b34fb"); // Headset Audio Gateway
    strcpy(btDeviceInfo.aUUIDs[9], "0000111e-0000-1000-8000-00805f9b34fb"); // Handsfree
    strcpy(btDeviceInfo.aUUIDs[10], "0000111f-0000-1000-8000-00805f9b34fb"); // Handsfree Audio Gateway
    strcpy(btDeviceInfo.aUUIDs[11], "00001124-0000-1000-8000-00805f9b34fb"); // Headset 2
    strcpy(btDeviceInfo.aUUIDs[12], "00001203-0000-1000-8000-00805f9b34fb"); // Generic Audio
    strcpy(btDeviceInfo.aUUIDs[13], "00001200-0000-1000-8000-00805f9b34fb"); // PnP Information
    strcpy(btDeviceInfo.aUUIDs[14], "00001204-0000-1000-8000-00805f9b34fb"); // Generic Attribute
    strcpy(btDeviceInfo.aUUIDs[15], "0000180f-0000-1000-8000-00805f9b34fb"); // Tile
    strcpy(btDeviceInfo.aUUIDs[16], "00001810-0000-1000-8000-00805f9b34fb"); // Tile
    strcpy(btDeviceInfo.aUUIDs[17], "00001811-0000-1000-8000-00805f9b34fb"); // Tile
    strcpy(btDeviceInfo.aUUIDs[18], "00001800-0000-1000-8000-00805f9b34fb"); // Generic Access
    strcpy(btDeviceInfo.aUUIDs[19], "00001801-0000-1000-8000-00805f9b34fb"); // Generic Attribute
    strcpy(btDeviceInfo.aUUIDs[20], "0000180a-0000-1000-8000-00805f9b34fb"); // Device Information
    strcpy(btDeviceInfo.aUUIDs[21], "0000180f-0000-1000-8000-00805f9b34fb"); // Battery Service
    strcpy(btDeviceInfo.aUUIDs[22], "00001124-0000-1000-8000-00805f9b34fb"); // HID
    strcpy(btDeviceInfo.aUUIDs[23], "00001125-0000-1000-8000-00805f9b34fb"); // HID
    strcpy(btDeviceInfo.aUUIDs[24], "0000180d-0000-1000-8000-00805f9b34fb"); // XBB
    strcpy(btDeviceInfo.aUUIDs[25], "0000180e-0000-1000-8000-00805f9b34fb"); // XBB
    strcpy(btDeviceInfo.aUUIDs[26], "0000180f-0000-1000-8000-00805f9b34fb"); // XBB
    strcpy(btDeviceInfo.aUUIDs[27], "00000000-0000-1000-8000-00805f9b34fb"); // Not Identified
    strcpy(btDeviceInfo.aUUIDs[28],"0x1812");
    
    
    // Act
    int result = btrCore_BTDeviceAuthenticationCb(enBTDevUnknown, &btDeviceInfo, btrCoreHdl);

    // Assert
    TEST_ASSERT_EQUAL(0, result);

    // Cleanup
    free(btrCoreHdl);
}



