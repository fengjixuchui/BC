/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2012-2013

FILE NAME
    sink_swat.c

DESCRIPTION
    Interface with the Subwoofer Audio Transfer Protocol and subwoofer 
    support
*/

#ifdef ENABLE_SUBWOOFER

/* Application includes */
#include "sink_swat.h"
#include "sink_debug.h"
#include "sink_private.h"
#include "sink_inquiry.h"
#include "sink_scan.h"
#include "sink_states.h"
#include "sink_statemanager.h"
#include "sink_devicemanager.h"

/* Library includes */
#include <swat.h>
#include <connection.h>
#include <bdaddr.h>
#include <audio.h>
#include <audio_plugin_if.h>
#include <csr_a2dp_decoder_common_plugin.h>


/* Firmware includes */
#include <panic.h>
#include <stdlib.h>
#include <string.h>
#include <ps.h>
#include <stdio.h>
#include <print.h>

/* The SWAT gain table used to send dB values to the subwoofer (indexed by the soundbar volume) */
static const uint8 swat_gain_table[ VOL_NUM_VOL_SETTINGS ] = {
      90,   /* Volume Gain 0    -45dB */
      78,   /* Volume Gain 1    -39dB */
      71,   /* Volume Gain 2  -35.5dB */
      66,   /* Volume Gain 3    -33dB */
      59,   /* Volume Gain 4  -29.5dB */
      54,   /* Volume Gain 5    -27dB */
      47,   /* Volume Gain 6  -23.5dB */
      42,   /* Volume Gain 7    -21dB */
      36,   /* Volume Gain 8    -18dB */
      30,   /* Volume Gain 9    -15dB */
      24,   /* Volume Gain 10   -12dB */
      18,   /* Volume Gain 11    -9dB */
      12,   /* Volume Gain 12    -6dB */
      6,    /* Volume Gain 13    -3dB */
      3,    /* Volume Gain 14  -1.5dB */
      0     /* Volume Gain 15     0dB */
};

#define NUM_SUB_TRIMS 11

/* The SWAT sub trim gain table used to send dB values to the subwoofer */
static const uint8 swat_sub_trim_table[ NUM_SUB_TRIMS ] = {
    20,  /* Sub gain 0, -10 dB */
    18,  /* Sub gain 1,  -9 dB */
    16,  /* Sub gain 2,  -8 dB */
    14,  /* Sub gain 3,  -7 dB */
    12,  /* Sub gain 4,  -6 dB */
    10,  /* Sub gain 5,  -5 dB */
    8,   /* Sub gain 6,  -4 dB */
    6,   /* Sub gain 7,  -3 dB */
    4,   /* Sub gain 8,  -2 dB */
    2,   /* Sub gain 9,  -1 dB */
    0    /* Sub gain 10,  0 dB */

};

#define SUB_VOL_UP   0  /* Used to know when sub volume is rising */
#define SUB_VOL_DOWN 1  /* Used to know when sub volume is falling */

/****************************************************************************
Prototypes for helper functions
*/


/****************************************************************************
NAME    
    getSubwooferBdAddrFromPs

RETURNS
    TRUE    if a paired subwoofer Bluetooth address was found
    FALSE   If there is no paired subwoofer or reading subwoofers bdaddr
            failed

NOTE
    theSink.rundata->subwoofer.bd_addr will be updated with the bluetooth address
    if it was found, or set to zero if not found.

DESCRIPTION
    Helper function to get the Bluetooth address of the paired subwoofer 
    device from the PS (if one exists)
*/
static bool getSubwooferBdAddrFromPs(void);


/****************************************************************************
NAME    
    sortSubwooferInquiryResults
    
DESCRIPTION
    Helper function to sort the inquiry results once inquiry search has
    completed.
*/
static void sortSubwooferInquiryResults(void);


/****************************************************************************
NAME    
    subwooferStartInqConnection
    
DESCRIPTION
    Helper function to start the connection process to the Inquiry results
*/
static void subwooferStartInqConnection(void);


/****************************************************************************
NAME    
    subwooferInqNextConnection
    
DESCRIPTION
    Helper function to connect to the next result in the list
*/
static void subwooferInqNextConnection(void);


/****************************************************************************
NAME    
    handleSwatSignallingConnectInd
    
DESCRIPTION
    Helper function to handle when a SWAT connection request is made by a
    remote device
*/
static void handleSwatSignallingConnectInd(SWAT_SIGNALLING_CONNECT_IND_T * ind);

        
/****************************************************************************
NAME    
    handleSwatSignallingConnectCfm
    
DESCRIPTION
    Helper function to handle when a SWAT connection attempt has completed
*/
static void handleSwatSignallingConnectCfm(SWAT_SIGNALLING_CONNECT_CFM_T * cfm);


/****************************************************************************
NAME    
    handleSwatMediaOpenCfm
    
DESCRIPTION
    Helper function to handle when a SWAT media connection attempt has 
    completed
*/
static void handleSwatMediaOpenCfm(SWAT_MEDIA_OPEN_CFM_T * cfm);


/****************************************************************************
NAME    
    handleSwatMediaCloseCfm
    
DESCRIPTION
    Helper function to handle when a SWAT media close has completed
*/
static void handleSwatMediaCloseCfm(SWAT_MEDIA_CLOSE_CFM_T * cfm);


/****************************************************************************
NAME    
    handleSwatMediaStartCfm
    
DESCRIPTION
    Helper function to handle when a SWAT media START has completed
*/
static void handleSwatMediaStartCfm(SWAT_MEDIA_START_CFM_T * cfm);


