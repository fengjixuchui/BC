/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013
*/

/*!
@file    sink_a2dp.c
@brief   a2dp initialisation and control functions
*/

#include "sink_debug.h"
#include "sink_statemanager.h"
#include "sink_states.h"
#include "sink_private.h"
#include "sink_a2dp.h"
#include "sink_debug.h"
#include "sink_devicemanager.h"
#include "sink_link_policy.h"
#include "sink_audio.h"
#include "sink_usb.h"
#include "sink_wired.h"
#include "sink_scan.h"
#include "sink_audio_routing.h"
#include "sink_slc.h"
#include "sink_device_id.h"


#ifdef ENABLE_AVRCP
#include "sink_tones.h"
#endif        

#ifdef ENABLE_GATT
#include "sink_gatt.h"
#endif

#include <bdaddr.h>
#include <a2dp.h>
#include <codec.h>
#include <connection.h>
#include <hfp.h>
#include <stdlib.h>
#include <memory.h>
#include <panic.h>
#include <ps.h>
#include <message.h>
#ifdef ENABLE_SOUNDBAR
#include <inquiry.h>
#endif /* ENABLE_SOUNDBAR */

#include <csr_a2dp_decoder_common_plugin.h>
#include "sink_slc.h"

#ifdef DEBUG_A2DP
#define A2DP_DEBUG(x) DEBUG(x)
#else
#define A2DP_DEBUG(x) 
#endif

static const sep_config_type sbc_sep_snk = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(sbc_caps_sink), sbc_caps_sink };
#ifdef ENABLE_PEER
static const sep_config_type sbc_sep_src = { SOURCE_SEID_MASK | SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(sbc_caps_sink), sbc_caps_sink };   /* Source shares same caps as sink */
#endif

/* not all codecs are available for some configurations, include this define to have access to all codec types  */
#ifdef INCLUDE_A2DP_EXTRA_CODECS
    static const sep_config_type mp3_sep_snk = { MP3_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(mp3_caps_sink), mp3_caps_sink };
    static const sep_config_type aac_sep_snk = { AAC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aac_caps_sink), aac_caps_sink };
    static const sep_config_type aptx_sep_snk = { APTX_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aptx_caps_sink), aptx_caps_sink };
#ifdef ENABLE_PEER
    static const sep_config_type mp3_sep_src = { SOURCE_SEID_MASK | MP3_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(mp3_caps_sink), mp3_caps_sink };   /* Source shares same caps as sink */
    static const sep_config_type aac_sep_src = { SOURCE_SEID_MASK | AAC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(aac_caps_sink), aac_caps_sink };   /* Source shares same caps as sink */
    static const sep_config_type aptx_sep_src = { SOURCE_SEID_MASK | APTX_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(aptx_caps_sink), aptx_caps_sink };   /* Source shares same caps as sink */
#endif
#ifdef INCLUDE_FASTSTREAM                
    static const sep_config_type faststream_sep = { FASTSTREAM_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(faststream_caps_sink), faststream_caps_sink };
#endif
#ifdef INCLUDE_APTX_ACL_SPRINT
    static const sep_config_type aptx_sprint_sep = { APTX_SPRINT_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aptx_acl_sprint_caps_sink), aptx_acl_sprint_caps_sink };
#endif
#endif


#define NUM_SEPS (sizeof(codecList)/sizeof(codec_list_element))

typedef struct
{
    unsigned                bit:8;      /* The bit position in PSKEY_USR_xxx to enable the codec. */
    const sep_config_type   *config;    /* The SEP config data. These configs are defined above. */
    TaskData                *plugin;    /* The audio plugin to use. */
} codec_list_element;

    /* Table which indicates which A2DP codecs are avaiable on the device.
       Add other codecs in priority order, from top to bottom of the table.
    */
    static const codec_list_element codecList[] = 
    {
#ifdef INCLUDE_A2DP_EXTRA_CODECS
#ifdef INCLUDE_APTX_ACL_SPRINT
        {APTX_SPRINT_CODEC_BIT, &aptx_sprint_sep, (TaskData *)&csr_aptx_acl_sprint_decoder_plugin},
#endif
        {APTX_CODEC_BIT, &aptx_sep_snk, (TaskData *)&csr_aptx_decoder_plugin},
        {AAC_CODEC_BIT, &aac_sep_snk, (TaskData *)&csr_aac_decoder_plugin},
        {MP3_CODEC_BIT, &mp3_sep_snk, (TaskData *)&csr_mp3_decoder_plugin},
#ifdef ENABLE_PEER
        {APTX_CODEC_BIT, &aptx_sep_src, (TaskData *)&csr_aptx_decoder_plugin},
        {AAC_CODEC_BIT, &aac_sep_src, (TaskData *)&csr_aac_decoder_plugin},
        {MP3_CODEC_BIT, &mp3_sep_src, (TaskData *)&csr_mp3_decoder_plugin},
#endif
#ifdef INCLUDE_FASTSTREAM                
        {FASTSTREAM_CODEC_BIT, &faststream_sep, (TaskData *)&csr_faststream_sink_plugin},
#endif
#endif
#ifdef ENABLE_PEER
        {SBC_CODEC_BIT, &sbc_sep_src, (TaskData *)&csr_sbc_decoder_plugin},
#endif
        {SBC_CODEC_BIT, &sbc_sep_snk, (TaskData *)&csr_sbc_decoder_plugin}
    };



#ifdef ENABLE_SOUNDBAR
/* Default Silence detection parameters */
#define SILENCE_THRESHOLD (0x000A)        /* Silence detection level threshold (0.000316 = -70 dBFS) */

#define SILENCE_TIMEOUT   (0x000A)        /* Silence duration before message sent to VM */
#endif /*ENABLE_SOUNDBAR*/

/****************************************************************************
  FUNCTIONS
*/


/*************************************************************************
NAME    
    InitA2dp
    
DESCRIPTION
    This function initialises the A2DP library and supported codecs

RETURNS
    A2DP_INIT_CFM message returned, handled by A2DP message handler
**************************************************************************/
void InitA2dp(void)
{
    uint16 i;
#ifdef ENABLE_SOUNDBAR
    silence_detect_settings silenceParams;
    uint16 ret_len;
#endif /*ENABLE_SOUNDBAR*/

    sep_data_type seps[NUM_SEPS];
    uint16 number_of_seps = 0;
    pio_config_type* pio;

    A2DP_DEBUG(("INIT: A2DP\n")); 
    A2DP_DEBUG(("INIT: NUM_SEPS=%u\n",NUM_SEPS)); 

  	/*allocate the memory for the a2dp link data */
#ifdef ENABLE_AVRCP
    theSink.a2dp_link_data = mallocPanic( sizeof(a2dp_data) + sizeof(avrcp_data) );
        /* initialise structure to 0 */    
    memset(theSink.a2dp_link_data, 0, ( sizeof(a2dp_data)  + sizeof(avrcp_data) ));  
#else    
	theSink.a2dp_link_data = mallocPanic( sizeof(a2dp_data));
    /* initialise structure to 0 */    
    memset(theSink.a2dp_link_data, 0, ( sizeof(a2dp_data)));      
#endif

    
    /* Make sure all references to mic parameters point to the right place */
    pio = &theSink.conf1->PIOIO;
    theSink.a2dp_link_data->a2dp_audio_connect_params.mic_params = &pio->digital;
    
    /* set default microphone source for back channel enabled dsp apps */
    theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_NOT_FITTED;
    theSink.a2dp_link_data->a2dp_audio_mode_params.mic_mute = SEND_PATH_UNMUTE;
    

    /* initialise device and stream id's to invalid as 0 is a valid value */
    theSink.a2dp_link_data->device_id[0] = INVALID_DEVICE_ID;
    theSink.a2dp_link_data->stream_id[0] = INVALID_STREAM_ID;   
    theSink.a2dp_link_data->device_id[1] = INVALID_DEVICE_ID;
    theSink.a2dp_link_data->stream_id[1] = INVALID_STREAM_ID;   

#ifdef ENABLE_SOUNDBAR
    
    /* Retrieve the silence detection settings from PSKEY_USR_42 */
    ret_len = PsRetrieve(PSKEY_SILENCE_DETECTION_SETTINGS, &silenceParams, sizeof(silence_detect_settings));

    /* If no key exists then load the default silence detection parameters */
    if(!ret_len)
    {
        theSink.a2dp_link_data->a2dp_audio_connect_params.silence_threshold = SILENCE_THRESHOLD;
        theSink.a2dp_link_data->a2dp_audio_connect_params.silence_trigger_time = SILENCE_TIMEOUT;
    }
    else
    {
        theSink.a2dp_link_data->a2dp_audio_connect_params.silence_threshold = silenceParams.threshold;
        theSink.a2dp_link_data->a2dp_audio_connect_params.silence_trigger_time = silenceParams.trigger_time;
    }
    
    A2DP_DEBUG(("SILENCE DETECT PARAMS FROM PSKEY THRESHOLD %x, TIMEOUT %x\n",
        theSink.a2dp_link_data->a2dp_audio_connect_params.silence_threshold,
        theSink.a2dp_link_data->a2dp_audio_connect_params.silence_trigger_time));    

    theSink.a2dp_link_data->a2dp_audio_connect_params.is_for_soundbar = TRUE; 
    theSink.a2dp_link_data->a2dp_audio_connect_params.speaker_pio = theSink.conf1->PIOIO.pio_outputs.DeviceAudioActivePIO ;
#endif/*ENABLE_SOUNDBAR*/
    
    /* only continue and initialise the A2DP library if it's actually required,
       library functions will return false if it is uninitialised */
    if(theSink.features.EnableA2dpStreaming)
    {
        /* Currently, we just support MP3 as optional codec so it is enabled in features block
        If more optional codecs are supported, we need use seperate PSKEY to enable codec ! */	        
        for (i=0; i<NUM_SEPS; i++)
        {
            if (codecList[i].bit==SBC_CODEC_BIT || (theSink.features.A2dpOptionalCodecsEnabled & (1<<codecList[i].bit)))
            {
                seps[number_of_seps].sep_config = codecList[i].config;
                seps[number_of_seps].in_use = FALSE;
                number_of_seps++;
                
                A2DP_DEBUG(("INIT: Codec Enabled %d\n",i)); 
            }
        }
        
        /* Initialise the A2DP library */
#ifdef ENABLE_PEER
        A2dpInit(&theSink.task, A2DP_INIT_ROLE_SINK | A2DP_INIT_ROLE_SOURCE, NULL, number_of_seps, seps, theSink.conf1->timeouts.A2dpLinkLossReconnectionTime_s);
#else
        A2dpInit(&theSink.task, A2DP_INIT_ROLE_SINK, NULL, number_of_seps, seps, theSink.conf1->timeouts.A2dpLinkLossReconnectionTime_s);
#endif
    }
}

/*************************************************************************
NAME    
    getA2dpIndex
    
DESCRIPTION
    This function tries to find a device id match in the array of a2dp links 
    to that device id passed in

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndex(uint16 DeviceId, uint16 * Index)
{
    uint8 i;
    
    /* go through A2dp connection looking for device_id match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successfull match found */
            if(theSink.a2dp_link_data->device_id[i] == DeviceId)
            {
                *Index = i;
            	A2DP_DEBUG(("A2dp: getIndex = %d\n",i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}

/*************************************************************************
NAME    
    getA2dpIndexFromSink
    
DESCRIPTION
    This function tries to find the a2dp device associated with the supplied 
    sink.  The supplied sink can be either a signalling or media channel.

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndexFromSink(Sink sink, uint16 * Index)
{
    uint8 i;
    
    if (!sink || !theSink.a2dp_link_data)
    {
        return FALSE;
    }
    
    /* go through A2dp connection looking for sink match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successfull match found */
            if(A2dpSignallingGetSink(theSink.a2dp_link_data->device_id[i]) == sink)
            {
                *Index = i;
                return TRUE;
            }
            
            if(A2dpMediaGetSink(theSink.a2dp_link_data->device_id[i], theSink.a2dp_link_data->stream_id[i]) == sink)
            {
                *Index = i;
                return TRUE;
            }
        }
    }
    
    /* no matches found so return not successful */    
    return FALSE;
}

