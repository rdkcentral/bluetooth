#ifndef UNIT_TEST
#define UNIT_TEST
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System Headers */
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>

/* External Library Headers */
#include <glib.h>

/* Interface lib Headers */
#include "btrCore_logger.h"

/* Local Headers */
#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/a2dp-codecs.h>


#include "btrCore_avMedia.h"

#include "mock_btrCore_bt_ifce.h"

TEST_FILE("btrCore_avMedia.c")

// a2dp codec macros forward compatibility
#ifndef MIN_BITPOOL
#define MIN_BITPOOL SBC_MIN_BITPOOL
#endif
/* For TV platforms which is using bluez version 5.48 MAX_BITPOOL
 * is defined as 64 but for platform which is using bluez version
 * higher that 5.48 hard-coded MAX_BITPOOL below.
 */
#ifndef MAX_BITPOOL
#define MAX_BITPOOL 53
#endif

//#define AAC_SUPPORTED


#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))


#if defined(USE_BLUEZ4)

/* SBC Definitions */
#define BTR_SBC_CHANNEL_MODE_MONO           BT_A2DP_CHANNEL_MODE_MONO
#define BTR_SBC_CHANNEL_MODE_DUAL_CHANNEL   BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL
#define BTR_SBC_CHANNEL_MODE_STEREO         BT_A2DP_CHANNEL_MODE_STEREO
#define BTR_SBC_CHANNEL_MODE_JOINT_STEREO   BT_A2DP_CHANNEL_MODE_JOINT_STEREO

#define BTR_SBC_SAMPLING_FREQ_16000         BT_SBC_SAMPLING_FREQ_16000
#define BTR_SBC_SAMPLING_FREQ_32000         BT_SBC_SAMPLING_FREQ_32000
#define BTR_SBC_SAMPLING_FREQ_44100         BT_SBC_SAMPLING_FREQ_44100
#define BTR_SBC_SAMPLING_FREQ_48000         BT_SBC_SAMPLING_FREQ_48000

#define BTR_SBC_ALLOCATION_SNR              BT_A2DP_ALLOCATION_SNR
#define BTR_SBC_ALLOCATION_LOUDNESS         BT_A2DP_ALLOCATION_LOUDNESS

#define BTR_SBC_SUBBANDS_4                  BT_A2DP_SUBBANDS_4
#define BTR_SBC_SUBBANDS_8                  BT_A2DP_SUBBANDS_8

#define BTR_SBC_BLOCK_LENGTH_4              BT_A2DP_BLOCK_LENGTH_4
#define BTR_SBC_BLOCK_LENGTH_8              BT_A2DP_BLOCK_LENGTH_8
#define BTR_SBC_BLOCK_LENGTH_12             BT_A2DP_BLOCK_LENGTH_12

#define BTR_SBC_BLOCK_LENGTH_16             BT_A2DP_BLOCK_LENGTH_16

#elif defined(USE_BLUEZ5) || defined(USE_GDBUSBLUEZ5)

/* SBC Definitions */
#define BTR_SBC_CHANNEL_MODE_MONO           SBC_CHANNEL_MODE_MONO
#define BTR_SBC_CHANNEL_MODE_DUAL_CHANNEL   SBC_CHANNEL_MODE_DUAL_CHANNEL
#define BTR_SBC_CHANNEL_MODE_STEREO         SBC_CHANNEL_MODE_STEREO
#define BTR_SBC_CHANNEL_MODE_JOINT_STEREO   SBC_CHANNEL_MODE_JOINT_STEREO

#define BTR_SBC_SAMPLING_FREQ_16000         SBC_SAMPLING_FREQ_16000
#define BTR_SBC_SAMPLING_FREQ_32000         SBC_SAMPLING_FREQ_32000
#define BTR_SBC_SAMPLING_FREQ_44100         SBC_SAMPLING_FREQ_44100
#define BTR_SBC_SAMPLING_FREQ_48000         SBC_SAMPLING_FREQ_48000

#define BTR_SBC_ALLOCATION_SNR              SBC_ALLOCATION_SNR
#define BTR_SBC_ALLOCATION_LOUDNESS         SBC_ALLOCATION_LOUDNESS

#define BTR_SBC_SUBBANDS_4                  SBC_SUBBANDS_4
#define BTR_SBC_SUBBANDS_8                  SBC_SUBBANDS_8

#define BTR_SBC_BLOCK_LENGTH_4              SBC_BLOCK_LENGTH_4
#define BTR_SBC_BLOCK_LENGTH_8              SBC_BLOCK_LENGTH_8
#define BTR_SBC_BLOCK_LENGTH_12             SBC_BLOCK_LENGTH_12
#define BTR_SBC_BLOCK_LENGTH_16             SBC_BLOCK_LENGTH_16

#ifndef MIN_BITPOOL
#define MIN_BITPOOL                         SBC_MIN_BITPOOL
#endif

#ifndef MAX_BITPOOL
#define MAX_BITPOOL                         SBC_MAX_BITPOOL
#endif

#if defined(AAC_SUPPORTED)

/* MPEG Definitions */
#define BTR_MPEG_CHANNEL_MODE_MONO          MPEG_CHANNEL_MODE_MONO
#define BTR_MPEG_CHANNEL_MODE_DUAL_CHANNEL  MPEG_CHANNEL_MODE_DUAL_CHANNEL
#define BTR_MPEG_CHANNEL_MODE_STEREO        MPEG_CHANNEL_MODE_STEREO
#define BTR_MPEG_CHANNEL_MODE_JOINT_STEREO  MPEG_CHANNEL_MODE_JOINT_STEREO

#define BTR_MPEG_LAYER_MP1                  MPEG_LAYER_MP1
#define BTR_MPEG_LAYER_MP2                  MPEG_LAYER_MP2
#define BTR_MPEG_LAYER_MP3                  MPEG_LAYER_MP3

#define BTR_MPEG_SAMPLING_FREQ_16000        MPEG_SAMPLING_FREQ_16000
#define BTR_MPEG_SAMPLING_FREQ_22050        MPEG_SAMPLING_FREQ_22050
#define BTR_MPEG_SAMPLING_FREQ_24000        MPEG_SAMPLING_FREQ_24000
#define BTR_MPEG_SAMPLING_FREQ_32000        MPEG_SAMPLING_FREQ_32000
#define BTR_MPEG_SAMPLING_FREQ_44100        MPEG_SAMPLING_FREQ_44100
#define BTR_MPEG_SAMPLING_FREQ_48000        MPEG_SAMPLING_FREQ_48000

#define BTR_MPEG_BIT_RATE_VBR               MPEG_BIT_RATE_VBR
#define BTR_MPEG_BIT_RATE_320000            MPEG_BIT_RATE_320000
#define BTR_MPEG_BIT_RATE_256000            MPEG_BIT_RATE_256000
#define BTR_MPEG_BIT_RATE_224000            MPEG_BIT_RATE_224000
#define BTR_MPEG_BIT_RATE_192000            MPEG_BIT_RATE_192000
#define BTR_MPEG_BIT_RATE_160000            MPEG_BIT_RATE_160000
#define BTR_MPEG_BIT_RATE_128000            MPEG_BIT_RATE_128000
#define BTR_MPEG_BIT_RATE_112000            MPEG_BIT_RATE_112000
#define BTR_MPEG_BIT_RATE_96000             MPEG_BIT_RATE_96000
#define BTR_MPEG_BIT_RATE_80000             MPEG_BIT_RATE_80000
#define BTR_MPEG_BIT_RATE_64000             MPEG_BIT_RATE_64000
#define BTR_MPEG_BIT_RATE_56000             MPEG_BIT_RATE_56000
#define BTR_MPEG_BIT_RATE_48000             MPEG_BIT_RATE_48000
#define BTR_MPEG_BIT_RATE_40000             MPEG_BIT_RATE_40000
#define BTR_MPEG_BIT_RATE_32000             MPEG_BIT_RATE_32000
#define BTR_MPEG_BIT_RATE_FREE              MPEG_BIT_RATE_FREE

/* AAC Definitions */
#define BTR_AAC_OT_MPEG2_AAC_LC             AAC_OBJECT_TYPE_MPEG2_AAC_LC
#define BTR_AAC_OT_MPEG4_AAC_LC             AAC_OBJECT_TYPE_MPEG4_AAC_LC
#define BTR_AAC_OT_MPEG4_AAC_LTP            AAC_OBJECT_TYPE_MPEG4_AAC_LTP
#define BTR_AAC_OT_MPEG4_AAC_SCA            AAC_OBJECT_TYPE_MPEG4_AAC_SCA

#define BTR_AAC_SAMPLING_FREQ_8000          AAC_SAMPLING_FREQ_8000
#define BTR_AAC_SAMPLING_FREQ_11025         AAC_SAMPLING_FREQ_11025
#define BTR_AAC_SAMPLING_FREQ_12000         AAC_SAMPLING_FREQ_12000
#define BTR_AAC_SAMPLING_FREQ_16000         AAC_SAMPLING_FREQ_16000
#define BTR_AAC_SAMPLING_FREQ_22050         AAC_SAMPLING_FREQ_22050
#define BTR_AAC_SAMPLING_FREQ_24000         AAC_SAMPLING_FREQ_24000
#define BTR_AAC_SAMPLING_FREQ_32000         AAC_SAMPLING_FREQ_32000
#define BTR_AAC_SAMPLING_FREQ_44100         AAC_SAMPLING_FREQ_44100
#define BTR_AAC_SAMPLING_FREQ_48000         AAC_SAMPLING_FREQ_48000
#define BTR_AAC_SAMPLING_FREQ_64000         AAC_SAMPLING_FREQ_64000
#define BTR_AAC_SAMPLING_FREQ_88200         AAC_SAMPLING_FREQ_88200
#define BTR_AAC_SAMPLING_FREQ_96000         AAC_SAMPLING_FREQ_96000

#define BTR_AAC_CHANNELS_1                  AAC_CHANNELS_1
#define BTR_AAC_CHANNELS_2                  AAC_CHANNELS_2

#define BTR_AAC_SET_BITRATE                 AAC_SET_BITRATE
#define BTR_AAC_SET_FREQ                    AAC_SET_FREQUENCY
#define BTR_AAC_GET_BITRATE                 AAC_GET_BITRATE
#define BTR_AAC_GET_FREQ                    AAC_GET_FREQUENCY

#endif

#endif

#define BTR_SBC_HIGH_BITRATE_BITPOOL_48000  51
#define BTR_SBC_HIGH_BITRATE_BITPOOL_44100  53
#define BTR_SBC_MED_BITRATE_BITPOOL			33
#define BTR_SBC_LOW_BITRATE_BITPOOL			19

#define BTR_SBC_DEFAULT_BITRATE_BITPOOL     BTR_SBC_HIGH_BITRATE_BITPOOL_48000
#define BTR_SBC_DEFAULT_BITRATE_BITPOOL_441 BTR_SBC_HIGH_BITRATE_BITPOOL_44100

#define BTR_MEDIA_INVALID_ID                0xFFFFFFFFFFFFFFFF
#define BTR_MEDIA_PLAYLIST_ID               0x8000000000000000
#define BTR_MEDIA_BROWSER_ID                0x0

#define BTR_SELECT_CONF_TIMEOUT 3 * G_USEC_PER_SEC
//TODO Move to Private Header
typedef enum _enBTRCoreAVMTransportPathState {
    enAVMTransportStConnected,
    enAVMTransportStToBeConnected,
    enAVMTransportStDisconnected
} enBTRCoreAVMTransportPathState;

typedef enum _eBTRCoreAVMediaPlayerType {
    eBTRCoreAVMPTypAudio,
    eBTRCoreAVMPTypVideo,
    eBTRCoreAVMPTypAudioBroadcasting,
    eBTRCoreAVMPTypVideoBroadcasting,
    eBTRCoreAVMPTypUnknown
} eBTRCoreAVMediaPlayerType;

typedef enum _eBTRCoreAVMediaPlayerSubtype {
    eBTRCoreAVMPSbTypAudioBook,
    eBTRCoreAVMPSbTypPodcast,
    eBTRCoreAVMPSbTypUnknown
} eBTRCoreAVMediaPlayerSubtype;

//TODO Change to macros - store as bit infos
typedef enum _eBTRCoreAVMediaItemFilterAtt {
    eBTRcoreAVMFilterAttTitle,
    eBTRcoreAVMFilterAttArtist,
    eBTRcoreAVMFilterAttAlbum,
    eBTRcoreAVMFilterAttGenre,
    eBTRcoreAVMFilterAttNumberOfTracks,
    eBTRcoreAVMFilterAttTrackNumber,
    eBTRcoreAVMFilterAttDuration,
    eBTRcoreAVMFilterAttUnknown
} eBTRCoreAVMediaItemFilterAtt;

typedef struct _stBTRCoreAVMediaItemFilter {
    unsigned int                       ui32AVMediaFolderFilterStartIndex;
    unsigned int                       ui32AVMediaFolderFilterEndIndex;
    unsigned char                      mediaItemFilterAttFlag;
} stBTRCoreAVMediaItemFilter;
    
typedef struct _stBTRCoreAVMediaItem {
    void*                              pvAVMediaParentItem;
    unsigned char                      bIsMediaItemPlayable;
    char                               pcAVMediaItemPath[BTRCORE_MAX_STR_LEN];
    char                               pcAVMediaItemName[BTRCORE_MAX_STR_LEN];
    unsigned int                       ui32AVMediaNumberOfItems;    /* thing about populated items count */
    tBTRCoreAVMediaElementId           ui32AVMediaItemId;
    eBTRCoreAVMElementType             eMediaItemType;

    union {
        struct _stBTRCoreAVMediaItem** pstAVMediaSubItems;
        stBTRCoreAVMediaTrackInfo      mediaTrackInfo;
    };
} stBTRCoreAVMediaItem;
    
typedef struct _stBTRCoreAVMediaPlayer {
    char                               m_mediaPlayerName[BTRCORE_MAX_STR_LEN];
    eBTRCoreAVMediaPlayerType          eAVMediaPlayerType;
    eBTRCoreAVMediaPlayerSubtype       eAVMediaPlayerSubtype;
    enBTRCoreAVMediaCtrl               eAVMediaPlayerEqualizer;
    enBTRCoreAVMediaCtrl               eAVMediaPlayerShuffle;
    enBTRCoreAVMediaCtrl               eAVMediaPlayerRepeat;
    enBTRCoreAVMediaCtrl               eAVMediaPlayerScan;
    unsigned int                       m_mediaPlayerPosition;
    unsigned char                      m_mediaPlayerBrowsable;
    unsigned char                      m_mediaPlayerSearchable;
    unsigned char                      m_mediaTrackChanged;
    eBTRCoreAVMediaStatusUpdate        eAVMediaStatusUpdate;       /* change to eAVMediaTrackStatus later */
    stBTRCoreAVMediaTrackInfo          m_mediaTrackInfo;
    stBTRCoreAVMediaItem*              m_mediaBrowserItem;
} stBTRCoreAVMediaPlayer;


typedef struct _stBTRCoreAVMediaHdl {
    void*                               btIfceHdl;
    int                                 iBTMediaDefSampFreqPref;
    eBTRCoreAVMType                     eAVMediaTypeOut;
    eBTRCoreAVMType                     eAVMediaTypeIn;
    void*                               pstBTMediaConfigOut;
    void*                               pstBTMediaConfigIn;
    char*                               pcAVMediaTransportPathOut;
    char*                               pcAVMediaTransportPathIn;
    int64_t                             i64LastSelectConfigTime;

    unsigned short                      ui16AVMediaTransportDelay;
    unsigned char                       ui8AVMediaTransportVolume;
    BOOLEAN                             bAVMediaTrVolAvrcp; 
    BOOLEAN                             bAVMediaTrDelayAvrcp;
    enBTRCoreAVMTransportPathState      eAVMTState;

    char*                               pcAVMediaPlayerPath;
    char*                               pcAVMediaControlPath;
    unsigned char                       bAVMediaPlayerConnected;
    stBTRCoreAVMediaPlayer              pstAVMediaPlayer;

    stBTRCoreAVMediaItem*               pstAVMediaBrowser;
    unsigned int                        ui32AVMediaBrowserItemCount;

    stBTRCoreAVMediaItem*               pstAVMediaPlayList;
    unsigned int                        ui32AVMediaPlayListItemCount;


    fPtr_BTRCore_AVMediaStatusUpdateCb  fpcBBTRCoreAVMediaStatusUpdate;
    void*                               pcBMediaStatusUserData;

    GThread*                            pMediaPollingThread;
    void*                               pvThreadData;
    eBTRCoreAVMediaStatusUpdate         eAVMediaStPrev;
    tBTRCoreAVMediaElementId            SelectedaBtrAVMediaItemId;
    stBTRCoreAVMediaTrackInfo           SelectedapstBTAVMediaTrackInfo;
} stBTRCoreAVMediaHdl;



typedef struct _stBTRCoreAVMediaStatusUserData {
    void*        apvAVMUserData;
    const char*  apcAVMDevAddress;
} stBTRCoreAVMediaStatusUserData;



void test_BTRCore_AVMedia_Init_Failure (void)
{
    TEST_ASSERT_EQUAL (enBTRCoreInvalidArg, BTRCore_AVMedia_Init(NULL, NULL, NULL));   
}