/****************************************************************************
NAME    
    handleSwatMediaSuspendCfm
    
DESCRIPTION
    Helper function to handle when a SWAT media SUSPEND has completed
*/
static void handleSwatMediaSuspendCfm(SWAT_MEDIA_SUSPEND_CFM_T * cfm);


/****************************************************************************
NAME    
    handleSwatSetVolumeCfm
    
DESCRIPTION
    Helper function to handle when a SWAT volume message has confirmed a
    volume change request or handle when the remote device has changed the
    SWAT volume.
*/
static void handleSwatSetVolumeCfm(SWAT_SET_VOLUME_CFM_T * cfm);


/****************************************************************************
NAME    
    handleSwatSignallingDisconnectCfm
    
DESCRIPTION
    Helper function to handle when a SWAT signalling channel has disconnected
*/
static void handleSwatSignallingDisconnectCfm(SWAT_SIGNALLING_DISCONNECT_CFM_T * message);


/****************************************************************************
NAME    
    handleSwatSignallingLinkLossInd
    
DESCRIPTION
    Helper function to handle when a link loss has occured with a connected
    SWAT (subwoofer) device
*/
static void handleSwatSignallingLinkLossInd(SWAT_SIGNALLING_LINKLOSS_IND_T * message);


/****************************************************************************
NAME    
    handleSwatSampleRateCfm
    
DESCRIPTION
    Helper function to handle when a remote device responds to a sample rate
    command
*/
static void handleSwatSampleRateCfm(SWAT_SAMPLE_RATE_CFM_T * message);


/*************************************************************************/
void handleSwatMessage(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case SWAT_INIT_CFM:
        {
            SWAT_DEBUG(("SWAT_INIT_CFM\n"));            
            
            /* Retrieve the paired subwoofer (if one exists) */
            getSubwooferBdAddrFromPs();
            
            /* Init the SWAT volume */
            theSink.rundata->subwoofer.swat_volume = 0xFF;
            
            /* Init the subwoofer trim volume */
            theSink.rundata->subwoofer.sub_trim_idx = DEFAULT_SUB_TRIM_INDEX;
            theSink.rundata->subwoofer.sub_trim = swat_sub_trim_table[ theSink.rundata->subwoofer.sub_trim_idx ];
        }
        break;
        case SWAT_SIGNALLING_CONNECT_IND:
        {
            handleSwatSignallingConnectInd( (SWAT_SIGNALLING_CONNECT_IND_T *)message );
        }
        break;
        case SWAT_SIGNALLING_CONNECT_CFM:
        {
            handleSwatSignallingConnectCfm( (SWAT_SIGNALLING_CONNECT_CFM_T *)message );
        }
        break;
        case SWAT_SIGNALLING_LINKLOSS_IND:
        {
            handleSwatSignallingLinkLossInd( (SWAT_SIGNALLING_LINKLOSS_IND_T *)message );
        }
        break;
        case SWAT_SET_VOLUME_CFM:
        {
            handleSwatSetVolumeCfm( (SWAT_SET_VOLUME_CFM_T *)message );
        }
        break;
        case SWAT_MEDIA_OPEN_CFM:
        {
            handleSwatMediaOpenCfm( (SWAT_MEDIA_OPEN_CFM_T *)message );
        }
        break;
        case SWAT_MEDIA_START_IND:
        {
            SWAT_DEBUG(("SW : SWAT_MEDIA_START_IND\n"));
        }
        break;
        case SWAT_MEDIA_START_CFM:
        {
            handleSwatMediaStartCfm( (SWAT_MEDIA_START_CFM_T *)message );
        }
        break;
        case SWAT_MEDIA_SUSPEND_IND:
        {
            SWAT_DEBUG(("SW : SWAT_MEDIA_SUSPEND_IND\n"));
        }
        break;
        case SWAT_MEDIA_SUSPEND_CFM:
        {
            handleSwatMediaSuspendCfm( (SWAT_MEDIA_SUSPEND_CFM_T *)message );
        }
        break;
        case SWAT_MEDIA_CLOSE_IND:
        {
            SWAT_DEBUG(("SW : SWAT_MEDIA_CLOSE_IND\n"));
        }
        break;
        case SWAT_MEDIA_CLOSE_CFM:
        {
            handleSwatMediaCloseCfm( (SWAT_MEDIA_CLOSE_CFM_T *)message );
        }
        break;
        case SWAT_SIGNALLING_DISCONNECT_CFM:
        {
            handleSwatSignallingDisconnectCfm( (SWAT_SIGNALLING_DISCONNECT_CFM_T *)message );
        }
        break;
        case SWAT_SAMPLE_RATE_CFM:
        {
            handleSwatSampleRateCfm( (SWAT_SAMPLE_RATE_CFM_T *)message );
        }
        break;
        default:
        {
            SWAT_DEBUG(("SW : Unhandled SWAT message ID[%x]\n",id));
        }
    }
}


/*************************************************************************/
void markSubwooferMostRecentDevice(void)
{
    /* Only mark if there is a paired subwoofer device */
    if (!BdaddrIsZero(&theSink.rundata->subwoofer.bd_addr))
    {
        ConnectionSmUpdateMruDevice((const bdaddr *)&theSink.rundata->subwoofer.bd_addr);
    }
}


