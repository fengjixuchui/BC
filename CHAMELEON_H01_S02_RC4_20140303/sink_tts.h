/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_tts.h
    
DESCRIPTION
    header file which defines the interface between the tts engine and the application
    
*/  

#ifndef SINK_TTS_H
#define SINK_TTS_H
#include "sink_debug.h"

#define TTS_NOT_DEFINED (0xFF)

/****************************************************************************

*/
void TTSInit( void );

/****************************************************************************

*/
void TTSConfigureEvent( sinkEvents_t pEvent , uint16 pPhrase );

/****************************************************************************

*/
void TTSConfigureVoicePrompts( uint8 size_index );

/****************************************************************************

*/
void TTSPlay(Task plugin, uint16 id, uint8 *data, uint16 size_data, bool can_queue, bool override);

/****************************************************************************

*/
bool TTSPlayEvent( sinkEvents_t pEvent );

/****************************************************************************
NAME 
    TTSPlayNumString
DESCRIPTION
    Play a numeric string using the TTS plugin
RETURNS    
*/
void TTSPlayNumString(uint16 size_num_string, uint8* num_string);

/****************************************************************************
NAME 
    TTSPlayNumber
DESCRIPTION
    Play a uint32 using the TTS plugin
RETURNS    
*/
void TTSPlayNumber(uint32 number);

/* **************************************************************************
   */


bool TTSPlayCallerNumber( const uint16 size_number, const uint8* number );

/****************************************************************************
NAME    
    TTSTerminate
    
DESCRIPTION
  	function to terminate a ring tone prematurely.
    
RETURNS
    
*/
bool TTSPlayCallerName( const uint16 size_name, const uint8* name );
   
/****************************************************************************
NAME    
    TTSTerminate
    
DESCRIPTION
  	function to terminate a ring tone prematurely.
    
RETURNS
    
*/
void TTSTerminate( void );

/****************************************************************************
NAME    
    TTSSelectTTSLanguageMode
    
DESCRIPTION
  	Move to next language
    
RETURNS
    
*/
void TTSSelectTTSLanguageMode( void );

#endif

