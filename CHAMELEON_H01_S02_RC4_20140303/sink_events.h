/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_events.h
    
DESCRIPTION
    Defines sink application user events
    
*/
#ifndef SINK_EVENTS_H
#define SINK_EVENTS_H


#ifndef BC4_HS_CONFIGURATOR
    #include <connection.h>
    #include <message.h>
    #include <app/message/system_message.h>
    #include <stdio.h>
#endif


#define EVENTS_MESSAGE_BASE (0x6000)

/*This enum is used as an index in an array - do not edit - without thinking*/
typedef enum sinkEventsTag
{
/*0x00*/    EventInvalid = EVENTS_MESSAGE_BASE,    
/*0x01*/    EventPowerOn , 
/*0x02*/    EventPowerOff ,  
/*0x03*/    EventEnterPairing ,
    
/*0x04*/    EventInitateVoiceDial ,
/*0x05*/    EventLastNumberRedial ,
/*0x06*/    EventAnswer , 
/*0x07*/    EventReject , 
    
/*0x08*/    EventCancelEnd , 
/*0x09*/    EventTransferToggle ,
/*0x0A*/    EventToggleMute   ,
/*0x0B*/    EventVolumeUp  ,
    
/*0x0C*/    EventVolumeDown ,
/*0x0D*/    EventToggleVolume,
/*0x0E*/    EventThreeWayReleaseAllHeld,
/*0x0F*/    EventThreeWayAcceptWaitingReleaseActive,
    
/*0x10*/    EventThreeWayAcceptWaitingHoldActive  ,
/*0x11*/    EventThreeWayAddHeldTo3Way  ,
/*0x12*/    EventThreeWayConnect2Disconnect,  
/*0x13*/    EventEnableDisableLeds  ,
    
/*0x14*/    EventResetPairedDeviceList,
/*0x15*/    EventEnterDutMode ,
/*0x16*/    EventPairingFail,
/*0x17*/    EventPairingSuccessful,
    
/*0x18*/    EventSCOLinkOpen ,
/*0x19*/    EventSCOLinkClose,
/*0x1A*/    EventLowBattery,
/*0x1B*/    EventEndOfCall ,
    
/*0x1C*/    EventEstablishSLC ,
/*0x1D*/    EventLEDEventComplete,
/*0x1E*/    EventChargeComplete,
/*0x1F*/    EventAutoSwitchOff,
    
/*0x20*/    EventChargeInProgress,
/*0x21*/    EventOkBattery,
/*0x22*/    EventChargerConnected,
/*0x23*/    EventChargerDisconnected,

/*0x24*/    EventSLCDisconnected ,
/*0x25*/    EventBatteryLevelRequest ,
/*0x26*/    EventLinkLoss ,
/*0x27*/    EventLimboTimeout ,

/*0x28*/    EventMuteOn ,
/*0x29*/    EventMuteOff ,
/*0x2a*/    EventMuteReminder ,
/*0x2b*/    EventResetComplete,

/*0x2C*/    EventEnterTXContTestMode,
/*0x2D*/    EventEnterDUTState,
/*0x2E*/    EventVolumeOrientationNormal,
/*0x2F*/    EventVolumeOrientationInvert,

/*0x30*/    EventNetworkOrServiceNotPresent,
/*0x31*/    EventNetworkOrServicePresent,
/*0x32*/    EventEnableLEDS,
/*0x33*/    EventDisableLEDS,

/*0x34*/    EventSLCConnected ,
/*0x35*/    EventError,
/*0x36*/    EventLongTimer,
/*0x37*/    EventVLongTimer,

/*0x38*/    EventEnablePowerOff,
/*0x39*/    EventBassBoostEnableDisableToggle,
/*0x3A*/    EventPlaceIncomingCallOnHold,
/*0x3B*/    EventAcceptHeldIncomingCall,

/*0x3C*/    EventRejectHeldIncomingCall,
/*0x3D*/	EventCancelLedIndication ,
/*0x3E*/    EventCallAnswered ,
/*0x3F*/    EventEnterPairingEmptyPDL,

/*0x40*/	EventReconnectFailed,
/*0x41*/    EventGasGauge0,
/*0x42*/    EventGasGauge1,
/*0x43*/    EventGasGauge2,

/*0x44*/    EventGasGauge3,
/*0x45*/    EventCheckForAudioTransfer,
/*0x46*/    EventEnterDFUMode,
/*0x47*/	EventGaiaAlertLEDs,
			
/*0x48*/	EventEnterServiceMode,
/*0x49*/    EventServiceModeEntered ,
/*0x4A*/	EventAudioMessage1,
/*0x4B*/	EventAudioMessage2,
			
/*0x4C*/	EventAudioMessage3,
/*0x4D*/	EventAudioMessage4,
/*0x4E*/    EventEnableVoicePrompts,
/*0x4f*/	EventDialStoredNumber,

/*0x50*/    EventDisableVoicePrompts,
/*0x51*/    EventChargeDisabled,
/*0x52*/	EventRestoreDefaults,
/*0x53*/    EventChargerGasGauge0,
			
/*0x54*/    EventChargerGasGauge1,
/*0x55*/    EventChargerGasGauge2,
/*0x56*/    EventChargerGasGauge3,
/*0x57*/    EventContinueSlcConnectRequest,
            
/*0x58*/    EventConnectableTimeout,
/*0x59*/    EventLastNumberRedial_AG2,
/*0x5A*/    EventInitateVoiceDial_AG2,
/*0x5B*/	EventConfirmationAccept,
			
/*0x5C*/	EventConfirmationReject,
/*0x5D*/	EventToggleDebugKeys,            
/*0x5E*/	EventTone1,
/*0x5F*/	EventTone2,
			
/*0x60*/	EventSelectTTSLanguageMode,
/*0x61*/	EventConfirmTTSLanguage,
/*0x62*/	EventEnableMultipoint,
/*0x63*/	EventDisableMultipoint,
            
/*0x64*/    EventStreamEstablish,
/*0x65*/    EventSLCConnectedAfterPowerOn,
/*0x66*/    EventResetLEDTimeout,
/*0x67*/    EventStartPagingInConnState,
            
/*0x68*/    EventStopPagingInConnState,
/*0x69*/    EventMultipointCallWaiting,
/*0x6A*/	EventRefreshEncryption,
/*0x6B*/    EventSwitchAudioMode,

/*0x6C*/	EventButtonLockingOn,
/*0x6D*/	EventButtonLockingOff,
/*0x6E*/	EventToggleButtonLocking,
/*0x6F*/	EventButtonBlockedByLock,

/*0x70*/	EventSpeechRecognitionTuningStart,
/*0x71*/	EventSpeechRecognitionTuningYes,	
/*0x72*/    EventRssiPair,
/*0x73*/    EventRssiPairReminder,

/*0x74*/	EventRssiPairTimeout,
/*0x75*/    EventBassBoostOn,
/*0x76*/    EventCheckRole,
/*0x77*/    EventMissedCall,

/*0x78*/	EventBassBoostOff,
/*0x79*/	EventA2dpConnected,
/*0x7a*/	EventA2dpDisconnected,
/*0x7b*/	Event3DEnhancementEnableDisableToggle,

/*0x7c*/	Event3DEnhancementOn,
/*0x7d*/	Event3DEnhancementOff,
/*0x7e*/	EventVolumeMax,
/*0x7f*/	EventVolumeMin,

/*0x80*/	EventCheckAudioRouting,
/*0x81*/    EventConfirmationRequest,
/*0x82*/    EventPasskeyDisplay,
/*0x83*/    EventPinCodeRequest,

/*0x84*/    EventSwitchToNextAudioSource,
/*0x85*/    EventSelectWiredAudioSource,             
/*0x86*/    EventPbapDialMch,
/*0x87*/    EventPbapDialIch,
            
/*0x88*/    EventEstablishPbap,
/*0x89*/    EventPbapDialFail, 
/*0x8A*/    EventSetWbsCodecs,
/*0x8B*/    EventOverrideResponse,
            
/*0x8C*/    EventCreateAudioConnection,
/*0x8D*/    EventSetWbsCodecsSendBAC,
/*0x8E*/    EventUpdateStoredNumber,            
/*0x8F*/    EventEnableIntelligentPowerManagement,
            
/*0x90*/    EventDisableIntelligentPowerManagement,
/*0x91*/    EventToggleIntelligentPowerManagement,    
/*0x92*/    EventEnterBootMode2,            
/*0x93*/    EventAvrcpPlayPause,
            
/*0x94*/    EventAvrcpStop,
/*0x95*/    EventAvrcpSkipForward,
/*0x96*/    EventAvrcpSkipBackward,
/*0x97*/    EventAvrcpFastForwardPress,
           
/*0x98*/    EventAvrcpFastForwardRelease,
/*0x99*/    EventAvrcpRewindPress,
/*0x9A*/    EventAvrcpRewindRelease,                 
/*0x9B*/    EventPbapSetPhonebook,
            
/*0x9C*/    EventPbapBrowseEntry,
/*0x9D*/    EventPbapBrowseList,
/*0x9E*/    EventPbapDownloadPhonebook,
/*0x9F*/	EventPbapSelectPhonebookObject,
            
/*0xA0*/	EventPbapBrowseComplete,
/*0xA1*/    EventSelectUSBAudioSource,
/*0xA2*/    EventSelectAG1AudioSource,           
/*0xA3*/    EventMapcMsgNotification,

/*0xA4*/    EventMapcMnsSuccess,
/*0xA5*/    EventMapcMnsFailed,            
/*0xA6*/    EventAvrcpToggleActive,
/*0xA7*/    EventAvrcpNextGroup,

/*0xA8*/    EventAvrcpPreviousGroup,
/*0xA9*/    EventUsbPlayPause,
/*0xAA*/    EventUsbStop,
/*0xAB*/    EventUsbFwd,

/*0xAC*/    EventUsbBck,
/*0xAD*/    EventCriticalBattery,
/*0xAE*/    EventRssiResume,
/*0xAF*/    EventSelectAG2AudioSource,

/*0xB0*/    EventPowerOnPanic,
/*0xB1*/    EventEstablishSLCOnPanic,
/*0xB2*/    EventTestDefrag,
/*0xB3*/    EventUsbDeadBatteryTimeout,

/*0xB4*/    EventUsbMute,
/*0xB5*/    EventUsbLowPowerMode,
/*0xB6*/    EventSpeechRecognitionStart,
/*0xB7*/    EventSpeechRecognitionStop,

/*0xB8*/    EventWiredAudioConnected,
/*0xB9*/    EventWiredAudioDisconnected,
/*0xBA*/    EventPrimaryDeviceConnected,
/*0xBB*/    EventSecondaryDeviceConnected,
            
/*0xBC*/    EventAudioTestMode,   
/*0xBD*/    EventToneTestMode,            
/*0xBE*/    EventKeyTestMode,            
/*0xBF*/    EventSpeechRecognitionTuningNo,
            
/*0xC0*/    EventGaiaUser1,
/*0xC1*/    EventGaiaUser2,
/*0xC2*/    EventGaiaUser3,
/*0xC3*/    EventGaiaUser4,

/*0xC4*/    EventGaiaUser5,
/*0xC5*/    EventGaiaUser6,
/*0xC6*/    EventGaiaUser7,
/*0xC7*/    EventGaiaUser8,

/*0xC8*/    EventUpdateAttributes,            
/*0xC9*/    EventSelectNoAudioSource,
/*0xCA*/    EventResetAvrcpMode,
/*0xCB*/    EventFmOn,

/*0xCC*/    EventFmOff,
/*0xCD*/    EventFmRxTuneUp,
/*0xCE*/    EventFmRxTuneDown,
/*0xCF*/    EventFmRxStore,

/*0xD0*/    EventFmRxTuneToStore,            
/*0xD1*/    EventSwapMediaChannel,
/*0xD2*/    EventCheckAudioAmpDrive,
/*0xD3*/    EventExternalMicConnected,
            
/*0xD4*/    EventExternalMicDisconnected,
/*0xD5*/    EventAvrcpPlay,
/*0xD6*/    EventAvrcpPause,
/*0xD7*/    EventEnableSSR,
            
/*0xD8*/    EventDisableSSR,            
/*0xD9*/    EventSbcCodec,            
/*0xDA*/    EventMp3Codec,
/*0xDB*/    EventAacCodec,
            
/*0xDC*/    EventAptxCodec,
/*0xDD*/    EventAptxLLCodec,            
/*0xDE*/    EventFaststreamCodec,
/*0xDF*/    EventNFCTagDetected,
            
/*0xE0*/    EventPbapGetPhonebookSize,
/*0xE1*/	EventFmRxErase,
/*0xE2*/    EventSubwooferStartInquiry,
/*0xE3*/    EventSubwooferCheckPairing,

/*0xE4*/    EventSubwooferOpenLLMedia,
/*0xE5*/    EventSubwooferOpenStdMedia,
/*0xE6*/    EventSubwooferCloseMedia,
/*0xE7*/    EventSubwooferStartStreaming,

/*0xE8*/    EventSubwooferSuspendStreaming,
/*0xE9*/    EventSubwooferDisconnect,
/*0xEA*/    EventSubwooferVolumeDown,
/*0xEB*/    EventSubwooferVolumeUp,

/*0xEC*/    EventRCVolumeUp,
/*0xED*/    EventRCVolumeDown,
/*0xEE*/    EventSelectFMAudioSource,
/*0xEF*/    EventPeerSessionInquire,

/*0xF0*/    EventPeerSessionConnDisc,
/*0xF1*/    EventPeerSessionEnd,
/*0xF2*/    EventSubwooferDeletePairing,
/*0xF3*/    EventAvrcpShuffleOff,

/*0xF4*/    EventAvrcpShuffleAllTrack,
/*0xF5*/    EventAvrcpShuffleGroup,
/*0xF6*/    EventAvrcpRepeatOff,
/*0xF7*/    EventAvrcpRepeatSingleTrack,

/*0xF8*/    EventAvrcpRepeatAllTrack,
/*0xF9*/    EventAvrcpRepeatGroup,
/*0xFA*/    EventEnterDriverlessDFUMode,
/*0xFB*/    EventELPatternMode,

/*0xFC*/    EventXYZSamplingMode
/*0xFD*/    /* ...3... */
/*0xFE*/    /* ...2... */
/*0xFF*/    /* ...1... */
/* Warning: This events list cannot go past 0xFF */
} sinkEvents_t; 

#define EVENTS_LAST_EVENT EventXYZSamplingMode

#define EVENTS_MAX_EVENTS ( (EVENTS_LAST_EVENT - EVENTS_MESSAGE_BASE) + 1 )

#endif
