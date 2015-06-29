/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_configmanager.c
    
DESCRIPTION
    Configuration manager for the device - resoponsible for extracting user information out of the 
    PSKEYs and initialising the configurable nature of the sink device components
    
*/

#include "sink_configmanager.h"
#include "sink_config.h"
#include "sink_buttonmanager.h"
#include "sink_led_manager.h"
#include "sink_statemanager.h"
#include "sink_powermanager.h"
#include "sink_volume.h"
#include "sink_devicemanager.h"
#include "sink_private.h"
#include "sink_events.h"
#include "sink_tones.h"
#include "sink_tts.h"
#include "sink_audio.h"

#include "sink_pio.h"

#include <csrtypes.h>
#include <ps.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <panic.h>
#include <boot.h>

#ifdef ENABLE_REMOTE
#include "sink_remote_control.h"
#endif

#ifdef DEBUG_CONFIG
#define CONF_DEBUG(x) DEBUG(x)
#else
#define CONF_DEBUG(x) 
#endif

#define HCI_PAGESCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_PAGESCAN_WINDOW_DEFAULT   (0x12)
#define HCI_INQUIRYSCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_INQUIRYSCAN_WINDOW_DEFAULT   (0x12)


#define DEFAULT_VOLUME_MUTE_REMINDER_TIME_SEC 10

/* Conversion macros */
#define MAKEWORD(a, b)      ((uint16)(((uint8)((uint32)(a) & 0xff)) | ((uint16)((uint8)((uint32)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((uint32)(((uint16)((uint32)(a) & 0xffff)) | ((uint32)((uint16)((uint32)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((uint16)((uint32)(l) & 0xffff))
#define HIWORD(l)           ((uint16)((uint32)(l) >> 16))
#define LOBYTE(w)           ((uint8)((uint32)(w) & 0xff))
#define HIBYTE(w)           ((uint8)((uint32)(w) >> 8))


/****************************************************************************
NAME 
  	configManagerKeyLengths

DESCRIPTION
 	Read the lengths of ps keys for tts, tone and led configuration
 
RETURNS
  	void
*/ 
static void configManagerKeyLengths( lengths_config_type * pLengths )
{		
	ConfigRetrieve(theSink.config_id , PSKEY_LENGTHS, pLengths, sizeof(lengths_config_type));
		
	DEBUG(("CONF: LENGTHS [%x][%x][%x][%x][%x][%x]\n" , pLengths->no_tts,
											pLengths->no_tts_languages,
											pLengths->no_led_filter,
											pLengths->no_led_states,
											pLengths->no_led_events,
											pLengths->no_tones )) ;
											
	/* Set led lengths straight away */											
	theSink.theLEDTask->gStatePatternsAllocated = pLengths->no_led_states;
	theSink.theLEDTask->gEventPatternsAllocated = pLengths->no_led_events;
	theSink.theLEDTask->gLMNumFiltersUsed = pLengths->no_led_filter;
    theSink.rundata->defrag = pLengths->defrag;
}

/****************************************************************************
NAME 
  	configManagerButtons

DESCRIPTION
 	Read the system event configuration from persistent store and configure
  	the buttons by mapping the associated events to them
 
RETURNS
  	void
*/  

