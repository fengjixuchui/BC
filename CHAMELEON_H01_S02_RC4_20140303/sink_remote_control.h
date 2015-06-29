/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2011-2012
Part of ADK 2.5

FILE NAME
    sink_remote_control.h        

DESCRIPTION
    Header file for Remote Control interface using GATT (Generic Attribute
    Profile) library

NOTES

*/

#ifndef _SINK_RC_H_
#define _SINK_RC_H_

#include <gatt.h>
#include "sink_private.h"

#ifdef DEBUG_RC_VERBOSE
#ifndef DEBUG_RC
#define DEBUG_RC
#endif
#define RC_DEBUG_VERBOSE(x) DEBUG(x)
#else
#define RC_DEBUG_VERBOSE(x)
#endif

#ifdef DEBUG_RC
#define RC_DEBUG(x) DEBUG(x)
#else
#define RC_DEBUG(x) 
#endif

#define UUID_BATTERY_SERVICE (0x180F)
#define UUID_HUMAN_INTERFACE_DEVICE_SERVICE (0x1812)
#define UUID_SCAN_PARAMETERS_SERVICE (0x1813)
#define GATT_NOTIFICATION_INDEX    2

#ifdef ENABLE_SOUNDBAR
#define GATT_LEN_HID_INPUT_REPORT                     (20)
#define CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_UUID_SIZE 4
#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_1 0x00002000
#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_2 0xd10211e1
#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_3 0x9b230002
#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_4 0x5b00a5a5

#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_CHARACTERISTIC_1  0x00002001
#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_CHARACTERISTIC_2  0xd10211e1
#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_CHARACTERISTIC_3  0x9b230002
#define UUID_CSR_SOUNDBAR_REMOTE_CONTROL_CHARACTERISTIC_4  0x5b00a5a5
#endif

#define UUID_HID_REPORT_REFERENCE (0x2908)
#define UUID_CLIENT_CHARACTERISTIC_CONFIGURATION (0x2902)
#define UUID_BATTERY_LEVEL_STATE (0x2A1B)
#define UUID_CHARACTERISTIC_REPORT (0x2A4D)
#define UUID_CHARACTERISTIC_PROTOCOL_MODE (0x2A4E)
#define UUID_SCAN_INTERVAL_WINDOW (0x2A4F)

#define CLIENT_CONFIGURATION_NOTIFICATION (0x0001)
#define CLIENT_CONFIGURATION_INDICATION (0x0002)

#define GATT_PROTOCOL_MODE_BOOT (0)

#define HID_INPUT_REPORT (0x01)

#define ATTR_LEN_HID_INPUT_REPORT (8)
        
#define HID_KEY_B (0x05)
#define HID_KEY_SPACE (0x2C)
#define HID_KEY_END (0x4D)
#define HID_KEY_RIGHT_ARROW (0x4F)
#define HID_KEY_LEFT_ARROW (0x50)
#define HID_KEY_DOWN_ARROW (0x51)
#define HID_KEY_UP_ARROW (0x52)
#define HID_KEY_KEYPAD_END (0x59)

#define HID_KEY_NEXT_TRACK (0xB5)
#define HID_KEY_PREVIOUS_TRACK (0xB6)
#define HID_KEY_STOP (0xB7)
#define HID_KEY_PLAY_PAUSE (0xCD)
#define HID_KEY_BASS_BOOST (0xE5)
#define HID_KEY_VOLUME_UP (0xE9)
#define HID_KEY_VOLUME_DOWN (0xEA)

#define RC_SCAN_TIMEOUT (D_SEC(60))
#define RC_SCAN_STOP (0x0001)

#define RC_MIN_RSSI (-50)



