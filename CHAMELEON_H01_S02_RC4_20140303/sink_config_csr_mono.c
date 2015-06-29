/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2012

FILE NAME
    sink_config_csr_stereo.c
    
DESCRIPTION
    This file contains the default configuration parameters for a CSR Mono Sink
*/



#ifndef BC4_HS_CONFIGURATOR
    #include "sink_config.h"
#else
    #include "Configurator_config_types.h"
#endif


#define NO_TTS_EVENTS       (0x0000)
#define NO_TTS_LANGUAGES    (0x0001)
#define NO_LED_FILTERS      (0x0009)
#define NO_LED_STATES       (0x000B)
#define NO_LED_EVENTS       (0x0005)
#define NO_TONE_EVENTS      (0x0025)
#define NO_VP_SAMPLES       (0x0000)
#define USER_TONE_LENGTH    (0x0000)
#define SIZE_AT_COMMANDS    (0x0000)
#define PS_DEFRAG           (0x0f0f)


/* PSKEY_USR_0 - Power config */
static const uint16 battery_config[sizeof(sink_power_config)+1] =
{
    sizeof(sink_power_config),   
    0x0414, 0x0014, /* vref = VREF - 20sec - 20sec */
    0x0514, 0x0014, /* vbat = VBAT - 20sec - 20sec */
    /*2.9v    3.0v    3.1v    3.3v    3.5v     End*/
    0x0191, 0x0596, 0x009B, 0x00A5, 0x00AF, 0x00FF,    
    0x0014, 0x0014, /* vthm = AIO0 - 20sec  - 20sec */
    0x0063,
    /*448mV 1317mV  end
    >50'C   >5'C    <=0'C*/
    0x013D, 0x02D9, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0614, 0x0014, /* vchg = VBYP - 20sec - 20sec */
    /*4.25v*/
    0x109A,
    /* Battery Events */
    0x07AD, /*EventCriticalBattery*/
    0x071A, /*EventLowBattery*/
    0x0341, /*EventGasGauge0*/
    0x0342, /*EventGasGauge1*/
    0x0343, /*EventGasGauge2*/
    0x0344, /*EventGasGauge3*/
    /* Charger Settings */
    0x0000, 0x0000, /* Charger off - Ichg 0mA   Vterm 4.2v */
    0x8096, 0x0000, /* Charger on  - Ichg 150mA Vterm 4.2v */
    0x0000, 0x0000, /* Charger off - Ichg 0mA   Vterm 4.2v */
    0x0000, 0x0000, /* Unused */
    0x0000, 0x0000, /* Unused */
    0x0000, 0x0000, /* Unused */
    0x0000, 0x0000, /* Unused */
    0x0000, 0x0000  /* Unused */
};

/* PSKEY_USR_1 - Button config */
static const uint16 button_config[sizeof(button_config_type)+1] =
{
  	sizeof(button_config_type),
   	0x01F4, /* Button double press time */
   	0x03E8, /* Button long press time */
   	0x09C4, /* Button very long press time */
   	0x0320, /* Button repeat time */
   	0x1388, /* Button Very Very Long Duration*/
    0x0000  /* default debounce settings*/
};


/* PSKEY_USR_2 - buttonpatterns */
#define NUM_BUTTON_PATTERNS 4


static const uint16 button_pattern_config[(sizeof(button_pattern_config_type ) * NUM_BUTTON_PATTERNS) +1] = 
{
    sizeof(button_pattern_config_type ) * NUM_BUTTON_PATTERNS ,
    0   ,0,0,0,0,0,0 ,
    0   ,0,0,0,0,0,0 ,
    0   ,0,0,0,0,0,0 ,
    0   ,0,0,0,0,0,0    
};


/* PSKEY_USR_3 - AT Commands */
static const uint16 at_commands[sizeof(config_uint16_type) * (SIZE_AT_COMMANDS + MAX_AT_COMMANDS_TO_SEND)] =
{
	SIZE_AT_COMMANDS + MAX_AT_COMMANDS_TO_SEND , 
    0x0000
};


/* PSKEY_USR_4 - Input/output PIO Block */
static const uint16 pio_block[sizeof(config_pio_block_type)] =
{
  	sizeof(pio_config_type),
  	0xFFFF, 0xFFFF,                   /* inputs */
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   /* outputs */
    0x0000, 0x0000,                   /* pio invert mask */
    0x6085, 0x6085,                   /* Analogue mics, drive mic bias 0, gain 5 */
    0x0000, 0x0000,                   /* Wired mode unused */
    0x0000, 0x0000                    /* No mapping */
};