static void configManagerButtons( void )
{  
	/* Allocate enough memory to hold event configuration */
    event_config_type* configA = (event_config_type*) mallocPanic(BM_EVENTS_PER_PS_BLOCK * sizeof(event_config_type));
    event_config_type* configB = (event_config_type*) mallocPanic(BM_EVENTS_PER_PS_BLOCK * sizeof(event_config_type));
    event_config_type* configC = (event_config_type*) mallocPanic(BM_EVENTS_PER_PS_BLOCK * sizeof(event_config_type));
       
    uint16 n;
    uint8  i = 0;
 
    ConfigRetrieve(theSink.config_id , PSKEY_EVENTS_A, configA, BM_EVENTS_PER_PS_BLOCK * sizeof(event_config_type)) ;
    ConfigRetrieve(theSink.config_id , PSKEY_EVENTS_B, configB, BM_EVENTS_PER_PS_BLOCK * sizeof(event_config_type)) ; 
    ConfigRetrieve(theSink.config_id , PSKEY_EVENTS_C, configC, BM_EVENTS_PER_PS_BLOCK * sizeof(event_config_type)) ;  
  
        /* Now we have the event configuration, map required events to system events */
    for(n = 0; n < BM_EVENTS_PER_PS_BLOCK; n++)
    { 
        CONF_DEBUG(("Co : AddMap indexes [%u,%u] Ev[%x][%x][%x]\n", n, i, configA[n].event , configB[n].event, configC[n].event )) ;
                       
           /* check to see if a valid pio mask is present, this includes the upper 2 bits of the state
              info as these are being used for bc5 as vreg enable and charger detect */
        if ( (configA[n].pio_mask)||(configA[n].state_mask & 0xC000))
            buttonManagerAddMapping ( &configA[n], i++ ); 
               
        if ( (configB[n].pio_mask)||(configB[n].state_mask & 0xC000))
            buttonManagerAddMapping ( &configB[n], i++ ); 
        
        if ( (configC[n].pio_mask)||(configC[n].state_mask & 0xC000))
            buttonManagerAddMapping ( &configC[n], i++ );         
                                                                 
   	}
    
    freePanic(configA) ;
    freePanic(configB) ; 
    freePanic(configC) ; 
    
    /* perform an initial pio check to see if any pio changes need processing following the completion
       of the configuration ps key reading */
    BMCheckButtonsAfterReadingConfig();


}
/****************************************************************************
NAME 
  	configManagerButtonPatterns

DESCRIPTION
  	Read and configure any buttonpattern matches that exist
 
RETURNS

*/
static void configManagerButtonPatterns( void ) 
{  
      		/* Allocate enough memory to hold event configuration */
    button_pattern_config_type* config = (button_pattern_config_type*) mallocPanic(BM_NUM_BUTTON_MATCH_PATTERNS * sizeof(button_pattern_config_type));
   
    CONF_DEBUG(("Co: No Button Patterns - %d\n", BM_NUM_BUTTON_MATCH_PATTERNS));
   
        /* Now read in event configuration */
    if(config)
    {     
        if(ConfigRetrieve(theSink.config_id , PSKEY_BUTTON_PATTERN_CONFIG, config, BM_NUM_BUTTON_MATCH_PATTERNS * sizeof(button_pattern_config_type)))
        {
            uint16 n;
     
           /* Now we have the event configuration, map required events to system events */
            for(n = 0; n < BM_NUM_BUTTON_MATCH_PATTERNS ; n++)
            {	 
     	      CONF_DEBUG(("Co : AddPattern Ev[%x]\n", config[n].event )) ;
                        
          			   /* Map PIO button event to system events in specified states */
          	    buttonManagerAddPatternMapping ( theSink.theButtonsTask , config[n].event , config[n].pattern, n ) ;
            }
        }
        else
 	    {
 	      CONF_DEBUG(("Co: !EvLen\n")) ;
        }
        freePanic(config) ;
    }    
}


/****************************************************************************
NAME 
  	configManagerLEDS

DESCRIPTION
  	Read the system LED configuration from persistent store and configure
  	the LEDS 
 
RETURNS
  	void
*/ 
static void configManagerLEDS( void )
{ 
  	/* 1. LED state configuration */
    	
    /* Providing there are states to configure */
    if((theSink.theLEDTask->gStatePatternsAllocated > 0) && (theSink.theLEDTask->gStatePatternsAllocated < SINK_NUM_STATES))
    {      		
   		ConfigRetrieve(theSink.config_id , PSKEY_LED_STATES, &theSink.theLEDTask->gStatePatterns, theSink.theLEDTask->gStatePatternsAllocated * sizeof(LEDPattern_t));
  	}      	    
    
    /* 2. LED event configuration */
      
    /* Providing there are events to configure */
    if((theSink.theLEDTask->gEventPatternsAllocated > 0) && (theSink.theLEDTask->gEventPatternsAllocated < EVENTS_MAX_EVENTS))
    {      		
   		ConfigRetrieve(theSink.config_id , PSKEY_LED_EVENTS, &theSink.theLEDTask->gEventPatterns, theSink.theLEDTask->gEventPatternsAllocated * sizeof(LEDPattern_t));
  	} 	    

    /* 3. LED event filter configuration */
	
    /* Providing there are states to configure */
    if((theSink.theLEDTask->gLMNumFiltersUsed > 0) && (theSink.theLEDTask->gLMNumFiltersUsed < LM_NUM_FILTER_EVENTS))
    {                    
           /* read from ps straight into filter config structure */
    	ConfigRetrieve(theSink.config_id , PSKEY_LED_FILTERS, &theSink.theLEDTask->gEventFilters, theSink.theLEDTask->gLMNumFiltersUsed * sizeof(LEDFilter_t));
    }
     
    /*tri colour behaviour*/  	
  	ConfigRetrieve(theSink.config_id , PSKEY_TRI_COL_LEDS, &theSink.theLEDTask->gTriColLeds,  sizeof(uint16)) ; 	
  	DEBUG(("CONF: TRICOL [%x][%x][%x]\n" , theSink.theLEDTask->gTriColLeds.TriCol_a ,
  										   theSink.theLEDTask->gTriColLeds.TriCol_b ,
  										   theSink.theLEDTask->gTriColLeds.TriCol_c )) ;
    
    
}

/****************************************************************************
NAME 
  	configManagerFeatureBlock

DESCRIPTION
  	Read the system feature block and configure system accordingly
 
RETURNS
  	void
*/
static void configManagerFeatureBlock( void ) 
{
    uint8 i;
	
    
    /* Read the feature block from persistent store */
  	ConfigRetrieve(theSink.config_id , PSKEY_FEATURE_BLOCK, &theSink.features, sizeof(feature_config_type)) ; 	
	
	/*Set the default volume level*/
    for(i=0;i<MAX_PROFILES;i++)
    {
        theSink.profile_data[i].audio.gSMVolumeLevel = theSink.features.DefaultVolume ;  
    }    
    
}


