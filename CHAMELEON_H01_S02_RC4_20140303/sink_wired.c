/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013

FILE NAME
    sink_wired.c        

DESCRIPTION
    Application level implementation of Wired Sink features

NOTES
    - Conditional on ENABLE_WIRED define
    - Output PIO is theSink.conf1->PIOIO.pio_outputs.wired_output
    - Input PIO is theSink.conf1->PIOIO.pio_inputs.wired_input
*/

#include "sink_private.h"
#include "sink_debug.h"
#include "sink_wired.h"
#include "sink_audio.h"
#include "sink_pio.h"
#include "sink_powermanager.h"
#include "sink_statemanager.h"
#include "sink_tones.h"
#include "sink_display.h"
#include "sink_audio_routing.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include <panic.h>
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <string.h>

#ifdef ENABLE_WIRED

#ifdef DEBUG_WIRED
    #define WIRED_DEBUG(x) DEBUG(x)
#else
    #define WIRED_DEBUG(x) 
#endif
/* Wired PIOs */
#define PIO_WIRED_DETECT theSink.conf1->PIOIO.pio_inputs.wired_input
#define PIO_WIRED_SELECT theSink.conf1->PIOIO.pio_outputs.PowerOnPIO
/* Wired audio params */
#define WIRED_READY      ((theSink.conf2 != NULL) && (PIO_WIRED_DETECT != PIN_INVALID))
#define WIRED_SINK       ((Sink)0xFFFF)
#define WIRED_RATE       48000
#define WIRED_PLUGIN     getA2dpPlugin(SBC_SEID)

#include <csr_a2dp_decoder_common_plugin.h>


/****************************************************************************
NAME 
    wiredAudioInit
    
DESCRIPTION
    Set up wired audio PIOs and configuration
    
RETURNS
    void
*/ 
void wiredAudioInit(void)
{
    if(WIRED_READY)
    {
        WIRED_DEBUG(("WIRE: Select %d Detect %d\n", PIO_WIRED_SELECT, PIO_WIRED_DETECT));
        /* Pull detect high, audio jack will pull it low */
        PioSetPio(PIO_WIRED_DETECT, pio_pull, TRUE);
        PioSetPio(PIO_WIRED_SELECT, pio_drive, TRUE);
        /* Set default volume */
        theSink.conf2->wired.volume = theSink.features.DefaultA2dpVolLevel;
        /* Check audio routing */
        audioHandleRouting(audio_source_none);
    }
}


/****************************************************************************
NAME 
    wiredAudioRoute
    
DESCRIPTION
    Route wired audio stream through DSP
    
RETURNS
    void
*/ 
void wiredAudioRoute(void)
{
    WIRED_DEBUG(("WIRE: Audio "));

    /* If config ready check PIO - Connected if low */
    if(WIRED_READY && !PioGetPio(PIO_WIRED_DETECT))
    {
        WIRED_DEBUG(("Connected [0x%04X]\n", (uint16)WIRED_SINK));
        /* If wired audio not already connected, connect it */
        if(theSink.routed_audio != WIRED_SINK)
        {
            AudioPluginFeatures features;
            uint16 volume = theSink.conf1->gVolMaps[theSink.conf2->wired.volume].A2dpGain;
            uint16 mode   = (volume == VOLUME_A2DP_MUTE_GAIN) ? AUDIO_MODE_MUTE_SPEAKER : AUDIO_MODE_CONNECTED;
            /* determine additional features applicable for this audio plugin */
            features.stereo = theSink.features.stereo;
            features.use_i2s_output = theSink.features.UseI2SOutputCapability;
            theSink.routed_audio = WIRED_SINK;
            /* Make sure we're using correct parameters for Wired audio */
            theSink.a2dp_link_data->a2dp_audio_connect_params.mode_params = &theSink.a2dp_link_data->a2dp_audio_mode_params;

#ifdef ENABLE_SUBWOOFER
            /* set the sub woofer link type prior to passing to audio connect */
            theSink.a2dp_link_data->a2dp_audio_connect_params.sub_woofer_type  = AUDIO_SUB_WOOFER_NONE;  
            theSink.a2dp_link_data->a2dp_audio_connect_params.sub_sink  = NULL;  
            /* bits inverted in dsp plugin */                
            sinkAudioSetEnhancement(MUSIC_CONFIG_SUB_WOOFER_BYPASS,TRUE);
#else
            /* no subwoofer support, set the sub woofer bypass bit in music config message sent o dsp */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SUB_WOOFER_BYPASS,FALSE);
#endif          

            WIRED_DEBUG(("WIRE: Routing (Vol %d)\n", volume));

            AudioConnect(WIRED_PLUGIN, WIRED_SINK, AUDIO_SINK_WIRED, theSink.codec_task, volume, WIRED_RATE, features, mode, 0, powerManagerGetLBIPM(), &theSink.a2dp_link_data->a2dp_audio_connect_params, &theSink.task);
                        
            audioControlLowPowerCodecs (FALSE) ;
        }
    }
    else
    {
        WIRED_DEBUG(("Disconnected\n"));
        wiredAudioDisconnect();
    }
}