/* PSKEY_USR_5 - hfp supported features */
static const uint16 HFP_features_config[8] =
{
  	6,
    (
    HFP_ADDITIONAL_AUDIO_PARAMS_ENABLED |
    sync_hv1  |
    sync_hv2  | 
    sync_hv3  | 
    sync_ev3  |    
    sync_ev4  |  
    sync_ev5  |
  /*sync_2ev3 |*/
    sync_3ev3 |
  /*sync_2ev5 |*/
    sync_3ev5
    ),
    0x0000 , 0x1f40, 	/*8000hz bandwidth*/
    0x000c,				/*latency*/
    0x0000,				/*voice quality*/
    0x0002				/*retx effort*/
};

/* PSKEY_USR_6 - timeouts */
static const uint16 timeouts[sizeof(config_timeouts)] =
{
  	17,
	0x012C ,	/*AutoSwitchOffTime_s*/
  	0x001E ,	/*AutocPowerOnTimeout_s*/ 
  	0x000A ,	/*NetworkServiceIndicatorRepeatTime_s*/  		
  	0x0003 ,	/*DisablePowerOffAcfterPowerOnTime_s*/ 
  	0x0258 ,	/*PairModeTimeout_s*/ 
  	0x000A ,    /*MuteRemindTime_s*/
    0x003C ,    /*Connectable Timeout*/
	0x0000 ,    /*PairModeTimeout if PDL empty*/
    0x0000 ,    /*ScrollPDLforReconnectionAttempts*/
	0x000F ,	/*EncryptionRefreshTimeout_m*/
	0x0078 , 	/*InquiryTimeout_s*/
	0x0064 ,  	/*SecondAGConnectDelayTime_s*/
	0x0005 ,    /*missed call indicator period in second*/
    0x0005 ,    /*missed call indicator repeat times*/
    0x003C ,    /*a2dp link loss reconnection period in seconds*/
    0x0003 ,    /*TTS language selection auto confirm timer*/
    0x1b58      /*SpeechRec voice prompt restart interval in mS */
};

/* PSKEY_USR_7 - tri col leds */
static const uint16 tri_col_leds[sizeof(config_uint16_type)] =
{
  	1,
    0xFEE0
};


/* PSKEY_USR_8 - Device Id information*/

static const uint16 deviceId_Info[sizeof(config_uint16_type)] =
{
  	1,
  	0
};


/* PSKEY_USR_9 - Number of tts events, 
                 tts supported language
				 Number of led filters
				 Number of led states
				 Number of tone events
                 Number of voice prompt samples
*/
static const uint16 lengths[sizeof(config_lengths_type)] =
{
  	sizeof(lengths_config_type) * 1,
	NO_TTS_EVENTS,      /* number of tts_event */
 	NO_TTS_LANGUAGES,   /* the supported tts language */
	NO_LED_FILTERS,	    /* number of led filters */
	NO_LED_STATES,	    /* number of led states */
	NO_LED_EVENTS,      /* Number of led events */
	NO_TONE_EVENTS,     /* number of tone events */
    NO_VP_SAMPLES,      /* number of voice prompts */
    USER_TONE_LENGTH,   /* length of usr 19 - user tone data */
    SIZE_AT_COMMANDS,   /* size of the AT commands config */
    PS_DEFRAG           /* defrag config */
};

/* PSKEY_USR_10 - BUTTON Translation */
/* uint8 Input Number 
   uint8 Input Source:2
         Input Number:6

   where Input Source = 0 = PIO
                        1 = Cap
                        2 = Reserved
                        3 = Reserved
         
         Input Number = 0 to 31
*/
static const uint16 ButtonTranslation[(sizeof(button_translation_type) * BM_NUM_BUTTON_TRANSLATIONS)+1] =
{
    18,
    0x0013,     /* input 0 is pio 0 */
    0x0115,     /* input 1 is pio 1 */
    0x0214,
    0x0312,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,
    0x0013,    
    0x1818,
    0x1919
};

/* PSKEY_USR_11 - Tone event configuration */
static const uint16 tts[(sizeof(tts_config_type)*NO_TTS_EVENTS)+1] =
{
  	sizeof(tts_config_type) * NO_TTS_EVENTS,
};