/****************************************************************************
NAME 
  	configManagerUserDefinedTones

DESCRIPTION
  	Attempt to read the user configured tones, if data exists it will be in the following format:
    
    uint16 offset in array to user tone 1,
    uint16 offset in array to user tone ....,
    uint16 offset in array to user tone 8,
    uint16[] user tone data
    
    To play a particular tone it can be access via gVariableTones, e.g. to access tone 1
    
    theSink.audioData.gConfigTones->gVariableTones[0] + (uint16)*theSink.audioData.gConfigTones->gVariableTones[0]
    
    or to access tone 2

    theSink.audioData.gConfigTones->gVariableTones[0] + (uint16)*theSink.audioData.gConfigTones->gVariableTones[1]
    
    and so on
 
RETURNS
  	void
*/
static void configManagerUserDefinedTones( uint16 KeyLength ) 
{
    /* if the keyLength is zero there are no user tones so don't malloc any memory */
    if(KeyLength)
    {
        /* malloc only enough memory to hold the configured tone data */
        uint16 * configTone = mallocPanic(KeyLength * sizeof(uint16));
    
        /* retrieve pskey data up to predetermined max size */
        uint16 size_ps_key = ConfigRetrieve( theSink.config_id, PSKEY_CONFIG_TONES , configTone , KeyLength );
      
        CONF_DEBUG(("Co : Configurable Tones Malloc size [%x]\n", KeyLength )) ;
        
        /* is there any configurable tone data, if present update pointer to tone data */
        if (size_ps_key)
        {
            /* the data is in the form of 8 x uint16 audio note start offsets followed by the 
               up to 8 lots of tone data */
            theSink.conf1->gConfigTones.gVariableTones = (ringtone_note*)&configTone[0];
        
        }
        /* no user configured tone data is available, so free previous malloc as not in use */
        else
        {
            /* no need to waste memory */
        	freePanic(configTone);      
        }
    }
    /* no tone data available, ensure data pointer is null */
    else
    {
            theSink.conf1->gConfigTones.gVariableTones = NULL;
    }
}

/****************************************************************************
NAME 
  configManagerConfiguration

DESCRIPTION
  Read configuration of pio i/o, timeouts, RSSI pairing and filter.
 
RETURNS
  void
*/ 
static void configManagerConfiguration ( void )
{
	ConfigRetrieve(theSink.config_id , PSKEY_TIMEOUTS, (void*)&theSink.conf1->timeouts, sizeof(Timeouts_t) ) ;
	ConfigRetrieve(theSink.config_id , PSKEY_RSSI_PAIRING, (void*)&theSink.conf2->rssi, sizeof(rssi_pairing_t) ) ; 
}

/****************************************************************************
NAME 
  	configManagerButtonDurations

DESCRIPTION
  	Read the button configuration from persistent store and configure
  	the button durations
 
RETURNS
  	void
*/ 
static void configManagerButtonDurations( void )
{
    if(ConfigRetrieve(theSink.config_id , PSKEY_BUTTON_CONFIG, &theSink.conf1->buttons_duration, sizeof(button_config_type)))
 	{
			/*buttonmanager keeps buttons block*/
  		buttonManagerConfigDurations ( theSink.theButtonsTask ,  &theSink.conf1->buttons_duration ); 
    }
}
    
/****************************************************************************
NAME 
  	configManagerButtonTranslations

DESCRIPTION
  	Read the button translation configuration from persistent store and configure
  	the button input assignments, these could be pio or capacitive touch sensors 
 
RETURNS
  	void
*/ 
static void configManagerButtonTranslations( void )
{
    /* get current input/button translations from eeprom or const if no eeprom */
    ConfigRetrieve(theSink.config_id , PSKEY_BUTTON_TRANSLATION, &theSink.theButtonsTask->gTranslations, (sizeof(button_translation_type) * BM_NUM_BUTTON_TRANSLATIONS));
}


/****************************************************************************
NAME 
 	configManagerPower

DESCRIPTION
 	Read the Power Manager configuration
 
RETURNS
 	void
*/ 
static void configManagerPower( void )
{
    sink_power_config power;

 	/* Read in the battery monitoring configuration */
	ConfigRetrieve(theSink.config_id , PSKEY_BATTERY_CONFIG, (void*)&power, sizeof(sink_power_config) ) ;
   
    /* Store power settings */
    theSink.conf1->power = power.settings;
    
  	/* Setup the power manager */
    powerManagerConfig(&power.config);
}

/****************************************************************************
NAME 
  	configManagerRadio

DESCRIPTION
  	Read the Radio configuration parameters
 
RETURNS
  	void
*/ 

