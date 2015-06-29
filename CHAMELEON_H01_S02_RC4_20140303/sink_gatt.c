#ifdef ENABLE_GATT 

/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_gatt.c
    
DESCRIPTION
    GATT functionality file. This currently initalises the Battery Service profile for use over LE and BR/EDR links.
    
************************************************************************/

#include "sink_gatt.h"

#include "sink_private.h"
#include "sink_debug.h"
#include "sink_devicemanager.h"
#include "sink_scan.h"

#include <gatt.h>
#include <batt_rep.h>
    

#define GATT_CHARACTERISTIC_APPEARANCE_UNKNOWN 0x0000

 
#ifdef DEBUG_GATT
    #define GATT_DEBUG(x) {printf x;}             
#else
    #define GATT_DEBUG(x) 
#endif


/****************************************************************************
NAME    
    sinkGattInitBatteryReport
    
DESCRIPTION
    Initialises the Battery Reporting (GATT-based) profile library with the local device
    name that is passed into the function.
    
RETURNS
    void
*/
void sinkGattInitBatteryReport(uint16 dev_name_len, const uint8 *dev_name)
{
    uint16 to_count = 0;
    uint16 from_count = 0;
    uint16 octet_h = 0;
    uint16 *packed_name = 0;
    uint16 packed_name_len = 0;

    if (dev_name_len % 2)
        packed_name_len = dev_name_len / 2 + 1;
    else
        packed_name_len = dev_name_len / 2;
    
    packed_name = mallocPanic(packed_name_len);
    
    if (packed_name)
    {    
        /* must convert the name into a packed uint16 array (see PSKEY_LOCAL_DEVICE for format) */
        for (from_count = 0; from_count < dev_name_len; from_count += 2)
        {
            octet_h = 0;
            if ((from_count + 1 ) < dev_name_len)
            {
                octet_h = dev_name[from_count + 1] << 8;
            }     
            packed_name[to_count++] = octet_h | dev_name[from_count];            
        }
    
        BattRepInit(&theSink.task,                   
                    GATT_CHARACTERISTIC_APPEARANCE_UNKNOWN,   
                    BATTERY_SERVICE_UUID,
                    BATTERY_LEVEL_UUID,
                    packed_name_len,
                    packed_name
                    );
        
        freePanic(packed_name);
    }
}


/****************************************************************************
NAME    
    sinkGattUpdateBatteryReportConnection
    
DESCRIPTION
    This is called when a connection occurs. 
    The LE advertising or connection state may need to be updated based on the new connection.
    
RETURNS
    void
*/
void sinkGattUpdateBatteryReportConnection(void)
{
    uint8 NoOfDevices = deviceManagerNumConnectedDevs();
    
    /* return immediately if GATT feature not enabled */
    if (theSink.features.gatt_enabled == GATT_DISABLED)
        return;
    
    /* turn off LE advertising */
    if (theSink.gatt_le)
    {
        ConnectionDmBleSetAdvertiseEnable(FALSE);
    }
            
    /* if BR\EDR for A2DP\HFP is connected and LE GATT is configured,
        then disconnect LE slave link so BR\EDR GATT can be connected */
    if (theSink.gatt_le && NoOfDevices)
    {
        theSink.gatt_le = 0; /* indicate that LE should not be used */
        BattRepDisconnectRequest();
    }
}


/****************************************************************************
NAME    
    sinkGattUpdateBatteryReportDisconnection
    
DESCRIPTION
    This is called when a disconnection occurs. 
    The LE advertising or BR\EDR state may need to be updated based on the disconnection.
    
RETURNS
    void
*/
void sinkGattUpdateBatteryReportDisconnection(void)
{
    uint8 NoOfDevices = deviceManagerNumConnectedDevs();
 
    if (theSink.features.gatt_enabled == GATT_LE_LINK)
    {
    /*  if LE disconnected restore BR/EDR connectable state */
        if (theSink.page_scan_enabled)
        {
            sinkEnableConnectable();
        }
        
    /*  if BR\EDR for A2DP\HFP isn't connected and LE GATT is configured,
        then revert to using LE GATT */
        if (!theSink.gatt_le && !NoOfDevices && !theSink.gatt_connection)
        {
            theSink.gatt_le = TRUE;
            
        /*  Request Disconnect to unregister any BR/EDR service record; we'll
            advertise BLE service when we get BATT_REP_DISCONNECT_IND */
            BattRepDisconnectRequest();
        }
    }
}