/*************************************************************************/
void handleSubwooferGetAuthDevice(CL_SM_GET_AUTH_DEVICE_CFM_T * cfm)
{
    /* If the subwoofer device bdaddr matches that of the PDL entry, Subwoofer is only device in PDL */
    if ( (theSink.rundata->subwoofer.check_pairing) && (cfm->status == success) )
    {
        if (BdaddrIsSame(&cfm->bd_addr, &theSink.rundata->subwoofer.bd_addr))
        {
            SWAT_DEBUG(("SW : Subwoofer is only device in PDL\n"));
            MessageSend(&theSink.task, EventEnterPairingEmptyPDL, 0);
        }
    }
}


/*************************************************************************/
void storeSubwooferBdaddr(const bdaddr * addr)
{
    /* Store the subwoofers Bluetooth address to PS */
    if (PsStore(PSKEY_SW_BDADDR, addr, 4) == 4)
    {
        SWAT_DEBUG(("SW : Wrote SW BDADDR to PS\n"));
    }
    else
    {
        SWAT_DEBUG(("SW : Failed to write subwoofer BDADDR to PS\n"));
    }
}


/*************************************************************************/
void handleEventSubwooferStartInquiry(void)
{
    /* Setup the inquiry state to indicate Sink is inquiring for a subwoofer device */
    theSink.inquiry.action = rssi_subwoofer;
    theSink.inquiry.state = inquiry_searching;
        
    /* Allocate memory to store the inquiry results */
    theSink.inquiry.results = (inquiry_result_t *)PanicNull(malloc(SW_MAX_INQUIRY_DEVS * sizeof(inquiry_result_t)));
    memset(theSink.inquiry.results, 0, (SW_MAX_INQUIRY_DEVS * sizeof(inquiry_result_t)));
        
    /* Inquire for devices with device class matching SW_CLASS_OF_DEVICE */
    ConnectionWriteInquiryMode(&theSink.task, inquiry_mode_eir);
    ConnectionInquire(&theSink.task, INQUIRY_LAP, SW_MAX_INQUIRY_TIME, SW_MAX_INQUIRY_DEVS, SUBWOOFER_CLASS_OF_DEVICE);
        
    SWAT_DEBUG(("SW : SW inquiry started\n"));
}


/*************************************************************************/
void handleSubwooferInquiryResult( CL_DM_INQUIRE_RESULT_T* result )
{
    uint8 counter;
    
    SWAT_DEBUG(("SW : SW inquiry result status = %x\n",result->status));


    /* Is the search complete? */
    if (result->status == inquiry_status_ready)
    {
        /* once scan attempt is now complete, update attempt counter */            
        if(theSink.rundata->subwoofer.inquiry_attempts) 
            theSink.rundata->subwoofer.inquiry_attempts--;

        /* Sort the list */
        sortSubwooferInquiryResults();
        
        /* Make connection attempts to the inquiry results */
        subwooferStartInqConnection();
    }
    else
    {
        /* Expect SW_MAX_INQUIRY_DEV number of unique inquiry results; process each one */
        for (counter=0; counter<SW_MAX_INQUIRY_DEVS; counter++)
        {
            /* If the discovered device already exists in the list, ignore it */
            if (BdaddrIsSame(&result->bd_addr, (const bdaddr *)&theSink.inquiry.results[counter]))
            {
                return;
            }
            /* If the current entry is empty, validate and add the device */
            if (BdaddrIsZero((const bdaddr *)&theSink.inquiry.results[counter]))
            {
                /* Check the RSSI value meets minimum required (if configured) */
                if (theSink.features.LimitRssiSuboowferPairing)
                {
                    if (result->rssi > (int)RSSI_CONF.threshold)
                    {
                        /* Add the device (RSSI meets threshold requirements */
                        SWAT_DEBUG(("SW : INQ Found SW device\n"));
                        theSink.inquiry.results[counter].bd_addr = result->bd_addr;
                        theSink.inquiry.results[counter].rssi    = result->rssi;
                    }
                    else
                    {
                        SWAT_DEBUG(("SW : INQ - Device signal too low [%d]\n", result->rssi));
                    }
                }
                else
                {
                    /* Add the device (RSSI restriction is disabled) */
                    SWAT_DEBUG(("SW : INQ Found SW device\n"));
                    theSink.inquiry.results[counter].bd_addr = result->bd_addr;
                    theSink.inquiry.results[counter].rssi    = result->rssi;
                }
            }
        }
    }
}


/*************************************************************************/
void handleEventSubwooferCheckPairing(void)
{
    /* get the latest swat bdaddr, if any stored */
    getSubwooferBdAddrFromPs();
            
    /* Only connect the subwoofer if it is not already connected */
    if (!SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        /* If there is a paired subwoofer device, make a SWAT connection request to the subwoofer */
        if ( !BdaddrIsZero((const bdaddr *)&theSink.rundata->subwoofer.bd_addr) )
        {
            SWAT_DEBUG(("SW : check subwoofer pairing: device exists, sub_trim = %d\n",theSink.rundata->subwoofer.sub_trim_idx));
        }
        else
        {
            SWAT_DEBUG(("SW : No paired SW - Starting SW inquiry search\n"));

            /* set the number of times to try an inquiry scan */            
            theSink.rundata->subwoofer.inquiry_attempts = SW_INQUIRY_ATTEMPTS;
            
            /* inquiry starting, make soundbar non connectable until completed scan */
            sinkDisableConnectable();

            /* start inquiry for sub woofer devices */
            MessageSend(&theSink.task, EventSubwooferStartInquiry, 0);
        }
    }
    else
    {
        SWAT_DEBUG(("SW : Subwoofer already connected\n"));
    }
}