/*************************************************************************
NAME    
    getA2dpStreamData
    
DESCRIPTION
    Function to retreive media sink and state for a given A2DP source

RETURNS
    void
**************************************************************************/
void getA2dpStreamData(a2dp_link_priority priority, Sink* sink, a2dp_stream_state* state)
{
    *state = a2dp_stream_idle;
    *sink  = NULL;

   	A2DP_DEBUG(("A2dp: getA2dpStreamData(%u)\n",(uint16)priority)); 
    if(theSink.a2dp_link_data)
    {
        A2DP_DEBUG(("A2dp: getA2dpStreamData - peer=%u connected=%u\n",theSink.a2dp_link_data->peer_device[priority],theSink.a2dp_link_data->connected[priority])); 
        if(theSink.a2dp_link_data->connected[priority])
        {
            *state = A2dpMediaGetState(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]) ;
            *sink  = A2dpMediaGetSink(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]) ;
            A2DP_DEBUG(("A2dp: getA2dpStreamData - state=%u sink=0x%X\n",*state, (uint16)*sink)); 
        }
    }
}

/*************************************************************************
NAME    
    getA2dpStreamRole
    
DESCRIPTION
    Function to retreive the role (source/sink) for a given A2DP source

RETURNS
    void
**************************************************************************/
void getA2dpStreamRole(a2dp_link_priority priority, a2dp_role_type* role)
{
    *role = a2dp_role_undefined;
    
    if(theSink.a2dp_link_data)
    {
        A2DP_DEBUG(("A2dp: getA2dpStreamRole - peer=%u connected=%u\n",theSink.a2dp_link_data->peer_device[priority],theSink.a2dp_link_data->connected[priority])); 
        if(theSink.a2dp_link_data->connected[priority])
        {
            *role = A2dpMediaGetRole(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]) ;
            A2DP_DEBUG(("A2dp: getA2dpStreamRole - role=%u priority=%u\n",*role,priority)); 
        }
    }
}

/*************************************************************************
NAME    
    getA2dpPlugin
    
DESCRIPTION
    This function returns the task of the appropriate audio plugin to be used
    for the selected codec type when connecting audio

RETURNS
    task of relevant audio plugin
**************************************************************************/
Task getA2dpPlugin(uint8 seid)
{
    uint16 i;
    
    for (i=0; i<NUM_SEPS; i++)
    {
        if (codecList[i].config && (codecList[i].config->seid == seid))
        {
            return codecList[i].plugin;
        }
    }
    
	/* No plugin found so Panic */
	Panic();
	return 0;
}


