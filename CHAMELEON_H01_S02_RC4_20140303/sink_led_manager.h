/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_led_manager.h
    
DESCRIPTION
    
*/
#ifndef SINK_LED_MANAGER_H
#define SINK_LED_MANAGER_H


#include "sink_private.h"
#include "sink_states.h"
#include "sink_events.h"


    void LedManagerMemoryInit(void);
    void LEDManagerInit ( void ) ;

	void LEDManagerIndicateEvent ( MessageId pEvent ) ;

	void LEDManagerIndicateState ( sinkState pState )  ;


	void LedManagerDisableLEDS ( void ) ;
	void LedManagerEnableLEDS  ( void ) ;

	void LedManagerToggleLEDS  ( void )  ;

	void LedManagerResetLEDIndications ( void ) ;

	void LEDManagerResetStateIndNumRepeatsComplete  ( void ) ;


	void LEDManagerCheckTimeoutState( void );

	void LedManagerForceDisable( bool disable );
        
	#ifdef DEBUG_LM
		void LMPrintPattern ( LEDPattern_t * pLED ) ;
	#endif
		
#endif