/*************************************************************************/
void handleEventSubwooferOpenLLMedia(void)
{
    /* Ensure there is a signalling channel before sending media open req */
    if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        if (SwatGetMediaSink(theSink.rundata->subwoofer.dev_id))
        {
            if (SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_STANDARD)
            {
                SWAT_DEBUG(("SW : Close SWAT_MEDIA_STANDARD & \n"));
                
                /* close STD latency media connection */
                handleEventSubwooferCloseMedia();

                SWAT_DEBUG(("SW : open SWAT_MEDIA_LOW_LATENCY\n"));
                
                /* open a low latency media connection */
                SwatMediaOpenRequest(theSink.rundata->subwoofer.dev_id, SWAT_MEDIA_LOW_LATENCY);
            }
            else
            {
                SWAT_DEBUG(("Low Latency MEDIA already open\n"));
            }
        }
        /* don't attempt to open if already in the processing of open a media connection */
        else if(SwatGetMediaState((theSink.rundata->subwoofer.dev_id)) != swat_media_opening)
        {
            /* No media open to the subwoofer so can just open a low latency channel */
            SWAT_DEBUG(("SW : Open SWAT_MEDIA_LOW_LATENCY, media state [%d]\n",SwatGetMediaState((theSink.rundata->subwoofer.dev_id))));
            SwatMediaOpenRequest(theSink.rundata->subwoofer.dev_id, SWAT_MEDIA_LOW_LATENCY);
        }
        else
        {
            SWAT_DEBUG(("SW : Open SWAT_MEDIA_LOW_LATENCY - already in process of opening\n"));           
        }
    }
    else
    {
        SWAT_DEBUG(("SW : EventSubwooferOpenLLMedia (SW Not yet connected)\n"));
    }
}


/*************************************************************************/
void handleEventSubwooferOpenStdMedia(void)
{
    /* Ensure there is a signalling channel before sending media open req */
    if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        if (SwatGetMediaSink(theSink.rundata->subwoofer.dev_id))
        {
            if (SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_LOW_LATENCY)
            {
                SWAT_DEBUG(("SW : Close SWAT_MEDIA_LOW_LATENCY & open SWAT_MEDIA_STANDARD\n"));
                /* close low latency media connection */
                handleEventSubwooferCloseMedia();
                /* open a standard media connection */
                SwatMediaOpenRequest(theSink.rundata->subwoofer.dev_id, SWAT_MEDIA_STANDARD);
            }
            else
            {
                SWAT_DEBUG(("SW : SWAT_MEDIA_STANDARD is already open\n"));
            }
        }
        /* don't attempt to open if already in the processing of open a media connection */
        else if(SwatGetMediaState((theSink.rundata->subwoofer.dev_id)) != swat_media_opening)
        {
            /* No media open to the subwoofer so can just open a standard latency channel */
            SWAT_DEBUG(("SW : Open SWAT_MEDIA_STANDARD\n"));
            SwatMediaOpenRequest(theSink.rundata->subwoofer.dev_id, SWAT_MEDIA_STANDARD);
        }
        else
        {
            SWAT_DEBUG(("SW : Open SWAT_MEDIA_STANDARD_LATENCY - already in process of opening\n"));           
        }
    }
    else
    {
        SWAT_DEBUG(("SW : EventSubwooferOpenStdMedia (SW Not Yet Connected)\n"));
    }
}


/*************************************************************************/
void handleEventSubwooferCloseMedia(void)
{
    /* Ensure there is an open media channel before sending media close req */
    if (SwatGetMediaSink(theSink.rundata->subwoofer.dev_id))
    {
        SWAT_DEBUG(("SW : EventSubwooferCloseMedia dev_id = %x media_type = %d\n",theSink.rundata->subwoofer.dev_id, SwatGetMediaType(theSink.rundata->subwoofer.dev_id)));
        SwatMediaCloseRequest(theSink.rundata->subwoofer.dev_id, SwatGetMediaType(theSink.rundata->subwoofer.dev_id));
    }
}


/*************************************************************************/
void handleEventSubwooferStartStreaming(void)
{
    /* Ensure there is an open media channel before sending START req */
    if (SwatGetMediaSink(theSink.rundata->subwoofer.dev_id))
    {
        /* obtain the sample rate of the plugin before starting to stream audio, it is
           important that the sample rate is sent over swat prior to starting to stream
           sub audio */
        if (!AudioGetA2DPSampleRate())
        {
            SWAT_DEBUG(("SW : sample rate not available yet\n"));
            /* dsp plugin not ready yet, try again shortly */
            MessageSendLater(&theSink.task, EventSubwooferStartStreaming, 0, 10);
        }
        /* plugin not loaded yet, sample rate unavailable, wait for it to be available */
        else
        {
            SWAT_DEBUG(("SW : Send sample rate then start streaming\n"));

            /* send sample rate to sub */
            sendSampleRateToSub(AudioGetA2DPSampleRate());
        
            /* start streaming */
            SwatMediaStartRequest(theSink.rundata->subwoofer.dev_id, SwatGetMediaType(theSink.rundata->subwoofer.dev_id));
        }
    }
}


/*************************************************************************/
void handleEventSubwooferSuspendStreaming(void)
{
    /* Ensure there is an open media channel before sending SUSPEND req */
    if (SwatGetMediaSink(theSink.rundata->subwoofer.dev_id))
    {
        SWAT_DEBUG(("SW : EventSubwooferSuspendStreaming\n"));
        SwatMediaSuspendRequest(theSink.rundata->subwoofer.dev_id, SwatGetMediaType(theSink.rundata->subwoofer.dev_id));
    }
}