#ifdef ENABLE_PEER
static bool getA2dpPeerIndex(uint16* Index)
{
    uint8 i;
    
    /* go through A2dp connection looking for device_id match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successfull match found */
            if(theSink.a2dp_link_data->peer_device[i] == remote_device_peer)
            {
                *Index = i;
            	A2DP_DEBUG(("A2dp: getPeerIndex = %d\n",i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}

static bool getA2dpAvIndex(uint16* Index)
{
    uint8 i;
    
    /* go through A2dp connection looking for device_id match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successfull match found */
            if(theSink.a2dp_link_data->peer_device[i] == remote_device_nonpeer)
            {
                *Index = i;
            	A2DP_DEBUG(("A2dp: getAvIndex = %d\n",i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}

static bool findStreamingAvSource(a2dp_link_priority* priority)
{
    A2DP_DEBUG(("findStreamingAvSource\n"));
    
    if (theSink.a2dp_link_data)
    {
        uint16 i;
        for (i = 0; i<MAX_A2DP_CONNECTIONS; i++)
        {
            A2DP_DEBUG(("... pri:%u\n", i));
            
            if ( theSink.a2dp_link_data->connected[i] )
            {   /* Found a connected device */
                uint16 device_id = theSink.a2dp_link_data->device_id[i];
                uint16 stream_id = theSink.a2dp_link_data->stream_id[i];
                
                A2DP_DEBUG(("...... dev:%u str:%u state:%u\n", device_id, stream_id, A2dpMediaGetState(device_id, stream_id)));
            
                switch ( A2dpMediaGetState(device_id, stream_id) )
                {
                case a2dp_stream_opening:
                case a2dp_stream_open:
                case a2dp_stream_starting:
                case a2dp_stream_streaming:
                case a2dp_stream_suspending:
                    A2DP_DEBUG(("......... role:%u\n",A2dpMediaGetRole(device_id, stream_id)));
                    if ( A2dpMediaGetRole(device_id, stream_id)==a2dp_sink )
                    {   /* We have a sink endpoint active to the remote device, therefore it is a source */
                        A2DP_DEBUG(("............ found sink\n"));
                        
                        if (priority != NULL)
                        {
                            *priority = i;
                        }
                        return TRUE;
                    }
                    break;
                    
                default:
                    break;
                }
            }
        }
    }
    
    return FALSE;
}

static void handleA2dpCodecConfigureIndFromPeer(A2DP_CODEC_CONFIGURE_IND_T* ind)
{
    a2dp_link_priority priority;
    
    A2DP_DEBUG(("A2DP InithandleA2dpCodecConfigureIndFromPeerSuccess dev:%u seid:0x%X\n", ind->device_id, ind->local_seid));
    
    /* Find Av Src device - there will be only one streaming src */
    if ( !findStreamingAvSource(&priority) )
    {   /* No streaming AV Src found */
        A2DP_DEBUG(("... Streaming Av Src not found pri:%u\n",priority));
        A2dpCodecConfigureResponse(ind->device_id, FALSE, ind->local_seid, 0, NULL);
    }
    else
    {
        /* Obtain AV Src media settings */
        a2dp_codec_settings* codec_settings = A2dpCodecGetSettings(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]);
        
        if ( !(codec_settings->seid & SOURCE_SEID_MASK) && ((codec_settings->seid | SOURCE_SEID_MASK) == ind->local_seid) )
        {   /* Source and sink seids use matching codec */
            /* Request same codec configuration settings as AV Source for peer device */
            A2DP_DEBUG(("... Configurimg codec dev:%u seid:0x%X\n",ind->device_id, ind->local_seid));
            A2dpCodecConfigureResponse(ind->device_id, TRUE, ind->local_seid, codec_settings->size_configured_codec_caps, codec_settings->configured_codec_caps);
        }
        else
        {   /* Source and sink seids do not use matching codec */
            A2DP_DEBUG(("... Non matching codecs dev:%u seid:0x%X\n",ind->device_id, ind->local_seid));
            A2dpCodecConfigureResponse(ind->device_id, FALSE, ind->local_seid, 0, NULL);
        }
        
        free(codec_settings);
    }
}
#endif

/****************************************************************************
NAME    
    sinkA2dpInitComplete
    
DESCRIPTION
    Sink A2DP initialisation has completed, check for success. 

RETURNS
    void
**************************************************************************/
void sinkA2dpInitComplete(const A2DP_INIT_CFM_T *msg)
{   
    /* check for successfull initialisation of A2DP libraray */
    if(msg->status == a2dp_success)
    {
        A2DP_DEBUG(("A2DP Init Success\n"));
    }
    else
    {
	    A2DP_DEBUG(("A2DP Init Failed [Status %d]\n", msg->status));
        Panic();
    }
}


/****************************************************************************
NAME    
    issueA2dpSignallingConnectResponse
    
DESCRIPTION
    Issue response to a signalling channel connect request, following discovery of the 
    remote device type. 

RETURNS
    void
**************************************************************************/
void issueA2dpSignallingConnectResponse(const bdaddr *bd_addr, remote_device peer_device)
{
    A2DP_DEBUG(("issueA2dpSignallingConnectResponse peer=%u\n",peer_device));
    
    if (theSink.a2dp_link_data)
    {
        uint16 idx;
        
        for_all_a2dp(idx)
        {
            if (!theSink.a2dp_link_data->connected[idx] && BdaddrIsSame(bd_addr, &theSink.a2dp_link_data->bd_addr[idx]))
            {
#ifdef ENABLE_PEER
                theSink.a2dp_link_data->peer_device[idx] = peer_device;
                
#ifdef DISABLE_PEER_PDL
                if (peer_device != remote_device_peer)
#endif
                {
                    sink_attributes attributes;
                    deviceManagerGetDefaultAttributes(&attributes, FALSE);
                    deviceManagerGetAttributes(&attributes, bd_addr);
                    attributes.peer_device = peer_device;
                    deviceManagerStoreAttributes(&attributes, bd_addr);
                }
#endif

                A2DP_DEBUG(("Accept\n"));
                A2dpSignallingConnectResponse(theSink.a2dp_link_data->device_id[idx],TRUE);
                
#ifdef ENABLE_AVRCP
                sinkAvrcpCheckManualConnectReset((bdaddr *)bd_addr);        
#endif                
            }
        }
    }
}


/*************************************************************************
NAME    
    handleA2DPSignallingConnectInd
    
DESCRIPTION
    handle a signalling channel connect indication

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnectInd(uint16 DeviceId, bdaddr SrcAddr)
{
    /* indicate that this is a remote connection */
    theSink.a2dp_link_data->remote_connection = TRUE;   
    
    /* before accepting check there isn't already a signalling channel connected to another AG */		
    if ( (theSink.features.EnableA2dpStreaming) &&
         ((!theSink.a2dp_link_data->connected[a2dp_primary]) || (!theSink.a2dp_link_data->connected[a2dp_secondary])) )
    {
        /* store the device_id for the new connection in the first available storage position */
        uint16 priority = (!theSink.a2dp_link_data->connected[a2dp_primary]) ? a2dp_primary : a2dp_secondary;
        
        A2DP_DEBUG(("Signalling Success, Primary ID = %x\n",DeviceId));
        theSink.a2dp_link_data->connected[priority] = FALSE;
        theSink.a2dp_link_data->device_id[priority] = DeviceId;
        theSink.a2dp_link_data->bd_addr[priority] = SrcAddr;            
        theSink.a2dp_link_data->list_id[priority] = deviceManagerSetPriority(&SrcAddr);
        
#ifdef ENABLE_PEER
        {
            sink_attributes attributes;
            deviceManagerGetDefaultAttributes(&attributes, FALSE);
            deviceManagerGetAttributes(&attributes, &SrcAddr);
            theSink.a2dp_link_data->peer_device[priority] = attributes.peer_device;
        }

        if (theSink.a2dp_link_data->peer_device[priority] == remote_device_unknown)
        {   /* Determine remote device type before accepting connection */
            A2DP_DEBUG(("Unknown device type - requesting device id record\n"));
            RequestRemoteDeviceId(&SrcAddr);
        }
        else
#endif
        {   /* Accept the connection */
            A2DP_DEBUG(("Accept\n"));
            A2dpSignallingConnectResponse(DeviceId,TRUE);
#ifdef ENABLE_AVRCP
            sinkAvrcpCheckManualConnectReset(&SrcAddr);        
#endif                
        }
    }
    else
    {
        A2DP_DEBUG(("Reject\n"));
        A2dpSignallingConnectResponse(DeviceId,FALSE);
    }
}


/*************************************************************************
NAME    
    handleA2DPSignallingConnected
    
DESCRIPTION
    handle a successfull confirm of a signalling channel connected

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnected(a2dp_status_code status, uint16 DeviceId, bdaddr SrcAddr)
{
    /* Continue connection procedure */
    if(!theSink.a2dp_link_data->remote_connection)    
    {
        MessageSendLater(&theSink.task,EventContinueSlcConnectRequest,0,theSink.conf1->timeouts.SecondAGConnectDelayTime_s);
    }
    else
    {
        /* reset remote connection indication flag */    
        theSink.a2dp_link_data->remote_connection = FALSE;
    }
            
    /* check for successfull connection */
    if (status != a2dp_success)
    {
        uint16 priority;
        
		A2DP_DEBUG(("Signalling Failed device=%u [Status %d]\n", DeviceId, status));
        
        /* If necessary, clear appropriate link data structure which will have been filled on an incoming connection */
        if ((status != a2dp_wrong_state) && (status != a2dp_max_connections))   /* TODO: Temp fix */
        if ( BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[priority=a2dp_primary]) || 
             BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[priority=a2dp_secondary]) )
        {
            A2DP_DEBUG(("Clearing link data for %u\n", priority));
            theSink.a2dp_link_data->peer_device[priority] = remote_device_unknown;
            theSink.a2dp_link_data->connected[priority] = FALSE;
            theSink.a2dp_link_data->device_id[priority] = INVALID_DEVICE_ID;
            BdaddrSetZero(&theSink.a2dp_link_data->bd_addr[priority]);
            theSink.a2dp_link_data->list_id[priority] = 0;
        }
        
#ifdef ENABLE_AVRCP
        sinkAvrcpCheckManualConnectReset(&SrcAddr);        
#endif

        /* if a failed inquiry connect then restart it */
        if((theSink.inquiry.action == rssi_pairing)&&(theSink.inquiry.session == inquiry_session_peer))        
        {
            inquiryStop();
            inquiryPair( inquiry_session_peer, FALSE );
        }
    }
    /* connection was successfull */
    else
	{
	    /* Send a message to request a role indication and make necessary changes as appropriate, message
           will be delayed if a device initiated connection to another device is still in progress */
        A2DP_DEBUG(("handleA2DPSignallingConnected: Asking for role check\n"));
        /* cancel any pending messages and replace with a new one */
        MessageCancelFirst(&theSink.task , EventCheckRole);    
        MessageSendConditionally (&theSink.task , EventCheckRole , NULL , &theSink.rundata->connection_in_progress  );
    
		/* check for a link loss condition, if the device has suffered a link loss and was
           succesfully reconnected by the a2dp library a 'signalling connected' event will be 
           generated, check for this and retain previous connected ID for this indication */
        if(((theSink.a2dp_link_data->connected[a2dp_primary])&&(BdaddrIsSame(&SrcAddr, &theSink.a2dp_link_data->bd_addr[a2dp_primary])))||
           ((theSink.a2dp_link_data->connected[a2dp_secondary])&&(BdaddrIsSame(&SrcAddr, &theSink.a2dp_link_data->bd_addr[a2dp_secondary]))))
        {
            /* reconnection is the result of a link loss, don't assign a new id */    		
            A2DP_DEBUG(("Signalling Connected following link loss [Status %d]\n", status));
        }
        else
        {
            /* store the device_id for the new connection in the first available storage position */
            if (BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[a2dp_primary]) || 
                (BdaddrIsZero(&theSink.a2dp_link_data->bd_addr[a2dp_primary]) && !theSink.a2dp_link_data->connected[a2dp_primary]))
            {
            	A2DP_DEBUG(("Signalling Success, Primary ID = %x\n",DeviceId));
                theSink.a2dp_link_data->connected[a2dp_primary] = TRUE;
                theSink.a2dp_link_data->device_id[a2dp_primary] = DeviceId;
                theSink.a2dp_link_data->bd_addr[a2dp_primary] = SrcAddr;            
                theSink.a2dp_link_data->list_id[a2dp_primary] = deviceManagerSetPriority(&SrcAddr);
                theSink.a2dp_link_data->media_reconnect[a2dp_primary] = FALSE;
            }
            /* this is the second A2DP signalling connection */
            else if (BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[a2dp_secondary]) || 
                     (BdaddrIsZero(&theSink.a2dp_link_data->bd_addr[a2dp_secondary]) && !theSink.a2dp_link_data->connected[a2dp_secondary]))
            {
            	A2DP_DEBUG(("Signalling Success, Secondary ID = %x\n",DeviceId));
                theSink.a2dp_link_data->connected[a2dp_secondary] = TRUE;
                theSink.a2dp_link_data->device_id[a2dp_secondary] = DeviceId;
                theSink.a2dp_link_data->bd_addr[a2dp_secondary] = SrcAddr;            
                theSink.a2dp_link_data->list_id[a2dp_secondary] = deviceManagerSetPriority(&SrcAddr);
                theSink.a2dp_link_data->media_reconnect[a2dp_secondary] = FALSE;
            }
        }
        
  		/* Ensure the underlying ACL is encrypted */       
        ConnectionSmEncrypt( &theSink.task , A2dpSignallingGetSink(DeviceId) , TRUE );
        ConnectionSetLinkSupervisionTimeout(A2dpSignallingGetSink(DeviceId), 0x1f80);

        /* We are now connected */
		if (stateManagerGetState() < deviceConnected && stateManagerGetState() != deviceLimbo)
            stateManagerEnterConnectedState(); 	
	
        /* update number of connected devices */
	    theSink.no_of_profiles_connected = deviceManagerNumConnectedDevs();

		/* If the device is off then disconnect */
		if (stateManagerGetState() == deviceLimbo)
    	{      
       		A2dpSignallingDisconnectRequest(DeviceId);
    	}
		else
		{
            a2dp_link_priority priority;
            sink_attributes attributes;
            
            /* Use default attributes if none exist is PS */
            deviceManagerGetDefaultAttributes(&attributes, FALSE);
            deviceManagerGetAttributes(&attributes, &SrcAddr);
                
			/* For a2dp connected Tone only */
			MessageSend(&theSink.task,  EventA2dpConnected, 0);	
					
            /* find structure index of deviceId */
            if(getA2dpIndex(DeviceId, (uint16*)&priority))
            {
                bdaddr HfpAddr;
                
                theSink.a2dp_link_data->gAvVolumeLevel[priority] = attributes.a2dp.volume;
                theSink.a2dp_link_data->clockMismatchRate[priority] = attributes.a2dp.clock_mismatch;
                
#ifdef ENABLE_PEER
                if (attributes.peer_device != remote_device_unknown)
                {   /* Only update link data if device type is already known to us */
                    theSink.a2dp_link_data->peer_device[priority] = attributes.peer_device;
                }
                
                if ((slcDetermineConnectAction() & AR_Rssi) && theSink.inquiry.results)
                {
                    theSink.a2dp_link_data->peer_device[priority] = (theSink.inquiry.results[theSink.inquiry.attempting].peer_device) ? remote_device_peer : remote_device_nonpeer;
                    attributes.peer_device = theSink.a2dp_link_data->peer_device[priority];
                }
                
            	A2DP_DEBUG(("Remote device type = %u\n",theSink.a2dp_link_data->peer_device[priority]));
#endif

#if defined ENABLE_PEER && defined DISABLE_PEER_PDL
                if (theSink.a2dp_link_data->peer_device[priority] != remote_device_peer)
#endif
                {   /* Make sure we store this device */
                    attributes.profiles |= sink_a2dp;
                    deviceManagerStoreAttributes(&attributes, &SrcAddr);
                }
                
                
                /* Enable A2dp link loss management if remote device does not have HFP connected */
                if( !(HfpLinkGetBdaddr(hfp_primary_link, &HfpAddr) && BdaddrIsSame(&SrcAddr, &HfpAddr)) &&
                    !(HfpLinkGetBdaddr(hfp_secondary_link, &HfpAddr) && BdaddrIsSame(&SrcAddr, &HfpAddr)) )
                {
                    A2dpDeviceManageLinkloss(DeviceId, TRUE);
                }
                
                /* check on signalling check indication if the a2dp was previously in a suspended state,
                   this can happen if the device has suspended a stream and the phone has chosen to drop
                   the signalling channel completely, open the media connection or the feature to open a media
                   channel on signalling connected option is enabled */
#ifdef ENABLE_PEER
                if (theSink.a2dp_link_data->peer_device[priority] != remote_device_peer)
#endif
                {   /* Unknown or non-peer device */
                if((theSink.a2dp_link_data->SuspendState[priority] == a2dp_local_suspended)||
                   (theSink.features.EnableA2dpMediaOpenOnConnection))
                    {
                        connectA2dpStream( priority, D_SEC(5) );
                    }
                }
#ifdef ENABLE_PEER
                else
                {   /* Peer device */
#ifdef AUDIOSHARING_NO_AAC_RELAY
                    uint16 AvId;
                    
                    /* Mark AAC as unavailable whilst in a ShareMe session */
                    A2dpCodecSetAvailable(a2dp_primary, AAC_SEID, FALSE);
                    A2dpCodecSetAvailable(a2dp_secondary, AAC_SEID, FALSE);
                    
                    /* Close any media stream to AV Source if it's using AAC */
                    if (getA2dpAvIndex(&AvId) && theSink.a2dp_link_data->seid[AvId]==AAC_SEID)
                    {
                        theSink.a2dp_link_data->media_reconnect[AvId] = TRUE;
                        A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]);
                    }
#endif
                    connectA2dpStream( priority, D_SEC(0) );
                }
#endif
			}
            
            /* if rssi pairing check to see if need to cancel rssi pairing or not */           
            if(theSink.inquiry.action == rssi_pairing)
            {
                /* if rssi pairing has completed then stop it progressing further */            
                if(!((theSink.features.PairIfPDLLessThan)&&( ConnectionTrustedDeviceListSize() < theSink.features.PairIfPDLLessThan )))
                {
                    inquiryStop();
                }
            }

#ifdef ENABLE_AVRCP
            {
            if(theSink.features.avrcp_enabled)
                {
#ifdef ENABLE_PEER
                    /* Peer devices do not support AVRCP between each other */
                    if (theSink.a2dp_link_data->peer_device[priority] != remote_device_peer)
#endif
                    {    
                        if (theSink.avrcp_link_data->avrcp_manual_connect)
                            theSink.avrcp_link_data->avrcp_play_addr = SrcAddr;
                        sinkAvrcpConnect(&theSink.a2dp_link_data->bd_addr[priority], DEFAULT_AVRCP_1ST_CONNECTION_DELAY);     
                    }
                }
            }
#endif            
		}
	}
}


