/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_volume.h
    
DESCRIPTION

    
*/

#ifndef SINK_VOLUME_H
#define SINK_VOLUME_H


#include "sink_volume.h"

#include <hfp.h>


#define VOL_NUM_VOL_SETTINGS     (16)


typedef enum  
{
    increase_volume,
    decrease_volume
}volume_direction;

/* Send 0 for mute on, non-zero for mute off */
#define VOLUME_MUTE_ON        0
#define VOLUME_MUTE_OFF       10

#define VOLUME_A2DP_MAX_LEVEL 15
#define VOLUME_A2DP_MIN_LEVEL 0
#define VOLUME_A2DP_MUTE_GAIN 0

#define VOLUME_HFP_MAX_LEVEL 15
#define VOLUME_HFP_MIN_LEVEL 0

#define VOLUME_FM_MAX_LEVEL 15
#define VOLUME_FM_MIN_LEVEL 0


/****************************************************************************
NAME 
 VolumeSetA2dp
DESCRIPTION
    sets the current A2dp volume
    
RETURNS
 void
    
*/
void VolumeSetA2dp(uint16 index, uint16 oldVolume, bool pPlayTone);


/****************************************************************************
NAME 
 VolumeCheckA2dp

DESCRIPTION
 check whether any a2dp connections are present and if these are currently active
 and routing audio to the device, if that is the case adjust the volume up or down
 as appropriate

RETURNS
 bool   TRUE if volume adjusted, FALSE if no streams found
    
*/
bool VolumeCheckA2dp(volume_direction dir);

/****************************************************************************
NAME 
 VolumeCheckA2dpMute

DESCRIPTION
 check whether any a2dp connections are at minimum volume and mutes them properly if they are

RETURNS
 bool   Returns true if stream muted
    
*/
bool VolumeCheckA2dpMute(void);


/****************************************************************************
NAME 
 VolumeSetHeadsetVolume

DESCRIPTION
 sets the internal speaker gain to the level corresponding to the phone volume level
    
RETURNS
 void
    
*/
void VolumeSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority );


/****************************************************************************
NAME 
 VolumeSendAndSetHeadsetVolume

DESCRIPTION
    sets the vol to the level corresponding to the phone volume level
    In addition - send a response to the AG indicating new volume level

RETURNS
 void
    
*/
void VolumeSendAndSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone , hfp_link_priority priority );


/****************************************************************************
NAME 
    VolumeGet

DESCRIPTION
    Returns the absolute HFP volume level

RETURNS
    void
    
*/
uint16 VolumeGet(void);


/****************************************************************************
NAME 
    VolumeSet

DESCRIPTION
    Sets HFP volume to absolute level

RETURNS
    void
    
*/
void VolumeSet(uint16 level);


/****************************************************************************
NAME 
    VolumeCheck

DESCRIPTION
    Increase/Decrease volume

RETURNS
 void
    
*/
void VolumeCheck(volume_direction dir);


/****************************************************************************
NAME 
 VolumeHandleSpeakerGainInd

DESCRIPTION
 Handle speaker gain change indication from the AG

RETURNS
 void
    
*/
void VolumeHandleSpeakerGainInd(HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T* ind);


/****************************************************************************
NAME 
 VolumeToggleMute

DESCRIPTION
 Toggles the mute state

RETURNS
 void
    
*/
void VolumeToggleMute( void );


/****************************************************************************
DESCRIPTION
    sends the current microphone volume to the AG on connection.
*/
void VolumeSendMicrophoneGain(hfp_link_priority priority, uint8 mic_gain);


/****************************************************************************
DESCRIPTION
    Set mute or unmute (mic gain of VOLUME_MUTE_ON - 0 is mute, all other 
    gain settings unmute).
*/
void VolumeSetMicrophoneGain(hfp_link_priority priority, uint8 mic_gain);


/****************************************************************************
DESCRIPTION
    Set mute or unmute remotely from AG if SyncMic feature bit is enabled
    (mic gain of VOLUME_MUTE_ON - 0 is mute, all other gain settings unmute).
*/
void VolumeSetMicrophoneGainCheckMute(hfp_link_priority priority, uint8 mic_gain);
        
/****************************************************************************
DESCRIPTION
    Determine whether the mute reminder tone should be played in the device, e.g. #
    if AG1 is in mute state but AG2 is not muted and is the active AG then the mute reminder
    tone will not be played, when AG1 becomes the active AG it will be heard.
*/
bool VolumePlayMuteToneQuery(void);

#endif

