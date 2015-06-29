/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_wired.h
    
DESCRIPTION
    
*/
#ifndef _SINK_WIRED_H_
#define _SINK_WIRED_H_

#include <stdlib.h>

#if defined(ENABLE_WIRED)
/* Wired */
typedef struct
{
    unsigned    reserved:8;
    unsigned    unused:4;
    unsigned    volume:4;
}wired_info;
#endif

/****************************************************************************
NAME 
    wiredAudioInit
    
DESCRIPTION
    Set up wired audio PIOs and configuration
    
RETURNS
    void
*/ 
#ifdef ENABLE_WIRED
void wiredAudioInit(void);
#else
#define wiredAudioInit() ((void)(0))
#endif


/****************************************************************************
NAME 
    wiredAudioRoute
    
DESCRIPTION
    Route wired audio stream through DSP
    
RETURNS
    void
*/ 
#ifdef ENABLE_WIRED
void wiredAudioRoute(void);
#else
#define wiredAudioRoute() ((void)(0))
#endif


/****************************************************************************
NAME 
    wiredAudioDisconnect
    
DESCRIPTION
    Force disconnect of wired audio
    
RETURNS
    void
*/ 
#ifdef ENABLE_WIRED
void wiredAudioDisconnect(void);
#else
#define wiredAudioDisconnect() ((void)(0))
#endif


/****************************************************************************
NAME 
    wiredAudioSinkMatch
    
DESCRIPTION
    Compare sink to wired audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
#ifdef ENABLE_WIRED
bool wiredAudioSinkMatch(Sink sink);
#else
#define wiredAudioSinkMatch(x) (FALSE)
#endif


/****************************************************************************
NAME 
    wiredAudioUpdateVolume
    
DESCRIPTION
    Adjust wired audio volume if currently being routed
    
RETURNS
    TRUE if wired audio being routed, FALSE otherwise
*/ 
#ifdef ENABLE_WIRED
bool wiredAudioUpdateVolume(volume_direction dir);
#else
#define wiredAudioUpdateVolume(x) (FALSE)
#endif

/****************************************************************************
NAME 
    wiredAudioConnected
    
DESCRIPTION
    Determine if wired audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/ 
#ifdef ENABLE_WIRED
bool wiredAudioConnected(void);
#else
#define wiredAudioConnected()   (FALSE)
#endif

/****************************************************************************
NAME 
    wiredAudioCanPowerOff
    
DESCRIPTION
    Determine if wired audio blocks power off
    
RETURNS
    TRUE if device can power off, otherwise FALSE
*/ 
#define wiredAudioCanPowerOff() ((!wiredAudioConnected()) || powerManagerIsVbatCritical())

#endif /* _SINK_WIRED_H_ */