/*************************************************************************/
void handleEventSubwooferDisconnect(void)
{
    SWAT_DEBUG(("SW : EventSubwooferDisconnect\n"));
    
    /* If there is a subwoofer connected, disconnect it */
    if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        SwatSignallingDisconnectRequest(theSink.rundata->subwoofer.dev_id);
    }
    else
    {
        SWAT_DEBUG(("SW : SW not connected\n"));
    }
}


/*************************************************************************/
void handleEventSubwooferVolumeUp(void)
{
    if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        SWAT_DEBUG(("SW : Volume Up: TrimIdx = [%d]\n",theSink.rundata->subwoofer.sub_trim_idx));
        /* Don't allow sub trim to go above maximum allowed value, otherwise increment and set the new sub trim gain */
        if (theSink.rundata->subwoofer.sub_trim_idx < (NUM_SUB_TRIMS-1))
        {   
            /* increase sub trim volume level */
            theSink.rundata->subwoofer.sub_trim_idx++;
    
            SWAT_DEBUG(("SW : Volume Up: Update: TrimIdx = [%d]\n",theSink.rundata->subwoofer.sub_trim_idx));

            /* update the new sub trim value */
            SwatSetVolume(theSink.rundata->subwoofer.dev_id, theSink.rundata->subwoofer.swat_volume, swat_sub_trim_table[ theSink.rundata->subwoofer.sub_trim_idx ]);
        }
    }
}


/*************************************************************************/
void handleEventSubwooferVolumeDown(void)
{
    if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        SWAT_DEBUG(("SW : Volume Down: TrimIdx = [%d]\n",theSink.rundata->subwoofer.sub_trim_idx));
        /* Don't allow sub trim index to go below zero, otherwise decrement and set the new sub trim gain */
        if (theSink.rundata->subwoofer.sub_trim_idx > 0)
        {
            /* decrease the sub trim value */
            theSink.rundata->subwoofer.sub_trim_idx--;

            SWAT_DEBUG(("SW : Volume Down: Update: TrimIdx = [%d]\n",theSink.rundata->subwoofer.sub_trim_idx));
            
            /* Only update the sub trim, don't update the SYSTEM_VOLUME */
            SwatSetVolume(theSink.rundata->subwoofer.dev_id, theSink.rundata->subwoofer.swat_volume, swat_sub_trim_table[ theSink.rundata->subwoofer.sub_trim_idx ]);
        }
    }
}


/*************************************************************************/
void handleEventSubwooferDeletePairing(void)
{
    /* Delete only the subwoofer device from the PDL */
    deleteSubwooferPairing(TRUE);
}


/*************************************************************************/
void updateSwatVolume(uint16 new_volume)
{
    if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        /* Only update if new_volume is different to current volume */
        if (swat_gain_table[ new_volume ] != theSink.rundata->subwoofer.swat_volume)
        {
            SWAT_DEBUG(("SW : Update SWAT volume from[%d] to[%d]\n", theSink.rundata->subwoofer.swat_volume, swat_gain_table[ new_volume ]));
            SwatSetVolume(theSink.rundata->subwoofer.dev_id, swat_gain_table[new_volume], theSink.rundata->subwoofer.sub_trim);
        }
    }
    else
    {
        /* Update the volume so when subwoofer connects we know what volume to send it */
        theSink.rundata->subwoofer.swat_volume = swat_gain_table[ new_volume ];
    }
}


/*************************************************************************/
void deleteSubwooferPairing(bool delete_link_key)
{
    /* Only can delete a subwoofer device if there is a paired subwoofer */
    if (!BdaddrIsZero(&theSink.rundata->subwoofer.bd_addr))
    {
        /* Disconnect the subwoofer if it is connected */
        if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
        {
            handleEventSubwooferDisconnect();
        }
        
        /* Delete the subwoofer from the paired device list? */
        if (delete_link_key)
        {
            ConnectionSmDeleteAuthDevice(&theSink.rundata->subwoofer.bd_addr);
        }
        
        /* Delete the subwoofers Bluetooth address from PS and clear from memory */
        PsStore(PSKEY_SW_BDADDR, NULL, 0);
        BdaddrSetZero(&theSink.rundata->subwoofer.bd_addr);
        
        SWAT_DEBUG(("SW : SW pairing deleted\n"));
    }
}


/*************************************************************************/
void sendSampleRateToSub(uint16 sample_rate)
{
    if (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
    {
        SWAT_DEBUG(("SW : Send Sample rate[%u] to sub\n", sample_rate));
        SwatSendSampleRateCommand(theSink.rundata->subwoofer.dev_id, sample_rate);
    }
}


/****************************************************************************
Subwoofer helper functions
*/


/*************************************************************************/
static bool getSubwooferBdAddrFromPs(void)
{
    /* If there is a paired subwoofer, store it's Bluetooth address */
    if (PsRetrieve(PSKEY_SW_BDADDR, (void *)&theSink.rundata->subwoofer.bd_addr, 4))
    {
        SWAT_DEBUG(("SW : PSKEY_SW_BDADDR [%04x %02x %06lx]\n", theSink.rundata->subwoofer.bd_addr.nap, theSink.rundata->subwoofer.bd_addr.uap, theSink.rundata->subwoofer.bd_addr.lap));
        return TRUE;
    }
    else
    {
        BdaddrSetZero(&theSink.rundata->subwoofer.bd_addr);
        return FALSE;
    }
}


/****************************************************************************/
static void handleSwatSignallingConnectInd(SWAT_SIGNALLING_CONNECT_IND_T * ind)
{
    /* Should the connection request be allowed? Reject connection if a sub is already connected or if in limbo state */
    if ( (SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id)) || (stateManagerGetState() == deviceLimbo) )
    {
        SWAT_DEBUG(("SW : Reject incoming SWAT connection request\n"));
        SwatSignallingConnectResponse(ind->device_id, ind->connection_id, ind->identifier, FALSE);
    }
    else
    {
        SWAT_DEBUG(("SW : Accept incoming SWAT connection request\n"));
        SwatSignallingConnectResponse(ind->device_id, ind->connection_id, ind->identifier, TRUE);

        /* Store the subwoofers Bluetooth address to PS if not already done so */
        if((BdaddrIsZero(&theSink.rundata->subwoofer.bd_addr))||(!BdaddrIsSame(&theSink.rundata->subwoofer.bd_addr, &ind->bd_addr)))
        {
            /* update USR_PSKEY_35 with sub woofer bluetooth adddress to connect to */
            PsStore(PSKEY_SW_BDADDR, &ind->bd_addr, 4);
            /* update local store for subsequent reconnections */
            memmove(&theSink.rundata->subwoofer.bd_addr, &ind->bd_addr, sizeof(bdaddr)); 
        }
    }
}


