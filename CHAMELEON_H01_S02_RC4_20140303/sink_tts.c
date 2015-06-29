/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_tts.c
    
DESCRIPTION
    module responsible for tts selection
    
*/

#include "sink_private.h"
#include "sink_debug.h"
#include "sink_tts.h"
#include "sink_events.h"
#include "sink_tones.h"
#include "sink_statemanager.h"
#include "sink_pio.h"
#include "vm.h"


#include <stddef.h>
#include <csrtypes.h>
#include <audio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <partition.h>
#include <csr_tone_plugin.h> 

#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
    #include <csr_simple_text_to_speech_plugin.h>
#endif

#ifdef ENABLE_VOICE_PROMPTS
    #include <csr_voice_prompts_plugin.h>
#endif

#ifdef DEBUG_TTS
    #define TTS_DEBUG(x) DEBUG(x)
#else
    #define TTS_DEBUG(x) 
#endif

/****************************************************************************
VARIABLES      
*/
#define TTS_CALLERID_NAME    (1000)
#define TTS_CALLERID_NUMBER  (1001)


/****************************************************************************
NAME
    TTSPlay
    
DESCRIPTION
    Conditionally call text-to-speech plugin
*/
void TTSPlay(Task plugin, uint16 id, uint8 *data, uint16 size_data, bool can_queue, bool override)
{
    if (theSink.tts_enabled)
    {
        AudioPluginFeatures features;

        /* determine additional features applicable for this audio plugin */
        features.stereo = theSink.features.stereo;
        features.use_i2s_output = theSink.features.UseI2SOutputCapability;

        /* turn on audio amp */
        PioSetPio ( theSink.conf1->PIOIO.pio_outputs.DeviceAudioActivePIO , pio_drive, TRUE) ;
        /* start check to turn amp off again if required */ 
        MessageSendLater(&theSink.task , EventCheckAudioAmpDrive, 0, 1000);    
        
#ifdef ENABLE_SQIFVP        
        /* If using multiple partitions for the voice prompt langages, mount the relevant partiton if required */

        /* find if the partition for this language is mounted */
        if (!((1<<theSink.tts_language) & theSink.rundata->partitions_mounted))
        {
            /* Mount the partition for this prompt */
            TTS_DEBUG(("TTSPlay mount SQIF partition %u (0x%x)\n", theSink.tts_language, theSink.rundata->partitions_mounted));
            if(!PartitionMountFilesystem(PARTITION_SERIAL_FLASH, theSink.tts_language , PARTITION_LOWER_PRIORITY))
                Panic();
            
            theSink.rundata->partitions_mounted |= (1<<theSink.tts_language);
            TTS_DEBUG(("TTSPlay SQIF partitions now mounted 0x%x\n", theSink.rundata->partitions_mounted ));
        }
#endif
        
        TTS_DEBUG(("TTSPlay %d + %d (lang:%u)\n", id, size_data, theSink.tts_language));
        AudioPlayTTS(plugin, id, data, size_data, theSink.tts_language, can_queue,
                     theSink.codec_task, TonesGetToneVolume(FALSE), features, override);
    }
}


/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

