/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_configmanager.h
    
DESCRIPTION
    Configuration manager for the sink device - resoponsible for extracting user information out of the 
    PSKEYs and initialising the configurable nature of the devices' components
    
*/
#ifndef SINK_CONFIG_MANAGER_H
#define SINK_CONFIG_MANAGER_H

#include <stdlib.h>
#include <pblock.h>
#include <audio.h>
#include <hfp.h>

/* DSP keys */
#define PSKEY_DSP_BASE  (50)
#define PSKEY_DSP(x)    (PSKEY_DSP_BASE + x)
#define PSKEY_DSP_SESSION_KEY     (PSKEY_DSP(49))
#define PSKEY_DSP_SESSION_SIZE    (21)

/* Persistent store key allocation - it is of vital importantance that this enum is in numerical
   order with no gaps as it is used as an index into the default configuration structure */
#define PSKEY_BASE  (0)

/***********************************************************************/
/***********************************************************************/
/* ***** do not alter order or insert gaps as device will panic ***** */
/***********************************************************************/
/***********************************************************************/
enum
{
 	PSKEY_BATTERY_CONFIG     				= PSKEY_BASE,
 	PSKEY_BUTTON_CONFIG     				= 1,
 	PSKEY_BUTTON_PATTERN_CONFIG     		= 2,
 	PSKEY_AT_COMMANDS            			= 3,
 	PSKEY_PIO_BLOCK                			= 4,
 	PSKEY_ADDITIONAL_HFP_SUPPORTED_FEATURES	= 5,
 	PSKEY_TIMEOUTS                  		= 6,
 	PSKEY_TRI_COL_LEDS        				= 7,
 	PSKEY_DEVICE_ID         				= 8,
	PSKEY_LENGTHS                    		= 9,
 	PSKEY_BUTTON_TRANSLATION   				= 10,
 	PSKEY_TTS                				= 11, 
	PSKEY_VOLUME_ORIENTATION    			= 12,
 	PSKEY_RADIO_CONFIG      				= 13,
 	PSKEY_SSR_PARAMS              			= 14,
 	PSKEY_FEATURE_BLOCK     				= 15,
 	PSKEY_SPEAKER_GAIN_MAPPING				= 16,
 	PSKEY_HFP_INIT		     				= 17,
 	PSKEY_LED_FILTERS      					= 18,
 	PSKEY_CONFIG_TONES	     				= 19,    /* used to configure the user defined tones */
 	PSKEY_LED_STATES      					= 20,
 	PSKEY_VOICE_PROMPTS	     				= 21,
	PSKEY_LED_EVENTS      					= 22,
 	PSKEY_EVENTS_A      					= 23,
 	PSKEY_EVENTS_B    						= 24,
 	PSKEY_EVENTS_C               			= 25,
	PSKEY_TONES                     		= 26,
    PSKEY_RSSI_PAIRING             			= 27,
    PSKEY_USB_CONFIG   						= 28,
    PSKEY_FM_CONFIG                			= 29,
 	PSKEY_PHONE_NUMBER						= 30,
	PSKEY_CONFIGURATION_ID					= 31,
    PSKEY_FM_FREQUENCY_STORE                = 32,	
	PSKEY_HID_REMOTE_CONTROL_KEY_MAPS		= 33,
    PSKEY_SILENCE_DETECTION_SETTINGS        = 34,
    PSKEY_SW_BDADDR                         = 35
};

    /*this key is not contiguous with the list above */
#define PSKEY_PAIRED_DEVICE_LIST_SIZE (40)

#define PSKEY_SQIF_PARTITIONS         (47)

/* Index to PSKEY PSKEY_LENGTHS */
enum
{
 	LENGTHS_NO_TTS     				= 0,
 	LENGTHS_NO_LED_FILTERS,
 	LENGTHS_NO_LED_STATES,
 	LENGTHS_NO_LED_EVENTS,
 	LENGTHS_NO_TONES,
    LENGTHS_NO_VP_SAMPLES
};