static void configManagerRadio( void )
{ 
  	if(!ConfigRetrieve(theSink.config_id , PSKEY_RADIO_CONFIG, &theSink.conf2->radio, sizeof(radio_config_type)))
  	{
    	/* Assume HCI defaults */
    	theSink.conf2->radio.page_scan_interval = HCI_PAGESCAN_INTERVAL_DEFAULT;
    	theSink.conf2->radio.page_scan_window = HCI_PAGESCAN_WINDOW_DEFAULT;
    	theSink.conf2->radio.inquiry_scan_interval = HCI_INQUIRYSCAN_INTERVAL_DEFAULT;
    	theSink.conf2->radio.inquiry_scan_window  = HCI_INQUIRYSCAN_WINDOW_DEFAULT;
  	}
}

/****************************************************************************
NAME 
  	configManagerVolume

DESCRIPTION
  	Read the Volume Mappings and set them up
 
RETURNS
  	void
*/ 
static void configManagerVolume( void )
{
    uint16 lSize = (VOL_NUM_VOL_SETTINGS * sizeof(VolMapping_t) );
    
    ConfigRetrieve(theSink.config_id , PSKEY_SPEAKER_GAIN_MAPPING, theSink.conf1->gVolMaps, lSize) ;
}


/****************************************************************************
NAME 
  	configManagerEventTone

DESCRIPTION
  	Configure an event tone only if one is defined
 
RETURNS
  	void
*/ 
static void configManagerEventTones( uint16 no_tones )
{ 
  	/* First read the number of events configured */	
  	if(no_tones)		
  	{
        /* Now read in tones configuration */
        if(ConfigRetrieve(theSink.config_id , PSKEY_TONES, &theSink.conf2->gEventTones, no_tones * sizeof(tone_config_type)))
        {
			/*set the last tone (empty) - used by the play tone routine to identify the last tone*/
			theSink.conf2->gEventTones[no_tones].tone = TONE_NOT_DEFINED ;			
        }  
     }                    
}            



/****************************************************************************
NAME 
  	configManagerEventTTSPhrases

DESCRIPTION
  	Configure an event tts phrase only if one is defined
 
RETURNS
  	void
*/ 
static void configManagerEventTTSPhrases( uint16 no_tts )
{
#ifdef TEXT_TO_SPEECH_PHRASES    
  	/* check the number of events configured */
	if(no_tts)
  	{
    	/* Now read in TTS configuration */
        ConfigRetrieve(theSink.config_id , PSKEY_TTS, &theSink.conf4->gEventTTSPhrases, no_tts * sizeof(tts_config_type));
        
        /* Terminate the list */
        theSink.conf4->gEventTTSPhrases[no_tts].tts_id = TTS_NOT_DEFINED;
    }
#endif
}

#ifdef ENABLE_SQIFVP
void configManagerSqifPartitionsInit( void )
{
    /* read currently free SQIF partitions */                          
    uint16 ret_len = 0;
    uint16 partition_data = 0;
 
    /* Read partition information from PS */
    ret_len = PsRetrieve(PSKEY_SQIF_PARTITIONS, &partition_data, sizeof(uint16));
     
    if(!ret_len)
    {            
        /* no key present - assume all partitions are in use */
        theSink.rundata->partitions_free = 0;
    }
    else
    {
        theSink.rundata->partitions_free = LOWORD(partition_data);
    }
    
    CONF_DEBUG(("CONF : Current SQIF partitions free 0x%x \n", theSink.rundata->partitions_free));
    
    /* Check that the currently selected languages partition is available */
    if((1<<theSink.tts_language) & theSink.rundata->partitions_free)
    {
        uint16 currentLang = theSink.tts_language;
        CONF_DEBUG(("CONF : Current SQIF VP partition (%u) not valid\n", currentLang));
        TTSSelectTTSLanguageMode();
        /* if the new selected language is the same then none could be found so disable TTS */
        if (currentLang == theSink.tts_language)
        {
            theSink.tts_enabled = FALSE;
            CONF_DEBUG(("CONF : Disabling TTS, no valid VP partitions\n"));
        }
    }
}
#endif  

static void configManagerVoicePromptsInit( uint16 no_vp , uint16 no_tts_languages )
{
#ifdef CSR_VOICE_PROMPTS
  	/* check the number of events configured and the supported tts languages */
	if(no_vp)
  	{
        /* Read in the PSKEY that tells us where the prompt header is */
        if(ConfigRetrieve(theSink.config_id , PSKEY_VOICE_PROMPTS, &theSink.conf4->vp_init_params, sizeof(vp_config_type)))
        {
#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION
            theSink.no_tts_languages = no_tts_languages;
            
#ifdef ENABLE_SQIFVP
            /* Initilaise as no partitions currently mounted */
            theSink.rundata->partitions_mounted = 0;
            
            configManagerSqifPartitionsInit(  );
#endif            
                 
#endif
            /* Pass configuration to the voice prompts plugin */
            TTSConfigureVoicePrompts(no_vp);
        }
    }
#endif /* CSR_VOICE_PROMPTS */
}