/* PSKEY_USR_12 (PSKEY_VOLUME_ORIENTATION)
    vol_orientation:1   0 (normal)
    led_disable:1       TRUE
    tts_language:4      0
    multipoint_enable:1 FALSE
    iir_enable:1        FALSE
    lbipm_enable:1      FALSE
    eq:2                0
    tts_enable:1        TRUE
    ssr_enable:1        TRUE
    unused:3            0
    audio_enchancments:16 0
    fm_frequency:16     0                       
*/
static const uint16 session_data[sizeof(session_data_type)+1] =
{
  	sizeof(session_data_type),
  	0x4018, 0x0000, 0x0000
};




/* PSKEY_USR_13 - Radio configuration parameters */
static const uint16 radio[sizeof(config_radio_type)] =
{
  	4,
  	0x0800, 0x0012, 0x0800, 0x0012
};

/* PSKEY_USR_14 - Subrate parameters
	Default: All Zero
*/
static const uint16 ssr_config[sizeof(subrate_t)+1] = 
{
	sizeof(subrate_t),	
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000
};

/* PSKEY_USR_15 - Features block */
static const uint16 features[sizeof(config_features_type)] =
{
  	6,
  	0x9332, 0x8C01, 0x11E7, 0x21A0, 0x3C00 , 0xA41A 
};

/* PSKEY_USR_16 - Volume gain mappings */
static const uint16 volume[33] =
{
  	32,
    0x1001, 0x0000, 
    0x2004, 0x0201, 
    0x3104, 0x0302, 
    0x4204, 0x0403, 
    0x5304, 0x0504, 
    0x6404, 0x0605, 
    0x7504, 0x0706, 
    0x8604, 0x0807, 
    0x9704, 0x0908, 
    0xa804, 0x0a09, 
    0xb904, 0x0b0a, 
    0xca04, 0x0c0b, 
    0xdb04, 0x0d0c, 
    0xec04, 0x0e0d, 
    0xfd04, 0x0f0e, 
    0xfe0a, 0x100f  
 	,
};


/* PSKEY_USR_17 - hfp initialisation parameters */
static const uint16 hfpInit[sizeof(hfp_init_params)+1] =
{
  	7,
  	hfp_handsfree_all | hfp_headset_profile,       /* supported profiles */
#ifdef INCLUDE_CVC   	
    HFP_NREC_FUNCTION | 
#endif
#if defined(TEXT_TO_SPEECH_DIGITS) || defined(TEXT_TO_SPEECH_NAMES)
    HFP_CLI_PRESENTATION | 
#endif
    HFP_CODEC_NEGOTIATION | HFP_THREE_WAY_CALLING | HFP_VOICE_RECOGNITION | 
    HFP_REMOTE_VOL_CONTROL | HFP_ENHANCED_CALL_STATUS, /* supported features */
    hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc, /* supported WBS codecs */
    0x4000,                                            /* optional indicators.service = hfp_indicator_on */
    0x0005,                                            /* Multipoint + Disable NREC */ 
    0x0A0A,                                            /* link loss timeout + link_loss_interval */ 
    0x1000                                             /* csr_features.batt_level = true */ 
};

/* PSKEY_USR_18 - LED filter configuration */
static const uint16 led_filters[(sizeof(led_filter_config_type) * NO_LED_FILTERS)+1] =
{
 	sizeof(led_filter_config_type) * NO_LED_FILTERS,
/*1*/        0x1a00, 0x8200, 0x0000, 	/*filter on with low batt*/
/*2*/        0x2200, 0x0010, 0x0000, 	/*filter off with charger connected*/
/*3*/        0x2100, 0x0010, 0x0000, 	/*filter off with EVBATok*/    
/*4*/        0x2000, 0x800E, 0x8000,    
/*5*/        0x1E00, 0x800F, 0x8000,    
/*6*/        0x1E00, 0x0040, 0x0000,    
/*7*/        0x2000, 0x005F, 0x0000,    
/*8*/        0x2300, 0x0040, 0x0000,    
/*9*/        0x2300, 0x0050, 0x0000,     
};

/* PSKEY_USR_19 - User Defined Tones configuration */
static const uint16 user_def_tones[sizeof(config_uint16_type)] =
{
  	1,
  	0
};

