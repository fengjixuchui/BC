/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013
*/

/*!
@file    sink_a2dp.h
@brief   Interface to the a2dp profile initialisation functions. 
*/

#ifndef _SINK_A2DP_INIT_H_
#define _SINK_A2DP_INIT_H_


#include <csr_a2dp_decoder_common_plugin.h>
#include "sink_private.h"
#include <a2dp.h>

/* Local stream end point codec IDs */
#define SOURCE_SEID_MASK        0x20        /*!< @brief Combined with a SEP codec id to produce an id for source codecs */
#define SBC_SEID                0x01        /*!< @brief Local Stream End Point ID for SBC codec */
#define MP3_SEID                0x02        /*!< @brief Local Stream End Point ID for MP3 codec */
#define AAC_SEID                0x03        /*!< @brief Local Stream End Point ID for AAC codec */
#define APTX_SEID               0x05        /*!< @brief Local Stream End Point ID for aptX codec */
#ifdef INCLUDE_FASTSTREAM
#define FASTSTREAM_SEID         0x04        /*!< @brief Local Stream End Point ID for FastStream codec */
#endif
#ifdef INCLUDE_APTX_ACL_SPRINT
#define APTX_SPRINT_SEID        0x06        /*!< @brief Local Stream End Point ID for aptX Sprint codec */
#endif


/* The bits used to enable codec support for A2DP, as read from PSKEY_CODEC_ENABLED */
#define SBC_CODEC_BIT           0xFF        /*!< @brief SBC is mandatory and always enabled */
#define MP3_CODEC_BIT           0           /*!< @brief Bit used to enable MP3 codec in PSKEY */
#define AAC_CODEC_BIT           1           /*!< @brief Bit used to enable AAC codec in PSKEY */
#define FASTSTREAM_CODEC_BIT    2           /*!< @brief Bit used to enable FastStream codec in PSKEY */
#define APTX_CODEC_BIT          3           /*!< @brief Bit used to enable aptX codec in PSKEY */
#define APTX_SPRINT_CODEC_BIT   4           /*!< @brief Bit used to enable aptX LL codec in PSKEY */

#define KALIMBA_RESOURCE_ID     1           /*!< @brief Resource ID for Kalimba */

#define MAX_A2DP_CONNECTIONS    2

/* Bits used to select which DAC channel is used to render audio, as read from PSKEY_FEATURE_BLOCK */


typedef enum
{
    a2dp_primary = 0x00,
    a2dp_secondary = 0x01,
    a2dp_pri_sec = 0x02                     
} a2dp_link_priority;

#define A2DP_DEVICE_ID(x) (theSink.a2dp_link_data ? theSink.a2dp_link_data->device_id[x] : 0)
#define for_all_a2dp(idx)      for(idx = 0; idx < MAX_A2DP_CONNECTIONS; idx++)

typedef enum
{
    a2dp_not_suspended,
    a2dp_local_suspended,
    a2dp_local_peer_suspended,
    a2dp_remote_suspended
} a2dp_suspend_state;


#ifdef ENABLE_AVRCP
typedef enum
{
    avrcp_support_unknown,
    avrcp_support_second_attempt,
    avrcp_support_unsupported,
    avrcp_support_supported
} avrcpSupport;
#endif

typedef struct
{
    a2dp_link_priority priority;
} EVENT_STREAM_ESTABLISH_T;

typedef struct
{
#ifdef ENABLE_AVRCP
    avrcpSupport       avrcp_support:2;
#else
    unsigned           unused:2;
#endif    
    unsigned           connected:1;
    unsigned           media_reconnect:1;
    remote_device      peer_device:2;
    a2dp_suspend_state SuspendState:2;
    unsigned seid:8;
    unsigned device_id:8;
    unsigned stream_id:8;
    unsigned list_id:8;
    unsigned gAvVolumeLevel:8;
    uint16 clockMismatchRate;
    bdaddr bd_addr;
} a2dp_instance_data;