/****************************************************************************
NAME 
 	InitConfigMemoryAtCommands

DESCRIPTION
    Dynamically allocate memory slot for AT commands (conf3)
 
RETURNS
 	void
*/ 
static void InitConfigMemoryAtCommands(uint16 atCmdsKeyLength)
{    
	/* Allocate memory for supported custom AT commands, malloc one extra word to ensure 
       a string terminator is present should the data read from ps be incorrect */
    theSink.conf3 = mallocPanic( sizeof(config_block3_t) + atCmdsKeyLength + 1);
    /* ensure that all data is set as string terminator in case of incorrect pskey 
       data being read */
    memset(theSink.conf3, 0, (sizeof(config_block3_t) + atCmdsKeyLength + 1));
	CONF_DEBUG(("INIT: Malloc size %d:\n", sizeof(config_block3_t) + atCmdsKeyLength ));
}


/****************************************************************************
NAME 
 	InitConfigMemory

DESCRIPTION
    Dynamic allocate memory slots.  
    Memory slots available to the application are limited, so store multiple configuration items in one slot.
 
RETURNS
 	void
*/ 

static void InitConfigMemory(lengths_config_type * keyLengths)
{    
	/* Allocate memory for SSR, PIO and radio data, and Event Tone */
    theSink.conf2 = mallocPanic( sizeof(config_block2_t) + (keyLengths->no_tones * sizeof(tone_config_type)) );
 	CONF_DEBUG(("INIT: Malloc size %d:\n", sizeof(config_block2_t) + (keyLengths->no_tones * sizeof(tone_config_type))));
    
    /* initialise the memory block to 0 */    
    memset(theSink.conf2,0,sizeof(config_block2_t) + (keyLengths->no_tones * sizeof(tone_config_type)));
    
    theSink.conf3 = NULL;
    
    /* Allocate memory for voice prompts config if required*/
    if(keyLengths->no_tts || keyLengths->no_vp)
    {
        theSink.conf4 = mallocPanic( sizeof(config_block4_t) + (keyLengths->no_tts * sizeof(tts_config_type)));
        CONF_DEBUG(("INIT: Malloc size %d:\n", sizeof(config_block4_t) + (keyLengths->no_tts * sizeof(tts_config_type))));
    }
    else
    {
        theSink.conf4 = NULL;
    }
}


/****************************************************************************
NAME 
 	configManagerAtCommands

DESCRIPTION
    Retrieve configurable AT command data from PS
 
RETURNS
 	void
*/ 
static void configManagerAtCommands(uint16 length)
{
    ConfigRetrieve(theSink.config_id, PSKEY_AT_COMMANDS, theSink.conf3, length);
}


/****************************************************************************
NAME 
 	configManagerPioMap

DESCRIPTION
    Read PIO config and map in configured PIOs.
 
RETURNS
 	void
*/ 
void configManagerPioMap(void)
{
    pio_config_type* pio;

	/* Allocate memory for Button data, volume mapping */
    theSink.conf1 = mallocPanic( sizeof(config_block1_t) );
    memset(theSink.conf1, 0, sizeof (config_block1_t));
	CONF_DEBUG(("INIT: Malloc size 1: [%d]\n",sizeof(config_block1_t)));    

    /* Retrieve config */
    pio = &theSink.conf1->PIOIO;
    ConfigRetrieve(theSink.config_id , PSKEY_PIO_BLOCK, pio, sizeof(pio_config_type));

    /* Make sure all references to mic parameters point to the right place */
    theSink.cvc_params.digital = &pio->digital;

    /* Map in any required pins */
    CONF_DEBUG(("INIT: Map PIO 0x%lX\n", pio->pio_map));
    PioSetMapPins32(pio->pio_map,pio->pio_map);
}


/****************************************************************************
NAME 
 	configManagerUsb

DESCRIPTION
    Retrieve USB config from PS
 
RETURNS
 	void
*/ 
#ifdef ENABLE_USB
void configManagerUsb(void)
{
    ConfigRetrieve(theSink.config_id, PSKEY_USB_CONFIG, &theSink.usb.config, sizeof(usb_config));
}
#endif

/****************************************************************************
NAME 
 	configManagerReadFmData

DESCRIPTION
    Retrieve FM config from PS
 
RETURNS
 	void
*/ 
#ifdef ENABLE_FM
void configManagerReadFmData(void)
{
    /* determine size of data required by fm library */
	int lSize = (sizeof(fm_rx_data_t) + FMRX_MAX_BUFFER_SIZE); 
	/*allocate the memory*/
	theSink.conf2->sink_fm_data.fm_plugin_data = mallocPanic( lSize );
    /* initialise structure */    
    memset(theSink.conf2->sink_fm_data.fm_plugin_data, 0, lSize);  
    /* get the FM config from pskey */
    ConfigRetrieve(theSink.config_id, PSKEY_FM_CONFIG, &theSink.conf2->sink_fm_data.fm_plugin_data->config, sizeof(fm_rx_config));

    /* get the FM stored freq from pskey */
    ConfigRetrieve(theSink.config_id, PSKEY_FM_FREQUENCY_STORE, &theSink.conf2->sink_fm_data.fmStoredFreq, sizeof(fm_stored_freq));   
}       
#endif
        