/* PSKEY_USR_20 - LED state configuration */
static const uint16 led_states[(sizeof(led_config_type) * NO_LED_STATES)+1] =
{
  	sizeof(led_config_type) * NO_LED_STATES,
    0x010A, 0x0A28, 0x0000, 0x2EF1,  /*connectable*/
	0x020A, 0x0A01, 0x0000, 0x2EF3,  /*conn_discoverable*/
	0x030A, 0x0A28, 0x0000, 0x3EF2,  /*connected*/
	
	0x0664, 0x6402, 0x4B00, 0x1EF2,  /*activecall SCO*/    	
	0x0864, 0x6401, 0x0000, 0x1EF1,  /*deviceThreeWayCallWaiting */
	0x0964, 0x6401, 0x0000, 0x1EF1,  /*deviceThreeWayCallOnHold*/
	0x0A64, 0x6401, 0x0000, 0x1EF1,  /*headsetThreeWayMultiCall*/
	0x0C64, 0x6401, 0x0000, 0x1EF1,  /*deviceActiveCallNoSCO*/
	
	0x0432, 0x3201, 0x0000, 0x1EF2,  /*deviceOutgoingCallEstablish*/
	0x0532, 0x3201, 0x0000, 0x1EF2,  /*deviceIncomingCallEstablish*/
    0x0D32, 0x6401, 0x0000, 0x2EF2,  /*deviceA2DPStreaming*/
   	
};

/* PSKEY_USR_21 - Voice Prompts Config */
static const uint16 vp_data[(sizeof(vp_config_type) + 1)] =
{
  	sizeof(vp_config_type),
    0001,                   /* Header in SPI flash */
    0000,0000               /* Address 000000 */
};


/* PSKEY_USR_22 - LED event configuration */
static const uint16 led_events[(sizeof(led_config_type) * NO_LED_EVENTS)+1] =
{
 	sizeof(led_config_type) * NO_LED_EVENTS,
                                        /* E - red , F - blu*/
    0x0164, 0x0000, 0x0000, 0x1EF4,     /* EventPowerOn  */
    0x0264, 0x0000, 0x0000, 0x1EF4,     /* EventPowerOff */
    0x140A, 0x0A00, 0x0000, 0x2EF4,     /* EventResetPairedDeviceList */
    0x460A, 0x0000, 0x0000, 0x0EFC,     /* EventEnterDFUMode */
    0x470C, 0x0C00, 0x0000, 0x8EFB      /* EventGaiaAlertLEDs */
};

/* PSKEY_USR_23 - System event configuration A */
static const uint16 events_a[(sizeof(event_config_type) * BM_EVENTS_PER_PS_BLOCK)+1] =
{       
    sizeof(event_config_type) * BM_EVENTS_PER_PS_BLOCK,

    0x0203, 0x0000, 0x7FFE,     /* EventPowerOff                            - B_VERY_LONG           - VREG                  */
    0x0102, 0x0000, 0x4001,     /* EventPowerOn                             - B_LONG                - VREG                  */
    0x030B, 0x0000, 0x4002,     /* EventEnterPairing                        - B_VERY_VERY_LONG      - VREG                  */
    0x0408, 0x0001, 0x200A,     /* EventInitateVoiceDial                    - B_SHORT_SINGLE        - IP0                   */

    0x0502, 0x0001, 0x0008,     /* EventLastNumberRedial                    - B_LONG                - IP0                   */
    0x4F04, 0x0001, 0x000A,     /* EventDialStoredNumber                    - B_DOUBLE              - IP0                   */
    0x0608, 0x0001, 0x0020,     /* EventAnswer                              - B_SHORT_SINGLE        - IP0                   */
    0x0702, 0x0001, 0x0020,     /* EventReject                              - B_LONG                - IP0                   */

    0x0808, 0x0001, 0x1050,     /* EventCancelEnd                           - B_SHORT_SINGLE        - IP0                   */
    0x0902, 0x0001, 0x1F40,     /* EventTransferToggle                      - B_LONG                - IP0                   */
    0x1C02, 0x0001, 0x0002,     /* EventEstablishSLC                        - B_LONG                - IP0                   */
    0x1C09, 0x0000, 0x4002,     /* EventEstablishSLC                        - B_LONG_RELEASE        - VREG                  */

    0x0F08, 0x0001, 0x0700,     /* EventThreeWayAcceptWaitingReleaseActive  - B_SHORT_SINGLE        - IP0                   */
    0x1004, 0x0001, 0x1340,     /* EventThreeWayAcceptWaitingHoldActive     - B_DOUBLE              - IP0                   */
    0x3A04, 0x0001, 0x0020,     /* EventPlaceIncomingCallOnHold             - B_DOUBLE              - IP0                   */
    0x3B08, 0x0001, 0x0800,     /* EventAcceptHeldIncomingCall              - B_SHORT_SINGLE        - IP0                   */

    0x3C02, 0x0001, 0x0800,     /* EventRejectHeldIncomingCall              - B_LONG                - IP0                   */
    0x2206, 0x0000, 0xBFFF,     /* EventChargerConnected                    - B_LOW_TO_HIGH         - CHG                   */
    0x2307, 0x0000, 0xBFFF,     /* EventChargerDisconnected                 - B_HIGH_TO_LOW         - CHG                   */
    0x1C0A, 0x0000, 0x4002,     /* EventEstablishSLC                        - B_VERY_LONG_RELEASE   - VREG                  */

    0x7202, 0x0001, 0x4006,     /* EventRssiPair                            - B_LONG                - IP0 & VREG            */   
    0x8E04, 0x0001, 0x400A      /* EventUpdateStoredNumber                  - B_DOUBLE              - IP0 & VREG            */

};


