/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2012-2013

FILE NAME
    sink_fm.c
    
DESCRIPTION
    This file handles connection and stream setup for FM Rx 
    (sink to plugin interfacing)
*/

#include "sink_private.h"
#include <string.h>
#include <fm_rx_plugin.h>
#include <fm_rx_api.h>
#include <csr_fm_audio_plugin.h>
#include <ps.h>

#include "sink_debug.h"
#include "sink_statemanager.h"
#include "sink_private.h"
#include "sink_tones.h"
#include "sink_swat.h"

#ifdef ENABLE_DISPLAY
#include "sink_display.h"
#include "display_plugin_if.h"
#endif

#ifdef ENABLE_FM
#include "sink_fm.h"


#ifdef DEBUG_FM
    #define FM_DEBUG(x) DEBUG(x)
    #define FM_ASSERT(x) { if (!(x)) { Panic(); } }
#else
    #define FM_DEBUG(x)
    #define FM_ASSERT(x) 
#endif

#define FM_PLUGIN_RX (TaskData *) &fm_rx_plugin

/****************************************************************************
NAME    
    sinkFmInit
    
DESCRIPTION
    initialises the fm hardware, mode is passed to determine which part of the FM
    system is to be configured

RETURNS
    nothing
*/   
void sinkFmInit(fm_mode mode)
{ 
    FM_DEBUG(("sinkFmInit \n"));

     switch (mode)
    {
        case FM_ENABLE_RX:
            fmRxInit(FM_PLUGIN_RX, &theSink.task, theSink.conf2->sink_fm_data.fm_plugin_data);
            break;
            
        case FM_ENABLE_TX:
            break;
            
        case FM_ENABLE_RX_TX:
            break;
            
        default:
            break;        
    }
}

/****************************************************************************
NAME    
    sinkFmRxAudioConnect
    
DESCRIPTION
    connects the I2S FM audio via the FM audio plugin which allows tones play
    and volume control
RETURNS
    nothing
*/   
void sinkFmRxAudioConnect(void)
{
    AudioPluginFeatures features;

    /* determine additional features applicable for this audio plugin */
    features.stereo = theSink.features.stereo;
    features.use_i2s_output = theSink.features.UseI2SOutputCapability;

    FM_DEBUG(("sinkFmRxAudioConnect \n"));
    
    /* connect audio using the fm audio plugin and initialise the I2S interface */            
    AudioConnect ( (TaskData *)&csr_fm_plugin,
                   theSink.routed_audio  ,
                   0 ,
                   theSink.codec_task ,
                   theSink.conf2->sink_fm_data.fmVol ,
                   FM_RATE ,
                   features ,
                   AUDIO_MODE_CONNECTED,
                   AUDIO_ROUTE_INTERNAL,
                   powerManagerGetLBIPM(),
                   NULL,
                   &theSink.task ) ;       
        
    /* tune to stored frequency after initilising the I2S interface otherwise FM receiver
       may be in an unknown state which may not work reliably */
    fmRxTuneFrequency(theSink.conf2->sink_fm_data.fmRxTunedFreq);
    
    /* set volume using the fm audio plugin */
    AudioSetVolume (theSink.conf2->sink_fm_data.fmVol  , TonesGetToneVolume(FALSE), theSink.codec_task ) ;
}


/****************************************************************************
NAME    
    sinkFmRxAudioDisconnect
    
DESCRIPTION
    Disconnects the FM audio via the audio library/FM audio plugin

RETURNS
    nothing
*/   
void sinkFmRxAudioDisconnect(void)
{
    FM_DEBUG(("sinkFmRxAudioDisconnect \n"));
    
    /* ensure FM is on and FM audio currently being routed to speaker */
    if ((theSink.conf2->sink_fm_data.fmRxOn) && sinkFmAudioSinkMatch(theSink.routed_audio))
    {
        FM_DEBUG(("FM audio disconnected \n"));

        /* reset routed audio */
        theSink.routed_audio = 0;
        
        /* disconnect FM audio via audio library */
        AudioDisconnect(); 

        /* Update limbo state */
        if (stateManagerGetState() == deviceLimbo )
            stateManagerUpdateLimboState();
    }
}


/****************************************************************************
NAME    
    sinkFmTuneUp
    
DESCRIPTION
    initiates an FM auto tune in an increasing frequency direction

RETURNS
    nothing
*/   
void sinkFmRxTuneUp(void)
{
    FM_DEBUG(("sinkFmRxTuneUp \n"));
    fmRxTuneUp();
}


/****************************************************************************
NAME    
    sinkFmTuneDown
    
DESCRIPTION
    initiates an FM auto tune in a decreasing frequency direction

RETURNS
    nothing
*/   
void sinkFmRxTuneDown(void)
{
    FM_DEBUG(("sinkFmRxTuneDown \n"));
    fmRxTuneDown();
}