/****************************************************************************
NAME 
 	configManagerHFP_SupportedFeatures

DESCRIPTION
    Gets the HFP Supported features set from PS
 
RETURNS
 	void
*/
void configManagerHFP_SupportedFeatures( void )
{
    ConfigRetrieve( theSink.config_id , PSKEY_ADDITIONAL_HFP_SUPPORTED_FEATURES , &theSink.HFP_supp_features , sizeof( HFP_features_type ) ) ;
}

/****************************************************************************
NAME 
 	configManagerHFP_Init

DESCRIPTION
    Gets the HFP initialisation parameters from PS
 
RETURNS
 	void
*/
void configManagerHFP_Init( hfp_init_params * hfp_params )
{     
    ConfigRetrieve( theSink.config_id , PSKEY_HFP_INIT , hfp_params , sizeof( hfp_init_params ) ) ;
    
}

/*************************************************************************
NAME    
    ConfigManagerSetupSsr
    
DESCRIPTION
    This function attempts to retreive SSR parameters from the PS, setting
	them to zero if none are found

RETURNS

*/
static void configManagerSetupSsr( void  ) 
{
	CONF_DEBUG(("CO: SSR\n")) ;

	/* Get the SSR params from the PS/Config */
	ConfigRetrieve(theSink.config_id , PSKEY_SSR_PARAMS, &theSink.conf2->ssr_data, sizeof(subrate_t));
}

/****************************************************************************
NAME 
 	configManagerEnableMultipoint
*/ 
void configManagerEnableMultipoint(bool enable)
{
    CONF_DEBUG(("CONF: Multipoint %s\n", enable ? "Enable" : "Disable"));
    if(enable)
    {
        /* Check we can make HFP multi point */
        if(HfpLinkSetMaxConnections(2))
        {
            /* If A2DP disabled or we can make it multi point then we're done */
            if(!theSink.features.EnableA2dpStreaming || A2dpConfigureMaxRemoteDevices(2))
            {
                CONF_DEBUG(("CONF: Success\n"));
                theSink.MultipointEnable = TRUE;
                return;
            }
        }
    }
    else
    {
        /* Check we can make HFP single point */
        if(HfpLinkSetMaxConnections(1))
        {
            /* If A2DP disabled or we can make it single point then we're done */
            if(!theSink.features.EnableA2dpStreaming || A2dpConfigureMaxRemoteDevices(1))
            {
                CONF_DEBUG(("CONF: Success\n"));
                theSink.MultipointEnable = FALSE;
                return;
            }
        }
    }
    CONF_DEBUG(("CONF: Failed\n"));
    /* Setting failed, make sure HFP setting is restored */
    HfpLinkSetMaxConnections(theSink.MultipointEnable ? 2 : 1);
}

/****************************************************************************
NAME 
 	configManagerReadSessionData
*/ 
static void configManagerReadSessionData( void ) 
{
    session_data_type lTemp ;
   
    /*read in the volume orientation*/
    ConfigRetrieve( theSink.config_id , PSKEY_VOLUME_ORIENTATION , &lTemp , sizeof( session_data_type ) ) ;
  
    theSink.VolumeOrientationIsInverted = lTemp.vol_orientation ;
    /* if the feature bit to reset led disable state after a reboot is set then enable the leds
       otherwise get the led enable state from ps */
    if(!theSink.features.ResetLEDEnableStateAfterReset)
    {
        theSink.theLEDTask->gLEDSEnabled    = lTemp.led_disable ;
    }
    else
    {
        theSink.theLEDTask->gLEDSEnabled    = TRUE;   
    }  
            
    theSink.tts_language                = lTemp.tts_language;
    theSink.tts_enabled                 = lTemp.tts_enable;
    theSink.iir_enabled                 = lTemp.iir_enable;
    theSink.lbipmEnable                 = lTemp.lbipm_enable;
    theSink.ssr_enabled                 = lTemp.ssr_enabled;
    theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = lTemp.eq;
    configManagerEnableMultipoint(lTemp.multipoint_enable);
    theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = (A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0 + (lTemp.audio_enhancements & A2DP_MUSIC_CONFIG_USER_EQ_SELECT));
    theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements = (lTemp.audio_enhancements & ~A2DP_MUSIC_CONFIG_USER_EQ_SELECT);
#ifdef ENABLE_FM
    theSink.conf2->sink_fm_data.fmRxTunedFreq = lTemp.fm_frequency;
#endif                                                
    CONF_DEBUG(("CONF : Rd Vol Inverted [%c], LED Disable [%c], Multipoint Enable [%c], IIR Enable [%c], LBIPM Enable [%c] EQ[%x]\n", theSink.VolumeOrientationIsInverted ? 'T':'F' ,
 																							 lTemp.led_disable ? 'T':'F' ,
        																					 theSink.MultipointEnable ? 'T':'F',
                                                                                             theSink.iir_enabled ? 'T':'F',
                                                                                             theSink.lbipmEnable ? 'T':'F',
                                                                                             theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing)) ; 
    
    
}

