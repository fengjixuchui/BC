#ifdef ENABLE_GATT
/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_gatt.h
    
DESCRIPTION
    GATT functionality header file.
    
*/
#ifndef SINK_GATT_H
#define SINK_GATT_H


#include <message.h>
#include <stdlib.h>


/* GATT link configuration */
enum
{
 	GATT_DISABLED   = 0,
 	GATT_LE_LINK    = 1,
 	GATT_BREDR_LINK = 2
};


/****************************************************************************
NAME    
    sinkGattInitBatteryReport
    
DESCRIPTION
    Initialises the Battery Reporting (GATT-based) profile library
    
RETURNS
    void
*/
void sinkGattInitBatteryReport(uint16 dev_name_len, const uint8 *dev_name);


/****************************************************************************
NAME    
    sinkGattUpdateBatteryReportConnection
    
DESCRIPTION
    This is called when a connection occurs. 
    The LE advertising or connection state may need to be updated based on the new connection.
    
RETURNS
    void
*/
void sinkGattUpdateBatteryReportConnection(void);


/****************************************************************************
NAME    
    sinkGattUpdateBatteryReportDisconnection
    
DESCRIPTION
    This is called when a disconnection occurs. 
    The LE advertising or BR\EDR state may need to be updated based on the disconnection.
    
RETURNS
    void
*/
void sinkGattUpdateBatteryReportDisconnection(void);


/*************************************************************************
NAME    
    handleGattBatteryReportMessage
    
DESCRIPTION
    Handles Gatt Battery Reporting messages

RETURNS
    
*/
void handleGattBatteryReportMessage(Task task, MessageId id, Message message);


#endif /* SINK_GATT_H */

#endif /* ENABLE_GATT */