/*************************************************************************
NAME    
    connectA2dpStream
    
DESCRIPTION
    Issues a request to the A2DP library to establish a media stream to a
    remote device.  The request can be delayed by a certain amount of time 
    if required.

RETURNS
    
**************************************************************************/
void connectA2dpStream (a2dp_link_priority priority, uint16 delay)
{
    A2DP_DEBUG(("A2dp: connectA2dpMedia[%u] delay=%u\n", priority, delay)); 
    
    if (!delay)
    {
        if (theSink.a2dp_link_data && theSink.a2dp_link_data->connected[priority])
        {
#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[priority] == remote_device_unknown)
            {   /* Still waiting for Device Id SDP search outcome issued in handleA2DPSignallingConnected() */
                EVENT_STREAM_ESTABLISH_T *message = PanicUnlessNew(EVENT_STREAM_ESTABLISH_T);
                
                message->priority = priority;
                MessageSendLater(&theSink.task, EventStreamEstablish, message, 200);  /* Ideally we'd send conditionally, but there isn't a suitable variable */
                
                A2DP_DEBUG(("local device is unknown, re-issue sstream establish event\n")); 
            }
            else if (theSink.a2dp_link_data->peer_device[priority] == remote_device_peer)
            {
                uint16 AvId;
                A2DP_DEBUG(("local device is peer\n"));
                if (getA2dpAvIndex(&AvId))
                {   /* Need to determine if AV Source is already streaming */
                    A2DP_DEBUG(("Have AV Source, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId])));
                    if ((A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_starting) ||
                        (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_streaming))
                   {   /* Open media channel to peer if AV Source is streaming */
                        uint8 seid = theSink.a2dp_link_data->seid[AvId] | SOURCE_SEID_MASK;
                        A2DP_DEBUG(("AV Source stream non-idle\n"));
#ifdef AUDIOSHARING_NO_AAC_RELAY
                        /* Don't open AAC based stream to peer device */
                        if (seid != (AAC_SEID | SOURCE_SEID_MASK))
#endif
                        {
                            A2DP_DEBUG(("Send open req to peer, seid=0x%X\n", seid));
                            A2dpMediaOpenRequest(theSink.a2dp_link_data->device_id[priority], 1, &seid);
                        }
                    }
                }
            }
            else if (theSink.a2dp_link_data->peer_device[priority] == remote_device_nonpeer)
#endif
            {   /* Open media channel to AV Source */
                A2DP_DEBUG(("local device is non-peer (AV Source)\n"));
                if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[priority], 0) == a2dp_stream_idle)
                {
                    A2DP_DEBUG(("AV Source stream idle\n"));
                    A2DP_DEBUG(("Send open req to AV Source, using defualt seid list\n"));
                    A2dpMediaOpenRequest(theSink.a2dp_link_data->device_id[priority], 0, NULL);
                }
            }
        }
    }
    else
    {
        EVENT_STREAM_ESTABLISH_T *message = PanicUnlessNew(EVENT_STREAM_ESTABLISH_T);
        
        message->priority = priority;
        MessageSendLater(&theSink.task, EventStreamEstablish, message, delay);
        
        A2DP_DEBUG(("... wait for %u msecs\n", delay)); 
    }
}


/*************************************************************************
NAME    
    handleA2DPOpenInd
    
DESCRIPTION
    handle an indication of an media channel open request, decide whether 
    to accept or reject it

RETURNS
    
**************************************************************************/
void handleA2DPOpenInd(uint16 DeviceId, uint8 seid)
{
   	A2DP_DEBUG(("A2dp: OpenInd DevId = %d, seid = 0x%X\n",DeviceId, seid)); 

#ifdef ENABLE_PEER
    {
        uint16 Id;
        
        if (getA2dpIndex(DeviceId, &Id))
        {
            theSink.a2dp_link_data->seid[Id] = seid;
            
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
            {   /* Just accept media stream from peer device */
                A2DP_DEBUG(("Ind from peer\n"));
                A2DP_DEBUG(("Send open resp to peer\n"));
                A2dpMediaOpenResponse(DeviceId, TRUE);
            }
            else
            {   /* Open ind from true AV source */
                A2DP_DEBUG(("Ind from non-peer (AV Source)\n"));
                A2DP_DEBUG(("Send open resp to AV Source\n"));
                A2dpMediaOpenResponse(DeviceId, TRUE);
            }
        }
    }
#else        
    /* accept this media connection */
    if(A2dpMediaOpenResponse(DeviceId, TRUE))    
    {
        uint16 Id;
		A2DP_DEBUG(("Open Success\n"));
           
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
            theSink.a2dp_link_data->device_id[Id] = DeviceId;

    }
#endif
}

/*************************************************************************
NAME    
    handleA2DPOpenCfm
    
DESCRIPTION
    handle a successfull confirm of a media channel open

RETURNS
    
**************************************************************************/
void handleA2DPOpenCfm(uint16 DeviceId, uint16 StreamId, uint8 seid, a2dp_status_code status)
{
    bool status_avrcp = FALSE;
    
	/* ensure successfull confirm status */
	if (status == a2dp_success)
	{
        uint16 Id;
#ifdef ENABLE_AVRCP            
        uint16 i;
#endif
        A2DP_DEBUG(("Open Success\n"));
           
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
        {
            /* set the current seid */         
            theSink.a2dp_link_data->device_id[Id] = DeviceId;
            theSink.a2dp_link_data->stream_id[Id] = StreamId;
            theSink.a2dp_link_data->seid[Id] = seid;
            theSink.a2dp_link_data->media_reconnect[Id] = FALSE;
       
#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
            {
                uint16 AvId;
                A2DP_DEBUG(("Cfm from peer\n"));
                if (getA2dpAvIndex(&AvId))
                {
                    A2DP_DEBUG(("Have AV Source, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId])));
                    if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_opening)
                    {   /* Peer media stream is now connected, accept AV source media stream */
                        A2DP_DEBUG(("Send open resp to AV source\n"));
                        A2dpMediaOpenResponse(theSink.a2dp_link_data->device_id[AvId], TRUE);
                    }
                    else if ((A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_starting) ||
                             (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_streaming))
                    {   /* Peer media stream is now connected, start peer media stream */
                        A2DP_DEBUG(("Send start req to peer\n"));
                        A2dpMediaStartRequest(DeviceId, StreamId);
                    }
                }
            }
#endif
            
           /* Start the Streaming if if in the suspended state */
           if(theSink.a2dp_link_data->SuspendState[Id] == a2dp_local_suspended)
           {          
#ifdef ENABLE_AVRCP            
                /* does the device support AVRCP and is AVRCP currently connected to this device? */
                for_all_avrcp(i)
                {    
                    /* ensure media is streaming and the avrcp channel is that requested to be paused */
                    if ((theSink.avrcp_link_data->connected[i])&& 
                        (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[Id], &theSink.avrcp_link_data->bd_addr[i])))
                    {
                        /* attempt to resume playing the a2dp stream */
                        status_avrcp = sinkAvrcpPlayPauseRequest(i,AVRCP_PLAY);
                        break;
                    }
                }
#endif
                /* if not avrcp enabled, use media start instead */
                if(!status_avrcp)
                {
                    A2dpMediaStartRequest(DeviceId, StreamId);
                    A2DP_DEBUG(("Open Success - suspended - start streaming\n"));
                }

                /* reset suspended state once start is sent*/            
                theSink.a2dp_link_data->SuspendState[Id] = a2dp_not_suspended;
            }
        }
	}
	else
	{
		A2DP_DEBUG(("Open Failure [result = %d]\n", status));
#ifdef ENABLE_PEER
        {
            uint16 Id;
            
            if (getA2dpIndex(DeviceId, &Id))
            {
                if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
                {
                    uint16 AvId;
                    A2DP_DEBUG(("Cfm from peer\n"));
                    if (getA2dpAvIndex(&AvId))
                    {
                        A2DP_DEBUG(("Have AV Src, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId])));
                        if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_opening)
                        {   /* Peer media channel failed to establish, just go ahead and accept media channel from AV source */
                            A2DP_DEBUG(("Send open resp to AV Src\n"));
                            A2dpMediaOpenResponse(theSink.a2dp_link_data->device_id[AvId], TRUE);
                        }
                        else if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_starting)
                        {   /* Peer media channel failed to establish, just go ahead and accept Start request from AV source */
                            A2DP_DEBUG(("Send start resp to AV Src\n"));
                            A2dpMediaStartResponse(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId], TRUE);
                        }
                    }
                }
            }
        }
#endif
	}	
}

/*************************************************************************
NAME    
    handleA2DPClose
    
DESCRIPTION
    handle the close of a media channel 

RETURNS
    
**************************************************************************/
static void handleA2DPClose(uint16 DeviceId, uint16 StreamId, a2dp_status_code status)
{		
    /* check the status of the close indication/confirm */    
    if((status == a2dp_success) || (status == a2dp_disconnect_link_loss))
    {
        Sink sink = A2dpSignallingGetSink(DeviceId);
        
       	A2DP_DEBUG(("A2dp: Close DevId = %d, StreamId = %d\n",DeviceId,StreamId)); 

        /* route the audio using the appropriate codec/plugin */
 	    audioHandleRouting(audio_source_none);
#ifdef ENABLE_SOUNDBAR
            /*Set back the LE SCAN priority to normal */
            InquirySetPriority(inquiry_normal_priority);
#endif /* ENABLE_SOUNDBAR */

        /* update the link policy */
	    linkPolicyUseA2dpSettings(DeviceId, StreamId, sink);

        /* change device state if currently in one of the A2DP specific states */
        if(stateManagerGetState() == deviceA2DPStreaming)
        {
            /* the enter connected state function will determine if the signalling
               channel is still open and make the approriate state change */
            stateManagerEnterConnectedState();
        }
        
        /* user configurable event notification */
        MessageSend(&theSink.task, EventA2dpDisconnected, 0);
 
#ifdef ENABLE_PEER
        {
            uint16 Id;
        
            if (getA2dpIndex(DeviceId, &Id))
            {
                /* Reset seid now that media channel has closed */
                theSink.a2dp_link_data->seid[Id] = 0;

                if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
                {   /* Peer device has closed its media channel, now look to see if AV source is trying to initiate streaming */
                    uint16 AvId;
                    A2DP_DEBUG(("Ind from peer\n"));
                    if (getA2dpAvIndex(&AvId))
                    {
                        A2DP_DEBUG(("Have AV Source, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId])));
                        if ((A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_streaming) ||
                            (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_starting))
                        {   /* AV source is either opening or starting a stream, peer needs to follow */
                            uint8 seid = theSink.a2dp_link_data->seid[AvId] | SOURCE_SEID_MASK;
#ifdef AUDIOSHARING_NO_AAC_RELAY
                            /* Don't open AAC based stream to peer device */
                            if (seid != (AAC_SEID | SOURCE_SEID_MASK))
#endif
                            {
                                A2DP_DEBUG(("Send open req to peer, seid=0x%X\n", seid));
                                A2dpMediaOpenRequest(theSink.a2dp_link_data->device_id[Id], 1, &seid);
                            }
                        }
                    }
                }
                else if (theSink.a2dp_link_data->peer_device[Id] == remote_device_nonpeer)
                {   /* AV Source closed it's media channel, might as well close any peer media channel too */
                    uint16 PeerId;
                    A2DP_DEBUG(("Ind from non peer (AV Source)\n"));
                    if (getA2dpPeerIndex(&PeerId))
                    {
                        A2DP_DEBUG(("Have peer, role=%u\n",A2dpMediaGetRole(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId])));
                        if (A2dpMediaGetRole(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]) == a2dp_source)
                        {   /* Peer has a media channel established and we are a source to it, so close */
                            A2DP_DEBUG(("Send close req to peer\n"));
                            A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]);
                        }
                    }
                    
#ifdef AUDIOSHARING_NO_AAC_RELAY
                    if (theSink.a2dp_link_data->media_reconnect[Id])
                    {   /* Disconnect signalling channel to AV Source (for IOP reasons) */
                        A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[Id]);
                    }
#endif
                }
            }
        }
#endif
        
#ifdef ENABLE_AVRCP
        if(theSink.features.avrcp_enabled)
        {
            uint16 Id;
            /* assume device is stopped for AVRCP 1.0 devices */
            if(getA2dpIndex(DeviceId, &Id))
                sinkAvrcpSetPlayStatus(&theSink.a2dp_link_data->bd_addr[Id], avrcp_play_status_stopped);
        }
#endif        
	}
    else
    {
       	A2DP_DEBUG(("A2dp: Close FAILED status = %d\n",status)); 
    }
}

