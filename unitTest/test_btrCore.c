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
   //s TEST_ASSERT_NULL(&hBTRCore);
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

int _mockgetprop(void *apBtConn, const char *apcOpIfcePath, enBTOpIfceType aenBtOpIfceType, unBTOpIfceProp aunBtOpIfceProp, void *apvVal)
{
 strncpy(apvVal ,"hello",strlen("hello"));
    
    return 0;
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
    ) {
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
  
 //   stBTRCoreMediaTrackInfo trackInfo;
    stBTRCoreBTDevice knownDevice ;
   const char* deviceAddress = "/path/to/device1";
     //const char* deviceAddress = NULL;
      memset(&hBTRCore, 0, sizeof(stBTRCoreHdl));
    memset(&knownDevice, 0, sizeof(stBTRCoreBTDevice));
      knownDevice.bDeviceConnected = FALSE;

     BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mockgetsupported);
      // BTRCore_AVMedia_GetTrackInfo_IgnoreAndReturn(1);
     // BTRCore_AVMedia_GetTrackInfo_ExpectAndReturn(hBTRCore.avMediaHdl, NULL,NULL, enBTRCoreSuccess);
     // btrCore_GetDeviceInfoKnown_ExpectAndReturn(&hBTRCore, devId, devType, &knownDevice, enBTRCoreSuccess);
    //  BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(-1);
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


   // btrCore_GetDeviceInfoKnown_ExpectAnyArgsAndReturn(enBTRCoreSuccess);
  //  btrCore_GetDeviceInfoKnown_ReturnThruPtr_pstKnownDevice(&knownDevice);
  // BTRCore_AVMedia_GetPositionInfo_ExpectAndReturn(0,&knownDevice,NULL,enBTRCoreSuccess);
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
  //  memset(&device, 0, sizeof(stBTRCoreBTDevice));
   // memset(&mediaPositionInfo, 0, sizeof(stBTRCoreMediaPositionInfo));

   // btrCore_GetDeviceInfoKnown_ExpectAnyArgsAndReturn(enBTRCoreSuccess);
   // btrCore_GetDeviceInfoKnown_ReturnThruPtr_pstKnownDevice(&knownDevice);
    BTRCore_AVMedia_GetPositionInfo_IgnoreAndReturn(enBTRCoreFailure);

    enBTRCoreRet ret = BTRCore_GetMediaPositionInfo(&hBTRCore, 0, 0, &mediaPositionInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


int _mock_BTGetPairedDeviceInfo1(
    void* apBtConn, const char* apBtAdapter, stBTPairedDeviceInfo* pairedDevices
) {
    pairedDevices->numberOfDevices = 1;
    pairedDevices->deviceInfo[0].bPaired = 1;
    strcpy(pairedDevices->deviceInfo[0].pcAddress, "00:11:22:33:44:55");
    return 0;
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



int _mock_BTGetPairedDeviceInfo(
    void* apBtConn, const char* apBtAdapter, stBTPairedDeviceInfo* pairedDevices
) {
    pairedDevices->numberOfDevices = 1;
    pairedDevices->deviceInfo[0].bPaired = 1;
    strcpy(pairedDevices->deviceInfo[0].pcAddress, "00:11:22:33:44:55");
    return 0;
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
   // BTRCore_AVMedia_GetMediaProperty_ExpectAndReturn(hBTRCore.avMediaHdl, "valid", mediaPropertyKey, mediaPropertyValue,enBTRCoreFailure);
     //   BTRCore_AVMedia_GetMediaProperty_StopIgnore();
    // Call the function under test
    // Call the function under test
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
   // hBTRCore.stKnownDevicesArr[0] = knownDevice;

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
   // btrCore_GetDeviceInfoKnown_StubWithCallback(_mock_btrCore_GetDeviceInfoKnown);


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
   // hBTRCore.stKnownDevicesArr[0] = knownDevice;

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

      //BtrCore_BTGetPairedDeviceInfo_IgnoreAndReturn(0);

        BtrCore_BTGetPairedDeviceInfo_StubWithCallback(_mock_BTGetPairedDeviceInfo3);
//btrCore_PopulateListOfPairedDevices_IgnoreAndReturn(enBTRCoreSuccess);

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

    // Assume btrCore_GetDeviceInfoKnown will successfully get device info 
  // btrCore_GetDeviceInfoKnown_ExpectAndReturn(hBTRCore, deviceId, enBTRCoreUnknown, &devType, &deviceInfo, &devStInfo, &deviceAddress, enBTRCoreSuccess);
  

    // Assume BtrCore_BTFindServiceSupported will successfully find the service 
    //BtrCore_BTFindServiceSupported_ExpectAndReturn(hBTRCore->connHdl, deviceAddress, UUID, XMLdata, 1);  // return 1 indicating service is found 

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

enBTRCoreRet mock_BTRCore_StatusCb (stBTRCoreDevStatusCBInfo* apstDevStatusCbInfo, void* apvUserData)
{
    return enBTRCoreSuccess;
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



enBTRCoreRet mock_BTRCore_MediaStatusCb (stBTRCoreMediaStatusCBInfo* apstMediaStatusCbInfo, void* apvUserData)
{
    return enBTRCoreSuccess;
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