/****************************************************************************
NAME 
 	configManagerWriteSessionData

*/ 
void configManagerWriteSessionData( void )
{
    session_data_type lTemp ;

    lTemp.vol_orientation   = theSink.gVolButtonsInverted;
    lTemp.led_disable       = theSink.theLEDTask->gLEDSEnabled;
	lTemp.tts_language      = theSink.tts_language;
    lTemp.tts_enable        = theSink.tts_enabled;
    lTemp.multipoint_enable = theSink.MultipointEnable;
    lTemp.iir_enable        = theSink.iir_enabled ;
    lTemp.lbipm_enable      = theSink.lbipmEnable;
    lTemp.ssr_enabled       = theSink.ssr_enabled;
    lTemp.eq                = 0;
    lTemp.unused            = 0;
    lTemp.audio_enhancements = ((theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing - A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0) & A2DP_MUSIC_CONFIG_USER_EQ_SELECT)|
                               (theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & ~A2DP_MUSIC_CONFIG_USER_EQ_SELECT);
#ifdef ENABLE_FM
    lTemp.fm_frequency      = theSink.conf2->sink_fm_data.fmRxTunedFreq;
#else
    lTemp.fm_frequency = 0;
#endif    

    CONF_DEBUG(("CONF : PS Write Vol Inverted[%c], LED Disable [%c], Multipoint Enable [%c], IIR Enable [%c], LBIPM Enable [%c] Enhancements[%x]\n" , 
						theSink.gVolButtonsInverted ? 'T':'F' , 
						lTemp.led_disable ? 'T':'F',
                        theSink.MultipointEnable ? 'T':'F',
                        theSink.iir_enabled ? 'T':'F',
                        theSink.lbipmEnable ? 'T':'F',
                        lTemp.audio_enhancements));
    
    (void) PsStore(PSKEY_VOLUME_ORIENTATION, &lTemp, sizeof(session_data_type)); 
    
}


/****************************************************************************
NAME 
  	configManagerRestoreDefaults

DESCRIPTION
    Restores default PSKEY settings.
	This function restores the following:
		1. PSKEY_VOLUME_ORIENTATION
		2. Enable TTS with default language
		3. Clears the paired device list
		4. Enables the LEDs
        5. Disable multipoint
        6. Disable lbipm
        7. Reset EQ
 
RETURNS
  	void
*/ 

void configManagerRestoreDefaults( void ) 
{
    CONF_DEBUG(("CO: Restore Defaults\n")) ;
    /*Set local values*/
	theSink.gVolButtonsInverted = FALSE ;
    
    theSink.tts_language = 0 ;
    theSink.tts_enabled = TRUE;
    configManagerEnableMultipoint(FALSE);
    theSink.iir_enabled = FALSE  ;
    theSink.lbipmEnable = FALSE ;
    theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = A2DP_MUSIC_PROCESSING_FULL;
    
	/*Reset PSKEYS*/
	(void)PsStore ( PSKEY_VOLUME_ORIENTATION , 0 , 0 ) ;
    
	/*Call function to reset the PDL*/
	deviceManagerRemoveAllDevices();
	
	if	(!(theSink.theLEDTask->gLEDSEnabled))
	{
		/*Enable the LEDs*/
		MessageSend (&theSink.task , EventEnableLEDS , 0) ;
	}
}


/****************************************************************************
NAME 
 	configManagerReadDspData
*/ 
static void configManagerReadDspData( void )
{
    /* Initialise DSP persistent store block */
    theSink.conf2->dsp_data.key.len   = PSKEY_DSP_SESSION_SIZE;
    theSink.conf2->dsp_data.key.key   = PSKEY_DSP_SESSION_KEY;
    theSink.conf2->dsp_data.key.cache = theSink.conf2->dsp_data.cache;
    PblockInit(&theSink.conf2->dsp_data.key);
}


/****************************************************************************
NAME 
 	configManagerWriteDspData
*/ 
void configManagerWriteDspData( void )
{
    /* Write DSP persistent store block */
    PblockStore();
}


/****************************************************************************
NAME 
  	configManagerFillPs

DESCRIPTION
  	Fill PS to the point defrag is required (for testing only)

RETURNS
  	void
*/
void configManagerFillPs(void)
{
    CONF_DEBUG(("CONF: Fill PS ")) ;
    if(theSink.rundata->defrag.key_size)
    {
        uint16 count     = PsFreeCount(theSink.rundata->defrag.key_size);
        uint16* buff     = mallocPanic(theSink.rundata->defrag.key_size);
        CONF_DEBUG(("[%d] ", count));
        if(count > theSink.rundata->defrag.key_minimum)
        {
            for(count = count - theSink.rundata->defrag.key_minimum ; count > 0; count --)
            {
                *buff = count;
                PsStore(PSKEY_CONFIGURATION_ID, buff, theSink.rundata->defrag.key_size);
            }
            PsStore(PSKEY_CONFIGURATION_ID, &theSink.config_id, sizeof(uint16));
        }
        CONF_DEBUG(("[%d]", PsFreeCount(theSink.rundata->defrag.key_size)));
    }
    CONF_DEBUG(("\n"));
}