/*************************************************************************
NAME    
    handleA2DPSignallingDisconnected
    
DESCRIPTION
    handle the disconnection of the signalling channel
RETURNS
    
**************************************************************************/
void handleA2DPSignallingDisconnected(uint16 DeviceId, a2dp_status_code status,  bdaddr SrcAddr)
{
    uint16 Id;
    bool reconnect = FALSE;
    
    /* check for successful disconnection status */
    if(getA2dpIndex(DeviceId, &Id))
    {
       	A2DP_DEBUG(("A2dp: SigDiscon DevId = %d\n",DeviceId)); 
#if defined ENABLE_PEER && defined DISABLE_PEER_PDL
        if (theSink.a2dp_link_data->peer_device[Id] != remote_device_peer)
#endif
        {   /* Store the attributes in PS */
            deviceManagerUpdateAttributes(&SrcAddr, sink_a2dp, 0, Id);   
        }
        
#ifdef AUDIOSHARING_NO_AAC_RELAY
        if ((theSink.a2dp_link_data->peer_device[Id] == remote_device_nonpeer) && (theSink.a2dp_link_data->media_reconnect[Id]))
        {
            reconnect = TRUE;
        }
#endif

        /* update the a2dp parameter values */
        theSink.a2dp_link_data->peer_device[Id] = remote_device_unknown;
        theSink.a2dp_link_data->media_reconnect[Id] = FALSE;
        BdaddrSetZero(&theSink.a2dp_link_data->bd_addr[Id]);
        theSink.a2dp_link_data->connected[Id] = FALSE;
        theSink.a2dp_link_data->SuspendState[Id] = a2dp_not_suspended;
        A2DP_DEBUG(("A2dp: SuspendState[%u] = %d\n",Id,theSink.a2dp_link_data->SuspendState[Id])); 
        theSink.a2dp_link_data->device_id[Id] = INVALID_DEVICE_ID;
        theSink.a2dp_link_data->stream_id[Id] = INVALID_STREAM_ID; 
        theSink.a2dp_link_data->list_id[Id] = INVALID_LIST_ID;
        theSink.a2dp_link_data->seid[Id] = 0;
#ifdef ENABLE_AVRCP
        theSink.a2dp_link_data->avrcp_support[Id] = avrcp_support_unknown;
#endif        

        /* update number of connected devices */
	    theSink.no_of_profiles_connected = deviceManagerNumConnectedDevs();
      
        /*if the device is off then this is disconnect as part of the power off cycle, otherwise check
          whether device needs to be made connectable */	
    	if ( stateManagerGetState() != deviceLimbo)
        {
            /* Kick role checking now a device has disconnected */
            linkPolicyCheckRoles();
            
            /* at least one device disconnected, re-enable connectable for another 60 seconds */
            sinkEnableMultipointConnectable();

            /* if the device state still shows connected and there are no profiles currently
               connected then update the device state to reflect the change of connections */
    	    if ((stateManagerIsConnected()) && (!theSink.no_of_profiles_connected))
    	    {
    	        stateManagerEnterConnectableState( FALSE ) ;
    	    }
        }
        
#ifdef ENABLE_AVRCP
        if(theSink.features.avrcp_enabled)
        {
            sinkAvrcpDisconnect(&SrcAddr);     
        }
#endif
        
        if (reconnect)
        {
            A2dpSignallingConnectRequest(&SrcAddr);
        }
#ifdef AUDIOSHARING_NO_AAC_RELAY
        else if (!getA2dpPeerIndex(&Id))
        {   /* Mark AAC as available again now that any ShareMe sessions have ended */
            A2dpCodecSetAvailable(a2dp_primary, AAC_SEID, TRUE);
            A2dpCodecSetAvailable(a2dp_secondary, AAC_SEID, TRUE);
        }
#endif

#ifdef ENABLE_GATT
        sinkGattUpdateBatteryReportDisconnection();      
#endif        

    }    
    else
       	A2DP_DEBUG(("A2dp: Sig Discon FAILED status = %d\n",status)); 

}
       
/*************************************************************************
NAME    
    handleA2DPSignallingLinkloss
    
DESCRIPTION
    handle the indication of a link loss
RETURNS
    
**************************************************************************/
void handleA2DPSignallingLinkloss(uint16 DeviceId)
{
    uint16 Id;
    
    if (getA2dpIndex(DeviceId, &Id))
    {
        /* Kick role checking now a device has disconnected */
        linkPolicyCheckRoles();
        
        audioHandleRouting(audio_source_none);
        
        if(theSink.features.GoConnectableDuringLinkLoss || (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer))
        {   /* Go connectable if feature enabled or remote is a peer device */
            sinkEnableConnectable(); 
            MessageCancelAll(&theSink.task, EventConnectableTimeout);   /* Ensure connectable mode does not get turned off */
        }

        MessageSend(&theSink.task,  EventLinkLoss, 0);
    }
}

/*************************************************************************
NAME    
    handleA2DPStartStreaming
    
DESCRIPTION
    handle the indication of media start ind
RETURNS
    
**************************************************************************/
void handleA2DPStartInd(uint16 DeviceId, uint16 StreamId)
{
#ifdef ENABLE_PEER
    uint16 Id;
    if (getA2dpIndex(DeviceId, &Id))
    {
        /* update the link policy */
        linkPolicyUseA2dpSettings(DeviceId, StreamId, A2dpSignallingGetSink(DeviceId));
        
        if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
        {   /* Just accept media stream from peer device */
            A2DP_DEBUG(("Ind from peer\n"));
            A2DP_DEBUG(("Send start resp to peer\n"));
            A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
        }
        else
        {   /* Open ind from true AV source */
            uint16 PeerId;
            A2DP_DEBUG(("Ind from non-peer (AV Source)\n"));
            if (!getA2dpPeerIndex(&PeerId))
            {   /* No peer device connected, so just accept the media stream */
                A2DP_DEBUG(("Do not have peer\n"));
                A2DP_DEBUG(("Send start resp to AV Source\n"));
                A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
            }
            else
            {   /* Need to establish stream between peer devices before accepting this one */
                A2DP_DEBUG(("Have peer, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->device_id[PeerId])));
                if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]) == a2dp_stream_idle)
                {   /* A media stream between the peer devices does not exist, we need to establish one */
                    uint8 seid = theSink.a2dp_link_data->seid[Id] | SOURCE_SEID_MASK;
                    A2DP_DEBUG(("Peer stream idle\n"));
#ifdef AUDIOSHARING_NO_AAC_RELAY
                    /* Don't open AAC based stream to peer device */
                    if (seid != (AAC_SEID | SOURCE_SEID_MASK))
#endif
                    {
                        A2DP_DEBUG(("Send open req to peer, seid=0x%X\n", seid));
                        A2dpMediaOpenRequest(theSink.a2dp_link_data->device_id[PeerId], 1, &seid);
                    }
                }
                else if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]) == a2dp_stream_open)
                {   /* Media stream exists, but no streming is occuring */
                    A2DP_DEBUG(("Peer stream open\n"));
                    if (A2dpMediaGetRole(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]) == a2dp_source)
                    {   /* Peer has a media channel established and we are a source to it, so issue start to this one */
                        A2DP_DEBUG(("Peer is a sink\n"));
                        A2DP_DEBUG(("Send start req to peer\n"));
                        A2dpMediaStartRequest(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]);
                    }
                    else
                    {   /* Peer media channel is a source and hence "owned" by other peer - close it */
                        A2DP_DEBUG(("Peer is a source\n"));
                        if (theSink.a2dp_link_data->SuspendState[PeerId] == a2dp_local_suspended)
                        {   /* We have suspended the Peer source device from our side, hence accept  */
                            /* this streaming request as we probably want to listen to the AV Source */
                            A2DP_DEBUG(("Peer suspended locally.  Send start resp to AV Source\n"));
                            A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
                        }
                        else
                        {
                            A2DP_DEBUG(("Send close req to peer\n"));
                            A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]);
                        }
                    }
                }
                else
                {   /* Media stream exists between peers and must be streaming from other peer. */  
                    /* Accept this streaming request as it is more IOP friendly to do so.       */
                    A2DP_DEBUG(("Peer stream streaming\n"));
                    A2DP_DEBUG(("Send start resp to AV Source\n"));
                    A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
                }
            }
        }
    }
#else
    
#if defined(ENABLE_SOUNDBAR) && defined(ENABLE_SUBWOOFER)
    /* Only accept START if the soundbar "source" is currently A2DP */
    if ( (theSink.rundata->routed_audio_source == audio_source_none) ||
         (theSink.rundata->routed_audio_source == audio_source_AG1) || 
         (theSink.rundata->routed_audio_source == audio_source_AG1) )
    {
        /* A2DP "source" is active so accept the START request */
        A2DP_DEBUG(("SW : Accept A2DP START\n"));
        A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
    }
    else
    {
        /* A2DP "source" is not active so reject the START request */
        A2DP_DEBUG(("SW : Reject A2DP START - Routed source is %d\n", theSink.rundata->routed_audio_source));
        A2dpMediaStartResponse(DeviceId, StreamId, FALSE);
    }
#else
    /* Always accept the media stream */
    A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
#endif
    
#endif
}