/*************************************************************************
NAME    
    handleGattBatteryReportMessage
    
DESCRIPTION
    Handles Gatt Battery Reporting messages

RETURNS
    
*/
void handleGattBatteryReportMessage(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case BATT_REP_INIT_CFM:
        {
            BATT_REP_INIT_CFM_T *batt_rep_init = (BATT_REP_INIT_CFM_T *)message;
            GATT_DEBUG(("GATT: BATT_REP_INIT_CFM %d\n", batt_rep_init->status));
            if (batt_rep_init->status == batt_rep_status_success)
            {
                /* Battery reporting initialisation success, now wait for incoming connection */
                BattRepConnectRequest(&theSink.task,
                                      (theSink.features.gatt_enabled == GATT_LE_LINK) ? batt_rep_le_slave_link : batt_rep_br_edr_slave_link
                                      );
                if (theSink.features.gatt_enabled == GATT_LE_LINK)
                    theSink.gatt_le = 1;
                else
                    theSink.gatt_le = 0;
            }
        }
        break;
      
        case BATT_REP_CONNECT_IND:
        {   
            /* Battery reporting connected */
            GATT_DEBUG(("GATT: BATT_REP_CONNECT_IND\n"));
            /* store new connection */
            theSink.gatt_connection = 1;
            /* check on the new connection */
            sinkGattUpdateBatteryReportConnection();
        }
        break;
    
        case BATT_REP_LEVEL_REQUEST_IND:
        {
            /* get current battery voltage */
            voltage_reading reading;
            uint8 battery_level = 0; 
            
            GATT_DEBUG(("GATT: BATT_REP_LEVEL_REQUEST_IND\n"));
            
            PowerBatteryGetVoltage(&reading);
            /* calculate %battery level using: (currentV - minV)/(maxV - minV)*100 */
            if (theSink.rundata->battery_limits.max_battery_v > theSink.rundata->battery_limits.min_battery_v)
            {
                if (reading.voltage < theSink.rundata->battery_limits.min_battery_v)
                {
                    battery_level = 0;
                }
                else if (reading.voltage > theSink.rundata->battery_limits.max_battery_v)
                {
                    battery_level = 100;
                }
                else
                {
                    battery_level = (uint8)(((uint32)(reading.voltage - theSink.rundata->battery_limits.min_battery_v)  * (uint32)100) / (uint32)(theSink.rundata->battery_limits.max_battery_v - theSink.rundata->battery_limits.min_battery_v));
                }
            }
            GATT_DEBUG(("    battery level %d\n", battery_level));
            /* Return current battery level */
            BattRepLevelResponse(battery_level);
        }
        break;

        case BATT_REP_DISCONNECT_IND:
        {
            /* Battery reporting disconnected */
            GATT_DEBUG(("GATT: BATT_REP_DISCONNECT_IND\n"));
            
            /* store disconnection */
            theSink.gatt_connection = 0;
            
            sinkGattUpdateBatteryReportDisconnection(); 
            /* Move to connect waiting state again for Battery Reporting.
                Use BR/EDR GATT if the local flag is set to not use LE. */
            BattRepConnectRequest(&theSink.task,
                                  (theSink.gatt_le == 1) ? batt_rep_le_slave_link : batt_rep_br_edr_slave_link
                                  );
        }
        break;
    
        default:
        {
        }
        break;
    }
}

#else /* ENABLE_GATT*/
static const int gatt_disabled;
#endif /* ENABLE_GATT */
