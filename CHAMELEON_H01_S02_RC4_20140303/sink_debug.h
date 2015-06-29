/***************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_debug.h
    
DESCRIPTION
    
*/

#ifndef _SINK_DEBUG_H_
#define _SINK_DEBUG_H_


#ifndef RELEASE_BUILD /*allows the release build to ensure all of the below are removed*/
    
    /*The individual configs*/
    
 
#ifndef DO_NOT_DOCUMENT

#endif 
 /*end of DO_NOT_DOCUMENT*/

    /*The global debug enable*/ 
    #define xDEBUG_PRINT_ENABLED

    #ifdef DEBUG_PRINT_ENABLED
        #define DEBUG(x) {printf x;}

        /*The individual Debug enables*/
            /*The main system messages*/
        #define DEBUG_MAIN
            /* RSSI pairing */
        #define DEBUG_INQx
                /*The button manager */
        #define DEBUG_BUT_MANx
            /*The low level button parsing*/
        #define DEBUG_BUTTONSx
            /*The call manager*/
        #define DEBUG_CALL_MANx
            /*The multipoint manager*/
        #define DEBUG_MULTI_MANx
            /*sink_audio.c*/
        #define DEBUG_AUDIOx
         /* sink_slc.c */
        #define DEBUG_SLCx
        /* sink_devicemanager.c */
        #define DEBUG_DEVx
        /* sink_link_policy.c */
        #define DEBUG_LPx
            /*The config manager */
        #define DEBUG_CONFIGx
            /*The LED manager */
        #define DEBUG_LMx
            /*The Lower level LED drive */
        #define DEBUG_LEDSx
            /*The Lower lvel PIO drive*/
        #define DEBUG_PIOx
            /*The power Manager*/
        #define DEBUG_POWERx
            /*tones*/
        #define DEBUG_TONESx
            /*Volume*/
        #define DEBUG_VOLUMEx
            /*State manager*/
        #define DEBUG_STATESx
            /*authentication*/
        #define DEBUG_AUTHx
            /*dimming LEDs*/
        #define DEBUG_DIMx

        #define DEBUG_A2DPx

        #define DEBUG_AVRCPx

        #define DEBUG_TTSx

        #define DEBUG_FILTER_ENABLEx

        #define DEBUG_CSR2CSRx      

        #define DEBUG_USBx

        #define DEBUG_MALLOCx
            
        #define DEBUG_PBAPx

        #define DEBUG_MAPCx

        #define DEBUG_GAIAx
        
        #define DEBUG_SPEECH_RECx
        
        #define DEBUG_WIREDx
        
        #define DEBUG_AT_COMMANDSx

        #define DEBUG_GATTx

        #define DEBUG_RCx
        #define DEBUG_RC_VERBOSEx
        
            /* Device Id */
        #define DEBUG_DIx

        #define DEBUG_DISPLAYx
        
        #define DEBUG_SWATx

        #define DEBUG_FMx

	 #define DEBUG_WM8987Lx

	 #define DEBUG_ISA1200Lx

	 #define DEBUG_MAX14521ELx

	 #define DEBUG_MMA8452QLx

	 #define DEBUG_MMA8452Q_OUTPUTL
    #else
        #define DEBUG(x) 
    #endif /*DEBUG_PRINT_ENABLED*/

        /* If you want to carry out cVc license key checking in Production test
           Then this needs to be enabled */
    #define CVC_PRODTESTx


#else /*RELEASE_BUILD*/    

/*used by the build script to include the debug but none of the individual debug components*/
    #ifdef DEBUG_BUILD 
        #define DEBUG(x) {printf x;}
    #else
        #define DEBUG(x) 
    #endif
        
#endif


#ifdef BC5_MULTIMEDIA
    #define NO_BOOST_CHARGE_TRAPS
    #define ENABLE_VOICE_PROMPTS
    #define DAC_ADC_LINK
#endif


#ifdef CSR8670
    #define INSTALL_PANIC_CHECK
    #define NO_BOOST_CHARGE_TRAPS
    #undef SINK_USB
    #define ENABLE_VOICE_PROMPTS
    #define HAVE_VBAT_SEL
    #define HAVE_FULL_USB_CHARGER_DETECTION
#endif


/* Text to Speech and Voice Prompts */

#ifdef FULL_TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH_DIGITS
 #define TEXT_TO_SPEECH_PHRASES
 #define TEXT_TO_SPEECH_NAMES
 #define TEXT_TO_SPEECH_LANGUAGESELECTION
#endif
 
#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH_DIGITS
#endif

#ifdef ENABLE_VOICE_PROMPTS
 #define CSR_VOICE_PROMPTS
 #define TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH_DIGITS
 #define TEXT_TO_SPEECH_PHRASES
 #define TEXT_TO_SPEECH_LANGUAGESELECTION
#endif

#define ISA1200_MOTOR_DRIVER	/*shin_140123*/
#define MAX14521E_EL_RAMP_DRIVER	/*shin_140123*/
#define MICBIAS_PIN8	/*shin_140127*/
#define VREG_EN_WITH_PIO_SUPPORTED	/*shin_140206*/
#define I2C_STREAM_SUPPORTED	/*shin_140213*/
#define MMA8452Q_SENSOR_SUPPORTED	/*shin_140213*/
#define MMA8452Q_TERMINAL_SUPPORTEDx	/*shin_140218*/
#define PEDOMETER_SUPPORTED	/*shin_140226*/
#endif /*_SINK_DEBUG_H_*/