/*************************************************************************
NAME    
    handleA2DPStartStreaming
    
DESCRIPTION
    handle the indication of media start cfm
RETURNS
    
**************************************************************************/
void handleA2DPStartStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status)
{   
    /* check success status of indication or confirm */
    if(status == a2dp_success)
    {
        uint16 Id;     
        Sink sink = A2dpMediaGetSink(DeviceId, StreamId);
        
        A2DP_DEBUG(("A2dp: StartStreaming DevId = %d, StreamId = %d\n",DeviceId,StreamId));    
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
        {
            A2DP_DEBUG(("A2dp: SuspendState[%u] = %d\n",Id,theSink.a2dp_link_data->SuspendState[Id])); 

            /* Ensure suspend state is cleared now streaming has started/resumed */
            theSink.a2dp_link_data->SuspendState[Id] = a2dp_not_suspended;        
            /* route the audio using the appropriate codec/plugin */
            audioHandleRouting(audio_source_none);
            
            /* enter the stream a2dp state if not in a call */
            stateManagerEnterA2dpStreamingState();
            /* update the link policy */
            linkPolicyUseA2dpSettings(DeviceId, StreamId, sink);
            /* set the current seid */         
            theSink.a2dp_link_data->stream_id[Id] = StreamId;

#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
            {   /* Peer media channel has started */
                uint16 AvId;
                A2DP_DEBUG(("Cfm from peer\n"));
                if (getA2dpAvIndex(&AvId))
                {
                    A2DP_DEBUG(("Have AV Source, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId])));
                    if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_starting)
                    {   /* AV Source is waiting for a response to a start streaming request, send it now */
                        A2DP_DEBUG(("AV stream starting\n"));
                        A2DP_DEBUG(("Send start resp to AV Source\n"));
                        A2dpMediaStartResponse(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId], TRUE);
                    }
                    
                    if (A2dpMediaGetRole(theSink.a2dp_link_data->device_id[Id], theSink.a2dp_link_data->stream_id[Id]) == a2dp_source)
                    {   /* Don't manage link loss on Peer connection when relaying audio, to prevent broken audio due to paging */
                        A2dpDeviceManageLinkloss(theSink.a2dp_link_data->device_id[Id], FALSE);
                    }
                    else if (A2dpMediaGetRole(theSink.a2dp_link_data->device_id[Id], theSink.a2dp_link_data->stream_id[Id]) == a2dp_sink)
                    {   /* Don't manage link loss to AG when Peer is streaming to us, to prevent broken audio due to paging */
                        A2dpDeviceManageLinkloss(theSink.a2dp_link_data->device_id[AvId], FALSE);
                    }
                }
            }
#endif
            
#ifdef ENABLE_AVRCP           
            if(theSink.features.avrcp_enabled)
            {
                /* assume device is playing for AVRCP 1.0 devices */
                sinkAvrcpSetPlayStatus(&theSink.a2dp_link_data->bd_addr[Id], avrcp_play_status_playing);
            }
#endif 
            /* Only update EQ mode for non peer devices i.e. for streams to be rendered locally */
            if (theSink.a2dp_link_data->peer_device[Id]!=remote_device_peer)
            {
                /* Set the Stored EQ mode, ensure the DSP is currently streaming A2DP data before trying to
               set EQ mode as it might be that the device has a SCO routed instead */
                if(theSink.routed_audio == sink)
                {
                    AUDIO_MODE_T mode = AUDIO_MODE_CONNECTED;
                
                    A2DP_DEBUG(("A2dp: StartStreaming Set EQ mode = %d\n",theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing)); 
                
                    /* if a2dp gain is 0 set the mode to mute otherwise very quiet audio will still be heard */
                    if (theSink.conf1->gVolMaps[ theSink.a2dp_link_data->gAvVolumeLevel[Id] ].A2dpGain == VOLUME_A2DP_MUTE_GAIN)
                    {
                        mode = AUDIO_MODE_MUTE_SPEAKER;
                    }
                    
                    /* set both EQ and Enhancements enables */
                    AudioSetMode(mode, &theSink.a2dp_link_data->a2dp_audio_mode_params);
                }
                else
                {
                    A2DP_DEBUG(("A2dp: Wrong sink Don't Set EQ mode = %d\n",theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing)); 
                }
            }
        }
    }
    else
    {
       	A2DP_DEBUG(("A2dp: StartStreaming FAILED status = %d\n",status)); 
#ifdef ENABLE_PEER
        {
            uint16 Id;
            
            if (getA2dpIndex(DeviceId, &Id) && (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer))
            {   /* Peer has rejected start of media channel, need to respond to any outstanding request from AV source */
                uint16 AvId;
                A2DP_DEBUG(("Cfm from peer\n"));
                if (getA2dpAvIndex(&AvId))
                {
                    A2DP_DEBUG(("Have AV Source, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId])));
                    if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_starting)
                    {   /* Accept AV source's request to start streaming */
                        A2DP_DEBUG(("AV stream starting\n"));
                        A2DP_DEBUG(("Send start resp to AV Source\n"));
                        A2dpMediaStartResponse(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId], TRUE);
                    }
                }
            }
        }
#endif
    }

}             

/*************************************************************************
NAME    
    handleA2DPSuspendStreaming
    
DESCRIPTION
    handle the indication of media suspend from either the ind or the cfm
RETURNS
    
**************************************************************************/
void handleA2DPSuspendStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status)
{
    Sink sink = A2dpMediaGetSink(DeviceId, StreamId);
    
#ifdef ENABLE_SOUNDBAR
    /*Set back the LE SCAN priority to normal */
    InquirySetPriority(inquiry_normal_priority);
#endif /* ENABLE_SOUNDBAR */

    /* if the suspend was not successfull, issue a close instead */
    if(status == a2dp_rejected_by_remote_device)
    {
       	A2DP_DEBUG(("A2dp: Suspend Failed= %x, try close DevId = %d, StreamId = %d\n",status,DeviceId,StreamId)); 
        /* suspend failed so close media streaming instead */
        A2dpMediaCloseRequest(DeviceId, StreamId);
    }
    /* check success status of indication or confirm */
    else 
    {
        uint16 Id;
        
        A2DP_DEBUG(("A2dp: Suspend Ok DevId = %d, StreamId = %d\n",DeviceId,StreamId)); 
 
        if(getA2dpIndex(DeviceId, &Id)) 
        {
            /* no longer streaming so enter connected state if applicable */    	
            if(stateManagerGetState() == deviceA2DPStreaming)
            {
                /* the enter connected state function will determine if the signalling
                   channel is still open and make the approriate state change */
                stateManagerEnterConnectedState();
            }
            
            /* route the audio using the appropriate codec/plugin */
            audioHandleRouting(audio_source_none);
            
            /* Ensure suspend state is set once audio been de-routed */
            if (theSink.a2dp_link_data->SuspendState[Id] == a2dp_not_suspended)
            {
                theSink.a2dp_link_data->SuspendState[Id] = a2dp_remote_suspended;
            }
            A2DP_DEBUG(("A2dp: SuspendState[%u] = %d\n",Id,theSink.a2dp_link_data->SuspendState[Id])); 
    
            /* update the link policy */
            linkPolicyUseA2dpSettings(DeviceId, StreamId, sink);
            
#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_nonpeer)
            {   /* AV Source suspended it's media channel, might as well suspend any peer media channel too */
                uint16 PeerId;
                A2DP_DEBUG(("Ind/Cfm from non-peer (AV Source)\n"));
                if (getA2dpPeerIndex(&PeerId))
                {
                    A2DP_DEBUG(("Have peer, role=%u\n",A2dpMediaGetRole(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId])));
                    if (A2dpMediaGetRole(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]) == a2dp_source)
                    {   /* Peer has a media channel established and we are a source to it, so suspend */
                        A2DP_DEBUG(("Peer is a sink\n"));
                        A2DP_DEBUG(("Send suspend req to peer\n"));
                        A2dpMediaSuspendRequest(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]);
                    }
                    
                    /* Ensure link loss is managed on Peer connection now AV Source has ceased streaming */
                    A2dpDeviceManageLinkloss(theSink.a2dp_link_data->device_id[PeerId], TRUE);
                }
                
                /* Ensure link loss is managed on AV Source connection now streaming has ceased */
                A2dpDeviceManageLinkloss(theSink.a2dp_link_data->device_id[Id], TRUE);
            }
            else if ((theSink.a2dp_link_data->peer_device[Id] == remote_device_peer) && (theSink.a2dp_link_data->SuspendState[Id] == a2dp_remote_suspended))
            {   /* Peer suspended it's media channel, look to see if local device has a streaming AV source */
                uint16 AvId;
                A2DP_DEBUG(("Ind/Cfm from peer\n"));
                if (getA2dpAvIndex(&AvId))
                {   /* Need to determine if AV Source is already streaming */
                    A2DP_DEBUG(("Have AV Source, stream state=%u\n",A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId])));
                    if ((A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_streaming) ||
                        (A2dpMediaGetState(theSink.a2dp_link_data->device_id[AvId], theSink.a2dp_link_data->stream_id[AvId]) == a2dp_stream_starting))
                    {   /* Check peer stream role */
                        A2DP_DEBUG(("AV Source stream non-idle\n"));
                        if (A2dpMediaGetRole(theSink.a2dp_link_data->device_id[Id], theSink.a2dp_link_data->stream_id[Id]) == a2dp_sink)
                        {   /* Peer is source, so need to close this stream so we can setup a new stream and become the source */
                            A2DP_DEBUG(("Peer is source, close stream\n"));
                            A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[Id], theSink.a2dp_link_data->stream_id[Id]);
                        }
                    }
                    
                    /* Ensure link loss is managed on AV Source connection now streaming has ceased */
                    A2dpDeviceManageLinkloss(theSink.a2dp_link_data->device_id[AvId], TRUE);
                }
                
                /* Ensure link loss is managed on Peer connection now AV Source has ceased streaming */
                A2dpDeviceManageLinkloss(theSink.a2dp_link_data->device_id[Id], TRUE);
            }
#endif
            
#ifdef ENABLE_AVRCP
            if (theSink.features.avrcp_enabled)
            {    
                /* assume device is paused for AVRCP 1.0 devices */
                sinkAvrcpSetPlayStatus(&theSink.a2dp_link_data->bd_addr[Id], avrcp_play_status_paused);
            }
#endif
        }
    }
}
  
/*************************************************************************
NAME    
    SuspendA2dpStream
    
DESCRIPTION
    called when it is necessary to suspend an a2dp media stream due to 
    having to process a call from a different AG. If the device supports
    AVRCP then issue a 'pause' which is far more reliable than trying a
    media_suspend request.
    
RETURNS
    
**************************************************************************/
void SuspendA2dpStream(a2dp_link_priority priority)
{
    bool status = FALSE;

#ifdef ENABLE_AVRCP            
    uint16 i;
#endif    
    A2DP_DEBUG(("A2dp: Suspend A2DP Stream %x\n",priority)); 

    /* set the local suspend status indicator */
    if (theSink.a2dp_link_data->SuspendState[priority] == a2dp_not_suspended)
    {
        theSink.a2dp_link_data->SuspendState[priority] = a2dp_local_suspended;
    }
    A2DP_DEBUG(("A2dp: SuspendState[%u] = %d\n",priority,theSink.a2dp_link_data->SuspendState[priority])); 

#ifdef ENABLE_AVRCP            
    /* does the device support AVRCP and is AVRCP currently connected to this device? */
    for_all_avrcp(i)
    {    
        /* ensure media is streaming and the avrcp channel is that requested to be paused */
        if ((theSink.avrcp_link_data->connected[i])&&(theSink.a2dp_link_data->connected[priority])&&
            (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[priority], &theSink.avrcp_link_data->bd_addr[i])))
        {
            /* check whether the a2dp connection is streaming data */
            if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[i], theSink.a2dp_link_data->stream_id[i]) == a2dp_stream_streaming)
            {
                /* attempt to pause the a2dp stream */
                status = sinkAvrcpPlayPauseRequest(i,AVRCP_PAUSE);
            }
            break;
        }
    }
#endif
    
    /* attempt to suspend stream if avrcp pause was not successfull, if not successfull then close it */
    if(!status)
    {
        if(!A2dpMediaSuspendRequest(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]))
        {
            /* suspend failed so close media streaming */
            A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]);
        }
    }

    /* no longer streaming so enter connected state if applicable */    	
    if(stateManagerGetState() == deviceA2DPStreaming)
    {
        /* the enter connected state function will determine if the signalling
           channel is still open and make the approriate state change */
        stateManagerEnterConnectedState();
    }
}


/*************************************************************************
NAME    
    a2dpSuspended
    
DESCRIPTION
    Helper to indicate whether A2DP is suspended on given source
RETURNS
    TRUE if A2DP suspended, otherwise FALSE
**************************************************************************/
a2dp_suspend_state a2dpSuspended(a2dp_link_priority priority)
{
    if(!theSink.a2dp_link_data) return FALSE;
    return theSink.a2dp_link_data->SuspendState[priority];
}
   