/* PSKEY_USR_24 - System event configuration B */
static const uint16 events_b[(sizeof(event_config_type) * BM_EVENTS_PER_PS_BLOCK)+1] =
{   
    sizeof(event_config_type) * BM_EVENTS_PER_PS_BLOCK,
    0x0b01, 0x0004, 0x3ffe, /* EventVolumeUp                            - B_SHORT               - IP2                 */
    0x0b05, 0x0004, 0x3ffe, /* EventVolumeUp                            - B_REPEAT              - IP2                 */
    0x0c01, 0x0002, 0x3ffe, /* EventVolumeDown                          - B_SHORT               - IP1                 */
    0x0c05, 0x0002, 0x3ffe, /* EventVolumeDown                          - B_REPEAT              - IP1                 */
                            
    0x0a01, 0x0006, 0x1f50, /* EventToggleMute                          - B_SHORT               - IP1 & IP2           */
    0x0e08, 0x0005, 0x1340, /* EventThreeWayReleaseAllHeld              - B_SHORT               - IP0 & IP2           */
    0x1101, 0x0003, 0x0300, /* EventThreeWayAddHeldTo3Way               - B_SHORT               - IP0 & IP1           */
    0x1202, 0x0005, 0x0600, /* EventThreeWayConnect2Disconnect          - B_LONG                - IP0 & IP2           */
                            
    0x0501, 0x0002, 0x4040, /* EventLastNumberRedial                    - B_SHORT               - IP0 & IP1           */
    0x0d03, 0x0006, 0x000e, /* EventToggleVolume                        - B_VERY_LONG           - IP1 & IP2           */
    0x6201, 0x0003, 0x4006, /* EventEnableMultipoint                    - B_SHORT               - IP0 & IP1 & IP2     */
    0x6303, 0x0003, 0x4006, /* EventDisableMultipoint                   - B_VERY_LONG           - IP0 & IP2 & IP1     */
                            
    0x140b, 0x0001, 0x5fff, /* EventResetPairedDeviceList               - B_VERY_VERY_LONG      - VREG & IP1          */
    0x460b, 0x0003, 0x4001, /* EventEnterDFUMode                        - B_VERY_VERY_LONG      - VREG & IP1 & IP2    */
    0x0000, 0x0000, 0x0000, /*  */
    0x0000, 0x0000, 0x0000, /*  */  
                            
    0x0000, 0x0000, 0x0000, /*  */
    0x0000, 0x0000, 0x0000, /*  */
    0x0000, 0x0000, 0x0000, /*  */
    0x0000, 0x0000, 0x0000, /*  */
                            
    0x0000, 0x0000, 0x0000, /*  */
    0x0000, 0x0000, 0x0000  /*  */

};

/* PSKEY_USR_25 - System event configuration C */
static const uint16 events_c[(sizeof(event_config_type) * BM_EVENTS_PER_PS_BLOCK)+1] =
{
    sizeof(event_config_type) * BM_EVENTS_PER_PS_BLOCK,
             
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,             
    
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,  	

    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,    
    
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,
    
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000
};