/*! \brief AD data types (see GAP section 11.1) */
typedef enum
{
    AD_TYPE_FLAGS                       = 0x01,   /*!< See Advertising Data Flags */
    AD_TYPE_SERVICE_UUID_16BIT          = 0x02,
    AD_TYPE_SERVICE_UUID_16BIT_LIST     = 0x03,
    AD_TYPE_SERVICE_UUID_32BIT          = 0x04,
    AD_TYPE_SERVICE_UUID_32BIT_LIST     = 0x05,
    AD_TYPE_SERVICE_UUID_128BIT         = 0x06,
    AD_TYPE_SERVICE_UUID_128BIT_LIST    = 0x07,
    AD_TYPE_LOCAL_NAME_SHORT            = 0x08,
    AD_TYPE_LOCAL_NAME_COMPLETE         = 0x09,
    AD_TYPE_TX_POWER                    = 0x0A,

    AD_TYPE_OOB_DEVICE_CLASS            = 0x0D,
    AD_TYPE_OOB_SSP_HASH                = 0x0E,
    AD_TYPE_OOB_SSP_RANDOM              = 0x0F,
    AD_TYPE_SM_TK                       = 0x10,
    AD_TYPE_SM_FLAGS                    = 0x11,   /*!< See Security Manager Flags */
    AD_TYPE_SLAVE_CONN_INTERVAL         = 0x12,
    AD_TYPE_SIGNED_DATA                 = 0x13,
    AD_TYPE_SERVICE_SOLICIT_UUID_16BIT  = 0x14,
    AD_TYPE_SERVICE_SOLICIT_UUID_128BIT = 0x15,
    AD_TYPE_SERVICE_DATA                = 0x16,

    AD_TYPE_MANUF                       = 0xFF

} ad_type_t;

#define DB_HANDLE                       0x1100

#define ATT_PROP_CONFIGURE_BROADCAST (0x01)
#define ATT_PROP_READ (0x02)
#define ATT_PROP_WRITE_CMD (0x04)
#define ATT_PROP_WRITE_REQ (0x08)
#define ATT_PROP_NOTIFY (0x10)
#define ATT_PROP_INDICATE (0x20)
#define ATT_PROP_WRITE_SIGNED (0x40)
#define ATT_PROP_EXTENDED (0x80)


typedef struct
{
    uint8 properties;
    uint8 handle_l;
    uint8 handle_h;
    uint8 uuid_l;
    uint8 uuid_h;
} hid_report_decl_t;



/*************************************************************************
NAME
    rcInitTask
    
DESCRIPTION
    Initialise the remote control HID/GATT handler task
*/
#ifdef ENABLE_SOUNDBAR
void rcInitTask(uint16 dev_name_len, const uint8 *dev_name);

#ifdef ENABLE_REMOTE
/*************************************************************************
NAME
    rcResetAvrcpEvents
    
DESCRIPTION
    Reset AVRCP Rewind and Fast forward button presses.
*/
void rcResetAvrcpButtonPress(void);
#endif

#else
void rcInitTask(void);
#endif



/*************************************************************************
NAME
    rcConnect
    
DESCRIPTION
    Connect to HID remote control
*/
void rcConnect(void);


/*************************************************************************
NAME
    rcDisconnect
    
DESCRIPTION
    Disconnect from HID remote control
*/
void rcDisconnect(void);


/*************************************************************************
NAME
    rcHandleAdvertisingReport
    
DESCRIPTION
    Handle BLE advertising reports, checking for connectable keyboard
*/
void rcHandleAdvertisingReport(CL_DM_BLE_ADVERTISING_REPORT_IND_T *report);


/*************************************************************************
NAME
    rcHandleSimplePairingComplete
    
DESCRIPTION
    Handle CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND
*/
void rcHandleSimplePairingComplete(CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T *ind);

#ifdef ENABLE_SOUNDBAR
/*************************************************************************
NAME
    rcSetScan
    
DESCRIPTION
    Enables or disables BLE Scanning.
*/
void rcSetScan(void);
/*************************************************************************
NAME
    rcStoreHidHandles
    
DESCRIPTION
     Store the hid report handle.
*/
void rcStoreHidHandles(uint16 handle);

/*************************************************************************
NAME
    rcResetPairedDevice
    
DESCRIPTION
     Resets the LE HID paired device data base.
*/
void rcResetPairedDevice(void);
#endif
#endif /* def _SINK_RC_H_ */
