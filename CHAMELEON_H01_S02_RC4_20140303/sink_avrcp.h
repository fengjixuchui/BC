#ifdef ENABLE_AVRCP
/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013
*/

/*!
@file    sink_avrcp.h
@brief   Interface to the AVRCP profile functionality. 
*/

#ifndef _SINK_AVRCP_H_
#define _SINK_AVRCP_H_


#include <message.h>
#include <app/message/system_message.h>

#include "sink_volume.h"

#include <avrcp.h>


/* Define to allow display of Now Playing information */
#define ENABLE_AVRCP_NOW_PLAYINGx
/* Define to allow display of Player Application Settings */
#define ENABLE_AVRCP_PLAYER_APP_SETTINGSx

/* Defines for general AVRCP operation */
#define MAX_AVRCP_CONNECTIONS 2                 /* max AVRCP connections allowed */
#define DEFAULT_AVRCP_1ST_CONNECTION_DELAY 2000 /* delay in millsecs before connecting AVRCP after A2DP signalling connection */
#define DEFAULT_AVRCP_2ND_CONNECTION_DELAY 500  /* delay in millsecs before connecting AVRCP with the 2nd attempt */
#define DEFAULT_AVRCP_NO_CONNECTION_DELAY 0     /* no delay before connecting AVRCP */
#define AVRCP_ABS_VOL_STEP_CHANGE 9             /* absolute volume change for each local volume up/down */
#define AVRCP_MAX_ABS_VOL 127                   /* the maximum value for absolute volume as defined in AVRCP spec */
#define AVRCP_MAX_PENDING_COMMANDS 4            /* maximum AVRCP commands that can be queued */
#define for_all_avrcp(idx) for(idx = 0; idx < MAX_AVRCP_CONNECTIONS; idx++)
#define AVRCP_MAX_LENGTH_CAPABILITIES 1         /* maximum no of capabilities supported by HS except for mandatory ones */

/* Macros for creating messages. */
#define MAKE_AVRCP_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/* Defines for fragmentation */
#define AVRCP_PACKET_TYPE_SINGLE 0
#define AVRCP_PACKET_TYPE_START 1
#define AVRCP_PACKET_TYPE_CONTINUE 2
#define AVRCP_PACKET_TYPE_END 3

#define AVRCP_ABORT_CONTINUING_TIMEOUT 5000 /* amount of time to wait to receive fragment of Metadata packet before aborting */

#define AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_HEADER_SIZE 8 /* amount of fixed data in Source of AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_T message before variable length data */
#define AVRCP_GET_APP_ATTRIBUTES_TEXT_CFM_HEADER_SIZE 4 /* amount of fixed data in Source of AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM_T message before variable length data */
#define AVRCP_GET_APP_VALUE_CFM_DATA_SIZE 2 /* amount of fixed data in Source of AVRCP_GET_APP_VALUE_CFM_T message before variable length data */

#define AVRCP_PLAYER_APP_SETTINGS_DATA_LEN    2   /* Minimum single attribute data len where 1-byte attribute and 1-byte setting */

/* Define for receiving Playback Position Changed information */
#define AVRCP_PLAYBACK_POSITION_TIME_INTERVAL 1

#define AVRCP_FF_REW_REPEAT_INTERVAL 1500

/* Define for power table settings for AVRCP*/
#define AVRCP_ACTIVE_MODE_INTERVAL  0x0a