/* PSKEY_USR_26 - Tone event configuration */
static const uint16 tone_events[(sizeof(tone_config_type) * NO_TONE_EVENTS)+1] =
{
  	sizeof(tone_config_type) * NO_TONE_EVENTS,
    0xFF1D, /*RingTone1*/
    0x1702, /*EventPairingSuccessful*/
    0x010B, /*EventPowerOn*/
    0x020C, /*EventPowerOff*/
    0x1C14, /*EventEstablishSLC*/
    0x1612, /*EventPairingFail*/
    0x030D, /*EventEnterPairing*/
    0x0502, /*EventLastNumberRedial*/
    0x060D, /*EventAnswer*/
    0x070E, /*EventReject*/
    0x0802, /*EventCancelEnd*/
    0x141A, /*EventResetPairedDeviceList*/
    0x280E, /*EventMuteOn*/
    0x290D, /*EventMuteOff*/
    0x2A1A, /*EventMuteReminder*/
    0x0902, /*EventTransferToggle*/
    0x0402, /*EventInitateVoiceDial*/
    0x623A, /*EventEnableMultipoint*/
    0x633B, /*EventDisableMultipoint*/
    0x6917, /*EventMultipointCallWaiting*/
    0xFE56, /*RingTone2*/
    0x0D46, /*EventToggleVolume*/
    0x2616, /*EventLinkLoss*/
    0x0E02, /*EventThreeWayReleaseAllHeld*/
    0x0F02, /*EventThreeWayAcceptWaitingReleaseActive*/
    0x1002, /*EventThreeWayAcceptWaitingHoldActive*/
    0x1102, /*EventThreeWayAddHeldTo3Way*/
    0x1202, /*EventThreeWayConnect2Disconnect*/
    0x3A02, /*EventPlaceIncomingCallOnHold*/
    0x3B0D, /*EventAcceptHeldIncomingCall*/
    0x3C0E, /*EventRejectHeldIncomingCall*/
    0x1A18, /*EventLowBattery*/
    0x7214, /*EventRssiPair*/
    0x7312, /*EventRssiPairReminder*/
    0x7412, /*EventRssiPairTimeout*/
    0xBA13, /*EventPrimaryDeviceConnected*/
    0xBB10  /*EventSecondaryDeviceConnected*/
};


/* PSKEY_USR_27 - RSSI Pairing configuration */
static const uint16 rssi[sizeof(rssi_pairing_t)+1] =
{
    sizeof(rssi_pairing_t),
    0xFFBA,        /* Inquiry Tx Power               */
    0xFFDD,        /* RSSI Threshold                 */
    0x0005,        /* RSSI difference threshold	     */	
    0x0000, 0x0000,/* Class of device filter         */	
    0xFFDD,        /* Conn RSSI Threshold            */
    0x0005,        /* Conn RSSI difference threshold */	
    0x100A,        /* Max 16 results, Max inquiry time (10 * 1.28)s */
    0x0020         /* No resume delay, 2 results, no additional features */
};


/* PSKEY_USR_28 - USB config */
static const uint16 usb_conf[sizeof(usb_config)+1] =
{
  	sizeof(usb_config),
    /* Disable USB by default */
  	0x0000, 0x8096, 0x1400, 0x0400, 0x905A, 0x805A, 0x905A, 0x805A, 0x8096, 0x8096, 0x8096, 0x0100
};

/* PSKEY_USR_29 - FM receiver configuration */
static const uint16 fm_conf[sizeof(fm_rx_config)+1] =
{
  	sizeof(fm_rx_config),
  	0x226a, /* seek_band_bottom */
    0x2a26, /* seek_band_top */
    0x000a, /* seek_freq_spacing */
    0x0014, /* seek_tune_rssi */
    0x0003, /* seek_tune_snr */
    0x0000, /* rsq_rssi_low */
    0x007f, /* rsq_rssi_high */
    0x0000, /* rsq_snr_low */
    0x007f, /* rsq_snr_high */
    0x0001, /* antenna_source */
    0x0003  /* hardware_pio for reset line */
};

/* PSKEY_USR_32 - FM freq store */
static const uint16 fm_freq_store[sizeof(fm_stored_freq)+1] =
{
   sizeof(fm_stored_freq),
   0x0000, /* freq1*/
   0x0000, /* freq2 */
   0x0000  /* freq3 */
};