/****************************************************************************
NAME 
  	configManagerDefragCheck

DESCRIPTION
  	Defrag PS if required.

RETURNS
  	void
*/
void configManagerDefragCheck(void)
{
    CONF_DEBUG(("CONF: Defrag Check ")) ;
    if(theSink.rundata->defrag.key_size)
    {
        uint16 count = PsFreeCount(theSink.rundata->defrag.key_size);
        CONF_DEBUG(("Enabled [%d] ", count)) ;
        if(count <= theSink.rundata->defrag.key_minimum)
        {
            CONF_DEBUG(("Flooding PS")) ;
            PsFlood();
            /* try to set the same boot mode; this triggers the target to reboot.*/
            BootSetMode(BootGetMode());
        }
    }
    CONF_DEBUG(("\n")) ;
}


/****************************************************************************
NAME 
  	configManagerInitMemory

DESCRIPTION
  	Init static size memory blocks that are required early in the boot sequence

RETURNS
  	void
    
*/

void configManagerInitMemory( void )  
{ 
#ifdef DEBUG_CONFIG
    uint8 slot_count = 1;
#endif
	/* Allocate memory for run time data*/
    theSink.rundata = mallocPanic( sizeof(runtime_block1_t) );
    memset(theSink.rundata, 0, sizeof (runtime_block1_t));
	CONF_DEBUG(("INIT: Malloc size %d: [%d]\n",slot_count++,sizeof(runtime_block1_t)));    
   
}
                
#if defined (ENABLE_REMOTE) && defined (ENABLE_SOUNDBAR)
/****************************************************************************
NAME 
 	configManagerHidkeyMap

DESCRIPTION
 	Read the HID configuration
 
RETURNS
 	void
*/ 
static void configManagerHidkeyMap( void )
{
    /* Read in the HID monitoring configuration */
    ConfigRetrieve(theSink.config_id, PSKEY_HID_REMOTE_CONTROL_KEY_MAPS, (void*)&theSink.rundata->hidConfig, sizeof(hid_config_t));
    rcStoreHidHandles(theSink.rundata->hidConfig.hidRepHandle);
}
#endif /* ENABLE_REMOTE && ENABLE_SOUNDBAR */                

/****************************************************************************
NAME 
  	configManagerInit

DESCRIPTION
  	The Configuration Manager is responsible for reading the user configuration
  	from the persistent store are setting up the system.  Each system component
  	is initialised in order.  Where appropriate, each configuration parameter
  	is limit checked and a default assigned if found to be out of range.

RETURNS
  	void
    
*/

void configManagerInit( void )  
{ 
    
	/* use a memory allocation for the lengths data to reduce stack usage */
    lengths_config_type * keyLengths = mallocPanic(sizeof(lengths_config_type));
	
		/* Read key lengths */
    configManagerKeyLengths(keyLengths);			

        /* Allocate the memory required for the configuration data */
    InitConfigMemory(keyLengths);
    
  	    /* Read and configure the button translations */
  	configManagerButtonTranslations( );

        /* Read and configure the button durations */
  	configManagerButtonDurations( );
	
  	    /* Read the system event configuration and configure the buttons */
    configManagerButtons( );
    
    /*configures the pattern button events*/
    configManagerButtonPatterns( ) ;

        /*Read and configure the event tones*/
    configManagerEventTones( keyLengths->no_tones ) ;
        /* Read and configure the system features */
  	configManagerFeatureBlock( );	
                                                        
    /* Read and configure the automatic switch off time*/
    configManagerConfiguration( );

    /* Must happen between features and session data... */
    InitA2dp();  
 
    /* Read and configure the user defined tones */    
  	configManagerUserDefinedTones( keyLengths->userTonesLength );	

  	    /* Read and configure the LEDs */
    configManagerLEDS();
   
        /* Read and configure the voume settings */
  	configManagerVolume( );
 
  	    /* Read and configure the power management system */
  	configManagerPower( );
 
  	    /* Read and configure the radio parameters */
  	configManagerRadio();
   
        /* Read and configure the volume orientation, LED Disable state, and tts_language */
	configManagerReadSessionData ( ) ; 

    configManagerReadDspData();
	
        /* Read and configure the sniff sub-rate parameters */
	configManagerSetupSsr ( ) ; 
	
    configManagerEventTTSPhrases( keyLengths->no_tts ) ;

    configManagerVoicePromptsInit( keyLengths->no_vp , keyLengths->no_tts_languages );
 
#if defined (ENABLE_REMOTE) && defined (ENABLE_SOUNDBAR)
    /* Read the hid remote control key mapping. */
    configManagerHidkeyMap();
 #endif

    /* don't allocate memory for AT commands if they're not required */
    if (keyLengths->size_at_commands)
    {        
        InitConfigMemoryAtCommands(keyLengths->size_at_commands);
        configManagerAtCommands(keyLengths->size_at_commands + sizeof(config_block3_t) - 1);
    }
    
#ifdef ENABLE_FM
    /* read the fm configuration data */
    configManagerReadFmData();
    
#endif
    /* release the memory used for the lengths key */
    freePanic(keyLengths);
}