typedef enum
{
    AVRCP_CTRL_PAUSE_PRESS,
    AVRCP_CTRL_PAUSE_RELEASE,
    AVRCP_CTRL_PLAY_PRESS,
    AVRCP_CTRL_PLAY_RELEASE,
    AVRCP_CTRL_FORWARD_PRESS,
    AVRCP_CTRL_FORWARD_RELEASE,
    AVRCP_CTRL_BACKWARD_PRESS,
    AVRCP_CTRL_BACKWARD_RELEASE,
    AVRCP_CTRL_STOP_PRESS,
    AVRCP_CTRL_STOP_RELEASE,
    AVRCP_CTRL_FF_PRESS,
    AVRCP_CTRL_FF_RELEASE,
    AVRCP_CTRL_REW_PRESS,
    AVRCP_CTRL_REW_RELEASE,
    AVRCP_CTRL_NEXT_GROUP,
    AVRCP_CTRL_PREVIOUS_GROUP,
    AVRCP_CTRL_ABORT_CONTINUING_RESPONSE
} avrcp_controls;

typedef enum
{
  AVRCP_CONTROL_SEND,
  AVRCP_CREATE_CONNECTION
} avrcp_ctrl_message;

typedef enum
{
  AVRCP_SHUFFLE_OFF = 1,
  AVRCP_SHUFFLE_ALL_TRACK,
  AVRCP_SHUFFLE_GROUP
} avrcp_shuffle_t;

typedef enum
{
  AVRCP_REPEAT_OFF = 1,
  AVRCP_REPEAT_SINGLE_TRACK,
  AVRCP_REPEAT_ALL_TRACK,
  AVRCP_REPEAT_GROUP
} avrcp_repeat_t;

typedef struct
{
    avrcp_controls control;
    uint16 index;
} AVRCP_CONTROL_SEND_T;

typedef struct
{
    bdaddr bd_addr;
} AVRCP_CREATE_CONNECTION_T;

typedef struct
{
    uint16 pdu_id;
} AVRCP_CTRL_ABORT_CONTINUING_RESPONSE_T;

typedef struct 
{
    TaskData            task;
    uint8               *data;
} SinkAvrcpCleanUpTask;


typedef struct
{
    TaskData avrcp_ctrl_handler[MAX_AVRCP_CONNECTIONS];
    SinkAvrcpCleanUpTask        dataCleanUpTask[MAX_AVRCP_CONNECTIONS];
    unsigned active_avrcp:2;
    unsigned avrcp_manual_connect:1;
    unsigned unused:13;
    uint16 extensions[MAX_AVRCP_CONNECTIONS];
    uint16 features[MAX_AVRCP_CONNECTIONS];
    uint16 event_capabilities[MAX_AVRCP_CONNECTIONS];
    bool connected[MAX_AVRCP_CONNECTIONS];
    AVRCP *avrcp[MAX_AVRCP_CONNECTIONS];
    bool pending_cmd[MAX_AVRCP_CONNECTIONS];
    uint16 cmd_queue_size[MAX_AVRCP_CONNECTIONS];
    bdaddr bd_addr[MAX_AVRCP_CONNECTIONS];
    uint16 registered_events[MAX_AVRCP_CONNECTIONS];
    uint16 play_status[MAX_AVRCP_CONNECTIONS];
    uint16 absolute_volume[MAX_AVRCP_CONNECTIONS];
    bdaddr avrcp_play_addr;
    bool   link_active[MAX_AVRCP_CONNECTIONS];
#ifdef ENABLE_AVRCP_BROWSING
    TaskData avrcp_browsing_handler[MAX_AVRCP_CONNECTIONS];
    uint16 browsing_channel[MAX_AVRCP_CONNECTIONS];   
    uint16 uid_counter[MAX_AVRCP_CONNECTIONS];
    uint16 media_player_features[MAX_AVRCP_CONNECTIONS];
    uint16 media_player_id[MAX_AVRCP_CONNECTIONS];
    uint16 browsing_scope[MAX_AVRCP_CONNECTIONS];
#endif
} avrcp_data;

typedef enum
{
    AVRCP_PAUSE,
    AVRCP_PLAY    
}avrcp_remote_actions;

/* initialisation */
void sinkAvrcpInit(void);

/* connection */
void sinkAvrcpConnect(const bdaddr *bd_addr, uint16 delay_time);