/* TODO: Optimise memory usage */
typedef struct
{
    bool remote_connection;
    bool connected[MAX_A2DP_CONNECTIONS];
    bool media_reconnect[MAX_A2DP_CONNECTIONS];
    bool micMuted[MAX_A2DP_CONNECTIONS];
#ifdef PEER_SCATTERNET_DEBUG   /* Scatternet debugging only */
    bool invert_ag_role[MAX_A2DP_CONNECTIONS];
#endif
    remote_device peer_device[MAX_A2DP_CONNECTIONS];
    a2dp_suspend_state SuspendState[MAX_A2DP_CONNECTIONS];
    uint16 device_id[MAX_A2DP_CONNECTIONS];
    uint16 stream_id[MAX_A2DP_CONNECTIONS];
    uint16 seid[MAX_A2DP_CONNECTIONS];  /* TODO: Optimise away */
    uint8 list_id[MAX_A2DP_CONNECTIONS];
    uint8 gAvVolumeLevel[MAX_A2DP_CONNECTIONS];
    bdaddr bd_addr[MAX_A2DP_CONNECTIONS];
    uint16 clockMismatchRate[MAX_A2DP_CONNECTIONS];
#ifdef ENABLE_AVRCP
    avrcpSupport avrcp_support[MAX_A2DP_CONNECTIONS];
#endif
    A2dpPluginConnectParams  a2dp_audio_connect_params;
    A2dpPluginModeParams     a2dp_audio_mode_params;
}a2dp_data;

/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
    InitA2dp
    
DESCRIPTION
    This function initialises the A2DP library.
*/
void InitA2dp(void);

/*************************************************************************
NAME    
    getA2dpIndex
    
DESCRIPTION
    This function tries to find a device id match in the array of a2dp links 
    to that device id passed in

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndex(uint16 DeviceId, uint16 * Index);

/*************************************************************************
NAME    
    getA2dpIndexFromSink
    
DESCRIPTION
    This function tries to find the a2dp device associated with the supplied 
    sink.  The supplied sink can be either a signalling or media channel.

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndexFromSink(Sink sink, uint16 * Index);

/*************************************************************************
NAME    
    InitSeidConnectPriority
    
DESCRIPTION
    Retrieves a list of the preferred Stream End Points to connect with.
*/
uint16 InitSeidConnectPriority(uint8 seid, uint8 *seid_list);


/*************************************************************************
NAME    
    getA2dpStreamData
    
DESCRIPTION
    Function to retreive media sink and state for a given A2DP source

RETURNS
    void
**************************************************************************/
void getA2dpStreamData(a2dp_link_priority priority, Sink* sink, a2dp_stream_state* state);


/*************************************************************************
NAME    
    getA2dpStreamRole
    
DESCRIPTION
    Function to retreive the role (source/sink) for a given A2DP source

RETURNS
    void
**************************************************************************/
void getA2dpStreamRole(a2dp_link_priority priority, a2dp_role_type* role);


/*************************************************************************
NAME    
    getA2dpPlugin
    
DESCRIPTION
    Retrieves the audio plugin for the requested SEP.
*/
Task getA2dpPlugin(uint8 seid);

/****************************************************************************
NAME    
    sinkA2dpInitComplete
    
DESCRIPTION
    Headset A2DP initialisation has completed, check for success. 

RETURNS
    void
**************************************************************************/
void sinkA2dpInitComplete(const A2DP_INIT_CFM_T *msg);

/****************************************************************************
NAME    
    issueA2dpSignallingConnectResponse
    
DESCRIPTION
    Issue response to a signalling channel connect request, following discovery of the 
    remote device type. 

RETURNS
    void
**************************************************************************/
void issueA2dpSignallingConnectResponse(const bdaddr *bd_addr, remote_device peer_device);

/*************************************************************************
NAME    
    handleA2DPSignallingConnectInd
    
DESCRIPTION
    handle a signalling channel connect indication

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnectInd(uint16 DeviceId, bdaddr SrcAddr);

/*************************************************************************
NAME    
    handleA2DPSignallingConnected
    
DESCRIPTION
    handle a successfull confirm of a signalling channel connected

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnected(a2dp_status_code status, uint16 DeviceId, bdaddr SrcAddr);

/*************************************************************************
NAME    
    connectA2dpStream
    
DESCRIPTION
    Issues a request to the A2DP library to establish a media stream to a
    remote device.  The request can be delayed by a certain amount of time 
    if required.

RETURNS
    
**************************************************************************/
void connectA2dpStream (a2dp_link_priority priority, uint16 delay);

/*************************************************************************
NAME    
    handleA2DPOpenInd
    
DESCRIPTION
    handle an indication of an media channel open request, decide whether 
    to accept or reject it

RETURNS
    
**************************************************************************/
void handleA2DPOpenInd(uint16 DeviceId, uint8 seid);

/*************************************************************************
NAME    
    handleA2DPOpenCfm
    
DESCRIPTION
    handle a successfull confirm of a media channel open

RETURNS
    
**************************************************************************/
void handleA2DPOpenCfm(uint16 DeviceId, uint16 StreamId, uint8 seid, a2dp_status_code status);