/****************************************************************************
NAME    
    sinkFmDisplayFreq
    
DESCRIPTION
    utility function for displaying FM station on LCD 
    A favourite station will be indicated by appending a (*)

RETURNS
    nothing
*/   
void sinkFmDisplayFreq(uint16 freq, fm_display_type type )
{
#if ENABLE_DISPLAY
    char display_freq[FM_DISPLAY_STR_LEN];
    uint8 len;
   
    if (type == FM_ADD_FAV_STATION)
    {      
        /*Add a star to a favourite station for the user to identify*/
        len = sprintf(display_freq, "%d.%d FM*", (freq/100), (freq%100)/10);
    }
    else
    {
        len = sprintf(display_freq, "%d.%d FM", (freq/100), (freq%100)/10);
    }

    FM_DEBUG(("FM display freq: %s  (len = %d)\n", display_freq, len));

    displayShowText((char*) display_freq,  len,  1, DISPLAY_TEXT_SCROLL_STATIC, 500, 2000, FALSE, 0);
#endif
}


/****************************************************************************
NAME    
    sinkFmGetIndex
    
DESCRIPTION
    utility function to get a index for requested operation
    In case of STORE, returns a free index.
    In case of ERASE, returns index corresponding to the requested freq

RETURNS
    index in PS key 
*/   
uint8 sinkFmGetIndex(fm_stored_freq *stored_freq, uint16 freq)
{
    uint8 index;

    for (index=0;index<FM_MAX_PRESET_STATIONS;index++) 
    {
        if (stored_freq->freq[index] == freq)
        {
            break;
        }
    }

    FM_DEBUG(("sinkFmGetIndex (%d)\n", index));

    return index;
}


/****************************************************************************
NAME    
    sinkFmUpdateAtIndex
    
DESCRIPTION
    utility function to update freq in Ps key at requested index

RETURNS
    nothing
*/   
void sinkFmUpdateAtIndex(uint8 index, uint16 freq, fm_stored_freq *stored_freq)
{
    FM_ASSERT(index<FM_MAX_PRESET_STATIONS);
    stored_freq->freq[index] = freq;
}


/****************************************************************************
NAME    
    sinkFmRxStoreFreq
    
DESCRIPTION
    Stores the currently tuned frequency to persistant store, if storage space available.
    User must delete a station to make space for newer stations

RETURNS
    nothing
*/   
void sinkFmRxStoreFreq(uint16 freq)
{
    fm_stored_freq *stored_freq = &theSink.conf2->sink_fm_data.fmStoredFreq;
    uint8 index = FM_MAX_PRESET_STATIONS;
  
    FM_DEBUG(("sinkFmRxStoreFreq freq %d\n", freq));
    
    if (freq != INVALID_FREQ)
    {
        /*If requested freq already present in PSKEY, ignore the request*/
        if (sinkFmGetIndex(stored_freq, freq) < FM_MAX_PRESET_STATIONS)
        {       
            FM_DEBUG(("Freq already stored - Do nothing\n"));     
        }
        else
        {
            /*Get free index*/
            index = sinkFmGetIndex(stored_freq, INVALID_FREQ);
            
            if (index < FM_MAX_PRESET_STATIONS)
            {
                FM_DEBUG(("Stored station %d at index %d \n", freq, index));
                sinkFmUpdateAtIndex(index, freq, stored_freq);
                    
                /* store requested frequency to ps user 32 */
                (void) PsStore(PSKEY_FM_FREQUENCY_STORE, stored_freq, sizeof(fm_stored_freq));

                /*Display stored freq with a favourite sign*/
                sinkFmDisplayFreq(freq, FM_ADD_FAV_STATION);
            }
            else /*If no free index available, storage is full, indicate to user*/
            {
#ifdef ENABLE_DISPLAY
                displayShowText(DISPLAYSTR_FM_STORAGE_FULL,  strlen(DISPLAYSTR_FM_STORAGE_FULL), 1, DISPLAY_TEXT_SCROLL_SCROLL, 500, 2000, FALSE, 5);
#endif
                FM_DEBUG(("FM storage full. Please delete a stored station. \n"));
            }            
        }
    }
}