/* PSKEY_USR_33 - Soundbar HID remote configuration */
static const uint16 soundbar_hid_remote_conf[sizeof(hid_config_t)+1] =
{
    sizeof(hid_config_t),
    0xe9ea,     /* hidkeyVolumeUp:hidkeyVolumeDown */
    0x3058,     /* hidkeyStandbyResume:hidkeySourceInput */
    0x0000,     /* hidRepHandle */
    0x2A3A,     /* hidAvrcpPlayPause:hidAvrcpStop */
    0x4f50,     /* hidAvrcpSkipFrd:hidAvrcpSkipBackwrd */
    0x413E,     /* hidAvrcpNextGrp:hidAvrcpPreviousGrp */
    0x3F40      /* hidAvrcpRewindPressRelease:hidAvrcpFFPressRelease */
};            
 
/* padding entries to maintain pskey key id numbers now that we've gone past the config_id number */
static const uint16 dummy1[1] =
{
    0
};

static const uint16 dummy2[1] =
{
    0
};


/* Default Configuration */
const config_type csr_mono_default_config = 
{
 	(key_type *)&battery_config,        	/* PSKEY_USR_0 - Battery configuration */
  	(key_type *)&button_config,       		/* PSKEY_USR_1 - Button configuration */
  	(key_type *)&button_pattern_config,		/* PSKEY_USR_2 - */
  	(key_type *)&at_commands,               /* PSKEY_USR_3 - AT Commands*/
  	(key_type *)&pio_block,        	        /* PSKEY_USR_4 - Input/output pio block*/
  	(key_type *)&HFP_features_config,		/* PSKEY_USR_5 - The HFP Supported features*/
  	(key_type *)&timeouts, 		        	/* PSKEY_USR_6 - Timeouts*/
  	(key_type *)&tri_col_leds,             	/* PSKEY_USR_7 - Tri Colour LEDS */
  	(key_type *)&deviceId_Info,             /* PSKEY_USR_8 - Device Id information*/
  	(key_type *)&lengths,                   /* PSKEY_USR_9 - length of tts, led filter, led states, led events, tones*/
  	(key_type *)&ButtonTranslation,         /* PSKEY_USR_10 - button translation */
  	(key_type *)&tts ,                  	/* PSKEY_USR_11 - unused4 */
  	(key_type *)&session_data,              /* PSKEY_USR_12 - session_data*/
  	(key_type *)&radio,            			/* PSKEY_USR_13 - Radio configuration parameters */
	(key_type *)&ssr_config,				/* PSKEY_USR_14 - SSR parameters */
  	(key_type *)&features,          		/* PSKEY_USR_15 - Features block */
  	(key_type *)&volume,           			/* PSKEY_USR_16 - Volume gain mappings */
  	(key_type *)&hfpInit,       			/* PSKEY_USR_17 - HFP initialisation parameters */
  	(key_type *)&led_filters,         		/* PSKEY_USR_18 - LED filter configuration */
  	(key_type *)&user_def_tones,        	/* PSKEY_USR_19 - User Defined Tones configuration */
  	(key_type *)&led_states,         		/* PSKEY_USR_20 - LED state configuration */
  	(key_type *)&vp_data,        			/* PSKEY_USR_21 - Voice Prompts Data */
  	(key_type *)&led_events,         		/* PSKEY_USR_22 - LED event configuration */
  	(key_type *)&events_a,          		/* PSKEY_USR_23 - System event configuration */
  	(key_type *)&events_b,           		/* PSKEY_USR_24 - System event configuration */
  	(key_type *)&events_c,       			/* PSKEY_USR_25 - System event configuration */
  	(key_type *)&tone_events,         		/* PSKEY_USR_26 - Tone event configuration */
   	(key_type *)&rssi,       				/* PSKEY_USR_27 - RSSI configuration */  
 	(key_type *)&usb_conf,               	/* PSKEY_USR_28 - USB */
	(key_type *)&fm_conf,              		/* PSKEY_USR_29 - FM configuration */
    (key_type *)&dummy1,                    /* PSKEY_USR_30 - one touch dial setting */    
    (key_type *)&dummy2,                    /* PSKEY_USR_31 - config id - do not move */    
    (key_type *)&fm_freq_store,             /* PSKEY_USR_32 - FM frequency storage */    
	(key_type *)&soundbar_hid_remote_conf   /* PSKEY_USR_33 - Soundbar HID remote configuration */   
};