/*************************************************************************
NAME    
    handleA2DPSignallingDisconnected
    
DESCRIPTION
    handle the disconnection of the signalling channel
RETURNS
    
**************************************************************************/
void handleA2DPSignallingDisconnected(uint16 DeviceId, a2dp_status_code status,  bdaddr SrcAddr);

/*************************************************************************
NAME    
    handleA2DPSignallingLinkloss
    
DESCRIPTION
    handle the indication of a link loss
RETURNS
    
**************************************************************************/
void handleA2DPSignallingLinkloss(uint16 DeviceId);

/*************************************************************************
NAME    
    handleA2DPStartInd
    
DESCRIPTION
    handle the indication of media start ind
RETURNS
    
**************************************************************************/
void handleA2DPStartInd(uint16 DeviceId, uint16 StreamId);

/*************************************************************************
NAME    
    handleA2DPStartStreaming
    
DESCRIPTION
    handle the indication of media start cfm
RETURNS
    
**************************************************************************/
void handleA2DPStartStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status);

/*************************************************************************
NAME    
    handleA2DPSuspendStreaming
    
DESCRIPTION
    handle the indication of media suspend from either the ind or the cfm
RETURNS
    
**************************************************************************/
void handleA2DPSuspendStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status);


/*************************************************************************
NAME    
    handleA2DPStoreClockMismatchRate
    
DESCRIPTION
    handle storing the clock mismatch rate for the active stream
RETURNS
    
**************************************************************************/
void handleA2DPStoreClockMismatchRate(uint16 clockMismatchRate);


/*************************************************************************
NAME    
    handleA2DPStoreCurrentEqBank
    
DESCRIPTION
    handle storing the current EQ bank
RETURNS
    
**************************************************************************/
void handleA2DPStoreCurrentEqBank(uint16 clockMismatchRate);


/* TODO - index below should be a2dp_link_priority not uint8.  */

/*************************************************************************
NAME    
    SuspendA2dpStream
    
DESCRIPTION
    called when it is necessary to suspend an a2dp media stream due to 
    having to process a call from a different AG 
RETURNS
    
**************************************************************************/
void SuspendA2dpStream(a2dp_link_priority priority);
        

/*************************************************************************
NAME    
    ad2pSuspended
    
DESCRIPTION
    Helper to indicate whether A2DP is suspended on given source
RETURNS
    TRUE if A2DP suspended, otherwise FALSE
**************************************************************************/
a2dp_suspend_state a2dpSuspended(a2dp_link_priority priority);


/*************************************************************************
NAME    
    ResumeA2dpStream
    
DESCRIPTION
    Called to resume a suspended A2DP stream
RETURNS
    
**************************************************************************/
void ResumeA2dpStream(a2dp_link_priority priority, a2dp_stream_state state, Sink sink);



#ifdef ENABLE_AVRCP
bool getA2dpVolume(const bdaddr *bd_addr, uint16 *a2dp_volume);


bool setA2dpVolume(const bdaddr *bd_addr, uint16 a2dp_volume);
#endif


/*************************************************************************
NAME    
    handleA2DPMessage
    
DESCRIPTION
    A2DP message Handler, this function handles all messages returned
    from the A2DP library and calls the relevant functions if required

RETURNS
    
**************************************************************************/
void handleA2DPMessage( Task task, MessageId id, Message message );

/*************************************************************************
NAME    
    disconnectAllA2dpAVRCP
    
DESCRIPTION
    disconnect any a2dp and avrcp connections
    
RETURNS
    
**************************************************************************/
void disconnectAllA2dpAvrcp(void);

/*************************************************************************
NAME    
    disconnectAllA2dpPeerDevices
    
DESCRIPTION
    disconnect any a2dp connections to any peer devices
    
RETURNS
    TRUE is any peer devices disconnected, FALSE otherwise
    
**************************************************************************/
bool disconnectAllA2dpPeerDevices (void);

/*************************************************************************
NAME    
    handleA2DPStoreEnhancments
    
DESCRIPTION
    handle storing the current enhancements settings
RETURNS
    
**************************************************************************/
void handleA2DPStoreEnhancements(uint16 enhancements);

/*************************************************************************
NAME    
    controlA2DPPeer
    
DESCRIPTION
    Issues AVDTP Start/Suspend command to current Peer device
    
RETURNS
    FALSE if no appropriate Peer device found
    TRUE if a command was issued
**************************************************************************/
bool controlA2DPPeer (uint16 event);

#endif /* _SINK_A2DP_INIT_H_ */