/****************************************************************************
NAME 
    wiredAudioDisconnect
    
DESCRIPTION
    Force disconnect of wired audio
    
RETURNS
    void
*/ 
void wiredAudioDisconnect(void)
{
    if(WIRED_READY && wiredAudioSinkMatch(theSink.routed_audio))
    {
        WIRED_DEBUG(("WIRE: Disconnect Audio\n"));
        AudioDisconnect();
        theSink.routed_audio = 0;

        /* Update limbo state */
        if (stateManagerGetState() == deviceLimbo )
            stateManagerUpdateLimboState();
    }
}


/****************************************************************************
NAME 
    wiredAudioSinkMatch
    
DESCRIPTION
    Compare sink to wired audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
bool wiredAudioSinkMatch(Sink sink)
{
    if(WIRED_READY)
        return sink == WIRED_SINK;
    return FALSE;
}


/****************************************************************************
NAME 
    wiredAudioUpdateVolume
    
DESCRIPTION
    Adjust wired audio volume if currently being routed
    
RETURNS
    TRUE if wired audio being routed, FALSE otherwise
*/ 
bool wiredAudioUpdateVolume(volume_direction dir)
{
    if(wiredAudioSinkMatch(theSink.routed_audio))
    {
        uint16 volume;
        VolMapping_t* mapping = &theSink.conf1->gVolMaps[theSink.conf2->wired.volume];
        theSink.conf2->wired.volume = (dir == increase_volume ? mapping->IncVol : mapping->DecVol);
        volume = theSink.conf1->gVolMaps[theSink.conf2->wired.volume].A2dpGain;
        
        displayUpdateVolume(theSink.conf2->wired.volume);
        
#ifdef ENABLE_SUBWOOFER
        updateSwatVolume(theSink.conf2->wired.volume);
#endif
        
        if(volume)
        {
            WIRED_DEBUG(("WIRE: Volume %s [0x%x]\n", (dir == increase_volume) ? "Up" : "Down", volume - 1));
            AudioSetVolume(volume - 1, TonesGetToneVolume(FALSE), theSink.codec_task);
            AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        }
        else
        {
            WIRED_DEBUG(("WIRE: Mute\n"));
            AudioSetMode(AUDIO_MODE_MUTE_SPEAKER, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        }
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
NAME 
    wiredAudioConnected
    
DESCRIPTION
    Determine if wired audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/ 
bool wiredAudioConnected(void)
{
    /* If Wired mode configured and ready check PIO */
    if(WIRED_READY)
        return !PioGetPio(PIO_WIRED_DETECT);

    /* Wired audio cannot be connected yet */
    return FALSE;
}


#endif /* ENABLE_WIRED */