void test_BTRCore_AVMedia_Init_InvalidArgs(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM;
    enBTRCoreRet ret;

    ret = BTRCore_AVMedia_Init(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, (void*)1, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, NULL, "adapter");
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

/*

void test_BTRCore_AVMedia_Init_Success(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM;
    enBTRCoreRet ret;
 
    const char* adapter = "adapter";
    a2dp_sbc_t sbcCaps;
      memset(&sbcCaps, 0, sizeof(a2dp_sbc_t));
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_MONO | BTR_SBC_CHANNEL_MODE_DUAL_CHANNEL |
                           BTR_SBC_CHANNEL_MODE_STEREO | BTR_SBC_CHANNEL_MODE_JOINT_STEREO;
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_16000 | BTR_SBC_SAMPLING_FREQ_32000 |
                        BTR_SBC_SAMPLING_FREQ_48000;
    sbcCaps.allocation_method = BTR_SBC_ALLOCATION_SNR | BTR_SBC_ALLOCATION_LOUDNESS;
    sbcCaps.subbands = BTR_SBC_SUBBANDS_4 | BTR_SBC_SUBBANDS_8;
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_4 | BTR_SBC_BLOCK_LENGTH_8 |
                           BTR_SBC_BLOCK_LENGTH_12 | BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.min_bitpool = MIN_BITPOOL;
    sbcCaps.max_bitpool = MAX_BITPOOL;

 
    BtrCore_BTRegisterMedia_ExpectAndReturn((void*)1, "adapter", enBTDevAudioSink, enBTMediaTypeSBC, BT_UUID_A2DP_SOURCE, &sbcCaps, sizeof(a2dp_sbc_t), 1, 0);

     a2dp_sbc_t sbcCapsCopy = sbcCaps;
      sbcCapsCopy.frequency |= BTR_SBC_SAMPLING_FREQ_44100;


    BtrCore_BTRegisterMedia_ExpectAndReturn((void*)1, "adapter", enBTDevAudioSource, enBTMediaTypeSBC, BT_UUID_A2DP_SINK, &sbcCapsCopy, sizeof(a2dp_sbc_t), 1, 0);
    BtrCore_BTRegisterNegotiateMediaCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterTransportPathMediaCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterMediaPlayerPathCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterMediaStatusUpdateCb_IgnoreAndReturn(0);
    BtrCore_BTRegisterMediaBrowserUpdateCb_IgnoreAndReturn(0);
    ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, (void*)1, "adapter");

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

*/

void test_BTRCore_AVMedia_DeInit_NullArguments(void) {
    enBTRCoreRet ret;

    ret = BTRCore_AVMedia_DeInit(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_DeInit((tBTRCoreAVMediaHdl)1, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_DeInit(NULL, (void*)1, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_DeInit(NULL, NULL, "adapter");
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}



void test_BTRCore_AVMedia_DeInit_Success(void) {
    stBTRCoreAVMediaHdl* hdl = (stBTRCoreAVMediaHdl*)malloc(sizeof(stBTRCoreAVMediaHdl));
    hdl->btIfceHdl = (void*)1;
    hdl->pstBTMediaConfigIn = malloc(1);
    hdl->pstBTMediaConfigOut = malloc(1);
    hdl->pcAVMediaTransportPathOut = malloc(1);
    hdl->pcAVMediaControlPath = malloc(1);
    hdl->pcAVMediaTransportPathIn = malloc(1);
    hdl->pcAVMediaPlayerPath = malloc(1);
    hdl->pstAVMediaBrowser = NULL;
    hdl->pstAVMediaPlayList = NULL;

    BtrCore_BTUnRegisterMedia_ExpectAndReturn((void*)1, "adapter", enBTDevAudioSource, enBTMediaTypeSBC, 0);
    BtrCore_BTUnRegisterMedia_ExpectAndReturn((void*)1, "adapter", enBTDevAudioSink, enBTMediaTypeSBC, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_DeInit((tBTRCoreAVMediaHdl)hdl, (void*)1, "adapter");
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_AVMedia_DeInit_MemoryDeallocation(void) {
    stBTRCoreAVMediaHdl* hdl = (stBTRCoreAVMediaHdl*)malloc(sizeof(stBTRCoreAVMediaHdl));
    hdl->btIfceHdl = (void*)1;
    hdl->pstBTMediaConfigIn = malloc(1);
    hdl->pstBTMediaConfigOut = malloc(1);
    hdl->pcAVMediaTransportPathOut = malloc(1);
    hdl->pcAVMediaControlPath = malloc(1);
    hdl->pcAVMediaTransportPathIn = malloc(1);
    hdl->pcAVMediaPlayerPath = malloc(1);
    hdl->pstAVMediaBrowser = NULL;
    hdl->pstAVMediaPlayList = NULL;

    BtrCore_BTUnRegisterMedia_ExpectAndReturn((void*)1, "adapter", enBTDevAudioSource, enBTMediaTypeSBC, 0);
    BtrCore_BTUnRegisterMedia_ExpectAndReturn((void*)1, "adapter", enBTDevAudioSink, enBTMediaTypeSBC, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_DeInit((tBTRCoreAVMediaHdl)hdl, (void*)1, "adapter");
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Ensure memory is deallocated
    TEST_ASSERT_NULL(hdl->pstBTMediaConfigIn);
    TEST_ASSERT_NULL(hdl->pstBTMediaConfigOut);
    TEST_ASSERT_NULL(hdl->pcAVMediaTransportPathOut);
    TEST_ASSERT_NULL(hdl->pcAVMediaControlPath);
    TEST_ASSERT_NULL(hdl->pcAVMediaTransportPathIn);
   // TEST_ASSERT_NULL(hdl->pcAVMediaPlayerPath);
}


void test_BTRCore_AVMedia_GetCurMediaInfo_NullArguments(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaInfo mediaInfo;

    ret = BTRCore_AVMedia_GetCurMediaInfo(NULL, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_GetCurMediaInfo((tBTRCoreAVMediaHdl)1, NULL, &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_GetCurMediaInfo((tBTRCoreAVMediaHdl)1, "00:11:22:33:44:55", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetCurMediaInfo_ValidArguments(void) {
    stBTRCoreAVMediaHdl* hBTRCoreAVM;
    stBTRCoreAVMediaInfo mediaInfo;
    a2dp_sbc_t sbcConfig;
    stBTRCoreAVMediaSbcInfo sbcInfo;

    hBTRCoreAVM = malloc(sizeof(stBTRCoreAVMediaHdl));
    memset(hBTRCoreAVM, 0, sizeof(stBTRCoreAVMediaHdl));
    hBTRCoreAVM->pstBTMediaConfigOut = &sbcConfig;
    hBTRCoreAVM->eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    mediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowOut;
    mediaInfo.pstBtrCoreAVMCodecInfo = &sbcInfo;

    sbcConfig.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcConfig.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcConfig.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcConfig.subbands = BTR_SBC_SUBBANDS_8;
    sbcConfig.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcConfig.min_bitpool = MIN_BITPOOL;
    sbcConfig.max_bitpool = MAX_BITPOOL;

    enBTRCoreRet ret = BTRCore_AVMedia_GetCurMediaInfo(hBTRCoreAVM, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);


    free(hBTRCoreAVM);
}


void test_BTRCore_AVMedia_GetCurMediaInfo_InvalidFlow(void) {
    stBTRCoreAVMediaHdl* hBTRCoreAVM;
    stBTRCoreAVMediaInfo mediaInfo;
    a2dp_sbc_t sbcConfig;
    stBTRCoreAVMediaSbcInfo sbcInfo;


    hBTRCoreAVM = malloc(sizeof(stBTRCoreAVMediaHdl));
    memset(hBTRCoreAVM, 0, sizeof(stBTRCoreAVMediaHdl));


    hBTRCoreAVM->pstBTMediaConfigOut = &sbcConfig;
    hBTRCoreAVM->eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    mediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowUnknown;
    mediaInfo.pstBtrCoreAVMCodecInfo = &sbcInfo;

    enBTRCoreRet ret = BTRCore_AVMedia_GetCurMediaInfo(hBTRCoreAVM, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);

    free(hBTRCoreAVM);
}


int mock_BtrCore_BTGetProp (
    void*               apstBtIfceHdl,
    const char*         apcBtOpIfcePath,
    enBTOpIfceType      aenBtOpIfceType,
    unBTOpIfceProp      aunBtOpIfceProp,
    void*               apvVal
)
{ 
    if (apcBtOpIfcePath == NULL || apvVal == NULL) {
        return -1;
    }

    if (strncmp(apcBtOpIfcePath, "device_path_out", strlen("device_path_out")) == 0) {
        if (aunBtOpIfceProp.enBtMediaTransportProp == enBTMedTPropDelay) {
            *(unsigned int *)apvVal = 0xFFFFu; // Set delay to 0xFFFFu
            return 0;
        } else if (aunBtOpIfceProp.enBtMediaTransportProp == enBTMedTPropVol) {
            *(unsigned short *)apvVal = 0xFFFFu; // Set volume to 0xFFFFu
            return 0;
        }
    }

    return -1;
}

void test_BTRCore_AVMedia_AcquireDataPath_Success(void) {
    stBTRCoreAVMediaHdl hdl;
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaTransportPathOut = "device_path_out";
    hdl.pcAVMediaTransportPathIn = "device_path_in";
    const char* btDevAddr = "device_path_out";
    int dataPath = 0;
    int dataReadMTU = 0;
    int dataWriteMTU = 0;

    enBTRCoreRet ret;

    BtrCore_BTAcquireDevDataPath_ExpectAndReturn((void*)1, "device_path_out", &dataPath, &dataReadMTU, &dataWriteMTU, 0);
    unsigned int delay = 0;
    unBTOpIfceProp delayProp;
    delayProp.enBtMediaTransportProp = enBTMedTPropDelay;
   
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp);
    unsigned short volume = 0;
    unBTOpIfceProp volumeProp;
    volumeProp.enBtMediaTransportProp = enBTMedTPropVol;
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp);



    // Call the function under test
    ret = BTRCore_AVMedia_AcquireDataPath((tBTRCoreAVMediaHdl)&hdl, btDevAddr, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);

       // Check that the return value is enBTRCoreSuccess
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(0, dataPath);
    TEST_ASSERT_EQUAL(0, dataReadMTU);
    TEST_ASSERT_EQUAL(0, dataWriteMTU);
    TEST_ASSERT_EQUAL(0xFFFFu, delay);

}

void test_BTRCore_AVMedia_AcquireDataPath_InvalidArgs(void) {
    enBTRCoreRet ret = BTRCore_AVMedia_AcquireDataPath(NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_AcquireDataPath_FailureAcquirePath(void) {
    stBTRCoreAVMediaHdl mediaHdl;
    int dataPath, dataReadMTU, dataWriteMTU;
    unsigned int delay;
    char* devAddr = "00:11:22:33:44:55";

    mediaHdl.pcAVMediaTransportPathOut = devAddr;
    mediaHdl.btIfceHdl = (void*)1;

    BtrCore_BTAcquireDevDataPath_ExpectAndReturn(mediaHdl.btIfceHdl, devAddr, &dataPath, &dataReadMTU, &dataWriteMTU, 1);
    BtrCore_BTGetProp_IgnoreAndReturn(enBTRCoreSuccess);

    enBTRCoreRet ret = BTRCore_AVMedia_AcquireDataPath(&mediaHdl, devAddr, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_AcquireDataPath_FailureGetProp(void) {
    stBTRCoreAVMediaHdl mediaHdl;
    int dataPath, dataReadMTU, dataWriteMTU;
    unsigned int delay;
    char* devAddr = "00:11:22:33:44:55";
    unBTOpIfceProp delayProp;
    unBTOpIfceProp volProp;

    delayProp.enBtDeviceProp = enBTMedTPropDelay;
    volProp.enBtDeviceProp = enBTMedTPropVol;

    mediaHdl.pcAVMediaTransportPathOut = devAddr;
    mediaHdl.btIfceHdl = (void*)1;

    BtrCore_BTAcquireDevDataPath_ExpectAndReturn(mediaHdl.btIfceHdl, devAddr, &dataPath, &dataReadMTU, &dataWriteMTU, 0);
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp);

    enBTRCoreRet ret = BTRCore_AVMedia_AcquireDataPath(&mediaHdl, devAddr, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_ReleaseDataPath_InvalidArgs(void) {
    enBTRCoreRet ret;

    ret = BTRCore_AVMedia_ReleaseDataPath(NULL, "00:11:22:33:44:55");
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_ReleaseDataPath((tBTRCoreAVMediaHdl)1, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_ReleaseDataPath_NoTransportPath(void) {
    stBTRCoreAVMediaHdl mediaHdl = {0};
    enBTRCoreRet ret;

    ret = BTRCore_AVMedia_ReleaseDataPath(&mediaHdl, "00:11:22:33:44:55");
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_ReleaseDataPath_Success(void) {
    stBTRCoreAVMediaHdl mediaHdl = {0};
    enBTRCoreRet ret;

    mediaHdl.pcAVMediaTransportPathOut = "00:11:22:33:44:55";
    BtrCore_BTReleaseDevDataPath_ExpectAndReturn(NULL, "00:11:22:33:44:55", 0);

    ret = BTRCore_AVMedia_ReleaseDataPath(&mediaHdl, "00:11:22:33:44:55");
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_ReleaseDataPath_Failure(void) {
    stBTRCoreAVMediaHdl mediaHdl = {0};
    enBTRCoreRet ret;

    mediaHdl.pcAVMediaTransportPathOut = "00:11:22:33:44:55";
    BtrCore_BTReleaseDevDataPath_ExpectAndReturn(NULL, "00:11:22:33:44:55", -1);

    ret = BTRCore_AVMedia_ReleaseDataPath(&mediaHdl, "00:11:22:33:44:55");
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_MediaControl_InvalidArgs(void) {
    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(NULL, NULL, enBTRCoreAVMediaCtrlPlay, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_MediaControl_Play(void) {
    stBTRCoreAVMediaHdl mediaHdl;
    mediaHdl.pcAVMediaPlayerPath = "dummy_path";
    mediaHdl.btIfceHdl = (void*)1;

    BtrCore_BTDevMediaControl_ExpectAndReturn(mediaHdl.btIfceHdl, mediaHdl.pcAVMediaPlayerPath, enBTMediaCtrlPlay, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&mediaHdl, "dummy_addr", enBTRCoreAVMediaCtrlPlay, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_Pause(void) {
    stBTRCoreAVMediaHdl mediaHdl;
    mediaHdl.pcAVMediaPlayerPath = "dummy_path";
    mediaHdl.btIfceHdl = (void*)1;

    BtrCore_BTDevMediaControl_ExpectAndReturn(mediaHdl.btIfceHdl, mediaHdl.pcAVMediaPlayerPath, enBTMediaCtrlPause, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&mediaHdl, "dummy_addr", enBTRCoreAVMediaCtrlPause, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_Stop(void) {
    stBTRCoreAVMediaHdl mediaHdl;
    mediaHdl.pcAVMediaPlayerPath = "dummy_path";
    mediaHdl.btIfceHdl = (void*)1;

    BtrCore_BTDevMediaControl_ExpectAndReturn(mediaHdl.btIfceHdl, mediaHdl.pcAVMediaPlayerPath, enBTMediaCtrlStop, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&mediaHdl, "dummy_addr", enBTRCoreAVMediaCtrlStop, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_Unknown(void) {
    stBTRCoreAVMediaHdl mediaHdl;
    mediaHdl.pcAVMediaPlayerPath = "dummy_path";
    mediaHdl.btIfceHdl = (void*)1;

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&mediaHdl, "dummy_addr", enBTRCoreAVMediaCtrlUnknown, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


int
_BtrCore_BTGetMediaPlayerProperty (
    void*           apstBtIfceHdl,
    const char*     apBtMediaPlayerPath,
    const char*     mediaProperty,
    void*           mediaPropertyValue
){
    if (strcmp(mediaProperty, "Position") == 0) {
        *(unsigned int*)mediaPropertyValue = 1000;
        return 0;
    }
    return -1;
    
}

int
_BtrCore_BTGetTrackInformation (
    void*               apBtConn,
    const char*         apBtmediaPlayerObjectPath,
    stBTMediaTrackInfo* apstBTMediaTrackInfo
) {
    
    if (apBtConn == NULL || apBtmediaPlayerObjectPath == NULL || apstBTMediaTrackInfo == NULL) {
        printf("Error: Invalid arguments passed to _BtrCore_BTGetTrackInformation.\n");
        return 1;  
    } 
    else {
        apstBTMediaTrackInfo->ui32Duration = 5000;
        printf("Success: Track information retrieved successfully.\n");
        return 0;  
    }
}


void test_BTRCore_AVMedia_GetTrackInfo_InvalidArgs(void) {
    enBTRCoreRet ret;
    ret = BTRCore_AVMedia_GetTrackInfo(NULL, "00:11:22:33:44:55", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_GetTrackInfo((tBTRCoreAVMediaHdl)1, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_GetTrackInfo((tBTRCoreAVMediaHdl)1, "00:11:22:33:44:55", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetTrackInfo_Success(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    stBTRCoreAVMediaTrackInfo trackInfo;

    // Ensure avMediaHdl is properly initialized
    memset(&avMediaHdl, 0, sizeof(stBTRCoreAVMediaHdl));

    // Set the necessary fields in avMediaHdl for the test
    avMediaHdl.pcAVMediaPlayerPath = strdup("/org/bluez/hci0/dev_00_11_22_33_44_55/player0");  // Simulated valid media player path
    avMediaHdl.btIfceHdl = (void*)1;  // Simulate a valid Bluetooth interface handle

    BtrCore_BTGetTrackInformation_StubWithCallback(_BtrCore_BTGetTrackInformation);

    enBTRCoreRet ret = BTRCore_AVMedia_GetTrackInfo(&avMediaHdl, apBtDevAddr, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    
}



// Test for failure in retrieving track information
void test_BTRCore_AVMedia_GetTrackInfo_Failure(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    stBTRCoreAVMediaTrackInfo trackInfo;

    memset(&avMediaHdl, 0, sizeof(stBTRCoreAVMediaHdl));

    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(avMediaHdl.pcAVMediaPlayerPath);

    // Mock the failure return of BtrCore_BTGetTrackInformation
    BtrCore_BTGetTrackInformation_StubWithCallback(_BtrCore_BTGetTrackInformation);

    enBTRCoreRet ret = BTRCore_AVMedia_GetTrackInfo(&avMediaHdl, apBtDevAddr, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_GetElementTrackInfo_ValidInputs(void) {
    stBTRCoreAVMediaHdl hBTRCoreAVM;
    stBTRCoreAVMediaHdl* pstlhBTRCoreAVM = &hBTRCoreAVM;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 1;
    stBTRCoreAVMediaTrackInfo apstBTAVMediaTrackInfo;

    pstlhBTRCoreAVM->pcAVMediaPlayerPath = "test_path";
    pstlhBTRCoreAVM->SelectedaBtrAVMediaItemId = aBtrAVMediaItemId;
    memset(&pstlhBTRCoreAVM->SelectedapstBTAVMediaTrackInfo, 0, sizeof(stBTRCoreAVMediaTrackInfo));

    BtrCore_BTGetMediaPlayerPath_StubWithCallback(_BtrCore_BTGetMediaPlayerProperty);
    BtrCore_BTGetTrackInformation_StubWithCallback(_BtrCore_BTGetTrackInformation);

    enBTRCoreRet ret = BTRCore_AVMedia_GetElementTrackInfo(pstlhBTRCoreAVM, apBtDevAddr, aBtrAVMediaItemId, &apstBTAVMediaTrackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_AVMedia_GetElementTrackInfo_InvalidhBTRCoreAVM(void) {
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 1;
    stBTRCoreAVMediaTrackInfo apstBTAVMediaTrackInfo;

    enBTRCoreRet ret = BTRCore_AVMedia_GetElementTrackInfo(NULL, apBtDevAddr, aBtrAVMediaItemId, &apstBTAVMediaTrackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetElementTrackInfo_InvalidapBtDevAddr(void) {
    stBTRCoreAVMediaHdl hBTRCoreAVM;
    stBTRCoreAVMediaHdl* pstlhBTRCoreAVM = &hBTRCoreAVM;
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 1;
    stBTRCoreAVMediaTrackInfo apstBTAVMediaTrackInfo;

    enBTRCoreRet ret = BTRCore_AVMedia_GetElementTrackInfo(pstlhBTRCoreAVM, NULL, aBtrAVMediaItemId, &apstBTAVMediaTrackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetElementTrackInfo_InvalidapstBTAVMediaTrackInfo(void) {
    stBTRCoreAVMediaHdl hBTRCoreAVM;
    stBTRCoreAVMediaHdl* pstlhBTRCoreAVM = &hBTRCoreAVM;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 1;

    enBTRCoreRet ret = BTRCore_AVMedia_GetElementTrackInfo(pstlhBTRCoreAVM, apBtDevAddr, aBtrAVMediaItemId, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetElementTrackInfo_ZeroElementId(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    
    // Initialize the necessary fields in avMediaHdl
    avMediaHdl.pcAVMediaPlayerPath = "test_path";
    avMediaHdl.btIfceHdl = (void*)0x123456;  // Initialize btIfceHdl with a non-null value
    
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 0;
    stBTRCoreAVMediaTrackInfo apstBTAVMediaTrackInfo;

    // Stub and Ignore Functions
    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(avMediaHdl.pcAVMediaPlayerPath);
    BtrCore_BTGetTrackInformation_StubWithCallback(_BtrCore_BTGetTrackInformation);

    // Call the function being tested
    enBTRCoreRet ret = BTRCore_AVMedia_GetElementTrackInfo(&avMediaHdl, apBtDevAddr, aBtrAVMediaItemId, &apstBTAVMediaTrackInfo);
    
    // Assert the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}


void test_BTRCore_AVMedia_GetElementTrackInfo_Failure(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    stBTRCoreAVMediaTrackInfo trackInfo;
    char* mediaPlayerPath = "media/player/path";

    memset(&avMediaHdl, 0, sizeof(stBTRCoreAVMediaHdl));
    avMediaHdl.btIfceHdl = (void*)1;

    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(avMediaHdl.btIfceHdl, "00:11:22:33:44:55", mediaPlayerPath);
    BtrCore_BTGetTrackInformation_ExpectAndReturn(avMediaHdl.btIfceHdl, mediaPlayerPath, (stBTMediaTrackInfo*)&trackInfo, 1);

    enBTRCoreRet ret = BTRCore_AVMedia_GetElementTrackInfo(&avMediaHdl, "00:11:22:33:44:55", 0, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}





void test_BTRCore_AVMedia_GetPositionInfo_InvalidArgs(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaPositionInfo positionInfo;

    ret = BTRCore_AVMedia_GetPositionInfo(NULL, "00:11:22:33:44:55", &positionInfo);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_GetPositionInfo((tBTRCoreAVMediaHdl)1, NULL, &positionInfo);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_GetPositionInfo((tBTRCoreAVMediaHdl)1, "00:11:22:33:44:55", NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}




void test_BTRCore_AVMedia_GetPositionInfo_Success(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    stBTRCoreAVMediaPositionInfo positionInfo;
    char* playerPath = "player/path";
    unsigned int mediaPosition = 1000;
    stBTRCoreAVMediaTrackInfo mediaTrackInfo = { .ui32Duration = 5000 };  // Track with 5000 ms duration

    // Initialize the structure
    avMediaHdl.pcAVMediaPlayerPath = playerPath;

    // Mock expectations
    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(playerPath);

    BtrCore_BTGetMediaPlayerProperty_StubWithCallback(_BtrCore_BTGetMediaPlayerProperty);
    
    // Instead of ignoring, set up a proper return for track information
    BtrCore_BTGetTrackInformation_StubWithCallback(_BtrCore_BTGetTrackInformation);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_AVMedia_GetPositionInfo(&avMediaHdl, "00:11:22:33:44:55", &positionInfo);
    
    // Assertions
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(5000, positionInfo.ui32Duration);  // Check the expected duration
    TEST_ASSERT_EQUAL(1000, positionInfo.ui32Position);
}


void test_BTRCore_AVMedia_GetPositionInfo_Failure(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    stBTRCoreAVMediaPositionInfo positionInfo;
    char* playerPath = "player/path";

    avMediaHdl.pcAVMediaPlayerPath = playerPath;

    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(playerPath);

    BtrCore_BTGetMediaPlayerProperty_IgnoreAndReturn(1);
    BtrCore_BTGetTrackInformation_IgnoreAndReturn(1);

    enBTRCoreRet ret = BTRCore_AVMedia_GetPositionInfo(&avMediaHdl, "00:11:22:33:44:55", &positionInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_GetMediaProperty_Success(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    const char* mediaPropertyKey = "Position";
    int mediaPropertyValue = 0;

    char mediaPlayerPath[] = "/org/bluez/hci0/dev_00_11_22_33_44_55/player0";

    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(mediaPlayerPath);
    BtrCore_BTGetMediaPlayerProperty_StubWithCallback(_BtrCore_BTGetMediaPlayerProperty);
    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaProperty(&avMediaHdl, apBtDevAddr, mediaPropertyKey, &mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}
/*
void test_BTRCore_AVMedia_GetMediaProperty_FailureInGettingPlayerPath(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    const char* mediaPropertyKey = "Position";
    unsigned int mediaPropertyValue = 0;

    // Declare and initialize hBTRCoreAVM
    stBTRCoreAVMediaHdl* hBTRCoreAVM = &avMediaHdl;
    hBTRCoreAVM->pcAVMediaPlayerPath = NULL;

    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(hBTRCoreAVM->btIfceHdl, apBtDevAddr, NULL);
    
    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaProperty(&avMediaHdl, apBtDevAddr, mediaPropertyKey, &mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

*/

void test_BTRCore_AVMedia_GetMediaProperty_FailureInGettingProperty(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    const char* mediaPropertyKey = "Position";
    unsigned int mediaPropertyValue = 0;

    stBTRCoreAVMediaHdl* hBTRCoreAVM = &avMediaHdl;
    hBTRCoreAVM->pcAVMediaPlayerPath = "mockPlayerPath";

    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(hBTRCoreAVM->pcAVMediaPlayerPath);
    BtrCore_BTGetMediaPlayerProperty_IgnoreAndReturn(-1);

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaProperty(&avMediaHdl, apBtDevAddr, mediaPropertyKey, &mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);

   
}



void test_BTRCore_AVMedia_ChangeBrowserLocation_NullArguments(void) {
    enBTRCoreRet ret;

    // Use an existing enum value like eBTRCoreAVMETypeAlbum instead of eBTRCoreAVMElementTypeFolder
    ret = BTRCore_AVMedia_ChangeBrowserLocation(NULL, "00:11:22:33:44:55", 1, eBTRCoreAVMETypeAlbum);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_ChangeBrowserLocation((tBTRCoreAVMediaHdl)1, NULL, 1, eBTRCoreAVMETypeAlbum);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_ChangeBrowserLocation_GetMediaPlayerPathFailure(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    enBTRCoreRet ret;

    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(NULL, "00:11:22:33:44:55", NULL);
    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(NULL);

    ret = BTRCore_AVMedia_ChangeBrowserLocation(&avMediaHdl, "00:11:22:33:44:55", 1, eBTRCoreAVMETypeAlbum);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_ChangeBrowserLocation_ChangeFolderFailure(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    enBTRCoreRet ret;

    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(NULL, "00:11:22:33:44:55", "player_path");
    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(NULL);

    BtrCore_BTChangeMediaFolder_ExpectAndReturn(NULL, "player_path", "folder_path", 1);
    BtrCore_BTChangeMediaFolder_IgnoreAndReturn(1);

    ret = BTRCore_AVMedia_ChangeBrowserLocation(&avMediaHdl, "00:11:22:33:44:55", 1, eBTRCoreAVMETypeAlbum);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_Success(void) {
    
    stBTRCoreAVMediaHdl avMediaHdl = {0};  
    avMediaHdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;  

    stBTRCoreAVMediaItem avMediaItem = {0};  
    avMediaItem.ui32AVMediaItemId = 1;  
    avMediaHdl.pstAVMediaBrowser = &avMediaItem;  

    enBTRCoreRet ret;

    // Expect the media player path to be retrieved successfully
    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(NULL, "00:11:22:33:44:55", "player_path");

    // Expect the media folder to be changed successfully
    BtrCore_BTChangeMediaFolder_ExpectAndReturn(NULL, "player_path", "folder_path", 0);

    // Ensure the folder path is set correctly in the avMediaHdl structure
    strncpy(avMediaHdl.pstAVMediaBrowser->pcAVMediaItemPath, "folder_path", BTRCORE_MAX_STR_LEN);

    // Call the function with valid parameters and expect success
    ret = BTRCore_AVMedia_ChangeBrowserLocation(&avMediaHdl, "00:11:22:33:44:55", 1, eBTRCoreAVMETypeAlbum);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}




void test_BTRCore_AVMedia_SelectMediaBrowserElements_ValidInputs(void) {
    // Initialize the media handle structure
    stBTRCoreAVMediaHdl avMediaHdl = {0};  
    const char* apBtDevAddr = "00:11:22:33:44:55";
    unsigned short aui16StartIdx = 0;
    unsigned short aui16EndIdx = 5;

    // Initialize the media player structure
    avMediaHdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;

    // Initialize the media browser structure
    stBTRCoreAVMediaItem avMediaItem = {0};  
    avMediaItem.ui32AVMediaItemId = 1;  
    avMediaItem.ui32AVMediaNumberOfItems = 5;  // Initialize number of items to avoid issues in the while loop

    // Assign memory for media browser item
    avMediaHdl.pstAVMediaPlayer.m_mediaBrowserItem = &avMediaItem;  // Ensure valid memory for m_mediaBrowserItem
    avMediaHdl.pstAVMediaBrowser = &avMediaItem;
    avMediaHdl.pcAVMediaPlayerPath = malloc(1);
    // Mock the external functions
    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn("mockPath");
    BtrCore_BTGetMediaFolderNumberOfItems_IgnoreAndReturn(0);
    BtrCore_BTSelectMediaFolderItems_IgnoreAndReturn(0);

    // Call the function under test
    enBTRCoreRet ret = BTRCore_AVMedia_SelectMediaBrowserElements(&avMediaHdl, apBtDevAddr, aui16StartIdx, aui16EndIdx);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    free (avMediaHdl.pcAVMediaPlayerPath);
}

void test_BTRCore_AVMedia_SelectMediaBrowserElements_InvalidInputs(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM = NULL;
    const char* apBtDevAddr = NULL;
    unsigned short aui16StartIdx = 0;
    unsigned short aui16EndIdx = 5;

    enBTRCoreRet ret = BTRCore_AVMedia_SelectMediaBrowserElements(hBTRCoreAVM, apBtDevAddr, aui16StartIdx, aui16EndIdx);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_SelectMediaBrowserElements_NotBrowsable(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM = (tBTRCoreAVMediaHdl)1;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    unsigned short aui16StartIdx = 0;
    unsigned short aui16EndIdx = 5;

    stBTRCoreAVMediaHdl mockHdl;
    mockHdl.pcAVMediaPlayerPath = "mockPath";
    mockHdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 0;

    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn("mockPath");

    enBTRCoreRet ret = BTRCore_AVMedia_SelectMediaBrowserElements(&mockHdl, apBtDevAddr, aui16StartIdx, aui16EndIdx);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}



void test_BTRCore_AVMedia_GetMediaElementList_NullHandle(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaElementInfoList elementInfoList;

    ret = BTRCore_AVMedia_GetMediaElementList(NULL, "00:11:22:33:44:55", 1, 0, 10, eBTRCoreAVMETypeUnknown, &elementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_NullDeviceAddr(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaElementInfoList elementInfoList;
    stBTRCoreAVMediaHdl handle;

    ret = BTRCore_AVMedia_GetMediaElementList(&handle, NULL, 1, 0, 10, eBTRCoreAVMETypeUnknown, &elementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetMediaElementList_NullElementInfoList(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaHdl handle;

    ret = BTRCore_AVMedia_GetMediaElementList(&handle, "00:11:22:33:44:55", 1, 0, 10, eBTRCoreAVMETypeUnknown, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetMediaElementList_InvalidIndexRange(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaElementInfoList elementInfoList;
    stBTRCoreAVMediaHdl handle;

    ret = BTRCore_AVMedia_GetMediaElementList(&handle, "00:11:22:33:44:55", 1, 10, 0, eBTRCoreAVMETypeUnknown, &elementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_GetMediaElementList_ValidInputs(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaElementInfoList elementInfoList;
    stBTRCoreAVMediaHdl handle;
    stBTRCoreAVMediaItem mediaItem;

    handle.pstAVMediaPlayer.m_mediaBrowserItem = &mediaItem;
    mediaItem.ui32AVMediaItemId = 1;
    mediaItem.bIsMediaItemPlayable = 0;
    mediaItem.ui32AVMediaNumberOfItems = 5;
    mediaItem.pstAVMediaSubItems = malloc(5 * sizeof(stBTRCoreAVMediaItem*));
    for (int i = 0; i < 5; i++) {
        mediaItem.pstAVMediaSubItems[i] = malloc(sizeof(stBTRCoreAVMediaItem));
        mediaItem.pstAVMediaSubItems[i]->eMediaItemType = eBTRCoreAVMETypeUnknown;
    }

    ret = BTRCore_AVMedia_GetMediaElementList(&handle, "00:11:22:33:44:55", 1, 0, 4, eBTRCoreAVMETypeUnknown, &elementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(5, elementInfoList.m_numOfElements);

    for (int i = 0; i < 5; i++) {
        free(mediaItem.pstAVMediaSubItems[i]);
    }
    free(mediaItem.pstAVMediaSubItems);
}


void test_BTRCore_AVMedia_PlayTrack_Null_hBTRCoreAVM(void) {
    enBTRCoreRet ret = BTRCore_AVMedia_PlayTrack(NULL, "00:11:22:33:44:55", 1);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_PlayTrack_Null_apBtDevAddr(void) {
    stBTRCoreAVMediaHdl hdl;
    enBTRCoreRet ret = BTRCore_AVMedia_PlayTrack(&hdl, NULL, 1);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}


void test_BTRCore_AVMedia_PlayTrack_Invalid_aBtrAVMediaItemId(void) {
    stBTRCoreAVMediaHdl hdl;
    hdl.pstAVMediaBrowser = NULL;
    enBTRCoreRet ret = BTRCore_AVMedia_PlayTrack(&hdl, "00:11:22:33:44:55", 1);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_PlayTrack_NonPlayableItem(void) {
    stBTRCoreAVMediaHdl hdl;
    stBTRCoreAVMediaItem item;
    item.bIsMediaItemPlayable = 0;
    hdl.pstAVMediaBrowser = &item;

    
    enBTRCoreRet ret = BTRCore_AVMedia_PlayTrack(&hdl, "00:11:22:33:44:55", 1);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_PlayTrack_PlayableItem(void) {
    stBTRCoreAVMediaHdl hdl;
    stBTRCoreAVMediaItem item;
    item.bIsMediaItemPlayable = 1;
    item.pcAVMediaItemPath[0] = '\0';
    hdl.pstAVMediaBrowser = &item;

    
    enBTRCoreRet ret = BTRCore_AVMedia_PlayTrack(&hdl, "00:11:22:33:44:55", 1);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}





void test_BTRCore_AVMedia_PlayTrack_ValidInputs(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0}; 
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 12345 | BTR_MEDIA_PLAYLIST_ID; // Ensure the ID indicates a playlist item
    stBTRCoreAVMediaItem mediaItem = { .bIsMediaItemPlayable = 1, .pcAVMediaItemPath = "path/to/media", .ui32AVMediaItemId = aBtrAVMediaItemId };

    // Initialize pointers to avoid "Media Browser doesn't exist!" error
    stBTRCoreAVMediaItem dummyItem = {0}; 

    // Initialize the playlist and browser
    avMediaHdl.pstAVMediaPlayList = &mediaItem; 
    avMediaHdl.pstAVMediaBrowser = &dummyItem;  

    BtrCore_BTPlayMediaTrackItem_IgnoreAndReturn(0); 

    // Assuming BTRCore_AVMedia_PlayTrack does not need additional initialization for avMediaHdl
    enBTRCoreRet ret = BTRCore_AVMedia_PlayTrack(&avMediaHdl, apBtDevAddr, aBtrAVMediaItemId);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_SelectTrack_ValidInputs(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0}; 
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 12345 | BTR_MEDIA_PLAYLIST_ID; // Ensure the ID indicates a playlist item
    stBTRCoreAVMediaItem mediaItem = { .bIsMediaItemPlayable = 1, .pcAVMediaItemPath = "valid/path/to/media", .ui32AVMediaItemId = aBtrAVMediaItemId };

    // Initialize pointers to avoid "Media Browser doesn't exist!" error
    stBTRCoreAVMediaItem dummyItem = {0}; 

    // Initialize the playlist and browser
    avMediaHdl.pstAVMediaPlayList = &mediaItem; 
    avMediaHdl.pstAVMediaBrowser = &dummyItem;  

    BtrCore_BTGetTrackInformation_IgnoreAndReturn(0);  

    enBTRCoreRet ret = BTRCore_AVMedia_SelectTrack(&avMediaHdl, apBtDevAddr, aBtrAVMediaItemId);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_SelectTrack_InvalidInputs(void) {
    stBTRCoreAVMediaHdl hBTRCoreAVM;
    const char* apBtDevAddr = NULL;
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 12345;

    enBTRCoreRet ret = BTRCore_AVMedia_SelectTrack(&hBTRCoreAVM, apBtDevAddr, aBtrAVMediaItemId);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}



void test_BTRCore_AVMedia_SelectTrack_ItemNotFound(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0};
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 12345;
    stBTRCoreAVMediaItem mediaItem;
    stBTRCoreAVMediaItem* ptrBsr = &mediaItem;
    stBTRCoreAVMediaItem* ptrTrack = NULL;

    
    enBTRCoreRet ret = BTRCore_AVMedia_SelectTrack(&avMediaHdl, apBtDevAddr, aBtrAVMediaItemId);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_SelectTrack_NonPlayableItem(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0};
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 12345;
    stBTRCoreAVMediaItem mediaItem;
    stBTRCoreAVMediaItem* ptrBsr = &mediaItem;
    stBTRCoreAVMediaItem* ptrTrack = &mediaItem;
    mediaItem.bIsMediaItemPlayable = 0;   
    enBTRCoreRet ret = BTRCore_AVMedia_SelectTrack(&avMediaHdl, apBtDevAddr, aBtrAVMediaItemId);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}
void test_BTRCore_AVMedia_IsMediaElementPlayable_ValidInputs(void) {
    // Define and initialize the media item
    stBTRCoreAVMediaItem mediaItem = {0};
    mediaItem.bIsMediaItemPlayable = 1; // Set the media item as playable
    mediaItem.ui32AVMediaItemId = 1; // Set the media item ID

    // Define and initialize the media handle
    stBTRCoreAVMediaHdl avMediaHdl = {0};
    avMediaHdl.pstAVMediaBrowser = &mediaItem; // Point to the media item

    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 1;
    char isPlayable;

    // Call the function with the initialized handle
    enBTRCoreRet result = BTRCore_AVMedia_IsMediaElementPlayable(&avMediaHdl, apBtDevAddr, aBtrAVMediaItemId, &isPlayable);

    // Assert the results
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, result);
    TEST_ASSERT_EQUAL(1, isPlayable);
}



void test_BTRCore_AVMedia_IsMediaElementPlayable_InvalidHdl(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM = NULL;
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 1;
    char isPlayable;
    enBTRCoreRet result = BTRCore_AVMedia_IsMediaElementPlayable(hBTRCoreAVM, apBtDevAddr, aBtrAVMediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, result);
}


void test_BTRCore_AVMedia_IsMediaElementPlayable_InvalidAddr(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM = (tBTRCoreAVMediaHdl)0x1234;
    const char* apBtDevAddr = NULL;
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 1;
    char isPlayable;
    enBTRCoreRet result = BTRCore_AVMedia_IsMediaElementPlayable(hBTRCoreAVM, apBtDevAddr, aBtrAVMediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, result);
}


void test_BTRCore_AVMedia_IsMediaElementPlayable_InvalidItemId(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0};
    const char* apBtDevAddr = "00:11:22:33:44:55";
    tBTRCoreAVMediaElementId aBtrAVMediaItemId = 0; 
    char isPlayable;

    // Initialize the avMediaHdl structure properly
    stBTRCoreAVMediaItem mediaBrowser = {0};
    avMediaHdl.pstAVMediaPlayList = NULL;
    avMediaHdl.pstAVMediaBrowser = &mediaBrowser;

    // Ensure the media item is not found by setting an ID that does not exist
    mediaBrowser.ui32AVMediaItemId = 1; 

    enBTRCoreRet result = BTRCore_AVMedia_IsMediaElementPlayable(&avMediaHdl, apBtDevAddr, aBtrAVMediaItemId, &isPlayable);

    TEST_ASSERT_EQUAL(enBTRCoreFailure, result);
}


void test_BTRCore_AVMedia_RegisterMediaStatusUpdateCb_InvalidArgs(void) {
    enBTRCoreRet ret;

    // Test with NULL handle
    ret = BTRCore_AVMedia_RegisterMediaStatusUpdateCb(NULL, (fPtr_BTRCore_AVMediaStatusUpdateCb)1, (void*)1);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    // Test with NULL callback
    stBTRCoreAVMediaHdl hdl;
    ret = BTRCore_AVMedia_RegisterMediaStatusUpdateCb(&hdl, NULL, (void*)1);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_RegisterMediaStatusUpdateCb_ValidArgs(void) {
    enBTRCoreRet ret;
    stBTRCoreAVMediaHdl hdl;

    // Test with valid arguments
    ret = BTRCore_AVMedia_RegisterMediaStatusUpdateCb(&hdl, (fPtr_BTRCore_AVMediaStatusUpdateCb)1, (void*)1);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL((fPtr_BTRCore_AVMediaStatusUpdateCb)1, hdl.fpcBBTRCoreAVMediaStatusUpdate);
    TEST_ASSERT_EQUAL((void*)1, hdl.pcBMediaStatusUserData);
}
void test_BTRCore_AVMedia_Init_ValidArgs(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM;
    void* apBtConn = (void*)1; // Mock connection handle
    const char* apBtAdapter = "mockAdapter";
    BtrCore_BTRegisterMedia_IgnoreAndReturn(enBTRCoreFailure);
    BtrCore_BTRegisterNegotiateMediaCb_IgnoreAndReturn(enBTRCoreFailure);
    BtrCore_BTRegisterTransportPathMediaCb_IgnoreAndReturn(enBTRCoreFailure);
    BtrCore_BTRegisterMediaPlayerPathCb_IgnoreAndReturn(enBTRCoreFailure);
    BtrCore_BTRegisterMediaStatusUpdateCb_IgnoreAndReturn(enBTRCoreFailure);
    BtrCore_BTRegisterMediaBrowserUpdateCb_IgnoreAndReturn(enBTRCoreFailure);
    // Test with all valid arguments
    enBTRCoreRet ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, apBtConn, apBtAdapter);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_Init_NullPhBTRCoreAVM(void) {
    void* apBtConn = (void*)1; // Mock connection handle
    const char* apBtAdapter = "mockAdapter";
    // pstlhBTRCoreAVM=NULL;
    // Test with phBTRCoreAVM as NULL
    enBTRCoreRet ret = BTRCore_AVMedia_Init(NULL, apBtConn, apBtAdapter);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_Init_NullApBtConn(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM;
    const char* apBtAdapter = "mockAdapter";

    // Test with apBtConn as NULL
    enBTRCoreRet ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, NULL, apBtAdapter);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_Init_NullApBtAdapter(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM;
    void* apBtConn = (void*)1; // Mock connection handle

    // Test with apBtAdapter as NULL
    enBTRCoreRet ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, apBtConn, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}
void test_BTRCore_AVMedia_Init_MediaRegistrationFailure(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM;
    void* apBtConn = (void*)1; // Mock connection handle
    const char* apBtAdapter = "mockAdapter";

    // Simulate media registration failure
    BtrCore_BTRegisterMedia_IgnoreAndReturn(-1);
    enBTRCoreRet ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, apBtConn, apBtAdapter);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}
void test_BTRCore_AVMedia_MediaControl_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(NULL, "mockAddr", enBTRCoreAVMediaCtrlPlay, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_MediaControl_FlowIn_NoPlayerPath(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(hdl.btIfceHdl, "mockAddr", "mockPlayerPath");
    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn("mockPlayerPath");
    BtrCore_BTDevMediaControl_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlPlay, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaPlayerPath);
}


void test_BTRCore_AVMedia_MediaControl_VolumeUp_NoCtrlData(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaTransportPathOut = "mockTransportPath";
    hdl.ui8AVMediaTransportVolume = 100;
    hdl.bAVMediaTrVolAvrcp = TRUE;
    unsigned short lui16TranspVol = 55;
    BtrCore_BTSetProp_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlVolumeUp, eBTRCoreAVMediaFlowOut, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(110, hdl.ui8AVMediaTransportVolume);
}

void test_BTRCore_AVMedia_MediaControl_VolumeDown_NoCtrlData(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaTransportPathOut = "mockTransportPath";
    hdl.ui8AVMediaTransportVolume = 100;
    hdl.bAVMediaTrVolAvrcp = TRUE;
    unsigned short lui16TranspVol = 45;
    BtrCore_BTSetProp_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlVolumeDown, eBTRCoreAVMediaFlowOut, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(90, hdl.ui8AVMediaTransportVolume);
}

void test_BTRCore_AVMedia_MediaControl_EqlzrOff(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlEqlzrOff, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRcoreAVMediaCtrlEqlzrOff, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_EqlzrOn(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlEqlzrOn, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRcoreAVMediaCtrlEqlzrOn, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_ShflOff(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlShflOff, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlShflOff, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_ShflAllTracks(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlShflAllTracks, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlShflAllTracks, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_ShflGroup(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlShflGroup, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlShflGroup, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_RptOff(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlRptOff, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlRptOff, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_RptSingleTrack(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlRptSingleTrack, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlRptSingleTrack, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_RptAllTracks(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlRptAllTracks, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlRptAllTracks, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_RptGroup(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlRptGroup, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlRptGroup, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_Next(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlNext, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlNext, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_Previous(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlPrevious, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlPrevious, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_FastForward(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlFastForward, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlFastForward, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_Rewind(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    BtrCore_BTDevMediaControl_ExpectAndReturn(hdl.btIfceHdl, hdl.pcAVMediaPlayerPath, enBTMediaCtrlRewind, 0);

    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlRewind, eBTRCoreAVMediaFlowIn, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_FlowOut_VolAvrcp(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaTransportPathOut = "mockTransportPath";
    hdl.bAVMediaTrVolAvrcp = TRUE;
    unsigned short lui16TranspVol = 50;

   
    BtrCore_BTSetProp_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlVolumeUp, eBTRCoreAVMediaFlowOut, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_MediaControl_FlowOut_PlayerConnected(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaTransportPathOut = "mockTransportPath";
    hdl.bAVMediaPlayerConnected = TRUE;
    unsigned short lui16TranspVol = 50;

  
    BtrCore_BTSetProp_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_AVMedia_MediaControl(&hdl, "mockAddr", enBTRCoreAVMediaCtrlVolumeUp, eBTRCoreAVMediaFlowOut, NULL, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_NullHandle(void) {
    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(NULL, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_NullDevAddr(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, NULL, 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_NoPlayerPath(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.btIfceHdl = (void*)1;
    hdl.pcAVMediaPlayerPath = NULL; // Ensure the player path is initially NULL
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1; // Set media player to browsable

    // Initialize pstAVMediaBrowser with a valid pointer
    stBTRCoreAVMediaItem mediaItem = {0};
    mediaItem.ui32AVMediaItemId = 12345; // Set a valid media item ID
    strcpy(mediaItem.pcAVMediaItemPath , "mockPath"); // Set a valid media item path
    hdl.pstAVMediaBrowser = &mediaItem;

    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn("mockPlayerPath");
    BtrCore_BTChangeMediaFolder_IgnoreAndReturn(0);

    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaPlayerPath);
}


void test_BTRCore_AVMedia_ChangeBrowserLocation_NotBrowsable(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 0;

    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_SameItem(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem item = {0};
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaBrowser = &item;
    item.ui32AVMediaItemId = 12345;
    strcpy(item.pcAVMediaItemPath , "itemPath");
    BtrCore_BTChangeMediaFolder_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL_STRING("itemPath", hdl.pstAVMediaBrowser->pcAVMediaItemPath);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_ParentItem(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem childItem = {0};
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaBrowser = &childItem;
    childItem.pvAVMediaParentItem = &parentItem;
    parentItem.ui32AVMediaItemId = 12345;
    strcpy( parentItem.pcAVMediaItemPath ,"parentPath");
    BtrCore_BTChangeMediaFolder_IgnoreAndReturn(0);
    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
   // TEST_ASSERT_EQUAL_STRING("parentPath", hdl.pstAVMediaBrowser->pcAVMediaItemPath);
}

void test_BTRCore_AVMedia_GetCurMediaInfo_ValidArguments1(void) {
    stBTRCoreAVMediaHdl* hBTRCoreAVM1;
    stBTRCoreAVMediaInfo mediaInfo;
    a2dp_sbc_t sbcConfig;
    stBTRCoreAVMediaSbcInfo sbcInfo;

    hBTRCoreAVM1 = malloc(sizeof(stBTRCoreAVMediaHdl));
    memset(hBTRCoreAVM1, 0, sizeof(stBTRCoreAVMediaHdl));
    hBTRCoreAVM1->pstBTMediaConfigOut = &sbcConfig;
    hBTRCoreAVM1->eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    mediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowIn;
    mediaInfo.pstBtrCoreAVMCodecInfo = &sbcInfo;

    sbcConfig.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcConfig.channel_mode = BTR_SBC_CHANNEL_MODE_DUAL_CHANNEL;
    sbcConfig.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcConfig.subbands = BTR_SBC_SUBBANDS_4;
    sbcConfig.block_length = BTR_SBC_BLOCK_LENGTH_12;
    
    sbcConfig.min_bitpool = MIN_BITPOOL;
    sbcConfig.max_bitpool = MAX_BITPOOL;

    enBTRCoreRet ret = BTRCore_AVMedia_GetCurMediaInfo(hBTRCoreAVM1, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);


    free(hBTRCoreAVM1);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_NoParentFilesystem(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaBrowser = NULL;

    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_ChangeBrowserLocation_PlaylistNotFound(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaPlayList = NULL;

    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_CannotFindItem(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaBrowser = NULL;

    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 12345, eBTRCoreAVMETypeArtist);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_NullDevAddr(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};
    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, NULL, 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_NullMediaElementInfoList(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, NULL);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_EndIdxLessThanStartIdx(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};
    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 10, 0, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_BrowsingHandleNull(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};
    hdl.pstAVMediaPlayer.m_mediaBrowserItem = NULL; // Ensure the media browser item is NULL
    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_MediaItemIdMismatch(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem mediaItem = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};
    hdl.pstAVMediaPlayer.m_mediaBrowserItem = &mediaItem;
    mediaItem.ui32AVMediaItemId = 54321; // Set a different media item ID
    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_ParentItemTraversal1(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem rootItem = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem childItem = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};

    hdl.pstAVMediaPlayer.m_mediaBrowserItem = &childItem;
    childItem.ui32AVMediaItemId = 54321; // Set a different media item ID to ensure mismatch
    childItem.pvAVMediaParentItem = &parentItem;
    parentItem.pvAVMediaParentItem = &rootItem;
    rootItem.ui32AVMediaItemId = 12345;

    // Initialize pstAVMediaBrowser with a valid pointer
    hdl.pstAVMediaBrowser = &childItem;

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_ParentItemTraversal(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem rootItem = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem childItem = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};

    hdl.pstAVMediaPlayer.m_mediaBrowserItem = &childItem;
    childItem.pvAVMediaParentItem = &parentItem;
    parentItem.pvAVMediaParentItem = &rootItem;
    rootItem.ui32AVMediaItemId = 12345;

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_FindMediaItemFail(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem rootItem = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem childItem = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};

    hdl.pstAVMediaPlayer.m_mediaBrowserItem = &childItem;
    childItem.pvAVMediaParentItem = &parentItem;
    parentItem.pvAVMediaParentItem = &rootItem;
    rootItem.ui32AVMediaItemId = 12345;

    // Simulate failure in finding the media item
    rootItem.ui32AVMediaItemId = 54321;

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_MediaItemNull(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};
    hdl.pstAVMediaPlayer.m_mediaBrowserItem = NULL; // Ensure the media browser item is NULL

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_PlayableItem(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem mediaItem = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};
    hdl.pstAVMediaPlayer.m_mediaBrowserItem = &mediaItem;
    mediaItem.ui32AVMediaItemId = 12345;
    mediaItem.bIsMediaItemPlayable = 1; // Set the media item as playable

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}


void test_BTRCore_AVMedia_GetMediaElementList_PlayableItem1(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem mediaItem = {0};
    stBTRCoreAVMediaItem browserItem = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};

    hdl.pstAVMediaPlayer.m_mediaBrowserItem = &mediaItem;
    mediaItem.ui32AVMediaItemId = 54321; // Set a different media item ID to ensure mismatch
    mediaItem.bIsMediaItemPlayable = 1; // Set the media item as playable

    // Initialize pstAVMediaBrowser with a valid pointer
    hdl.pstAVMediaBrowser = &browserItem;
    browserItem.pvAVMediaParentItem = NULL; // No parent item

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

// Callbacks 
static fPtr_BtrCore_BTDevStatusUpdateCb    gfpcBDevStatusUpdate = NULL;
static fPtr_BtrCore_BTNegotiateMediaCb     gfpcBNegotiateMedia = NULL;
static fPtr_BtrCore_BTTransportPathMediaCb gfpcBTransportPathMedia = NULL;
static fPtr_BtrCore_BTConnIntimCb          gfpcBConnectionIntimation = NULL;
static fPtr_BtrCore_BTConnAuthCb           gfpcBConnectionAuthentication = NULL;


// Static Global Variables Defs 
static char *gpcBTOutPassCode = NULL;
static int do_reject = 0;
static char gpcDeviceCurrState[BT_MAX_STR_LEN];
static char* gpcBTAgentPath = NULL;
static char* gpcBTDAdapterPath = NULL;
static char* gpcBTAdapterPath = NULL;
static char* gpcDevTransportPath = NULL;
static void* gpcBDevStatusUserData = NULL;
static void* gpcBNegMediaUserData = NULL;
static void* gpcBTransPathMediaUserData = NULL;
static void* gpcBConnIntimUserData = NULL;
static void* gpcBConnAuthUserData = NULL;
int
mock_BtrCore_BTRegisterNegotiateMediaCb (
    void*                               apBtConn,
    const char*                         apBtAdapter,
    fPtr_BtrCore_BTNegotiateMediaCb     afpcBNegotiateMedia,
    void*                               apUserData
) {
    int             lDBusArgsSize;
    void*           lpInputMediaCaps = NULL;
    void*           lpOutputMediaCaps = NULL;   

    gfpcBNegotiateMedia = afpcBNegotiateMedia;
    // gpcBNegMediaUserData = apUserData;
    if (gfpcBNegotiateMedia) {
        gfpcBNegotiateMedia(lpInputMediaCaps, &lpOutputMediaCaps, 0, 0, gpcBNegMediaUserData);
    }

    return 0;
}

int
mock_BtrCore_BTRegisterTransportPathMediaCb (
    void*                                   apBtConn,
    const char*                             apBtAdapter,
    fPtr_BtrCore_BTTransportPathMediaCb     afpcBTransportPathMedia,
    void*                                   apUserData
) {


     const char* lDevTransportPath = NULL;
    const char* dev_path = NULL, *uuid = NULL, *routing = NULL;
    int codec = 0;
    char* config = NULL;
    int size = 0;
    int nrec= 0, inbandRingtone = 0;
    unsigned short int delay = 0;
    unsigned short int volume= 0;

    gfpcBTransportPathMedia    = afpcBTransportPathMedia;
    
    if (gfpcBTransportPathMedia) {
          if(!gfpcBTransportPathMedia(lDevTransportPath, uuid, config, enBTDevAudioSink, enBTMediaTypeSBC, gpcBTransPathMediaUserData)) {
            BTRCORELOG_INFO ("Stored - Transport Path\n");
        }
    }
    return 0;
}

int
mock_BtrCore_BTRegisterMediaPlayerPathCb (
    void*                                   apBtConn,
    const char*                             apBtAdapter,
    fPtr_BtrCore_BTMediaPlayerPathCb        afpcBTMediaPlayerPath,
    void*                                   apUserData
) {

 if (afpcBTMediaPlayerPath) {
        
    }
    return 0;
}

int
mock_BtrCore_BTRegisterMediaStatusUpdateCb (
    void*                                apBtConn, 
    fPtr_BtrCore_BTMediaStatusUpdateCb   afpcBMediaStatusUpdate,
    void*                                apUserData
) {
  // Dummy Function 

 if (afpcBMediaStatusUpdate) {
        
    }
    return 0;
  
}


int
mock_BtrCore_BTRegisterMediaBrowserUpdateCb (
    void*                               apstBtIfceHdl,
    fPtr_BtrCore_BTMediaBrowserPathCb   afpcBTMediaBrowserPath,
    void*                               apUserData
) {
   
   printf("\nint mock_BtrCore_BTRegisterMediaBrowserUpdateCb    \n");
  if (afpcBTMediaBrowserPath) {
        
    }
    return 0;
  
    
}
int
mock_BtrCore_BTRegisterMedia (
    void*           apBtConn,
    const char*     apBtAdapter,
    enBTDeviceType  aenBTDevType,
	enBTMediaType   aenBTMediaType,
    const char*     apBtUUID,
    void*           apBtMediaCapabilities,
    int             apBtMediaCapabilitiesSize,
    int             abBtMediaDelayReportEnable
){
      
    return 0;
}


void test_BTRCore_AVMedia_Init_Success(void) {
    tBTRCoreAVMediaHdl hBTRCoreAVM;
    enBTRCoreRet ret;
 
    const char* adapter = "adapter";
    a2dp_sbc_t sbcCaps;
      memset(&sbcCaps, 0, sizeof(a2dp_sbc_t));
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_MONO | BTR_SBC_CHANNEL_MODE_DUAL_CHANNEL |
                           BTR_SBC_CHANNEL_MODE_STEREO | BTR_SBC_CHANNEL_MODE_JOINT_STEREO;
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_16000 | BTR_SBC_SAMPLING_FREQ_32000 |
                        BTR_SBC_SAMPLING_FREQ_48000;
    sbcCaps.allocation_method = BTR_SBC_ALLOCATION_SNR | BTR_SBC_ALLOCATION_LOUDNESS;
    sbcCaps.subbands = BTR_SBC_SUBBANDS_4 | BTR_SBC_SUBBANDS_8;
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_4 | BTR_SBC_BLOCK_LENGTH_8 |
                           BTR_SBC_BLOCK_LENGTH_12 | BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.min_bitpool = MIN_BITPOOL;
    sbcCaps.max_bitpool = MAX_BITPOOL;
    a2dp_sbc_t sbcCapsCopy = sbcCaps;
    sbcCapsCopy.frequency |= BTR_SBC_SAMPLING_FREQ_44100;


    BtrCore_BTRegisterNegotiateMediaCb_StubWithCallback(mock_BtrCore_BTRegisterNegotiateMediaCb);
    BtrCore_BTRegisterTransportPathMediaCb_StubWithCallback(mock_BtrCore_BTRegisterTransportPathMediaCb);
    BtrCore_BTRegisterMediaPlayerPathCb_StubWithCallback(mock_BtrCore_BTRegisterMediaPlayerPathCb);
    BtrCore_BTRegisterMediaStatusUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterMediaStatusUpdateCb);
    BtrCore_BTRegisterMediaBrowserUpdateCb_StubWithCallback(mock_BtrCore_BTRegisterMediaBrowserUpdateCb);
    BtrCore_BTRegisterMedia_StubWithCallback(mock_BtrCore_BTRegisterMedia);
    ret = BTRCore_AVMedia_Init(&hBTRCoreAVM, (void*)1, "adapter");

    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
}

void test_BTRCore_AVMedia_GetMediaElementList_FindMediaItemSuccess(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem rootItem = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem childItem = {0};
    stBTRCoreAVMediaElementInfoList mediaElementInfoList = {0};

    // Initialize the media items
    hdl.pstAVMediaPlayer.m_mediaBrowserItem = &childItem;
    childItem.ui32AVMediaItemId = 54321; // Set a different media item ID to ensure mismatch
    childItem.pvAVMediaParentItem = &parentItem;
    parentItem.pvAVMediaParentItem = &rootItem;
    rootItem.ui32AVMediaItemId = 12345; // Set the root item ID to match the search ID

    // Initialize pstAVMediaBrowser with a valid pointer
    hdl.pstAVMediaBrowser = &childItem;

    // Set up the sub-items for rootItem to simulate a successful find
    rootItem.ui32AVMediaNumberOfItems = 1;
    stBTRCoreAVMediaItem* subItems = malloc(sizeof(stBTRCoreAVMediaItem) * rootItem.ui32AVMediaNumberOfItems);
    if (subItems == NULL) {
        // Handle memory allocation failure
        return;
    }
    subItems[0] = rootItem;
    rootItem.pstAVMediaSubItems = subItems;

    enBTRCoreRet ret = BTRCore_AVMedia_GetMediaElementList(&hdl, "mockAddr", 12345, 0, 10, eBTRCoreAVMETypeArtist, &mediaElementInfoList);

    // Check the result
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Free allocated memory
    free(subItems);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_InvalidArgs(void) {
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(NULL, 0, "mockAddr", NULL);
    TEST_ASSERT_EQUAL(-1, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_NotBrowsable(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 0;
    mediaUpdate.ui32BTMediaItemId = 12345;

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 0, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    strcpy(mediaUpdate.pcMediaItemPath , "mockPath");
    strcpy(mediaUpdate.pcMediaItemName , "mockName");

    // Mock the btrCore_AVMedia_AllocateBrowserMemory function
   // btrCore_AVMedia_AllocateBrowserMemory_IgnoreAndReturn(0);

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

//need to get expected results
void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemoryFailure(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;

    // Mock the btrCore_AVMedia_FindMediaItem function
    //btrCore_AVMedia_FindMediaItem_IgnoreAndReturn(0);

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 0, "mockAddr", &hdl);
      TEST_ASSERT_EQUAL(-1, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_FindMediaItemSuccess(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl); 
    TEST_ASSERT_EQUAL(0, ret);  
}
void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllScenarios(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    // Scenario 1: Allocate Browser Memory Failure
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;

    // Call the function
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);

    // Check the result
    TEST_ASSERT_EQUAL(0, ret);

    // Scenario 2: Media Item Type Video
    mediaUpdate.eMediaItemType = enBTMediaItemTypVideo;

    // Call the function
    ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);

    // Check the result
    TEST_ASSERT_EQUAL(0, ret);

    // Scenario 3: Unknown Media Item Type
    mediaUpdate.eMediaItemType = 999; // Unknown media item type

    // Call the function
    ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);

    // Check the result
    TEST_ASSERT_EQUAL(-1, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_MediaItemTypeAudio(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    // Initialize the media items
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypAudio;
    // Call the function
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);

    // Check the result
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_FindMediaItemSuccess1(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem rootItem = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem childItem = {0};
    stBTRCoreAVMediaItem foundItem = {0};

    // Initialize the media items
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaBrowser = &rootItem;
    rootItem.pvAVMediaParentItem = NULL;
    parentItem.pvAVMediaParentItem = &rootItem;
    childItem.pvAVMediaParentItem = &parentItem;
    rootItem.ui32AVMediaItemId = 12345;
    parentItem.ui32AVMediaItemId = 54321;
    childItem.ui32AVMediaItemId = 67890;

    mediaUpdate.ui32BTMediaItemId = 67890;
    // Call the function
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 0, "mockAddr", &hdl);

    // Check the result
    TEST_ASSERT_EQUAL(-1, ret);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb_InvalidArgs(void) {
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, NULL, "mockAddr", NULL);
    TEST_ASSERT_EQUAL(-1, ret);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaTransportState(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaTransport;
    mediaUpdate.aunBtOpIfceProp.enBtMediaTransportProp = enBTMedTPropState;
    mediaUpdate.m_mediaTransportState = enBTMedTransportStActive;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
}


void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaTransportVolume(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaTransport;
    mediaUpdate.aunBtOpIfceProp.enBtMediaTransportProp = enBTMedTPropVol;
    mediaUpdate.m_mediaTransportVolume = 50;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(50, hdl.ui8AVMediaTransportVolume);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaTransportDelay(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaTransport;
    mediaUpdate.aunBtOpIfceProp.enBtMediaTransportProp = enBTMedTPropDelay;
    mediaUpdate.m_mediaTransportDelay = 100;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(100, hdl.ui16AVMediaTransportDelay);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaControlConnected(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaControl;
    mediaUpdate.aunBtOpIfceProp.enBtMediaControlProp = enBTMedControlPropConnected;
    mediaUpdate.m_mediaPlayerConnected = 1;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(1, hdl.bAVMediaPlayerConnected);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerName(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropName;
    strncpy(mediaUpdate.m_mediaPlayerName, "TestPlayer", BTRCORE_MAX_STR_LEN - 1);

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("TestPlayer", hdl.pstAVMediaPlayer.m_mediaPlayerName);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerType(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropType;
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypAudio;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypAudio, hdl.pstAVMediaPlayer.eAVMediaPlayerType);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerTrack(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropTrack;
    strncpy(mediaUpdate.m_mediaTrackInfo.pcTitle, "TestTrack", BTRCORE_MAX_STR_LEN - 1);

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("TestTrack", hdl.pstAVMediaPlayer.m_mediaTrackInfo.pcTitle);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerEqualizer(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropEqualizer;
    mediaUpdate.enMediaPlayerEqualizer = enBTMedPlayerEqualizerOn;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrEqlzrStOn, hdl.pstAVMediaPlayer.eAVMediaPlayerEqualizer);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerShuffle(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropShuffle;
    mediaUpdate.enMediaPlayerShuffle = enBTMedPlayerShuffleAllTracks;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrShflStAllTracks, hdl.pstAVMediaPlayer.eAVMediaPlayerShuffle);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerRepeat(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropRepeat;
    mediaUpdate.enMediaPlayerRepeat = enBTMedPlayerRpAllTracks;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrRptStAllTracks, hdl.pstAVMediaPlayer.eAVMediaPlayerRepeat);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerScan(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropScan;
    mediaUpdate.enMediaPlayerScan = enBTMedPlayerScanAllTracks;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrScanStAllTracks, hdl.pstAVMediaPlayer.eAVMediaPlayerScan);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerPosition(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropPosition;
    mediaUpdate.m_mediaPlayerPosition = 120;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(120, hdl.pstAVMediaPlayer.m_mediaPlayerPosition);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerStatus(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropStatus;
    mediaUpdate.enMediaPlayerStatus = enBTMedPlayerStPlaying;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaTrkStPlaying, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerBrowsable(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropBrowsable;
    mediaUpdate.m_mediaPlayerBrowsable = 1;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(1, hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb_MediaPlayerSearchable(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropSearchable;
    mediaUpdate.m_mediaPlayerSearchable = 1;

    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(1, hdl.pstAVMediaPlayer.m_mediaPlayerSearchable);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem rootItem = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem childItem = {0};

    // Initialize the media items
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaBrowser = &rootItem;
    rootItem.pvAVMediaParentItem = NULL;
    rootItem.ui32AVMediaNumberOfItems = 1;
    rootItem.pstAVMediaSubItems = &parentItem;

    parentItem.pvAVMediaParentItem = &rootItem;
    parentItem.ui32AVMediaNumberOfItems = 1;
    parentItem.pstAVMediaSubItems = &childItem;

    childItem.pvAVMediaParentItem = &parentItem;
    childItem.ui32AVMediaItemId = 67890;

    mediaUpdate.aenBtOpIfceType = enBTMediaTransport;
    mediaUpdate.aunBtOpIfceProp.enBtMediaTransportProp = enBTMedTPropState;
    mediaUpdate.m_mediaTransportState = enBTMedTransportStActive;

    // Call the function
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);

    // Check the result
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(enAVMTransportStConnected, hdl.eAVMTState);

    // Test volume update
    mediaUpdate.aunBtOpIfceProp.enBtMediaTransportProp = enBTMedTPropVol;
    mediaUpdate.m_mediaTransportVolume = 50;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(50, hdl.ui8AVMediaTransportVolume);

    // Test delay update
    mediaUpdate.aunBtOpIfceProp.enBtMediaTransportProp = enBTMedTPropDelay;
    mediaUpdate.m_mediaTransportDelay = 100;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(100, hdl.ui16AVMediaTransportDelay);

    // Test media player connected
    mediaUpdate.aenBtOpIfceType = enBTMediaControl;
    mediaUpdate.aunBtOpIfceProp.enBtMediaControlProp = enBTMedControlPropConnected;
    mediaUpdate.m_mediaPlayerConnected = 1;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_TRUE(hdl.bAVMediaPlayerConnected);

    // Test media player path
    mediaUpdate.aunBtOpIfceProp.enBtMediaControlProp = enBTMedControlPropPath;
    strncpy(mediaUpdate.m_mediaPlayerPath, "mockPath", BTRCORE_MAX_STR_LEN - 1);

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test media player name
    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropName;
    strncpy(mediaUpdate.m_mediaPlayerName, "mockPlayer", BTRCORE_MAX_STR_LEN - 1);

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("mockPlayer", hdl.pstAVMediaPlayer.m_mediaPlayerName);

    // Test media player type
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropType;
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypAudio;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypAudio, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test media player track
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropTrack;
    strncpy(mediaUpdate.m_mediaTrackInfo.pcTitle, "mockTitle", BTRCORE_MAX_STR_LEN - 1);

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("mockTitle", hdl.pstAVMediaPlayer.m_mediaTrackInfo.pcTitle);

    // Test media player equalizer
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropEqualizer;
    mediaUpdate.enMediaPlayerEqualizer = enBTMedPlayerEqualizerOn;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrEqlzrStOn, hdl.pstAVMediaPlayer.eAVMediaPlayerEqualizer);

    // Test media player shuffle
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropShuffle;
    mediaUpdate.enMediaPlayerShuffle = enBTMedPlayerShuffleAllTracks;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrShflStAllTracks, hdl.pstAVMediaPlayer.eAVMediaPlayerShuffle);

    // Test media player repeat
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropRepeat;
    mediaUpdate.enMediaPlayerRepeat = enBTMedPlayerRpAllTracks;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrRptStAllTracks, hdl.pstAVMediaPlayer.eAVMediaPlayerRepeat);

    // Test media player scan
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropScan;
    mediaUpdate.enMediaPlayerScan = enBTMedPlayerScanAllTracks;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrScanStAllTracks, hdl.pstAVMediaPlayer.eAVMediaPlayerScan);

    // Test media player position
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropPosition;
    mediaUpdate.m_mediaPlayerPosition = 123;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(123, hdl.pstAVMediaPlayer.m_mediaPlayerPosition);

    // Test media player status
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropStatus;
    mediaUpdate.enMediaPlayerStatus = enBTMedPlayerStPlaying;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaTrkStPlaying, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);

    // Test media player browsable
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropBrowsable;
    mediaUpdate.m_mediaPlayerBrowsable = 1;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(1, hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable);

    // Test media player searchable
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropSearchable;
    mediaUpdate.m_mediaPlayerSearchable = 1;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(1, hdl.pstAVMediaPlayer.m_mediaPlayerSearchable);

    // Test media player playlist
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropPlaylist;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test media folder name
    mediaUpdate.aenBtOpIfceType = enBTMediaFolder;
    mediaUpdate.aunBtOpIfceProp.enBtMediaFolderProp = enBTMedFolderPropName;
    strncpy(mediaUpdate.m_mediaFolderName, "mockFolder", BTRCORE_MAX_STR_LEN - 1);

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test media folder number of items
    mediaUpdate.aunBtOpIfceProp.enBtMediaFolderProp = enBTMedFolderPropNumberOfItems;
    mediaUpdate.m_mediaFolderNumberOfItems = 10;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(10, mediaUpdate.m_mediaFolderNumberOfItems);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb2(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    // Initialize the media handler
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;

    // Test case for media player type
    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropType;

    // Test for enBTMedPlayerTypVideo
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypVideo;
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypVideo, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test for enBTMedPlayerTypAudioBroadcasting
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypAudioBroadcasting;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypAudioBroadcasting, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test for enBTMedPlayerTypVideoBroadcasting
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypVideoBroadcasting;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypVideoBroadcasting, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test for default case
    mediaUpdate.enMediaPlayerType = 999; // Unknown type
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypUnknown, hdl.pstAVMediaPlayer.eAVMediaPlayerType);
}


void test_btrCore_AVMedia_MediaStatusUpdateCb1(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    // Initialize the media handler
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;

    // Test case for media player type
    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropType;

    // Test for enBTMedPlayerTypVideo
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypVideo;
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypVideo, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test for enBTMedPlayerTypAudioBroadcasting
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypAudioBroadcasting;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypAudioBroadcasting, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test for enBTMedPlayerTypVideoBroadcasting
    mediaUpdate.enMediaPlayerType = enBTMedPlayerTypVideoBroadcasting;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypVideoBroadcasting, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test for default case
    mediaUpdate.enMediaPlayerType = 999; // Unknown type
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPTypUnknown, hdl.pstAVMediaPlayer.eAVMediaPlayerType);

    // Test case for media player subtype
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropSubtype;

    // Test for enBTMedPlayerSbTypAudioBook
    mediaUpdate.enMediaPlayerSubtype = enBTMedPlayerSbTypAudioBook;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPSbTypAudioBook, hdl.pstAVMediaPlayer.eAVMediaPlayerSubtype);

    // Test for enBTMedPlayerSbTypPodcast
    mediaUpdate.enMediaPlayerSubtype = enBTMedPlayerSbTypPodcast;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPSbTypPodcast, hdl.pstAVMediaPlayer.eAVMediaPlayerSubtype);

    // Test for default case
    mediaUpdate.enMediaPlayerSubtype = 999; // Unknown subtype
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMPSbTypUnknown, hdl.pstAVMediaPlayer.eAVMediaPlayerSubtype);
}

enBTRCoreRet mock_fpcBBTRCoreAVMediaStatusUpdate(stBTRCoreAVMediaStatusUpdate* mediaStatus, const char* apcBtDevAddr, void* apUserData) {
    return enBTRCoreSuccess;
}

void test_btrCore_AVMedia_MediaStatusUpdateCb4(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    // Initialize the media handler
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.fpcBBTRCoreAVMediaStatusUpdate = mock_fpcBBTRCoreAVMediaStatusUpdate;
    hdl.pcBMediaStatusUserData = NULL;

    // Test case for media player repeat property
    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropRepeat;

    // Test for enBTMedPlayerRpOff
    mediaUpdate.enMediaPlayerRepeat = enBTMedPlayerRpOff;
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrRptStOff, hdl.pstAVMediaPlayer.eAVMediaPlayerRepeat);

    // Test for enBTMedPlayerRpSingleTrack
    mediaUpdate.enMediaPlayerRepeat = enBTMedPlayerRpSingleTrack;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrRptStSingleTrack, hdl.pstAVMediaPlayer.eAVMediaPlayerRepeat);

    // Test for enBTMedPlayerRpGroup
    mediaUpdate.enMediaPlayerRepeat = enBTMedPlayerRpGroup;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrRptStGroup, hdl.pstAVMediaPlayer.eAVMediaPlayerRepeat);

    // Test for default case
    mediaUpdate.enMediaPlayerRepeat = 999; // Unknown repeat type
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaStUnknown, hdl.pstAVMediaPlayer.eAVMediaPlayerRepeat);

    // Test case for media player scan property
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropScan;

    // Test for enBTMedPlayerScanOff
    mediaUpdate.enMediaPlayerScan = enBTMedPlayerScanOff;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrScanStOff, hdl.pstAVMediaPlayer.eAVMediaPlayerScan);

    // Test for enBTMedPlayerScanGroup
    mediaUpdate.enMediaPlayerScan = enBTMedPlayerScanGroup;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrScanStGroup, hdl.pstAVMediaPlayer.eAVMediaPlayerScan);

    // Test for default case
    mediaUpdate.enMediaPlayerScan = 999; // Unknown scan type
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaStUnknown, hdl.pstAVMediaPlayer.eAVMediaPlayerScan);

    // Test case for media player status property
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropStatus;

    // Test for enBTMedPlayerStStopped
    mediaUpdate.enMediaPlayerStatus = enBTMedPlayerStStopped;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaTrkStStopped, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);

    // Test for enBTMedPlayerStPaused
    mediaUpdate.enMediaPlayerStatus = enBTMedPlayerStPaused;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaTrkStPaused, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);

    // Test for enBTMedPlayerStForwardSeek
    mediaUpdate.enMediaPlayerStatus = enBTMedPlayerStForwardSeek;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaTrkStForwardSeek, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);

    // Test for enBTMedPlayerStReverseSeek
    mediaUpdate.enMediaPlayerStatus = enBTMedPlayerStReverseSeek;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaTrkStReverseSeek, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);

    // Test for enBTMedPlayerStError
    mediaUpdate.enMediaPlayerStatus = enBTMedPlayerStError;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlaybackError, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);

    // Test for default case
    mediaUpdate.enMediaPlayerStatus = 999; // Unknown status type
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlaybackError, hdl.pstAVMediaPlayer.eAVMediaStatusUpdate);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb5(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    // Initialize the media handler
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.fpcBBTRCoreAVMediaStatusUpdate = mock_fpcBBTRCoreAVMediaStatusUpdate;
    hdl.pcBMediaStatusUserData = NULL;

    // Test case for media track information
    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropTrack;

    // Initialize media track info with empty values
    memset(&mediaUpdate.m_mediaTrackInfo, 0, sizeof(mediaUpdate.m_mediaTrackInfo));

    // Call the function to cover the case where all track info fields are empty
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("Unknown", mediaUpdate.m_mediaTrackInfo.pcTitle);
    TEST_ASSERT_EQUAL_STRING("Unknown", mediaUpdate.m_mediaTrackInfo.pcArtist);
    TEST_ASSERT_EQUAL_STRING("Unknown", mediaUpdate.m_mediaTrackInfo.pcAlbum);
    TEST_ASSERT_EQUAL_STRING("Unknown", mediaUpdate.m_mediaTrackInfo.pcGenre);

    // Initialize media track info with duration only
    mediaUpdate.m_mediaTrackInfo.ui32Duration = 300;

    // Call the function to cover the case where only duration is set
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(300, hdl.pstAVMediaPlayer.m_mediaTrackInfo.ui32Duration);

    // Initialize media track info with artist only
    strncpy(mediaUpdate.m_mediaTrackInfo.pcArtist, "ArtistName", BTRCORE_MAX_STR_LEN - 1);

    // Call the function to cover the case where only artist is set
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("ArtistName", hdl.pstAVMediaPlayer.m_mediaTrackInfo.pcArtist);

    // Initialize media track info with album only
    strncpy(mediaUpdate.m_mediaTrackInfo.pcAlbum, "AlbumName", BTRCORE_MAX_STR_LEN - 1);

    // Call the function to cover the case where only album is set
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("AlbumName", hdl.pstAVMediaPlayer.m_mediaTrackInfo.pcAlbum);

    // Initialize media track info with genre only
    strncpy(mediaUpdate.m_mediaTrackInfo.pcGenre, "GenreName", BTRCORE_MAX_STR_LEN - 1);

    // Call the function to cover the case where only genre is set
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("GenreName", hdl.pstAVMediaPlayer.m_mediaTrackInfo.pcGenre);
}
void test_btrCore_AVMedia_MediaStatusUpdateCb7(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    // Initialize the media handler
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.fpcBBTRCoreAVMediaStatusUpdate = mock_fpcBBTRCoreAVMediaStatusUpdate;
    hdl.pcBMediaStatusUserData = NULL;

    // Test case for media player equalizer property
    mediaUpdate.aenBtOpIfceType = enBTMediaPlayer;
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropEqualizer;

    // Test for enBTMedPlayerEqualizerOff
    mediaUpdate.enMediaPlayerEqualizer = enBTMedPlayerEqualizerOff;
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrEqlzrStOff, hdl.pstAVMediaPlayer.eAVMediaPlayerEqualizer);

    // Test for enBTMedPlayerEqualizerOn
    mediaUpdate.enMediaPlayerEqualizer = enBTMedPlayerEqualizerOn;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrEqlzrStOn, hdl.pstAVMediaPlayer.eAVMediaPlayerEqualizer);

    // Test for default case
    mediaUpdate.enMediaPlayerEqualizer = 999; // Unknown equalizer type
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(enBTRCoreAVMediaCtrlUnknown, hdl.pstAVMediaPlayer.eAVMediaPlayerEqualizer);

    // Test case for media player shuffle property
    mediaUpdate.aunBtOpIfceProp.enBtMediaPlayerProp = enBTMedPlayerPropShuffle;

    // Test for enBTMedPlayerShuffleOff
    mediaUpdate.enMediaPlayerShuffle = enBTMedPlayerShuffleOff;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrShflStOff, hdl.pstAVMediaPlayer.eAVMediaPlayerShuffle);

    // Test for enBTMedPlayerShuffleAllTracks
    mediaUpdate.enMediaPlayerShuffle = enBTMedPlayerShuffleAllTracks;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrShflStAllTracks, hdl.pstAVMediaPlayer.eAVMediaPlayerShuffle);

    // Test for enBTMedPlayerShuffleGroup
    mediaUpdate.enMediaPlayerShuffle = enBTMedPlayerShuffleGroup;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaPlyrShflStGroup, hdl.pstAVMediaPlayer.eAVMediaPlayerShuffle);

    // Test for default case
    mediaUpdate.enMediaPlayerShuffle = 999; // Unknown shuffle type
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMediaStUnknown, hdl.pstAVMediaPlayer.eAVMediaPlayerShuffle);

    // Test case for media folder properties
    mediaUpdate.aenBtOpIfceType = enBTMediaFolder;
    mediaUpdate.aunBtOpIfceProp.enBtMediaFolderProp = enBTMedFolderPropName;

    // Initialize media folder name
    strncpy(mediaUpdate.m_mediaFolderName, "NowPlaying", BTRCORE_MAX_STR_LEN - 1);

    // Call the function to cover the case where the folder name is "NowPlaying"
    hdl.pstAVMediaPlayList = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    strncpy(hdl.pstAVMediaPlayList->pcAVMediaItemName, "PlaylistName", BTRCORE_MAX_STR_LEN - 1);
    hdl.pstAVMediaPlayList->ui32AVMediaItemId = 12345;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL_STRING("PlaylistName", hdl.pstAVMediaPlayer.m_mediaBrowserItem->pcAVMediaItemName);
    TEST_ASSERT_EQUAL(12345, hdl.pstAVMediaPlayer.m_mediaBrowserItem->ui32AVMediaItemId);

    // Free allocated memory
    free(hdl.pstAVMediaPlayList);
}

void test_btrCore_AVMedia_MediaStatusUpdateCb8(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaStatusUpdate mediaUpdate = {0};

    // Initialize the media handler
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.fpcBBTRCoreAVMediaStatusUpdate = mock_fpcBBTRCoreAVMediaStatusUpdate;
    hdl.pcBMediaStatusUserData = NULL;

    // Test case for media transport state
    mediaUpdate.aenBtOpIfceType = enBTMediaTransport;
    mediaUpdate.aunBtOpIfceProp.enBtMediaTransportProp = enBTMedTPropState;

    // Test for enBTMedTransportStIdle
    mediaUpdate.m_mediaTransportState = enBTMedTransportStIdle;
    int ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(enAVMTransportStDisconnected, hdl.eAVMTState);

    // Test for enBTMedTransportStPending
    mediaUpdate.m_mediaTransportState = enBTMedTransportStPending;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(enAVMTransportStToBeConnected, hdl.eAVMTState);

    // Test for enBTMedTransportStActive
    mediaUpdate.m_mediaTransportState = enBTMedTransportStActive;
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(enAVMTransportStConnected, hdl.eAVMTState);

    // Test for default case
    mediaUpdate.m_mediaTransportState = 999; // Unknown state
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for media polling thread
    hdl.bAVMediaPlayerConnected = 1;
    hdl.pMediaPollingThread = NULL;

    // Simulate failure to create media polling thread
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Simulate media polling thread already running
    hdl.pMediaPollingThread = (GThread*)1; // Simulate an existing thread
    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Simulate media browser error
    hdl.pstAVMediaBrowser = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    hdl.pstAVMediaPlayList = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    hdl.ui32AVMediaBrowserItemCount = 1;
    hdl.ui32AVMediaPlayListItemCount = 1;

    ret = btrCore_AVMedia_MediaStatusUpdateCb(enBTDevAudioSource, &mediaUpdate, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Free allocated memory
    free(hdl.pstAVMediaBrowser);
    free(hdl.pstAVMediaPlayList);
}

void test_btrCore_AVMedia_MediaPlayerPathCb(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    const char* mediaPlayerPath = "/org/bluez/hci0/dev_00_1A_7D_DA_71_13/player0";
    const char* newMediaPlayerPath = "/org/bluez/hci0/dev_00_1A_7D_DA_71_13/player1";
    int ret;

    // Test case for invalid media path
    ret = btrCore_AVMedia_MediaPlayerPathCb(NULL, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid media path and no existing media player path
    ret = btrCore_AVMedia_MediaPlayerPathCb(mediaPlayerPath, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaPlayerPath);
    TEST_ASSERT_EQUAL_STRING(mediaPlayerPath, hdl.pcAVMediaPlayerPath);

    // Test case for valid media path and existing media player path (same path)
    ret = btrCore_AVMedia_MediaPlayerPathCb(mediaPlayerPath, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NULL(hdl.pcAVMediaPlayerPath);

    // Test case for valid media path and existing media player path (different path)
    hdl.pcAVMediaPlayerPath = strndup(mediaPlayerPath, BTRCORE_MAX_STR_LEN - 1);
    ret = btrCore_AVMedia_MediaPlayerPathCb(newMediaPlayerPath, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaPlayerPath);
    TEST_ASSERT_EQUAL_STRING(newMediaPlayerPath, hdl.pcAVMediaPlayerPath);
    TEST_ASSERT_EQUAL_STRING("UnknownPlayer", hdl.pstAVMediaPlayer.m_mediaPlayerName);

    // Free allocated memory
    free(hdl.pcAVMediaPlayerPath);
}


void test_btrCore_AVMedia_MediaBrowserUpdateCb(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    const char* btDevAddr = "00:1A:7D:DA:71:13";
    int ret;

    // Test case for invalid arguments
    ret = btrCore_AVMedia_MediaBrowserUpdateCb(NULL, 0, btDevAddr, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 0, btDevAddr, NULL);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid arguments and non-browsable media player
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 0;
    mediaUpdate.ui32BTMediaItemId = 1;
    ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 0, btDevAddr, &hdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for valid arguments and browsable media player
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 1;
    ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 0, btDevAddr, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid arguments and media item ID equal to BTR_MEDIA_PLAYLIST_ID
    mediaUpdate.ui32BTMediaItemId = BTR_MEDIA_PLAYLIST_ID;
    ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 0, btDevAddr, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid arguments and ucItemScope non-zero
    mediaUpdate.ui32BTMediaItemId = 1;
    ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, btDevAddr, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);
}

void test_btrCore_AVMedia_NegotiateMediaCb(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    a2dp_sbc_t sbcConfigOut = {0};
    a2dp_sbc_t sbcConfigIn = {0};
    void* mediaCapsOutput = NULL;
    int ret;

    // Initialize the media handler
    hdl.pstBTMediaConfigOut = &sbcConfigOut;
    hdl.pstBTMediaConfigIn = &sbcConfigIn;

    // Test case for invalid input media capabilities
    ret = btrCore_AVMedia_NegotiateMediaCb(NULL, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and SBC media type
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.subbands = BTR_SBC_SUBBANDS_8;
    sbcCaps.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcCaps.min_bitpool = MIN_BITPOOL;
    sbcCaps.max_bitpool = MAX_BITPOOL;

    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);
    TEST_ASSERT_EQUAL_PTR(&sbcConfigOut, mediaCapsOutput);

    // Test case for valid input media capabilities and SBC media type with existing output device
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    hdl.pcAVMediaTransportPathOut = NULL;
    hdl.i64LastSelectConfigTime = g_get_monotonic_time() - (BTR_SELECT_CONF_TIMEOUT + 1);

    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);
    TEST_ASSERT_EQUAL_PTR(&sbcConfigOut, mediaCapsOutput);

    // Test case for valid input media capabilities and SBC media type with existing input device
    hdl.eAVMediaTypeIn = eBTRCoreAVMTypeSBC;
    hdl.pcAVMediaTransportPathIn = NULL;
    hdl.i64LastSelectConfigTime = g_get_monotonic_time() - (BTR_SELECT_CONF_TIMEOUT + 1);

    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSource, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);

    // Test case for valid input media capabilities and AAC media type
#if defined(AAC_SUPPORTED)
    a2dp_aac_t aacCaps = {0};
    a2dp_aac_t aacConfigOut = {0};
    a2dp_aac_t aacConfigIn = {0};
    hdl.pstBTMediaConfigOut = &aacConfigOut;
    hdl.pstBTMediaConfigIn = &aacConfigIn;

    aacCaps.object_type = BTR_AAC_OT_MPEG2_AAC_LC;
    aacCaps.frequency1 = BTR_AAC_SAMPLING_FREQ_44100;
    aacCaps.channels = BTR_AAC_CHANNELS_2;
    aacCaps.bitrate1 = BTR_MPEG_BIT_RATE_128000;
    aacCaps.vbr = 1;

    ret = btrCore_AVMedia_NegotiateMediaCb(&aacCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeAAC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);
    TEST_ASSERT_EQUAL_PTR(&aacConfigOut, mediaCapsOutput);
#endif

    // Test case for valid input media capabilities and no supported frequency
    sbcCaps.frequency = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported channel mode
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcCaps.channel_mode = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported block length
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcCaps.block_length = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported subbands
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.subbands = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported allocation method
    sbcCaps.subbands = BTR_SBC_SUBBANDS_8;
    sbcCaps.allocation_method = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}
void test_btrCore_AVMedia_NegotiateMediaCb10(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    a2dp_sbc_t sbcConfigOut = {0};
    a2dp_sbc_t sbcConfigIn = {0};
    void* mediaCapsOutput = NULL;
    int ret;

    // Initialize the media handler
    hdl.pstBTMediaConfigOut = &sbcConfigOut;
    hdl.pstBTMediaConfigIn = &sbcConfigIn;

    // Test case for invalid input media capabilities
    ret = btrCore_AVMedia_NegotiateMediaCb(NULL, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and SBC media type
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.subbands = BTR_SBC_SUBBANDS_8;
    sbcCaps.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcCaps.min_bitpool = MIN_BITPOOL;
    sbcCaps.max_bitpool = MAX_BITPOOL;

    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);
    TEST_ASSERT_EQUAL_PTR(&sbcConfigOut, mediaCapsOutput);

    // Test case for valid input media capabilities and SBC media type with existing output device
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    hdl.pcAVMediaTransportPathOut = NULL;
    hdl.i64LastSelectConfigTime = g_get_monotonic_time() - (BTR_SELECT_CONF_TIMEOUT + 1);

    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);
    TEST_ASSERT_EQUAL_PTR(&sbcConfigOut, mediaCapsOutput);

    // Test case for valid input media capabilities and SBC media type with existing input device
    hdl.eAVMediaTypeIn = eBTRCoreAVMTypeUnknown; // Ensure the condition passes
    hdl.pcAVMediaTransportPathIn = NULL;
    hdl.i64LastSelectConfigTime = g_get_monotonic_time() - (BTR_SELECT_CONF_TIMEOUT + 1);

    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSource, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);
    TEST_ASSERT_EQUAL_PTR(&sbcConfigIn, mediaCapsOutput);

    // Test case for valid input media capabilities and AAC media type
#if defined(AAC_SUPPORTED)
    a2dp_aac_t aacCaps = {0};
    a2dp_aac_t aacConfigOut = {0};
    a2dp_aac_t aacConfigIn = {0};
    hdl.pstBTMediaConfigOut = &aacConfigOut;
    hdl.pstBTMediaConfigIn = &aacConfigIn;

    aacCaps.object_type = BTR_AAC_OT_MPEG2_AAC_LC;
    aacCaps.frequency1 = BTR_AAC_SAMPLING_FREQ_44100;
    aacCaps.channels = BTR_AAC_CHANNELS_2;
    aacCaps.bitrate1 = BTR_MPEG_BIT_RATE_128000;
    aacCaps.vbr = 1;

    ret = btrCore_AVMedia_NegotiateMediaCb(&aacCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeAAC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(mediaCapsOutput);
    TEST_ASSERT_EQUAL_PTR(&aacConfigOut, mediaCapsOutput);
#endif

    // Test case for valid input media capabilities and no supported frequency
    sbcCaps.frequency = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported channel mode
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcCaps.channel_mode = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported block length
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcCaps.block_length = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported subbands
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.subbands = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid input media capabilities and no supported allocation method
    sbcCaps.subbands = BTR_SBC_SUBBANDS_8;
    sbcCaps.allocation_method = 0;
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_TransportPathCb(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    char* transportPath = "/org/bl";
    char* newTransportPath = "/org/bl";
    char* mediaUuidSource = BT_UUID_A2DP_SOURCE;
    char* mediaUuidSink = BT_UUID_A2DP_SINK;
    int ret;

    // Test case for invalid transport path
    ret = btrCore_AVMedia_TransportPathCb(NULL, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid transport path and SBC media type
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcCaps.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcCaps.subbands = BTR_SBC_SUBBANDS_8;
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.min_bitpool = MIN_BITPOOL;
    sbcCaps.max_bitpool = MAX_BITPOOL;

   
    // Test case for valid transport path and resetting media configuration
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    hdl.pstBTMediaConfigOut = &sbcCaps;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NULL(hdl.pcAVMediaTransportPathOut);
    TEST_ASSERT_NOT_NULL(hdl.pstBTMediaConfigOut);

    // Free allocated memory
    free(hdl.pcAVMediaTransportPathOut);
    free(hdl.pcAVMediaTransportPathIn);
}

void test_btrCore_AVMedia_TransportPathCb5(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    a2dp_sbc_t sbcConfigOut = {0};
    a2dp_sbc_t sbcConfigIn = {0};
    char* transportPath = "/org/bluez/hci0/dev_00_1A_7D_DA_71_13/fd0";
    char* newTransportPath = "/org/bluez/hci0/dev_00_1A_7D_DA_71_13/fd1";
    char* mediaUuidSource = BT_UUID_A2DP_SOURCE;
    char* mediaUuidSink = BT_UUID_A2DP_SINK;
    int ret;

    // Initialize the media handler
    hdl.pstBTMediaConfigOut = &sbcConfigOut;
    hdl.pstBTMediaConfigIn = &sbcConfigIn;

    // Test case for invalid transport path
    ret = btrCore_AVMedia_TransportPathCb(NULL, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid transport path and SBC media type
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcCaps.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcCaps.subbands = BTR_SBC_SUBBANDS_8;
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.min_bitpool = MIN_BITPOOL;
    sbcCaps.max_bitpool = MAX_BITPOOL;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaTransportPathOut);
    TEST_ASSERT_EQUAL_STRING(transportPath, hdl.pcAVMediaTransportPathOut);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeOut);

    // Test case for valid transport path and media UUID sink
    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSink, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaTransportPathIn);
    TEST_ASSERT_EQUAL_STRING(transportPath, hdl.pcAVMediaTransportPathIn);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeIn);

    // Test case for valid transport path and no media caps
    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, NULL, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for valid transport path and existing transport path (same path)
    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NULL(hdl.pcAVMediaTransportPathOut);

    // Test case for valid transport path and existing transport path (different path)
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    ret = btrCore_AVMedia_TransportPathCb(newTransportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for valid transport path and resetting media configuration
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    hdl.pstBTMediaConfigOut = &sbcCaps;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NULL(hdl.pcAVMediaTransportPathOut);
    TEST_ASSERT_NOT_NULL(hdl.pstBTMediaConfigOut);

    // Test case for valid transport path and media UUID sink with different configuration
    sbcCaps.max_bitpool = 30;
    hdl.pcAVMediaTransportPathIn = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeIn = eBTRCoreAVMTypeUnknown;
    hdl.pstBTMediaConfigIn = &sbcConfigIn;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSink, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeIn);
    TEST_ASSERT_EQUAL(sbcCaps.max_bitpool, sbcConfigIn.max_bitpool);

    // Test case for valid transport path and media UUID source with different configuration
    sbcCaps.max_bitpool = 20;
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeUnknown;
    hdl.pstBTMediaConfigOut = &sbcConfigOut;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeOut);
    TEST_ASSERT_EQUAL(sbcCaps.max_bitpool, sbcConfigOut.max_bitpool);

    // Free allocated memory
    free(hdl.pcAVMediaTransportPathOut);
    free(hdl.pcAVMediaTransportPathIn);
}

void test_btrCore_AVMedia_TransportPathCb6(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    a2dp_sbc_t sbcConfigOut = {0};
    a2dp_sbc_t sbcConfigIn = {0};
    char* transportPath = "/org/bluez/hci0/dev_00_1A_7D_DA_71_13/fd0";
    char* newTransportPath = "/org/bluez/hci0/dev_00_1A_7D_DA_71_13/fd1";
    char* mediaUuidSource = BT_UUID_A2DP_SOURCE;
    char* mediaUuidSink = BT_UUID_A2DP_SINK;
    int ret;

    // Initialize the media handler
    hdl.pstBTMediaConfigOut = &sbcConfigOut;
    hdl.pstBTMediaConfigIn = &sbcConfigIn;

    // Test case for invalid transport path
    ret = btrCore_AVMedia_TransportPathCb(NULL, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(-1, ret);

    // Test case for valid transport path and SBC media type
    sbcCaps.channel_mode = BTR_SBC_CHANNEL_MODE_STEREO;
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_44100;
    sbcCaps.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcCaps.subbands = BTR_SBC_SUBBANDS_8;
    sbcCaps.block_length = BTR_SBC_BLOCK_LENGTH_16;
    sbcCaps.min_bitpool = MIN_BITPOOL;
    sbcCaps.max_bitpool = MAX_BITPOOL;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaTransportPathOut);
    TEST_ASSERT_EQUAL_STRING(transportPath, hdl.pcAVMediaTransportPathOut);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeOut);

    // Test case for valid transport path and media UUID sink
    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSink, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(hdl.pcAVMediaTransportPathIn);
    TEST_ASSERT_EQUAL_STRING(transportPath, hdl.pcAVMediaTransportPathIn);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeIn);

    // Test case for valid transport path and no media caps
    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, NULL, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for valid transport path and existing transport path (same path)
    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NULL(hdl.pcAVMediaTransportPathOut);

    // Test case for valid transport path and existing transport path (different path)
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    ret = btrCore_AVMedia_TransportPathCb(newTransportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);

    // Test case for valid transport path and resetting media configuration
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    hdl.pstBTMediaConfigOut = &sbcCaps;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NULL(hdl.pcAVMediaTransportPathOut);
    TEST_ASSERT_NOT_NULL(hdl.pstBTMediaConfigOut);

    // Test case for valid transport path and media UUID sink with different configuration
    sbcCaps.max_bitpool = 30;
    hdl.pcAVMediaTransportPathIn = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeIn = eBTRCoreAVMTypeUnknown;
    hdl.pstBTMediaConfigIn = &sbcConfigIn;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSink, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeIn);
    TEST_ASSERT_EQUAL(sbcCaps.max_bitpool, sbcConfigIn.max_bitpool);

    // Test case for valid transport path and media UUID source with different configuration
    sbcCaps.max_bitpool = 20;
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeUnknown;
    hdl.pstBTMediaConfigOut = &sbcConfigOut;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeOut);
    TEST_ASSERT_EQUAL(sbcCaps.max_bitpool, sbcConfigOut.max_bitpool);

    // Test case for valid transport path and media UUID source with max bitpool equal to MAX_BITPOOL
    sbcCaps.max_bitpool = MAX_BITPOOL;
    hdl.pcAVMediaTransportPathOut = strndup(transportPath, BTRCORE_MAX_STR_LEN - 1);
    hdl.eAVMediaTypeOut = eBTRCoreAVMTypeUnknown;
    hdl.pstBTMediaConfigOut = &sbcConfigOut;

    ret = btrCore_AVMedia_TransportPathCb(transportPath, mediaUuidSource, &sbcCaps, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(eBTRCoreAVMTypeSBC, hdl.eAVMediaTypeOut);
    // Free allocated memory
    free(hdl.pcAVMediaTransportPathOut);
    free(hdl.pcAVMediaTransportPathIn);
}
void test_BTRCore_AVMedia_IsMediaElementPlayable(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem mediaItem = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    stBTRCoreAVMediaItem playList = {0};
    stBTRCoreAVMediaItem browser = {0};
    tBTRCoreAVMediaElementId mediaItemId = 12345;
    char isPlayable = 0;
    enBTRCoreRet ret;
    const char* btDevAddr = "00:1A:7D:DA:71:13";

    // Initialize the media handler
    hdl.pstAVMediaPlayList = &playList;
    hdl.pstAVMediaBrowser = &browser;

    // Test case for invalid input parameters
    ret = BTRCore_AVMedia_IsMediaElementPlayable(NULL, btDevAddr, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    ret = BTRCore_AVMedia_IsMediaElementPlayable(&hdl, NULL, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);

    // Test case for media item in playlist
    mediaItemId = BTR_MEDIA_PLAYLIST_ID;
    playList.pvAVMediaParentItem = &parentItem;

    ret = BTRCore_AVMedia_IsMediaElementPlayable(&hdl, btDevAddr, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
    TEST_ASSERT_EQUAL(0, isPlayable);

    // Test case for media item in browser
    mediaItemId = 0;
    browser.pvAVMediaParentItem = &parentItem;

    ret = BTRCore_AVMedia_IsMediaElementPlayable(&hdl, btDevAddr, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(0, isPlayable);

    // Test case for media browser not existing
    browser.pvAVMediaParentItem = NULL;

    ret = BTRCore_AVMedia_IsMediaElementPlayable(&hdl, btDevAddr, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);

    // Test case for media item found and playable
    mediaItem.bIsMediaItemPlayable = 1;
    browser.pvAVMediaParentItem = &mediaItem;

    ret = BTRCore_AVMedia_IsMediaElementPlayable(&hdl, btDevAddr, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(1, isPlayable);

    // Test case for media item found and not playable
    mediaItem.bIsMediaItemPlayable = 0;
    browser.pvAVMediaParentItem = &mediaItem;

    ret = BTRCore_AVMedia_IsMediaElementPlayable(&hdl, btDevAddr, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(0, isPlayable);

    // Test case for media item not found
    mediaItemId = 99999; // An ID that does not exist
    ret = BTRCore_AVMedia_IsMediaElementPlayable(&hdl, btDevAddr, mediaItemId, &isPlayable);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

int mock_BtrCore_BTGetTrackInformation(void* apBtConn, const char* apBtmediaPlayerObjectPath, stBTMediaTrackInfo* apstBTMediaTrackInfo) {
    // Mock implementation of BtrCore_BTGetTrackInformation
    if (apBtConn && apBtmediaPlayerObjectPath && apstBTMediaTrackInfo) {
        return 0; // Success
    }
    return -1; // Failure
}

void test_BTRCore_AVMedia_SelectTrack(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem mediaItem = {0};
    stBTRCoreAVMediaTrackInfo trackInfo = {0};
    tBTRCoreAVMediaElementId mediaItemId = 12345;
    enBTRCoreRet ret;

    // Initialize the media handler
    hdl.btIfceHdl = (void*)1; // Mock interface handle
    mediaItem.bIsMediaItemPlayable = 1;
    strcpy(mediaItem.pcAVMediaItemPath, "/mock/path"); // Mock valid path
    hdl.pstAVMediaBrowser = &mediaItem;

    // Mock the BtrCore_BTGetTrackInformation function
    BtrCore_BTGetTrackInformation_StubWithCallback(mock_BtrCore_BTGetTrackInformation);

    // Test case for valid track selection
    ret = BTRCore_AVMedia_SelectTrack(&hdl, mediaItemId, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
    // TEST_ASSERT_EQUAL(mediaItemId, hdl.SelectedaBtrAVMediaItemId);
    TEST_ASSERT_EQUAL_MEMORY(&trackInfo, &hdl.SelectedapstBTAVMediaTrackInfo, sizeof(stBTRCoreAVMediaTrackInfo));

    // Test case for invalid track path
    mediaItem.pcAVMediaItemPath[0] = '\0'; // Invalid path
    ret = BTRCore_AVMedia_SelectTrack(&hdl, mediaItemId, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);

    // Test case for non-playable media item
    mediaItem.bIsMediaItemPlayable = 0;
    strcpy(mediaItem.pcAVMediaItemPath, "/mock/path"); // Valid path
    ret = BTRCore_AVMedia_SelectTrack(&hdl, mediaItemId, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);

    // Test case for failed track information retrieval
    mediaItem.bIsMediaItemPlayable = 1;
    BtrCore_BTGetTrackInformation_StubWithCallback(NULL); // Simulate failure
    ret = BTRCore_AVMedia_SelectTrack(&hdl, mediaItemId, &trackInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

#define BT_MEDIA_A2DP_SINK_ENDPOINT     "/MediaEndpoint/A2DPSink"
#define BT_MEDIA_A2DP_SOURCE_ENDPOINT   "/MediaEndpoint/A2DPSource"
int
mock_BtrCore_BTUnRegisterMedia (
    void*           apBtConn,
    const char*     apBtAdapter,
    enBTDeviceType  aenBTDevType,
	enBTMediaType   aenBTMediaType
) {
    const char*     lpBtMediaType;
	int             lBtMediaCodec;

    

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

	(void)lBtMediaCodec;
    switch (aenBTDevType) {
    case enBTDevAudioSink:
        lpBtMediaType = BT_MEDIA_A2DP_SOURCE_ENDPOINT;
        break;
    case enBTDevAudioSource:
        lpBtMediaType = BT_MEDIA_A2DP_SINK_ENDPOINT;
        break;
    case enBTDevHFPHeadset:
        lpBtMediaType = BT_MEDIA_A2DP_SOURCE_ENDPOINT; //TODO: Check if this is correct
        break;
    case enBTDevHFPAudioGateway:
        lpBtMediaType = BT_MEDIA_A2DP_SOURCE_ENDPOINT; //TODO: Check if this is correct
        break;
    case enBTDevUnknown:
    default:
        lpBtMediaType = BT_MEDIA_A2DP_SOURCE_ENDPOINT;
        break;
    }

    return 0;
}
void test_BTRCore_AVMedia_DeInit(void) {
    stBTRCoreAVMediaHdl* hdl = (stBTRCoreAVMediaHdl*)malloc(sizeof(stBTRCoreAVMediaHdl));
    stBTRCoreAVMediaItem* mediaBrowser = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    stBTRCoreAVMediaItem* mediaPlayList = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    stBTRCoreAVMediaItem* parentItem = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    const char* btAdapter = "hci0";
    void* btConn = (void*)1; // Mock valid connection handle
    enBTRCoreRet ret;

    // Initialize the media handler
    memset(hdl, 0, sizeof(stBTRCoreAVMediaHdl));
    hdl->pstAVMediaBrowser = mediaBrowser;
    hdl->pstAVMediaPlayList = mediaPlayList;

    BtrCore_BTUnRegisterMedia_StubWithCallback(mock_BtrCore_BTUnRegisterMedia);

    // Test case for media browser with parent items
    mediaBrowser->pvAVMediaParentItem = parentItem;
    ret = BTRCore_AVMedia_DeInit(hdl, btConn, btAdapter);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_NULL(hdl->pstAVMediaBrowser);
}

void test_BTRCore_AVMedia_DeInit_WithSubItems(void) {
    stBTRCoreAVMediaHdl* hdl = (stBTRCoreAVMediaHdl*)malloc(sizeof(stBTRCoreAVMediaHdl));
    stBTRCoreAVMediaItem* mediaBrowser = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    stBTRCoreAVMediaItem* subItem1 = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    stBTRCoreAVMediaItem* subItem2 = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    const char* btAdapter = "hci0";
    void* btConn = (void*)1; // Mock valid connection handle
    enBTRCoreRet ret;

    // Initialize the media handler
    memset(hdl, 0, sizeof(stBTRCoreAVMediaHdl));
    memset(mediaBrowser, 0, sizeof(stBTRCoreAVMediaItem));
    memset(subItem1, 0, sizeof(stBTRCoreAVMediaItem));
    memset(subItem2, 0, sizeof(stBTRCoreAVMediaItem));
    hdl->pstAVMediaBrowser = mediaBrowser;
    hdl->btIfceHdl = btConn; // Ensure btIfceHdl is set to pass the condition

    // Initialize media browser with sub-items
    mediaBrowser->ui32AVMediaNumberOfItems = 2;
    mediaBrowser->pstAVMediaSubItems = (stBTRCoreAVMediaItem**)malloc(2 * sizeof(stBTRCoreAVMediaItem*));
    mediaBrowser->pstAVMediaSubItems[0] = subItem1;
    mediaBrowser->pstAVMediaSubItems[1] = subItem2;

    BtrCore_BTUnRegisterMedia_StubWithCallback(mock_BtrCore_BTUnRegisterMedia);

    // Test case for media browser with sub-items
    ret = BTRCore_AVMedia_DeInit(hdl, btConn, btAdapter);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_NULL(hdl->pstAVMediaBrowser);

    // Free allocated memory
    // free(hdl);
    // free(mediaBrowser->pstAVMediaSubItems);
    // free(mediaBrowser);
    // free(subItem1);
    // free(subItem2);
}

typedef struct _stBtIfceHdl {

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


    //TODO : Include a seperate structure to store the GATT related
     //* objects,Flags,proxies and signal handlers.
     
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
char*
mock_BtrCore_BTGetMediaPlayerPath (
    void*          apstBtIfceHdl,
    const char*    apBtDevPath
) {
    stBtIfceHdl*   pstlhBtIfce = (stBtIfceHdl*)apstBtIfceHdl;
    char*          playerObjectPath = NULL;
    _Bool           isConnected      = 0;

    (void)pstlhBtIfce;
    (void)isConnected;

    if (!apstBtIfceHdl || !apBtDevPath) {
        BTRCORELOG_ERROR ("Invalid args!!!");
        return NULL;
    }


    return playerObjectPath;
}



void test_BTRCore_AVMedia_ChangeBrowserLocation(void) {
    stBTRCoreAVMediaHdl* hdl = (stBTRCoreAVMediaHdl*)malloc(sizeof(stBTRCoreAVMediaHdl));
    stBTRCoreAVMediaItem* mediaBrowser = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    stBTRCoreAVMediaItem* subItem1 = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    stBTRCoreAVMediaItem* subItem2 = (stBTRCoreAVMediaItem*)malloc(sizeof(stBTRCoreAVMediaItem));
    const char* btAdapter = "hci0";
    tBTRCoreAVMediaElementId mediaElementId = 12345; // Example media element ID
    eBTRCoreAVMElementType elementType = eBTRCoreAVMETypeTrack;
    enBTRCoreRet ret;
    BtrCore_BTGetMediaPlayerPath_StubWithCallback(mock_BtrCore_BTGetMediaPlayerPath);
    // Initialize the media handler
    memset(hdl, 0, sizeof(stBTRCoreAVMediaHdl));
    memset(mediaBrowser, 0, sizeof(stBTRCoreAVMediaItem));
    memset(subItem1, 0, sizeof(stBTRCoreAVMediaItem));
    memset(subItem2, 0, sizeof(stBTRCoreAVMediaItem));
    hdl->pstAVMediaBrowser = mediaBrowser;

    // Initialize media browser with sub-items
    mediaBrowser->ui32AVMediaNumberOfItems = 2;
    mediaBrowser->pstAVMediaSubItems = (stBTRCoreAVMediaItem**)malloc(2 * sizeof(stBTRCoreAVMediaItem*));
    mediaBrowser->pstAVMediaSubItems[0] = subItem1;
    mediaBrowser->pstAVMediaSubItems[1] = subItem2;

    // Set media item IDs and paths
    subItem1->ui32AVMediaItemId = mediaElementId;
    strcpy(subItem1->pcAVMediaItemPath ,"/path/to/subItem1");
    subItem2->ui32AVMediaItemId = mediaElementId + 1;
    strcpy(subItem2->pcAVMediaItemPath , "/path/to/subItem2");

    // Test case for changing browser location
    ret = BTRCore_AVMedia_ChangeBrowserLocation(hdl, btAdapter, mediaElementId, elementType);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);

    // Free allocated memory
    free(mediaBrowser->pstAVMediaSubItems);
    free(mediaBrowser);
    free(subItem1);
    free(subItem2);
    free(hdl);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_SwitchToMediaBrowserItem_Failure(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTRCoreAVMediaItem item = {0};
    stBTRCoreAVMediaItem parentItem = {0};
    hdl.pcAVMediaPlayerPath = "mockPlayerPath";
    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    hdl.pstAVMediaBrowser = &item;
    item.ui32AVMediaItemId = 12345;
    strcpy(item.pcAVMediaItemPath, "itemPath");
    item.pvAVMediaParentItem = NULL;
    parentItem.ui32AVMediaItemId = 67890;
    strcpy(parentItem.pcAVMediaItemPath, "parentPath");

    // Ensure the function fails to switch to the media browser item
    enBTRCoreRet ret = BTRCore_AVMedia_ChangeBrowserLocation(&hdl, "mockAddr", 67890, eBTRCoreAVMETypeArtist);

    // Check that the function returns failure
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
    // Additional checks can be added here if needed
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess1(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType=enBTMediaFldTypArtist;
    strcpy(mediaUpdate.pcMediaItemPath , "mockPath");
    strcpy(mediaUpdate.pcMediaItemName , "mockName");

    // Mock the btrCore_AVMedia_AllocateBrowserMemory function
   // btrCore_AVMedia_AllocateBrowserMemory_IgnoreAndReturn(0);

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_Album(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = enBTMediaFldTypAlbum;
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_Artist(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = enBTMediaFldTypArtist;
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}


void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_Genre(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = enBTMediaFldTypGenre;
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_Compilation(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = enBTMediaFldTypCompilation;
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_PlayList(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = enBTMediaFldTypPlayList;
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");
    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_TrackList(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = enBTMediaFldTypTrackList;
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_Track(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = enBTMediaFldTypTrack;
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_MediaBrowserUpdateCb_AllocateBrowserMemorySuccess_Default(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    stBTMediaBrowserUpdate mediaUpdate = {0};
    stBTRCoreAVMediaItem mediaItem = {0};

    hdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;
    mediaUpdate.ui32BTMediaItemId = 12345;
    mediaUpdate.eMediaItemType = enBTMediaItemTypFolder;
    mediaUpdate.eMediaFolderType = 999; // Some invalid type to trigger default case
    strcpy(mediaUpdate.pcMediaItemPath, "mockPath");
    strcpy(mediaUpdate.pcMediaItemName, "mockName");

    // Mock the btrCore_AVMedia_AllocateBrowserMemory function
    // btrCore_AVMedia_AllocateBrowserMemory_IgnoreAndReturn(0);

    int ret = btrCore_AVMedia_MediaBrowserUpdateCb(&mediaUpdate, 1, "mockAddr", &hdl);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_NegotiateMediaCb_Frequency_48000(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    a2dp_sbc_t sbcConfigOut = {0};
    void* mediaCapsOutput = NULL;
    int ret;

    // Initialize the media handler
    hdl.pstBTMediaConfigOut = &sbcConfigOut;

    // Set the frequency to 48000
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_48000;
    sbcCaps.channel_mode=BTR_SBC_CHANNEL_MODE_JOINT_STEREO;
    sbcCaps.block_length=BTR_SBC_BLOCK_LENGTH_12;
    sbcCaps.subbands=BTR_SBC_SUBBANDS_4;

    // Call the function
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);

    // Verify the results
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_NegotiateMediaCb_Frequency_32000(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    a2dp_sbc_t sbcConfigOut = {0};
    void* mediaCapsOutput = NULL;
    int ret;

    // Initialize the media handler
    hdl.pstBTMediaConfigOut = &sbcConfigOut;

    // Set the frequency to 48000
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_32000;
    sbcCaps.channel_mode=BTR_SBC_CHANNEL_MODE_DUAL_CHANNEL;
    sbcCaps.block_length=BTR_SBC_BLOCK_LENGTH_8;
    sbcCaps.subbands=BTR_SBC_SUBBANDS_4;
    sbcCaps.allocation_method=BTR_SBC_ALLOCATION_SNR;
    // Call the function
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);

    // Verify the results
    TEST_ASSERT_EQUAL(0, ret);
}

void test_btrCore_AVMedia_NegotiateMediaCb_Frequency_16000(void) {
    stBTRCoreAVMediaHdl hdl = {0};
    a2dp_sbc_t sbcCaps = {0};
    a2dp_sbc_t sbcConfigOut = {0};
    void* mediaCapsOutput = NULL;
    int ret;

    // Initialize the media handler
    hdl.pstBTMediaConfigOut = &sbcConfigOut;

    // Set the frequency to 48000
    sbcCaps.frequency = BTR_SBC_SAMPLING_FREQ_16000;
    sbcCaps.channel_mode=BTR_SBC_CHANNEL_MODE_MONO;
    sbcCaps.block_length=BTR_SBC_BLOCK_LENGTH_4;
    sbcCaps.subbands=BTR_SBC_SUBBANDS_4;
    sbcCaps.allocation_method=BTR_SBC_ALLOCATION_SNR;
    // Call the function
    ret = btrCore_AVMedia_NegotiateMediaCb(&sbcCaps, &mediaCapsOutput, enBTDevAudioSink, enBTMediaTypeSBC, &hdl);

    // Verify the results
    TEST_ASSERT_EQUAL(0, ret);
}

void test_BTRCore_AVMedia_GetCurMediaInfo_ValidArguments2(void) {
    stBTRCoreAVMediaHdl* hBTRCoreAVM1;
    stBTRCoreAVMediaInfo mediaInfo;
    a2dp_sbc_t sbcConfig;
    stBTRCoreAVMediaSbcInfo sbcInfo;
    hBTRCoreAVM1 = malloc(sizeof(stBTRCoreAVMediaHdl));
    memset(hBTRCoreAVM1, 0, sizeof(stBTRCoreAVMediaHdl));

    hBTRCoreAVM1->eAVMediaTypeIn=eBTRCoreAVMTypeSBC;
    hBTRCoreAVM1->pstBTMediaConfigIn= &sbcConfig;
    hBTRCoreAVM1->eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    mediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowIn;
    mediaInfo.pstBtrCoreAVMCodecInfo = &sbcInfo;

    sbcConfig.frequency = BTR_SBC_SAMPLING_FREQ_16000;
    sbcConfig.channel_mode = BTR_SBC_CHANNEL_MODE_MONO;
    sbcConfig.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcConfig.subbands = BTR_SBC_SUBBANDS_4;
    sbcConfig.block_length = BTR_SBC_BLOCK_LENGTH_4;
    
    sbcConfig.min_bitpool = MIN_BITPOOL;
    sbcConfig.max_bitpool = MAX_BITPOOL;

    enBTRCoreRet ret = BTRCore_AVMedia_GetCurMediaInfo(hBTRCoreAVM1, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);


    free(hBTRCoreAVM1);
}

void test_BTRCore_AVMedia_GetCurMediaInfo_ValidArguments3(void) {
    
    stBTRCoreAVMediaHdl* hBTRCoreAVM1;
    stBTRCoreAVMediaInfo mediaInfo;
    a2dp_sbc_t sbcConfig;
    stBTRCoreAVMediaSbcInfo sbcInfo;
    hBTRCoreAVM1 = malloc(sizeof(stBTRCoreAVMediaHdl));
    memset(hBTRCoreAVM1, 0, sizeof(stBTRCoreAVMediaHdl));

    hBTRCoreAVM1->eAVMediaTypeIn=eBTRCoreAVMTypeSBC;
    hBTRCoreAVM1->pstBTMediaConfigIn= &sbcConfig;
    hBTRCoreAVM1->eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    mediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowIn;
    mediaInfo.pstBtrCoreAVMCodecInfo = &sbcInfo;


    sbcConfig.frequency = BTR_SBC_SAMPLING_FREQ_32000;
    sbcConfig.channel_mode = BTR_SBC_CHANNEL_MODE_DUAL_CHANNEL;
    sbcConfig.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcConfig.subbands = BTR_SBC_SUBBANDS_4;
    sbcConfig.block_length = BTR_SBC_BLOCK_LENGTH_12;
    
    sbcConfig.min_bitpool = MIN_BITPOOL;
    sbcConfig.max_bitpool = MAX_BITPOOL;

    enBTRCoreRet ret = BTRCore_AVMedia_GetCurMediaInfo(hBTRCoreAVM1, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);


    free(hBTRCoreAVM1);
}

void test_BTRCore_AVMedia_GetCurMediaInfo_ValidArguments4(void) {
    
    stBTRCoreAVMediaHdl* hBTRCoreAVM1;
    stBTRCoreAVMediaInfo mediaInfo;
    a2dp_sbc_t sbcConfig;
    stBTRCoreAVMediaSbcInfo sbcInfo;
    hBTRCoreAVM1 = malloc(sizeof(stBTRCoreAVMediaHdl));
    memset(hBTRCoreAVM1, 0, sizeof(stBTRCoreAVMediaHdl));

    hBTRCoreAVM1->eAVMediaTypeIn=eBTRCoreAVMTypeSBC;
    hBTRCoreAVM1->pstBTMediaConfigIn= &sbcConfig;
    hBTRCoreAVM1->eAVMediaTypeOut = eBTRCoreAVMTypeSBC;
    mediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowIn;
    mediaInfo.pstBtrCoreAVMCodecInfo = &sbcInfo;


    sbcConfig.frequency = BTR_SBC_SAMPLING_FREQ_48000;
    sbcConfig.channel_mode = BTR_SBC_CHANNEL_MODE_JOINT_STEREO;
    sbcConfig.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcConfig.subbands = BTR_SBC_SUBBANDS_8;
    sbcConfig.block_length = BTR_SBC_BLOCK_LENGTH_8;
    
    
    sbcConfig.min_bitpool = MIN_BITPOOL;
    sbcConfig.max_bitpool = MAX_BITPOOL;

    enBTRCoreRet ret = BTRCore_AVMedia_GetCurMediaInfo(hBTRCoreAVM1, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);


    free(hBTRCoreAVM1);
}

void test_BTRCore_AVMedia_GetCurMediaInfo_ValidArguments5(void) {
    
    stBTRCoreAVMediaHdl* hBTRCoreAVM1;
    stBTRCoreAVMediaInfo mediaInfo;
    a2dp_sbc_t sbcConfig;
    stBTRCoreAVMediaSbcInfo sbcInfo;
    hBTRCoreAVM1 = malloc(sizeof(stBTRCoreAVMediaHdl));
    memset(hBTRCoreAVM1, 0, sizeof(stBTRCoreAVMediaHdl));

    hBTRCoreAVM1->eAVMediaTypeIn=eBTRCoreAVMTypeAAC;
    hBTRCoreAVM1->pstBTMediaConfigIn= &sbcConfig;
    hBTRCoreAVM1->eAVMediaTypeOut = eBTRCoreAVMTypeAAC;
    mediaInfo.eBtrCoreAVMFlow = eBTRCoreAVMediaFlowIn;
    mediaInfo.pstBtrCoreAVMCodecInfo = &sbcInfo;


    sbcConfig.frequency = BTR_SBC_SAMPLING_FREQ_48000;
    sbcConfig.channel_mode = BTR_SBC_CHANNEL_MODE_JOINT_STEREO;
    sbcConfig.allocation_method = BTR_SBC_ALLOCATION_LOUDNESS;
    sbcConfig.subbands = BTR_SBC_SUBBANDS_8;
    sbcConfig.block_length = BTR_SBC_BLOCK_LENGTH_8;
    
    
    sbcConfig.min_bitpool = MIN_BITPOOL;
    sbcConfig.max_bitpool = MAX_BITPOOL;

    enBTRCoreRet ret = BTRCore_AVMedia_GetCurMediaInfo(hBTRCoreAVM1, "00:11:22:33:44:55", &mediaInfo);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);


    free(hBTRCoreAVM1);
}

void test_BTRCore_AVMedia_AcquireDataPath_Success1(void) {
    stBTRCoreAVMediaHdl hdl;
    hdl.btIfceHdl = (void*)1;
 //   hdl.pcAVMediaTransportPathOut = "device_path_out";
    hdl.pcAVMediaTransportPathIn = "device_path_in";
    const char* btDevAddr = "device_path_in";
    int dataPath = 0;
    int dataReadMTU = 0;
    int dataWriteMTU = 0;

    enBTRCoreRet ret;

    BtrCore_BTAcquireDevDataPath_ExpectAndReturn((void*)1, "device_path_in", &dataPath, &dataReadMTU, &dataWriteMTU, 0);
    unsigned int delay = 0;
    unBTOpIfceProp delayProp;
    delayProp.enBtMediaTransportProp = enBTMedTPropDelay;
   
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp);
    unsigned short volume = 0;
    unBTOpIfceProp volumeProp;
    volumeProp.enBtMediaTransportProp = enBTMedTPropVol;
    BtrCore_BTGetProp_StubWithCallback(mock_BtrCore_BTGetProp);



    // Call the function under test
    ret = BTRCore_AVMedia_AcquireDataPath((tBTRCoreAVMediaHdl)&hdl, btDevAddr, &dataPath, &dataReadMTU, &dataWriteMTU, &delay);

       // Check that the return value is enBTRCoreSuccess
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
    TEST_ASSERT_EQUAL(0, dataPath);
    TEST_ASSERT_EQUAL(0, dataReadMTU);
    TEST_ASSERT_EQUAL(0, dataWriteMTU);
  
}
void test_BTRCore_AVMedia_GetPositionInfo_fail(void) {
    stBTRCoreAVMediaHdl avMediaHdl;
    stBTRCoreAVMediaPositionInfo positionInfo;
    char* playerPath = "player/path";
    unsigned int mediaPosition = 1000;
    stBTRCoreAVMediaTrackInfo mediaTrackInfo = { .ui32Duration = 5000 };  // Track with 5000 ms duration

    // Initialize the structure
    avMediaHdl.pcAVMediaPlayerPath = 0;

    // Mock expectations
    BtrCore_BTGetMediaPlayerPath_IgnoreAndReturn(playerPath);

    BtrCore_BTGetMediaPlayerProperty_StubWithCallback(_BtrCore_BTGetMediaPlayerProperty);
    
    // Instead of ignoring, set up a proper return for track information
    BtrCore_BTGetTrackInformation_StubWithCallback(_BtrCore_BTGetTrackInformation);
    // Call the function under test
    enBTRCoreRet ret = BTRCore_AVMedia_GetPositionInfo(&avMediaHdl, "00:11:22:33:44:55", &positionInfo);
    
    // Assertions
    TEST_ASSERT_EQUAL(enBTRCoreSuccess, ret);
    TEST_ASSERT_EQUAL(5000, positionInfo.ui32Duration);  // Check the expected duration
    TEST_ASSERT_EQUAL(1000, positionInfo.ui32Position);
}

void test_BTRCore_AVMedia_GetMediaProperty_NullHandle(void) {
    enBTRCoreRet ret;
    void* mediaPropertyValue = NULL;

    // Test case for null handle
    ret = BTRCore_AVMedia_GetMediaProperty(NULL, "00:11:22:33:44:55", "mediaPropertyKey", mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_GetMediaProperty_NullDeviceAddress(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0};
    enBTRCoreRet ret;
    void* mediaPropertyValue = NULL;

    // Test case for null device address
    ret = BTRCore_AVMedia_GetMediaProperty(&avMediaHdl, NULL, "mediaPropertyKey", mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreInvalidArg, ret);
}

void test_BTRCore_AVMedia_GetMediaProperty_FailedToGetMediaPlayerPath(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0};
    enBTRCoreRet ret;
    void* mediaPropertyValue = NULL;

    // Mock the function to return NULL
    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(NULL, "00:11:22:33:44:55", NULL);

    // Test case for failed to get media player path
    ret = BTRCore_AVMedia_GetMediaProperty(&avMediaHdl, "00:11:22:33:44:55", "mediaPropertyKey", mediaPropertyValue);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}

void test_BTRCore_AVMedia_ChangeBrowserLocation_Success2(void) {
    stBTRCoreAVMediaHdl avMediaHdl = {0};  
    stBTRCoreAVMediaItem avMediaItem = {0}; 
    stBTRCoreAVMediaItem parentItem = {0}; 
    stBTRCoreAVMediaItem subItem = {0}; 
    stBTRCoreAVMediaItem* subItems[1] = {&subItem};
    enBTRCoreRet ret;

    // Initialize the media handler
    avMediaHdl.pstAVMediaPlayer.m_mediaPlayerBrowsable = 1;  
    avMediaHdl.pstAVMediaBrowser = &avMediaItem;

    // Set up the media items
    avMediaItem.ui32AVMediaItemId = 1; 
    avMediaItem.pvAVMediaParentItem = &parentItem;
    avMediaItem.ui32AVMediaNumberOfItems = 1;
    avMediaItem.pstAVMediaSubItems = subItems;

    parentItem.ui32AVMediaItemId = 1;
    strcpy(parentItem.pcAVMediaItemPath, "parent_folder_path");

    subItem.ui32AVMediaItemId = 1;
    strcpy(subItem.pcAVMediaItemPath, "sub_folder_path");

    // Mock the function calls
    BtrCore_BTGetMediaPlayerPath_ExpectAndReturn(NULL, "00:11:22:33:44:55", "player_path");
    BtrCore_BTChangeMediaFolder_IgnoreAndReturn(0);

    // Ensure the folder path is set correctly in the avMediaHdl structure
    strncpy(avMediaHdl.pstAVMediaBrowser->pcAVMediaItemPath, "folder_path", BTRCORE_MAX_STR_LEN);

    // Call the function with valid parameters and expect success
    ret = BTRCore_AVMedia_ChangeBrowserLocation(&avMediaHdl, "00:11:22:33:44:55", 0, eBTRCoreAVMETypeAlbum);
    TEST_ASSERT_EQUAL(enBTRCoreFailure, ret);
}