/****************************************************************************
NAME    
    sinkFmRxEraseFreq
    
DESCRIPTION
    erases the currently tuned frequency if it is stored in persistant store

RETURNS
    nothing
*/   
void sinkFmRxEraseFreq(uint16 freq)
{    
    fm_stored_freq *stored_freq = &theSink.conf2->sink_fm_data.fmStoredFreq;
    uint8 index = FM_MAX_PRESET_STATIONS;
    
    FM_DEBUG(("sinkFmRxEraseFreq \n"));

    /*Get index where requested freq is stored*/
    index = sinkFmGetIndex(stored_freq, freq);
    
    /*If no free index available, storage is full, indicate to user*/
    if (index < FM_MAX_PRESET_STATIONS)
    {    
        FM_DEBUG(("Erased station %d at index %d \n", freq, index));

        sinkFmUpdateAtIndex(index, INVALID_FREQ, stored_freq);

        FM_DEBUG(("Station tuned to Freq %d erased \n", freq));

        /* erase the frequency stored in ps user 32 */
        (void) PsStore(PSKEY_FM_FREQUENCY_STORE, stored_freq, sizeof(fm_stored_freq));

        /*Display stored freq without a favourite sign, continue tuning until changed by user*/
        sinkFmDisplayFreq(freq, FM_DEL_FAV_STATION);
    }
}

/****************************************************************************
NAME    
    sinkFmGetNextStoredFreq
    
DESCRIPTION
    utility function to update freq in Ps key at requested index

RETURNS
    nothing
*/   
uint16 sinkFmGetNextStoredFreq(uint16 curr_freq, fm_stored_freq *stored_freq)
{
    uint16 firstValidFreq = INVALID_FREQ;
    bool bFound = FALSE;
    uint8 index = 0;

    while (index < FM_MAX_PRESET_STATIONS)
    {
        if (stored_freq->freq[index])/*check for valid freq in stored list*/
        {
            if (firstValidFreq == INVALID_FREQ)
            {
                /*Store the first valid freq for cases where the curr_freq is not present in stored list*/
                firstValidFreq = stored_freq->freq[index];
            }

            if (stored_freq->freq[index] == curr_freq)
            {
                bFound = TRUE;
            }
            else if (bFound == TRUE)
            {
                return stored_freq->freq[index];
            }
        }
        index++;
    }

    return firstValidFreq;
}


/****************************************************************************
NAME    
    sinkFmRxTuneToStore
    
DESCRIPTION
    tunes the FM receiver to the stored frequency in increasing order
    If currently tuned freq is the first stored freq, then the second entry in the stored list will be played.
    If the second entry is zero, the third entry will be tuned to.
    If no station is stored, request is ignored.

RETURNS
    nothing
*/   
void sinkFmRxTuneToStore(uint16 current_freq)
{
    uint16 tune_freq = INVALID_FREQ;
    fm_stored_freq *stored_freq = &theSink.conf2->sink_fm_data.fmStoredFreq;

    FM_DEBUG(("sinkFmRxTuneToStore current freq %d \n", current_freq));     

    /*Check if currently tuned freq is stored, then tune to the next entry*/
    tune_freq = sinkFmGetNextStoredFreq(current_freq, stored_freq);
    
    FM_DEBUG(("Tune to freq %d \n", tune_freq));

    /* ensure valid frequency before attempting to tune to it */
    if(tune_freq!=INVALID_FREQ)
        fmRxTuneFrequency(tune_freq);
    else
        MessageSend ( &theSink.task, EventError, 0) ;
}


/****************************************************************************
NAME    
    sinkFmRxUpdateVolume
    
DESCRIPTION
    Sets the volume output of the FM reciever chip itself, codec volume control
    is via the FM audio plugin

RETURNS
    nothing
*/   
void sinkFmRxUpdateVolume(uint8 vol)
{
    FM_DEBUG(("sinkFmRxUpdateVolume \n"));
    
    /* FM receiver internal volume is scaled 0 to 64, default volume * 3 gives
       a volume level comparable to other audio sources */
    fmRxUpdateVolume(vol * 3);
}

/****************************************************************************
NAME    
    sinkFmAudioSinkMatch
    
DESCRIPTION
    checks whether the sink passed in is the same as the FM audio sink

RETURNS
    true or false
*/   
bool sinkFmAudioSinkMatch(Sink sink)
{
    if(theSink.conf2->sink_fm_data.fmRxOn)
        return sink == FM_SINK;
    return FALSE;
}

/****************************************************************************
NAME    
    IsSinkFmRxOn
    
DESCRIPTION
    check whether the FM receiver is on

RETURNS
    true or false
*/   
bool IsSinkFmRxOn(void)
{
    return ((theSink.conf2->sink_fm_data.fmRxOn)?(TRUE):(FALSE));
}

/****************************************************************************
NAME    
    sinkFmRxPowerOff
    
DESCRIPTION
    Poewrs off the FM hardware to save power and free memory used

RETURNS
    nothing
*/   
void sinkFmRxPowerOff(void)
{    
    FM_DEBUG(("sinkFmRxPowerOff \n"));
    fmRxPowerOff();
    /* store current frequency in ps */
    configManagerWriteSessionData () ;          
}

#endif /*ENABLE_FM*/