/****************************************************************************/
static void handleSwatSignallingConnectCfm(SWAT_SIGNALLING_CONNECT_CFM_T * cfm)
{
    /* Was the connection successful? */
    if ((cfm->status == swat_success) || (cfm->status == swat_reconnect_success))
    {
        sink_attributes attributes;

        SWAT_DEBUG(("SW : SWAT_SIGNALLING_CONNECT_CFM\n"));
        
        /* Is this connection result part of subwoofer inquiry? If so, end the subwoofer inquiry */
        if (theSink.inquiry.action == rssi_subwoofer)
        {
            theSink.inquiry.action = rssi_none;
            free(theSink.inquiry.results);
            theSink.inquiry.results = NULL;

            /* inquiry complete, make soundbar connectable again */
            sinkEnableConnectable();
            
            /* continue connecting to AG's */
            MessageSend(&theSink.task, EventEstablishSLC, 0);
        }
        
        /* Store the (SWAT assigned) subwoofer device ID & the SWAT signalling sink  */
        theSink.rundata->subwoofer.dev_id = cfm->device_id;
                
        /* recheck the audio routing to see if the sub needs to be utilised */
        audioHandleRouting(audio_source_none);

        /* schedule a link policy check, cancel any pending messages and replace with a new one */
        MessageCancelFirst(&theSink.task , EventCheckRole);    
        MessageSendConditionally (&theSink.task , EventCheckRole , NULL , &theSink.rundata->connection_in_progress  );
        
        /* Set link supervision timeout to 5seconds (8000 * 0.625ms)*/
        ConnectionSetLinkSupervisionTimeout(SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id), 8000);
        
        /* Store the subwoofers Bluetooth address to PS if not already done so */
        if((BdaddrIsZero(&theSink.rundata->subwoofer.bd_addr))||(!BdaddrIsSame(&theSink.rundata->subwoofer.bd_addr, SwatGetBdaddr(theSink.rundata->subwoofer.dev_id))))
        {
            /* update USR_PSKEY_35 with sub woofer bluetooth adddress to connect to */
            PsStore(PSKEY_SW_BDADDR, SwatGetBdaddr(theSink.rundata->subwoofer.dev_id), 4);
            /* update local store for subsequent reconnections */
            memmove(&theSink.rundata->subwoofer.bd_addr, SwatGetBdaddr(theSink.rundata->subwoofer.dev_id), sizeof(bdaddr)); 
        }

        /* get the stored sub trim value, Use default attributes if none exist is PS */
        deviceManagerGetDefaultAttributes(&attributes, TRUE);
        /* check attributes exist, if not create them */        
        if(!deviceManagerGetAttributes(&attributes, SwatGetBdaddr(theSink.rundata->subwoofer.dev_id)))
        {
            /* Setup some default attributes for the subwoofer */
            deviceManagerStoreDefaultAttributes(SwatGetBdaddr(theSink.rundata->subwoofer.dev_id), TRUE);
        }
        theSink.rundata->subwoofer.sub_trim_idx = attributes.sub.sub_trim;
        theSink.rundata->subwoofer.sub_trim = swat_sub_trim_table[ theSink.rundata->subwoofer.sub_trim_idx ];
        
        /* Sync audio gains with subwoofer */
        SwatSetVolume(theSink.rundata->subwoofer.dev_id, theSink.rundata->subwoofer.swat_volume, theSink.rundata->subwoofer.sub_trim);   
        
    }
    /* Did the link loss reconnection fail? */
    else if (cfm->status == swat_reconnect_failed)
    {
        SWAT_DEBUG(("SW : Link loss reconnection failed\n"));
    }
    /* Is the failed connection part of the subwoofer inquiry? If so, try next device*/
    else if (theSink.inquiry.action == rssi_subwoofer)
    {
        SWAT_DEBUG(("SW : Sig Connect Failure, status: %d \n",cfm->status));
        /* Connection did not succeed, try connecting next inquiry result */
        subwooferInqNextConnection();
        return;
    }
    /* Was there an L2CAP connection error? */
    else if (cfm->status == swat_l2cap_error)
    {
        SWAT_DEBUG(("SW : Signalling L2CAP connection error : try connection again\n"));
    }
    /* Did the connection "just fail"? */
    else
    {
        SWAT_DEBUG(("SW : Subwoofer connection failed [%x]\n", cfm->status));
    }
}