void TTSConfigureVoicePrompts( uint8 size_index )
{
#ifdef CSR_VOICE_PROMPTS
    TTS_DEBUG(("Setup VP Indexing: %d prompts\n",size_index));
    AudioVoicePromptsInit((TaskData *)&csr_voice_prompts_plugin, size_index, &theSink.conf4->vp_init_params, theSink.no_tts_languages);
#endif /* CSR_VOICE_PROMPTS */
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/
bool TTSPlayEvent ( sinkEvents_t pEvent )
{  
#ifdef TEXT_TO_SPEECH_PHRASES
    uint16 lEventIndex = pEvent - EVENTS_MESSAGE_BASE ;
	uint16 state_mask  = 1 << stateManagerGetState();
    tts_config_type* ptr  = NULL;
	TaskData * task    = NULL;
    bool event_match   = FALSE;
    
    if(theSink.conf4)
    {
        ptr = theSink.conf4->gEventTTSPhrases;
    }
    else
    {
        /* no config */
        return FALSE;
    }
    
#ifdef FULL_TEXT_TO_SPEECH
	task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;	
#endif
 
#ifdef ENABLE_VOICE_PROMPTS
    task = (TaskData *) &csr_voice_prompts_plugin;
#endif
    /* While we have a valid TTS event */
    while(ptr->tts_id != TTS_NOT_DEFINED)
    {           
        /* Play TTS if the event matches and we're not in a blocked state or in streaming A2DP state */
        if((ptr->event == lEventIndex) && 
           ((ptr->state_mask & state_mask) && 
           (!(ptr->sco_block && theSink.routed_audio)||(state_mask & (1<<deviceA2DPStreaming)))))
        {
            TTS_DEBUG(("TTS: EvPl[%x][%x][%x][%x]\n", pEvent, lEventIndex, ptr->event, ptr->tts_id )) ;
            event_match = TRUE;
            switch(pEvent)
            {
                case EventMuteReminder:
                case TONE_TYPE_RING_1:
                case TONE_TYPE_RING_2:
                
                    /* never queue mute reminders to protect against the case that the prompt is longer 
                    than the mute reminder timer */
       	            TTSPlay(task, (uint16) ptr->tts_id, NULL, 0, FALSE, ptr->cancel_queue_play_immediate);
                break;
                default:
                   TTSPlay(task, (uint16) ptr->tts_id,  NULL, 0, TRUE, ptr->cancel_queue_play_immediate);
                 break;
            }   
        }
        ptr++;
    }
    return event_match;
#else
    return FALSE;
#endif /* TEXT_TO_SPEECH_PHRASES */
} 

/****************************************************************************
NAME 
    TTSPlayNumString
DESCRIPTION
    Play a numeric string using the TTS plugin
RETURNS    
*/

void TTSPlayNumString(uint16 size_num_string, uint8* num_string)
{
    uint8 * data = num_string;
    uint8 i;
    
#ifdef TEXT_TO_SPEECH_DIGITS
#ifdef FULL_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
#endif

#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &csr_simple_text_to_speech_plugin;
#endif
    
#ifdef ENABLE_VOICE_PROMPTS
    TaskData * task = (TaskData *) &csr_voice_prompts_plugin;
#endif
    /* it is important that the string only contains numeric characters, check 
       this is the case prior to passing to the voice prompts plugin otherwise
       it will panic */
    if(size_num_string)
    {
        /* check each character in the string is a numeric character */
        for(i=0;i<size_num_string;i++)
        {
            /* if non-numeric characters are found then exit without doing anything
               as data is invalid */
            if((*data < 0x30)||(*data > 0x39))
                return;
        }
    }
        
    TTSPlay(task, TTS_CALLERID_NUMBER, num_string, size_num_string, FALSE, FALSE);
#endif
}

/****************************************************************************
NAME 
    TTSPlayNumber
DESCRIPTION
    Play a uint32 using the TTS plugin
RETURNS    
*/

void TTSPlayNumber(uint32 number)
{
#ifdef TEXT_TO_SPEECH_DIGITS
    char num_string[7];
    sprintf(num_string, "%06ld",number);
    TTSPlayNumString(strlen(num_string), (uint8*)num_string);
#endif
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

bool TTSPlayCallerNumber( const uint16 size_number, const uint8* number ) 
{
#ifdef TEXT_TO_SPEECH_DIGITS
	if(theSink.features.VoicePromptNumbers)
    {
        if(theSink.RepeatCallerIDFlag && size_number > 0) 
        { 
            theSink.RepeatCallerIDFlag = FALSE;
            TTSPlayNumString(size_number, (uint8*)number);
            return TRUE;
        }
    }
#endif
    return FALSE;
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/
bool TTSPlayCallerName( const uint16 size_name, const uint8* name ) 
{
#ifdef TEXT_TO_SPEECH_NAMES
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
    
	if(size_name > 0) 
	{
        TTSPlay(task, TTS_CALLERID_NAME, (uint8 *) name, size_name, FALSE);
        return TRUE;
	}
#endif
    return FALSE;
}

/****************************************************************************
NAME    
    TTSTerminate
    
DESCRIPTION
  	function to terminate a TTS prematurely.
    
RETURNS
    
*/
void TTSTerminate( void )
{
#ifdef TEXT_TO_SPEECH
#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &csr_simple_text_to_speech_plugin;
#endif
	
#ifdef FULL_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
#endif
	
#ifdef ENABLE_VOICE_PROMPTS
    TaskData * task = (TaskData *) &csr_voice_prompts_plugin;
#endif
    /* Do nothing if TTS Terminate Disabled */
    if(!theSink.features.DisableTTSTerminate)
    {
        /* Make sure we cancel any pending TTS */
        MessageCancelAll(task, AUDIO_PLUGIN_PLAY_TTS_MSG);
        /* Stop the current TTS phrase */
	    AudioStopTTS( task ) ;
    }
#endif /* TEXT_TO_SPEECH */
}  
      
/****************************************************************************
NAME    
    TTSSelectTTSLanguageMode
    
DESCRIPTION
  	function to select a TTS language.
    
RETURNS
    
*/
void TTSSelectTTSLanguageMode( void )
{
#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION
#ifdef ENABLE_SQIFVP     
    uint16 current_lang = theSink.tts_language;
#endif    
    uint16 delay = theSink.conf1->timeouts.LanguageConfirmTime_s;

#ifdef ENABLE_SQIFVP  
    /* if using Multiple partitions in SQIF for voice prompts make sure we choose one with prompts in it */
    do
    {
        theSink.tts_language++;
	    if(theSink.tts_language >= theSink.no_tts_languages)
            theSink.tts_language = 0;   
        
        TTS_DEBUG(("TTS: Select language [%u][%u][%u][0x%x]\n", theSink.tts_language,current_lang, theSink.no_tts_languages,theSink.rundata->partitions_free )) ;
    }
    while (((1<<theSink.tts_language) & theSink.rundata->partitions_free) && (theSink.tts_language != current_lang));

#else    
    theSink.tts_language++;
	if(theSink.tts_language >= theSink.no_tts_languages)
        theSink.tts_language = 0;
    
    TTS_DEBUG(("TTS: Select language [%u]\n", theSink.tts_language )) ;
#endif

    if(delay)
    {
        MessageCancelAll(&theSink.task, EventConfirmTTSLanguage);
        MessageSendLater(&theSink.task, EventConfirmTTSLanguage, 0, D_SEC(delay));
    }
#endif
}

