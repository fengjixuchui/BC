/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2010-2013
Part of ADK 2.5.1

FILE NAME
    sink_gaia.h        

DESCRIPTION
    Header file for interface with Generic Application Interface Architecture
    library

NOTES

*/
#ifndef _SINK_GAIA_H_
#define _SINK_GAIA_H_

#include <gaia.h>
#include "sink_private.h"

#ifdef DEBUG_GAIA
#define GAIA_DEBUG(x) DEBUG(x)
#else
#define GAIA_DEBUG(x) 
#endif

#define GAIA_API_MINOR_VERSION (1)

#define GAIA_CONFIGURATION_LENGTH_POWER (28)
#define GAIA_CONFIGURATION_LENGTH_HFP (24)
#define GAIA_CONFIGURATION_LENGTH_RSSI (14)

#define GAIA_TONE_BUFFER_SIZE (94)
#define GAIA_TONE_MAX_LENGTH ((GAIA_TONE_BUFFER_SIZE - 4) / 2)

typedef struct
{
    unsigned word:8;
    unsigned posn:4;
    unsigned size:4;
} gaia_feature_map_t;


typedef struct
{
    unsigned fixed:1;
    unsigned size:15;
} gaia_config_entry_size_t;


/*************************************************************************
NAME
    gaiaReportPioChange
    
DESCRIPTION
    Relay any registered PIO Change events to the Gaia client
    We handle the PIO-like GAIA_EVENT_CHARGER_CONNECTION here too
*/
void gaiaReportPioChange(uint32 pio_state);


/*************************************************************************
NAME
    gaiaReportEvent
    
DESCRIPTION
    Relay any significant application events to the Gaia client
*/
void gaiaReportEvent(uint16 id);


/*************************************************************************
NAME
    gaiaReportUserEvent
    
DESCRIPTION
    Relay any user-generated events to the Gaia client
*/
void gaiaReportUserEvent(uint16 id);

        
/*************************************************************************
NAME
    gaiaReportSpeechRecResult
    
DESCRIPTION
    Relay a speech recognition result to the Gaia client
*/
void gaiaReportSpeechRecResult(uint16 id);


/*************************************************************************
NAME
    handleGaiaMessage
    
DESCRIPTION
    Handle messages passed up from the Gaia library
*/
void handleGaiaMessage(Task task, MessageId id, Message message);

#endif /*_SINK_GAIA_H_*/