/*************************************************************************
NAME    
    ResumeA2dpStream
    
DESCRIPTION
    Called to resume a suspended A2DP stream
RETURNS
    
**************************************************************************/
void ResumeA2dpStream(a2dp_link_priority priority, a2dp_stream_state state, Sink sink)
{
    bool status_avrcp = FALSE;
#ifdef ENABLE_AVRCP            
    uint16 i;
#endif
    
    A2DP_DEBUG(("A2dp: ResumeA2dpStream\n" )) ;   

    /* need to check whether the signalling channel hsa been dropped by the AV/AG source */
    if(A2dpSignallingGetState(theSink.a2dp_link_data->device_id[priority]) == a2dp_signalling_connected)
    {
        /* is media channel still open? or is it streaming already? */
        if(state == a2dp_stream_open)
        {
#ifdef ENABLE_AVRCP            
            /* does the device support AVRCP and is AVRCP currently connected to this device? */
            for_all_avrcp(i)
            {    
                /* ensure media is streaming and the avrcp channel is that requested to be paused */
                if ((theSink.avrcp_link_data->connected[i])&&(theSink.a2dp_link_data->connected[priority])&&
                    (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[priority], &theSink.avrcp_link_data->bd_addr[i])))
                {
                    /* attempt to resume playing the a2dp stream */
                    status_avrcp = sinkAvrcpPlayPauseRequest(i,AVRCP_PLAY);
                    /* update state */
                    theSink.a2dp_link_data->SuspendState[priority] = a2dp_not_suspended;
                    break;
                }
            }
#endif
            /* if not successful in resuming play via avrcp try a media start instead */  
            if(!status_avrcp)
            {
                A2DP_DEBUG(("A2dp: Media Start\n" )) ;   
                A2dpMediaStartRequest(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]);
                /* reset the SuspendState indicator */
                theSink.a2dp_link_data->SuspendState[priority] = a2dp_not_suspended;
                A2DP_DEBUG(("A2dp: SuspendState[%u] = %d\n",priority,theSink.a2dp_link_data->SuspendState[priority])); 
            }
        }
        /* media channel wasn't open, source not supporting suspend */            
        else if(state < a2dp_stream_open) 
        {
            A2DP_DEBUG(("A2dp: Media Open\n" )) ;   
            connectA2dpStream( priority, 0 );
        }
        /* recovery if media has resumed streaming reconnect its audio */
        else if(state == a2dp_stream_streaming)
        {
#ifdef ENABLE_AVRCP            
            /* does the device support AVRCP and is AVRCP currently connected to this device? */
            for_all_avrcp(i)
            {    
                /* ensure media is streaming and the avrcp channel is that requested to be paused */
                if ((theSink.avrcp_link_data->connected[i])&&(theSink.a2dp_link_data->connected[priority])&&
                    (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[priority], &theSink.avrcp_link_data->bd_addr[i])))
                {
                    /* attempt to resume playing the a2dp stream */
                    status_avrcp = sinkAvrcpPlayPauseRequest(i,AVRCP_PLAY);
                    break;
                }
            }
#endif
            theSink.a2dp_link_data->SuspendState[priority] = a2dp_not_suspended;
            A2DP_DEBUG(("A2dp: SuspendState[%u] = %d\n",priority,theSink.a2dp_link_data->SuspendState[priority])); 
            A2dpRouteAudio(priority, sink);
        }
    }
    /* signalling channel is no longer present so attempt to reconnect it */
    else
    {
        A2DP_DEBUG(("A2dp: Connect Signalling\n" )) ;   
        A2dpSignallingConnectRequest(&theSink.a2dp_link_data->bd_addr[priority]);
    }
}


#ifdef ENABLE_AVRCP
/*************************************************************************
NAME    
    getA2dpVolume
    
DESCRIPTION
    Retrieve the A2DP volume for the connection to the device with the address specified.
    
RETURNS
    Returns TRUE if the volume was retrieved, FALSE otherwise.
    The actual volume is returned in the a2dp_volume variable.
    
**************************************************************************/
bool getA2dpVolume(const bdaddr *bd_addr, uint16 *a2dp_volume)
{
    uint8 i;
    
    /* go through A2dp connection looking for match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its bdaddr */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a match is found return its volume level and a
               status of successfull match found */
            if(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[i], bd_addr))
            {
                *a2dp_volume = theSink.a2dp_link_data->gAvVolumeLevel[i];
            	A2DP_DEBUG(("A2dp: getVolume = %d\n", i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}  


/*************************************************************************
NAME    
    setA2dpVolume
    
DESCRIPTION
    Sets the A2DP volume for the connection to the device with the address specified.
    
RETURNS
    Returns TRUE if the volume was set, FALSE otherwise.
    
**************************************************************************/
bool setA2dpVolume(const bdaddr *bd_addr, uint16 a2dp_volume)
{
    uint8 i;
    
    /* go through A2dp connection looking for match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its bdaddr */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a match is found set its volume level and a
               status of successfull match found */
            if(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[i], bd_addr))
            {
                /* get current volume for this profile */
                uint16 lOldVol = theSink.a2dp_link_data->gAvVolumeLevel[i];
                
                theSink.a2dp_link_data->gAvVolumeLevel[i] = a2dp_volume;                                                                      
              
            	A2DP_DEBUG(("A2dp: setVolume = %d\n", i));
                
                if(theSink.routed_audio && (theSink.routed_audio == A2dpMediaGetSink(theSink.a2dp_link_data->device_id[i], theSink.a2dp_link_data->stream_id[i])))
                {
                    if(theSink.a2dp_link_data->gAvVolumeLevel[i] == VOLUME_A2DP_MAX_LEVEL)             
                        MessageSend ( &theSink.task , EventVolumeMax , 0 );
              
                    if(theSink.a2dp_link_data->gAvVolumeLevel[i] == VOLUME_A2DP_MIN_LEVEL)                             
                        MessageSend ( &theSink.task , EventVolumeMin , 0 );
                    
                    VolumeSetA2dp(i, lOldVol, theSink.features.PlayLocalVolumeTone);
                }
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}    
#endif

/*************************************************************************
NAME    
    handleA2DPStoreClockMismatchRate
    
DESCRIPTION
    handle storing the clock mismatch rate for the active stream
RETURNS
    
**************************************************************************/
void handleA2DPStoreClockMismatchRate(uint16 clockMismatchRate)   
{
    a2dp_stream_state a2dpStatePri = a2dp_stream_idle;
    a2dp_stream_state a2dpStateSec = a2dp_stream_idle;
    Sink a2dpSinkPri = 0;
    Sink a2dpSinkSec = 0;
        
    /* if a2dp connected obtain the current streaming state for primary a2dp connection */
    getA2dpStreamData(a2dp_primary, &a2dpSinkPri, &a2dpStatePri);

    /* if a2dp connected obtain the current streaming state for secondary a2dp connection */
    getA2dpStreamData(a2dp_secondary, &a2dpSinkSec, &a2dpStateSec);
 
    /* Determine which a2dp source this is for */
    if((a2dpStatePri == a2dp_stream_streaming) && (a2dpSinkPri == theSink.routed_audio))  
    {
        A2DP_DEBUG(("A2dp: store pri. clk mismatch = %x\n", clockMismatchRate));
        theSink.a2dp_link_data->clockMismatchRate[a2dp_primary] = clockMismatchRate;
    }
    else if((a2dpStateSec == a2dp_stream_streaming) && (a2dpSinkSec == theSink.routed_audio))  
    {
        A2DP_DEBUG(("A2dp: store sec. clk mismatch = %x\n", clockMismatchRate));
        theSink.a2dp_link_data->clockMismatchRate[a2dp_secondary] = clockMismatchRate;
    }
    else
    {
        A2DP_DEBUG(("A2dp: ERROR NO A2DP STREAM, clk mismatch = %x\n", clockMismatchRate));
    }
}
 

/*************************************************************************
NAME    
    handleA2DPStoreCurrentEqBank
    
DESCRIPTION
    handle storing the current EQ bank
RETURNS
    
**************************************************************************/
void handleA2DPStoreCurrentEqBank (uint16 currentEQ)   
{
    uint16 abs_eq = A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0 + currentEQ;
               
    A2DP_DEBUG(("A2dp: Current EQ = %x, store = %x\n", currentEQ, abs_eq));

    /* Make sure current EQ setting is no longer set to next and store it */
    if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing != abs_eq)
    {
        A2DP_DEBUG(("A2dp: Update Current EQ = %x\n", abs_eq));
        theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = abs_eq;
        configManagerWriteSessionData () ; 
    }
}

/*************************************************************************
NAME    
    handleA2DPStoreEnhancments
    
DESCRIPTION
    handle storing the current enhancements settings
RETURNS
    
**************************************************************************/
void handleA2DPStoreEnhancements(uint16 enhancements)
{
    /* add in the data valid flag, this signifies that the user has altered the 3d or bass boost
       enhancements, these values should now be used instead of dsp default values that have been
       created by the ufe */
    enhancements |= (theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_DATA_VALID); 
    
    A2DP_DEBUG(("A2dp: store enhancements = %x was %x\n", enhancements,theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements));

    /* Make sure current EQ setting is no longer set to next and store it */
    if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements != enhancements)
    {
        A2DP_DEBUG(("A2dp: update enhancements = %x\n", enhancements));
        theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements = enhancements;
        configManagerWriteSessionData () ; 
    }
}

/*************************************************************************
NAME    
    disconnectAllA2dpAVRCP
    
DESCRIPTION
    disconnect any a2dp and avrcp connections
    
RETURNS
    
**************************************************************************/
void disconnectAllA2dpAvrcp(void)
{
    uint8 i;

#ifdef ENABLE_AVRCP
    if(theSink.features.avrcp_enabled)    
    {
        sinkAvrcpDisconnectAll();
    }
#endif  
    if(theSink.a2dp_link_data)
    {
        /* disconnect any a2dp signalling channels */
        for_all_a2dp(i)
        {
            /* if the a2dp link is connected, disconnect it */
            if(theSink.a2dp_link_data->connected[i])         
                A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[i]);
        }
    }  
}               

/*************************************************************************
NAME    
    disconnectAllA2dpPeerDevices
    
DESCRIPTION
    Disconnect any a2dp connections to any peer devices
    
RETURNS
    TRUE is any peer devices disconnected, FALSE otherwise
    
**************************************************************************/
bool disconnectAllA2dpPeerDevices (void)
{
    uint8 i;
    bool disc_req = FALSE;

    if(theSink.a2dp_link_data)
    {
        /* disconnect any a2dp signalling channels to peer devices */
        for_all_a2dp(i)
        {
            /* if the a2dp link is connected, disconnect it */
            if ((theSink.a2dp_link_data->connected[i]) && (theSink.a2dp_link_data->peer_device[i]==remote_device_peer))
            {
                A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[i]);
                disc_req = TRUE;
            }
        }
    }  
    
    return disc_req;
}

#ifdef ENABLE_PEER
static bool getCurrentPeer (a2dp_link_priority *id, a2dp_stream_state *state, Sink *sink)
{
    a2dp_stream_state a2dpStatePri = a2dp_stream_idle;
    a2dp_stream_state a2dpStateSec = a2dp_stream_idle;
    Sink a2dpSinkPri = 0;
    Sink a2dpSinkSec = 0;
        
    /* if a2dp connected obtain the current streaming state for primary and secondary a2dp connections */
    getA2dpStreamData(a2dp_primary, &a2dpSinkPri, &a2dpStatePri);
    getA2dpStreamData(a2dp_secondary, &a2dpSinkSec, &a2dpStateSec);

    if (a2dpSinkPri || a2dpSinkSec)
    {   /* There is at least one A2DP media channel connected */
        if (theSink.routed_audio)
        {   /* Audio is routed from somewhere */
            if ((a2dpSinkPri == theSink.routed_audio) && (theSink.a2dp_link_data->peer_device[a2dp_primary] == remote_device_peer))  
            {   /* It's from the primary A2DP device, which is a Peer */
                *id = a2dp_primary;
                *state = a2dpStatePri;
                *sink = a2dpSinkPri;
                A2DP_DEBUG(("A2dp: getCurrentPeer Priority=%u, State=%u, Sink=%X\n", *id, *state, (uint16)*sink));
                return TRUE;
            }
            
            if ((a2dpSinkSec == theSink.routed_audio) && (theSink.a2dp_link_data->peer_device[a2dp_secondary] == remote_device_peer))  
            {   /* It's from the secondary A2DP device, which is a Peer */
                *id = a2dp_secondary;
                *state = a2dpStateSec;
                *sink = a2dpSinkSec;
                A2DP_DEBUG(("A2dp: getCurrentPeer Priority=%u, State=%u, Sink=%X\n", *id, *state, (uint16)*sink));
                return TRUE;
            }
            
            /* Routed audio not from an A2DP based Peer device */
            A2DP_DEBUG(("A2dp: getCurrentPeer - routed sink not matched\n"));
            return FALSE;
        }
        
        /* No audio currently routed. Let's find out if we have a locally paused Peer device */
        if ((a2dpStatePri == a2dp_stream_open) && (theSink.a2dp_link_data->peer_device[a2dp_primary] == remote_device_peer) && (theSink.a2dp_link_data->SuspendState[a2dp_primary] == a2dp_local_peer_suspended))
        {
            *id = a2dp_primary;
            *state = a2dpStatePri;
            *sink = a2dpSinkPri;
            A2DP_DEBUG(("A2dp: getCurrentPeer Priority=%u, State=%u, Sink=%X\n", *id, *state, (uint16)*sink));
            return TRUE;
        }
        
        if ((a2dpStateSec == a2dp_stream_open) && (theSink.a2dp_link_data->peer_device[a2dp_secondary] == remote_device_peer) && (theSink.a2dp_link_data->SuspendState[a2dp_secondary] == a2dp_local_peer_suspended))
        {
            *id = a2dp_secondary;
            *state = a2dpStateSec;
            *sink = a2dpSinkSec;
            A2DP_DEBUG(("A2dp: getCurrentPeer Priority=%u, State=%u, Sink=%X\n", *id, *state, (uint16)*sink));
            return TRUE;
        }
    }
    
    A2DP_DEBUG(("A2dp: getCurrentPeer - none found\n"));
    return FALSE;
}
#endif