/* Persistant store vol orientation definition */
typedef struct
{
    unsigned    vol_orientation:1 ;   
    unsigned    led_disable:1 ;
	unsigned    tts_language:4;
    unsigned    multipoint_enable:1; 
    unsigned    iir_enable:1;
    unsigned    lbipm_enable:1;
    unsigned    eq:2 ; 
    unsigned    tts_enable:1;
    unsigned    ssr_enabled:1;    
    unsigned    unused:3 ;
    unsigned    audio_enhancements:16;
    unsigned    fm_frequency:16;
}session_data_type;

typedef struct
{
    pblock_key  key;
    uint16      cache[PSKEY_DSP_SESSION_SIZE];
} dsp_data_type;

#define MAX_EVENTS ( EVENTS_MAX_EVENTS )


/* Persistent store LED configuration definition */
typedef struct
{
 	unsigned 	state:8;
 	unsigned 	on_time:8;
 	unsigned 	off_time:8;
 	unsigned  	repeat_time:8;
 	unsigned  	dim_time:8;
 	unsigned  	timeout:8;
 	unsigned 	number_flashes:4;
 	unsigned 	led_a:4;
 	unsigned 	led_b:4;
    unsigned    overide_disable:1;
 	unsigned 	colour:3;
}led_config_type;

typedef struct
{
 	unsigned 	event:8;
 	unsigned 	speed:8;
     
    unsigned 	active:1;
    unsigned    dummy:1 ;
 	unsigned 	speed_action:2;
 	unsigned  	colour:4;
    unsigned    filter_to_cancel:4 ;
    unsigned    overide_led:4 ;   
    
    unsigned    overide_led_active:1 ;
    unsigned    dummy2:2;
    unsigned    follower_led_active:1 ;
    unsigned    follower_led_delay_50ms:4;
    unsigned    overide_disable:1 ;
    unsigned    dummy3:7 ;
    
    
}led_filter_config_type;

#define MAX_STATES (SINK_NUM_STATES)
#define MAX_LED_EVENTS (20)
#define MAX_LED_STATES (SINK_NUM_STATES)
#define MAX_LED_FILTERS (LM_NUM_FILTER_EVENTS)

/* LED patterns */
typedef enum
{
 	led_state_pattern,
 	led_event_pattern
}configType;

typedef struct 
{
	unsigned event:8 ;
	unsigned tone:8 ;
}tone_config_type ;

typedef struct
{
	unsigned event:8 ;
	unsigned tts_id:8 ;
    unsigned cancel_queue_play_immediate:1 ;
    unsigned sco_block:1;
    unsigned state_mask:14 ;
}tts_config_type;

#define MAX_VP_SAMPLES (15)

/* Pull in voice_prompts_index type from audio_plugin_if.h */
typedef voice_prompts_index vp_config_type;

typedef struct
{
    uint16 event;
    uint32 pattern[6];
}button_pattern_config_type ;

typedef struct
{
    unsigned key_size:8;
    unsigned key_minimum:8;
} defrag_config;

typedef struct
{
    uint16          no_tts;
	uint16          no_tts_languages;
	uint16          no_led_filter;
	uint16          no_led_states;
	uint16          no_led_events;
	uint16          no_tones;
    uint16          no_vp;
    uint16          userTonesLength;
    uint16          size_at_commands;
    defrag_config   defrag;
}lengths_config_type ;

#define SINK_DEVICE_ID_STRICT_SIZE     (sizeof(uint16) * 4)
#define SINK_DEVICE_ID_SW_VERSION_SIZE 4
typedef struct
{
    uint16 vendor_id_source;
    uint16 vendor_id;
    uint16 product_id;
    uint16 bcd_version;
    uint16 sw_version[SINK_DEVICE_ID_SW_VERSION_SIZE];   /* Original software version number, which is not part of the Device ID spec */
} sink_device_id;

typedef struct
{
    uint16 threshold ;   /* threshold (16 bit fractional value - aligned to MSB in DSP) */
    uint16 trigger_time; /* trigger time in seconds (16 bit int) */
} silence_detect_settings;

/* Pairing timeout action */
enum
{
 	PAIRTIMEOUT_CONNECTABLE   		= 0,
 	PAIRTIMEOUT_POWER_OFF     		= 1,
 	PAIRTIMEOUT_POWER_OFF_IF_NO_PDL = 2
};