/****************************************************************************/
static void handleSwatSignallingLinkLossInd(SWAT_SIGNALLING_LINKLOSS_IND_T * message)
{
    SWAT_DEBUG(("SW : SWAT_SIGNALLING_LINKLOSS_IND\n"));
    
    /* Was media connected? If so, update the DSP application */
    if (SwatGetMediaSink(theSink.rundata->subwoofer.dev_id))
    {
        /* Update the DSP as the subwoofer is no longer connnected */
        SWAT_DEBUG(("DISCONNECT MEDIA\n"));
        AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_NONE, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
    }
        
    /* Link loss reconnection will be done by the subwoofer so just go connectable and wait */
    sinkEnableConnectable();
}


/****************************************************************************/
static void handleSwatSignallingDisconnectCfm(SWAT_SIGNALLING_DISCONNECT_CFM_T * cfm)
{
    if ((cfm->status == swat_success) || (cfm->status == swat_disconnect_link_loss))
    {
        SWAT_DEBUG(("Subwoofer disconnected\n"));
        
        /* If media was streaming to subwoofer, update DSP app that sub has disconnected */
        if (SwatGetMediaSink(theSink.rundata->subwoofer.dev_id))
        {
            AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_NONE, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
        }

        /* Make device connectable so subwoofer can reconnect */
        sinkEnableConnectable();

        /* Update/Store the attributes in PS */
        deviceManagerUpdateAttributes(SwatGetBdaddr(theSink.rundata->subwoofer.dev_id), sink_swat, 0, 0);   
    }
    else
    {
        SWAT_DEBUG(("SW : SWAT_SIGNALLING_DISCONNECT_CFM failed STATUS[%x]\n",cfm->status));
    }
}


/****************************************************************************/
static void handleSwatMediaOpenCfm(SWAT_MEDIA_OPEN_CFM_T * cfm)
{
    if (cfm->status == swat_success)
    {
        SWAT_DEBUG(("SW : SWAT media connected\n"));
               
        /* Disable rate Matching for low latency media channels */
        if (SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_LOW_LATENCY)
        {
            PanicFalse(SourceConfigure(StreamSourceFromSink(SwatGetMediaSink(theSink.rundata->subwoofer.dev_id)), VM_SOURCE_SCO_RATEMATCH_ENABLE, 0));
        }
        
        /* Request to START streaming audio data to the subwoofer */
        MessageSend(&theSink.task, EventSubwooferStartStreaming, 0);
    }
    /* If media failed to open due to a signalling error, retry the connection */
    else if (cfm->status == swat_signalling_no_response)
    {
        SWAT_DEBUG(("SW : SWAT media open cfm failed, retry\n"));
        SwatMediaOpenRequest(theSink.rundata->subwoofer.dev_id, cfm->media_type);
    }
    /* Media failed to open */
    else
    {
        SWAT_DEBUG(("SW : SWAT media TYPE[%x] failed to open [%x]\n", cfm->media_type, cfm->status));
    }
}


/****************************************************************************/
static void handleSwatMediaCloseCfm(SWAT_MEDIA_CLOSE_CFM_T * cfm)
{
    if (cfm->status == swat_success)
    {
        SWAT_DEBUG(("SW : SWAT_MEDIA_CLOSE_CFM\n"));
        
        /* Unroute the audio */
        AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_NONE, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
    }
    else
    {
        /* The media channel needs tidying up (will produce another SWAT_MEDIA_CLOSE_CFM with successful status)*/
        SwatTidyUpMediaChannel(theSink.rundata->subwoofer.dev_id, cfm->media_type);
    }
}


/****************************************************************************/
static void handleSwatMediaStartCfm(SWAT_MEDIA_START_CFM_T * cfm)
{
    /* If the START was successful, route the audio */
    if (cfm->status == swat_success)
    {
        /* Which media channel has started? Route appropriatley */
        if (cfm->media_type == SWAT_MEDIA_LOW_LATENCY)
        {
            SWAT_DEBUG(("SW : Connect Kalimba to ESCO\n"));
            if(!AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_ESCO, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id)))
                SWAT_DEBUG(("ACSW FAIL\n"));
        }   
        else if (cfm->media_type == SWAT_MEDIA_STANDARD)
        {
            SWAT_DEBUG(("SW : Connect Kalimba to L2CAP\n"));
            if(!AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_L2CAP, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id)))
                SWAT_DEBUG(("ACSW FAIL\n"));
        }
        else
        {
            /* Should never get here, but handle by not routing any audio */
            SWAT_DEBUG(("SW : SWAT_MEDIA_START_CFM ERROR : media TYPE[%x]\n", cfm->media_type));
            AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_NONE, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
        }
    }
    else
    {
        SWAT_DEBUG(("SW : handleSwatMediaStartCfm FAILED = %d\n",cfm->status));
    }
}


/****************************************************************************/
static void handleSwatMediaSuspendCfm(SWAT_MEDIA_SUSPEND_CFM_T * cfm)
{
    if (cfm->status == swat_success)
    {
        SWAT_DEBUG(("SW : Stop forwarding audio to subwoofer\n"));
        AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_NONE, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));    
    }
}