#ifdef ENABLE_PEER
bool controlA2DPPeer (uint16 event)
{
    a2dp_link_priority PeerId;
    a2dp_stream_state PeerState;
    Sink PeerSink;
    
    A2DP_DEBUG(("A2dp: controlA2DPPeer event=%x\n", event));
    
    if ( getCurrentPeer(&PeerId, &PeerState, &PeerSink) )
    {   /* A Peer device is either the current/last streaming device */
        if (A2dpMediaGetRole(theSink.a2dp_link_data->device_id[PeerId], theSink.a2dp_link_data->stream_id[PeerId]) == a2dp_sink)
        {   /* And it is operating as a sink */
            A2DP_DEBUG(("A2dp: controlA2DPPeer a2dp_sink found, SuspendState=%u\n", theSink.a2dp_link_data->SuspendState[PeerId]));
            if (theSink.a2dp_link_data->SuspendState[PeerId] == a2dp_not_suspended)
            {   /* Currently not suspended */
                if ((event==EventAvrcpPlayPause) || (event==EventAvrcpPause) || (event==EventAvrcpStop))
                {
                    theSink.a2dp_link_data->SuspendState[PeerId] = a2dp_local_peer_suspended;  /* Should not be overwritten by SuspendA2dpStream */
                    SuspendA2dpStream(PeerId);
                    A2DP_DEBUG(("A2dp: controlA2DPPeer - event %x actioned\n", event));
                    return TRUE;
                }
            }
            else if (theSink.a2dp_link_data->SuspendState[PeerId] == a2dp_local_peer_suspended)
            {   /* Currently suspended locally.  A remotely suspended sink would mean the sharing device needed to suspend for some reason */
                if ((event==EventAvrcpPlayPause) || (event==EventAvrcpPlay))
                {
                    ResumeA2dpStream(PeerId, PeerState, PeerSink);
                    A2DP_DEBUG(("A2dp: controlA2DPPeer - event %x actioned\n", event));
                    return TRUE;
                }
            }
        }
    }
    
    A2DP_DEBUG(("A2dp: controlA2DPPeer - event %x NOT actioned\n", event));
    return FALSE;
}
#endif

/*************************************************************************
NAME    
    handleA2DPMessage
    
DESCRIPTION
    A2DP message Handler, this function handles all messages returned
    from the A2DP library and calls the relevant functions if required

RETURNS
    
**************************************************************************/
void handleA2DPMessage( Task task, MessageId id, Message message )
{
    A2DP_DEBUG(("A2DP_MSG id=%x : \n",id));
    
    switch (id)
    {
/******************/
/* INITIALISATION */
/******************/
        
        /* confirmation of the initialisation of the A2DP library */
        case A2DP_INIT_CFM:
            A2DP_DEBUG(("A2DP_INIT_CFM : \n"));
            sinkA2dpInitComplete((A2DP_INIT_CFM_T *) message);
        break;

/*****************************/        
/* SIGNALING CHANNEL CONTROL */
/*****************************/

        /* indication of a remote source trying to make a signalling connection */		
	    case A2DP_SIGNALLING_CONNECT_IND:
	        A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_IND : \n"));
            handleA2DPSignallingConnectInd( ((A2DP_SIGNALLING_CONNECT_IND_T *)message)->device_id,
                                            ((A2DP_SIGNALLING_CONNECT_IND_T *)message)->addr );
		break;

        /* confirmation of a signalling connection attempt, successfull or not */
	    case A2DP_SIGNALLING_CONNECT_CFM:
            A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_CFM : \n"));
	    	handleA2DPSignallingConnected(((A2DP_SIGNALLING_CONNECT_CFM_T*)message)->status, 
	    								  ((A2DP_SIGNALLING_CONNECT_CFM_T*)message)->device_id, 
                                          ((A2DP_SIGNALLING_CONNECT_CFM_T*)message)->addr);
	    break;
        
        /* indication of a signalling channel disconnection having occured */
    	case A2DP_SIGNALLING_DISCONNECT_IND:
            A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_DISCONNECT_IND : \n"));
        	handleA2DPSignallingDisconnected(((A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->device_id,
                                             ((A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->status,
                                             ((A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->addr);
		break;
        
/*************************/        
/* MEDIA CHANNEL CONTROL */        
/*************************/
        
        /* indication of a remote device attempting to open a media channel */      
        case A2DP_MEDIA_OPEN_IND:
            A2DP_DEBUG(("A2DP_OPEN_IND : \n"));
        	handleA2DPOpenInd(((A2DP_MEDIA_OPEN_IND_T*)message)->device_id,
                              ((A2DP_MEDIA_OPEN_IND_T*)message)->seid);
        break;
		
        /* confirmation of request to open a media channel */
        case A2DP_MEDIA_OPEN_CFM:
            A2DP_DEBUG(("A2DP_OPEN_CFM : \n"));
        	handleA2DPOpenCfm(((A2DP_MEDIA_OPEN_CFM_T*)message)->device_id, 
    						  ((A2DP_MEDIA_OPEN_CFM_T*)message)->stream_id, 
    						  ((A2DP_MEDIA_OPEN_CFM_T*)message)->seid, 
    						  ((A2DP_MEDIA_OPEN_CFM_T*)message)->status);
        break;
        	
        /* indication of a request to close the media channel, remotely generated */
        case A2DP_MEDIA_CLOSE_IND:
            A2DP_DEBUG(("A2DP_CLOSE_IND : \n"));
            handleA2DPClose(((A2DP_MEDIA_CLOSE_IND_T*)message)->device_id,
                            ((A2DP_MEDIA_CLOSE_IND_T*)message)->stream_id,
                            ((A2DP_MEDIA_CLOSE_IND_T*)message)->status);
        break;

        /* confirmation of the close of the media channel, locally generated  */
        case A2DP_MEDIA_CLOSE_CFM:
           A2DP_DEBUG(("A2DP_CLOSE_CFM : \n"));
           handleA2DPClose(0,0,a2dp_success);
        break;

/**********************/          
/*  STREAMING CONTROL */
/**********************/          
        
        /* indication of start of media streaming from remote source */
        case A2DP_MEDIA_START_IND:
            A2DP_DEBUG(("A2DP_START_IND : \n"));
         	handleA2DPStartInd(((A2DP_MEDIA_START_IND_T*)message)->device_id,
                               ((A2DP_MEDIA_START_IND_T*)message)->stream_id);
        break;
		
        /* confirmation of a local request to start media streaming */
        case A2DP_MEDIA_START_CFM:
            A2DP_DEBUG(("A2DP_START_CFM : \n"));
    	    handleA2DPStartStreaming(((A2DP_MEDIA_START_CFM_T*)message)->device_id,
                                     ((A2DP_MEDIA_START_CFM_T*)message)->stream_id,
                                     ((A2DP_MEDIA_START_CFM_T*)message)->status);
        break;
        
        case A2DP_MEDIA_SUSPEND_IND:
            A2DP_DEBUG(("A2DP_SUSPEND_IND : \n"));
        	handleA2DPSuspendStreaming(((A2DP_MEDIA_SUSPEND_IND_T*)message)->device_id,
                                       ((A2DP_MEDIA_SUSPEND_IND_T*)message)->stream_id,
                                         a2dp_success);
        break;
		
        case A2DP_MEDIA_SUSPEND_CFM:
            A2DP_DEBUG(("A2DP_SUSPEND_CFM : \n"));
        	handleA2DPSuspendStreaming(((A2DP_MEDIA_SUSPEND_CFM_T*)message)->device_id,
                                       ((A2DP_MEDIA_SUSPEND_CFM_T*)message)->stream_id,
                                       ((A2DP_MEDIA_SUSPEND_CFM_T*)message)->status);
        break;

/*************************/
/* MISC CONTROL MESSAGES */
/*************************/
        
        case A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND:
            A2DP_DEBUG(("A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND : seid=0x%X delay=%u\n", ((A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND_T*)message)->seid, 
                                                                                       ((A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND_T*)message)->delay));
             /* Only received for source SEIDs.  Use delay value to aid AV synchronisation */                                                                        
        break;

        case A2DP_MEDIA_AV_SYNC_DELAY_IND:
            A2DP_DEBUG(("A2DP_MEDIA_AV_SYNC_DELAY_IND : seid=0x%X\n",((A2DP_MEDIA_AV_SYNC_DELAY_IND_T*)message)->seid));
            /* Currently supplying a zero AV Sync delay value, which will cause device to operate with the same latency 
               as pre A2DP v1.3 devices */
            A2dpMediaAvSyncDelayResponse(((A2DP_MEDIA_AV_SYNC_DELAY_IND_T*)message)->device_id,
                                         ((A2DP_MEDIA_AV_SYNC_DELAY_IND_T*)message)->seid,
                                         0);
        break;
        
        case A2DP_MEDIA_AV_SYNC_DELAY_CFM:
            A2DP_DEBUG(("A2DP_MEDIA_AV_SYNC_DELAY_CFM : \n"));
        break;
        
        /* link loss indication */
        case A2DP_SIGNALLING_LINKLOSS_IND:
            A2DP_DEBUG(("A2DP_SIGNALLING_LINKLOSS_IND : \n"));
            handleA2DPSignallingLinkloss(((A2DP_SIGNALLING_LINKLOSS_IND_T*)message)->device_id);
        break;           
		
        case A2DP_CODEC_CONFIGURE_IND:
            A2DP_DEBUG(("A2DP_CODEC_CONFIGURE_IND : \n"));
#ifdef ENABLE_PEER
            handleA2dpCodecConfigureIndFromPeer((A2DP_CODEC_CONFIGURE_IND_T *)message);
#endif
        break;
            
	    case A2DP_ENCRYPTION_CHANGE_IND:
            A2DP_DEBUG(("A2DP_ENCRYPTION_CHANGE_IND : \n"));
		break;
			
        default:       
	    	A2DP_DEBUG(("A2DP UNHANDLED MSG: 0x%x\n",id));
        break;
    }    
}