/* PSKEY_USR33 (LE HID Remote Control configuration) (7 words)*/
typedef struct
{
    unsigned hidkeyVolumeUp:8;
    unsigned hidkeyVolumeDown:8;
    unsigned hidkeyStandbyResume : 8;
    unsigned hidkeySourceInput : 8;
    uint16   hidRepHandle;
    unsigned hidAvrcpPlayPause:8;
    unsigned hidAvrcpStop:8;
    unsigned hidAvrcpSkipFrd:8;
    unsigned hidAvrcpSkipBackwrd:8;
    unsigned hidAvrcpNextGrp:8;
    unsigned hidAvrcpPreviousGrp:8;
    unsigned hidAvrcpRewindPressRelease:8;
    unsigned hidAvrcpFFPressRelease:8;
} hid_config_t;


/****************************************************************************
NAME 
 	configManagerInit

DESCRIPTION
 	Initialises all subcomponents in order

RETURNS
 	void
    
*/
void configManagerInit( void );  

/****************************************************************************
NAME 
  	configManagerInitMemory

DESCRIPTION
  	Init static size memory blocks that are required early in the boot sequence

RETURNS
  	void
    
*/

void configManagerInitMemory( void ) ;

/****************************************************************************
NAME 
 	configManagerPioMap

DESCRIPTION
    Read PIO config and map in configured PIOs.
 
RETURNS
 	void
*/ 
void configManagerPioMap(void);

/****************************************************************************
NAME 
 	configManagerUsb

DESCRIPTION
    Retrieve USB config from PS
 
RETURNS
 	void
*/ 
#ifdef ENABLE_USB
void configManagerUsb(void);
#else
#define configManagerUsb() ((void)(0))
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
void configManagerReadFmData(void);
#else
#define configManagerReadFmData() ((void)(0))   
#endif    
        
/****************************************************************************
NAME 
 	configManagerHFP_SupportedFeatures

DESCRIPTION
    Gets the 1.5 Supported features set from PS - exposed as this needs to be done prior to a HFPInit() 
 
RETURNS
 	void
*/ 
void configManagerHFP_SupportedFeatures( void ) ;

/****************************************************************************
NAME 
 	configManagerHFP_Init

DESCRIPTION
    Gets the HFP initialisation parameters from PS
 
RETURNS
 	void
*/
void configManagerHFP_Init( hfp_init_params * hfp_params );

/****************************************************************************
NAME 
 	configManagerEnableMultipoint

DESCRIPTION
    Enable or disable multipoint
 
RETURNS
 	void
*/ 
void configManagerEnableMultipoint(bool enable);

/****************************************************************************
NAME 
 	configManagerWriteSessionData

DESCRIPTION
    Stores the persistent session data across power cycles.
	This includes information like volume button orientation 
	TTS language etc.
 
RETURNS
 	void
*/ 
void configManagerWriteSessionData( void ) ;

/****************************************************************************
NAME 
 	configManagerWriteDspData

*/ 
void configManagerWriteDspData( void );

/****************************************************************************
NAME 
  	configManagerFillPs

DESCRIPTION
  	Fill PS to the point defrag is required (for testing only)

RETURNS
  	void
*/
void configManagerFillPs(void);

/****************************************************************************
NAME 
  	configManagerDefragCheck

DESCRIPTION
  	Defrag PS if required.

RETURNS
  	void
    
*/
void configManagerDefragCheck(void);

/****************************************************************************
NAME 
  	configManagerRestoreDefaults

DESCRIPTION
    Restores default PSKEY settings.
	This function restores the following:
		1. PSKEY_VOLUME_ORIENTATION
		2. PSKEY_PHONE_NUMBER
		3. Clears the paired device list
		4. Enables the LEDs
 
RETURNS
  	void
*/ 

void configManagerRestoreDefaults( void ) ;

/****************************************************************************
NAME 
  	configManagerSqifPartitionsInit

DESCRIPTION
    Reads the PSKEY containing the bitmask of partitions with voice prompts
    currently in use and does associated initialisation.
 
RETURNS
  	void
*/ 
#ifdef ENABLE_SQIFVP
void configManagerSqifPartitionsInit( void );
#endif

#endif   