/****************************************************************************/
static void handleSwatSetVolumeCfm(SWAT_SET_VOLUME_CFM_T * cfm)
{
    SWAT_DEBUG(("SW : SWAT_VOLUME[%d] SUB_TRIM[%d]\n", cfm->volume, cfm->sub_trim));
    
    /* Store the synchronised volume (dB level NOT index) */
    theSink.rundata->subwoofer.swat_volume = cfm->volume;
    
    /* Was the subwoofer trim volume modified? */
    if (theSink.rundata->subwoofer.sub_trim != cfm->sub_trim)
    {
        /* Store the new subwoofer trim gain */
        theSink.rundata->subwoofer.sub_trim = cfm->sub_trim;
        
        /* Update/Store the attributes in PS */
        deviceManagerUpdateAttributes(SwatGetBdaddr(theSink.rundata->subwoofer.dev_id), sink_swat, 0, 0);   
    }        
}


/****************************************************************************/
static void sortSubwooferInquiryResults(void)
{
    uint8 counter, sub_counter;
    inquiry_result_t temp;
    
    /* Sort the list based on RSSI */
    for (counter=0; counter<SW_MAX_INQUIRY_DEVS; counter++)
    {
        /* Check there's actually an item in the list to compare */
        if (!BdaddrIsZero(&theSink.inquiry.results[counter].bd_addr))
        {
            /* Compare current result against other results and change position if necessary */
            for (sub_counter=0; sub_counter<SW_MAX_INQUIRY_DEVS; sub_counter++)
            {
                /* Only compare against real items in the list */
                if (BdaddrIsZero(&theSink.inquiry.results[sub_counter].bd_addr) == FALSE)
                {
                    /* Don't compare result against itself */
                    if ((theSink.inquiry.results[counter].rssi > theSink.inquiry.results[sub_counter].rssi) && (sub_counter != counter))
                    {
                        /* Current result has a lesser RSSI with compared result so swap their positions in list */
                        temp = theSink.inquiry.results[counter];
                        theSink.inquiry.results[counter] = theSink.inquiry.results[sub_counter];
                        theSink.inquiry.results[sub_counter] = temp;
                    }
                }
                else
                {
                    /* No other results to compare against */
                    break;
                }
            }
        }
        else
        {
            /* No more results to sort */
            break;
        }
    }
#ifdef DEBUG_SWAT
    for (counter=0; counter<SW_MAX_INQUIRY_DEVS; counter++)
    {
        SWAT_DEBUG(("SW : theSink.inquiry.results[%d] = ADDR[%04x %02x %06lx] RSSI[%d]\n", counter, theSink.inquiry.results[counter].bd_addr.nap, theSink.inquiry.results[counter].bd_addr.uap, theSink.inquiry.results[counter].bd_addr.lap, theSink.inquiry.results[counter].rssi));
    }
#endif
}


/****************************************************************************/
static void subwooferStartInqConnection(void)
{
    /* Check a subwoofer device was found by the inquiry search */
    if (BdaddrIsZero((const bdaddr *)&theSink.inquiry.results[0].bd_addr))
    {
        SWAT_DEBUG(("No subwoofer device found by inquiry\n"));
        theSink.inquiry.action = rssi_none;
        free(theSink.inquiry.results);
        theSink.inquiry.results = NULL;
        MessageCancelFirst(&theSink.task, EventSubwooferStartInquiry);
        /* are there any more scan attempts available? */            
        if(theSink.rundata->subwoofer.inquiry_attempts) 
        {
            /* try another sub woofer inquiry search at a later time in case sub
               wasn't available at this time */
            MessageSendLater(&theSink.task, EventSubwooferStartInquiry, 0, SW_INQUIRY_RETRY_TIME);
        }
        /* inquiry now complete, continue with connecting to AG's */
        else
        {
            /* inquiry complete, make soundbar connectable again */
            sinkEnableConnectable();
            /* now attempt to connect AG's */
            MessageSend(&theSink.task, EventEstablishSLC, 0);
        }
    }
    else
    {
        /* Attempt to connect to the first result */
        theSink.inquiry.attempting = 0;
        SwatSignallingConnectRequest(&theSink.inquiry.results[0].bd_addr);
    }
}


/****************************************************************************/
static void subwooferInqNextConnection(void)
{
    theSink.inquiry.attempting++;
    
    /* Check there is another result to make a connection request to */
    if ((theSink.inquiry.attempting >= SW_MAX_INQUIRY_DEVS)||
        (BdaddrIsZero((const bdaddr *)&theSink.inquiry.results[theSink.inquiry.attempting].bd_addr)))
    {
        SWAT_DEBUG(("No more subwoofer devices found by inquiry\n"));
        theSink.inquiry.action = rssi_none;
        free(theSink.inquiry.results);
        theSink.inquiry.results = NULL;
        /* are there any more scan attempts available? */            
        if(theSink.rundata->subwoofer.inquiry_attempts) 
        {
            /* try another sub woofer inquiry search at a later time in case sub
               wasn't available at this time */
            MessageSendLater(&theSink.task, EventSubwooferStartInquiry, 0, SW_INQUIRY_RETRY_TIME);
        }
        /* inquiry now complete, continue with connecting to AG's */
        else
        {
            /* inquiry complete, make soundbar connectable again */
            sinkEnableConnectable();
            MessageSend(&theSink.task, EventEstablishSLC, 0);
        }
    }
    else
    {
        /* Attempt to connect to the result */
        SwatSignallingConnectRequest(&theSink.inquiry.results[theSink.inquiry.attempting].bd_addr);
    }
}

/****************************************************************************/
static void handleSwatSampleRateCfm(SWAT_SAMPLE_RATE_CFM_T * cfm)
{
    SWAT_DEBUG(("SWAT_SAMPLE_RATE_CFM\n"));
}


#else /* ENABLE_SUBWOOFER */

static const int dummy_swat;  /* ISO C forbids an empty source file */

#endif /* ENABLE_SUBWOOFER */