void sinkAvrcpDisconnect(const bdaddr *bd_addr);
        
void sinkAvrcpDisconnectAll(void);

void sinkAvrcpAclClosed(bdaddr bd_addr);  

/* media commands */
void sinkAvrcpPlay(void);

void sinkAvrcpPause(void);

void sinkAvrcpPlayPause(void);

void sinkAvrcpStop(void);

void sinkAvrcpSkipForward(void);

void sinkAvrcpSkipBackward(void);

void sinkAvrcpFastForwardPress(void);

void sinkAvrcpFastForwardRelease(void);

void sinkAvrcpRewindPress(void);

void sinkAvrcpRewindRelease(void);

void sinkAvrcpNextGroup(void);

void sinkAvrcpPreviousGroup(void);

/* general helper functions */
void sinkAvrcpToggleActiveConnection(void);

void sinkAvrcpVolumeStepChange(volume_direction operation, uint16 step_change);

void sinkAvrcpSetLocalVolume(uint16 Index, uint16 a2dp_volume);

bool sinkAvrcpGetIndexFromInstance(AVRCP *avrcp, uint16 *Index);

bool sinkAvrcpGetIndexFromBdaddr(const bdaddr *bd_addr, uint16 *Index, bool require_connection);

void sinkAvrcpSetActiveConnection(const bdaddr *bd_addr);

uint16 sinkAvrcpGetActiveConnection(void);

void sinkAvrcpUdpateActiveConnection(void);

uint16 sinkAvrcpGetNumberConnections(void);

void sinkAvrcpSetPlayStatus(const bdaddr *bd_addr, uint16 play_status);

bool sinkAvrcpCheckManualConnectReset(bdaddr *bd_addr);

void sinkAvrcpDisplayMediaAttributes(uint32 attribute_id, uint16 attribute_length, const uint8 *data);

#ifdef ENABLE_AVRCP_NOW_PLAYING
/* Now Playing information commands */
void sinkAvrcpRetrieveNowPlayingRequest(uint32 track_index_high, uint32 track_index_low, bool full_attributes);

void sinkAvrcpRetrieveNowPlayingNoBrowsingRequest(uint16 Index, bool full_attributes);
#endif /* ENABLE_AVRCP_NOW_PLAYING */

#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS
/* Player Application Settings commands */ 
bool sinkAvrcpListPlayerAttributesRequest(void);

bool sinkAvrcpListPlayerAttributesTextRequest(uint16 size_attributes, Source attributes);

bool sinkAvrcpListPlayerValuesRequest(uint8 attribute_id);

bool sinkAvrcpGetPlayerValueRequest(uint16 size_attributes, Source attributes);

bool sinkAvrcpListPlayerValueTextRequest(uint16 attribute_id, uint16 size_values, Source values);

bool sinkAvrcpSetPlayerValueRequest(uint16 size_attributes, Source attributes);

bool sinkAvrcpInformBatteryStatusRequest(avrcp_battery_status status);
#endif /* ENABLE_AVRCP_PLAYER_APP_SETTINGS */

bool sinkAvrcpPlayPauseRequest(uint16 Index, avrcp_remote_actions action);

/* message handler for AVRCP */
void sinkAvrcpHandleMessage(Task task, MessageId id, Message message);

void sinkAvrcpManualConnect(void);

void sinkAvrcpDataCleanUp(Task task, MessageId id, Message message);

Source sinkAvrcpSourceFromData(SinkAvrcpCleanUpTask *avrcp, uint8 *data, uint16 length);

void sinkAvrcpShuffle(avrcp_shuffle_t type);

uint16 sinkAvrcpGetShuffleType(avrcp_shuffle_t type);

void sinkAvrcpRepeat(avrcp_repeat_t type);

uint16 sinkAvrcpGetRepeatType(avrcp_repeat_t type);



#endif /* _SINK_AVRCP_H_ */


#endif /* ENABLE_AVRCP */

