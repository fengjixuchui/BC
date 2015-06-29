/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2011-2012
Part of ADK 2.5

FILE NAME
    sink_remote_control.c

DESCRIPTION
    Remote Control interface using the GATT library

NOTES

*/
#ifdef ENABLE_REMOTE
#include "sink_remote_control.h"
#include "sink_statemanager.h"

#ifdef ENABLE_SOUNDBAR
#include <ps.h>
#include "bdaddr.h"
#endif



/*************************************************************************
NAME
    set_speed_parameters
    
DESCRIPTION
    Request connection parameters to enable quick setup or to save energy
*/
static void set_speed_parameters(bool quick)
{
    ble_connection_params params;
            
    RC_DEBUG(("RC: set_speed_parameters %u\n", quick));
          
    params.scan_interval = 16;
    params.scan_window = 16;
    params.conn_interval_min = 6;
    params.supervision_timeout = 400;
    params.conn_attempt_timeout = 8192;
    params.adv_interval_min = 32;
    params.adv_interval_max = 16384;
    params.conn_latency_max = 12;
    params.supervision_timeout_min = 10;
    params.supervision_timeout_max = 3200;
    params.own_address_type = TYPED_BDADDR_PUBLIC;
    
    if (quick)
    {
        params.conn_interval_max = 40;
        params.conn_latency = 0;
    }
    
    else
    {
        params.conn_interval_max = 3200;
        params.conn_latency = 12;
    }
    
    ConnectionDmBleSetConnectionParametersReq(&params);
}

#ifndef ENABLE_SOUNDBAR
/*************************************************************************
NAME
    write_characteristic_8
    
DESCRIPTION
    Write an 8-bit characteristic value
*/
static void write_characteristic_8(uint16 handle, uint8 value)
{    
    RC_DEBUG(("RC: write %04X %02X\n", handle, value));

    GattWriteCharacteristicValueRequest(&theSink.rundata->remoteControl.task, theSink.rundata->remoteControl.cid, handle, 
        1, &value); 
}
#endif

/*************************************************************************
NAME
    write_characteristic_16
    
DESCRIPTION
    Write a 16-bit characteristic value
*/
static void write_characteristic_16(uint16 handle, uint16 value)
{
    uint8 data[2];

    RC_DEBUG(("RC: write %04X %04X\n", handle, value));

    data[0] = value & 0xFF;
    data[1] = value >> 8;

    GattWriteCharacteristicValueRequest(&theSink.rundata->remoteControl.task, theSink.rundata->remoteControl.cid, handle, 
        2, data); 
}

/*************************************************************************
    gatt_message_handler
    
DESCRIPTION
    Handle messages passed up from the GATT library
*/
static void gatt_message_handler(Task task, MessageId id, Message message)
{
    switch (theSink.rundata->remoteControl.state)
    {
        case hgs_starting:
        {
            switch(id)
            {
                case GATT_INIT_CFM:
                {
                    GATT_INIT_CFM_T *m = (GATT_INIT_CFM_T *) message;
                    RC_DEBUG(("RC: GATT_INIT_CFM %u\n", m->status));
                    if (m->status == gatt_status_success)
                        theSink.rundata->remoteControl.state = hgs_idle;

                    set_speed_parameters(TRUE);

                    /*  Set scan interval to 800 * 625µs = 500ms, scan window half that */
                    ConnectionDmBleSetScanParametersReq(FALSE, FALSE, FALSE, 800, 400);
#ifdef ENABLE_SOUNDBAR
                    /* Enable scanning for LE remote connection */
                    rcSetScan();
#endif
                }
                break;

                default :
                    RC_DEBUG(("RC: Un Handled message in hgs_starting\n"));
            }
        }
        break;

        case hgs_connecting:
        {
           switch(id)
            {
                case GATT_CONNECT_CFM:
                {
                    GATT_CONNECT_CFM_T *m = (GATT_CONNECT_CFM_T *) message;
#ifdef ENABLE_SOUNDBAR
                    gatt_uuid_t uuid128[CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_UUID_SIZE] = 
                                          {UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_1, UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_2, 
                                           UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_3, UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_4};
#endif
                    RC_DEBUG(("RC: GATT_CONNECT_CFM %u\n", m->status));

                    if(m->status == gatt_status_initialising)
                    {
                        /* Store the Connection Identifier. */
                        theSink.rundata->remoteControl.cid = m->cid;
                    }
                    else if ((m->status == gatt_status_success) && (theSink.rundata->remoteControl.cid == m->cid))
                    {
#ifdef ENABLE_SOUNDBAR
                        RC_DEBUG_VERBOSE(("GattDiscoverPrimaryServiceRequest \n"));
                        GattDiscoverPrimaryServiceRequest(&theSink.rundata->remoteControl.task, theSink.rundata->remoteControl.cid, gatt_uuid128, uuid128);

                        if (!theSink.rundata->remoteControl.report_handle)
                        {
                            ConnectionDmBleSecurityReq(&theSink.rundata->remoteControl.task,
                                                       &(m->taddr), 
                                                       ble_security_encrypted_bonded,
                                                       ble_connection_master_whitelist);
                        }
#endif
                        theSink.rundata->remoteControl.state = hgs_discovery;
                    }
                    else
                    {
                        /* Gatt connection failed */
                        theSink.rundata->remoteControl.cid = 0;
#ifdef ENABLE_SOUNDBAR                        
                        /* Assuming that remote end start advertising the reports. */
                        rcSetScan();
#endif
                    }
                }
                break;

                default:
                    RC_DEBUG(("RC: Un Handled message in hgs_connecting\n"));
            }
        }
        break;

        case hgs_discovery:
        {
           switch (id)
            {
                case GATT_DISCONNECT_IND:
                {
                    GATT_DISCONNECT_IND_T *m = (GATT_DISCONNECT_IND_T *) message;
                    RC_DEBUG(("RC: GATT_DISCONNECT_IND: %d\n", m->status));

                    if (m->cid == theSink.rundata->remoteControl.cid)
                    {
                        theSink.rundata->remoteControl.state = hgs_idle;
                        theSink.rundata->remoteControl.cid = 0;

                        if (m->status == gatt_status_link_loss)
                        {
                            RC_DEBUG(("RC: link loss... in state %d\n", stateManagerGetState()));

#ifndef ENABLE_SOUNDBAR
                            if (stateManagerGetState() != deviceLimbo)
                            {
                                MessageSend(&theSink.task, EventLinkLoss, NULL);
#endif
                                rcConnect();
#ifndef ENABLE_SOUNDBAR
                            }
#endif
                        }
                    }
                }
                break;

                case GATT_DISCOVER_PRIMARY_SERVICE_CFM:
                {
                    GATT_DISCOVER_PRIMARY_SERVICE_CFM_T *m = (GATT_DISCOVER_PRIMARY_SERVICE_CFM_T *) message;
                    
                    RC_DEBUG(("RC: GATT_DISCOVER_PRIMARY_SERVICE_CFM %u\n", m->status));
                    RC_DEBUG(("RC: UUID Type=%04X U=%04lX H=%04X End=%04X M=%c\n", 
                               m->uuid_type, m->uuid[0], m->handle, m->end, m->more_to_come ? 'T' : 'F'));

                    if (m->status == gatt_status_success)
                    {
#ifdef ENABLE_SOUNDBAR
                        if((m->uuid_type == gatt_uuid128) && (m->uuid[0] == UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_1))
#else
                        if((m->uuid_type == gatt_uuid16) && (m->uuid[0] == UUID_HUMAN_INTERFACE_DEVICE_SERVICE))
#endif
                        {
                            GattDiscoverAllCharacteristicsRequest(&theSink.rundata->remoteControl.task, theSink.rundata->remoteControl.cid, m->handle, m->end);
                            GattDiscoverAllCharacteristicDescriptorsRequest(&theSink.rundata->remoteControl.task, theSink.rundata->remoteControl.cid, m->handle, m->end);
                        }
                    }
                }
                break;

                case GATT_DISCOVER_ALL_CHARACTERISTICS_CFM:
                {
                    GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_T *m = (GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_T *) message;
                    
                    RC_DEBUG_VERBOSE(("RC: GATT_DISCOVER_ALL_CHARACTERISTICS_CFM D=%04X U=%04lX H=%04X M=%c\n", 
                               m->declaration, m->uuid[0], m->handle, m->more_to_come ? 'T' : 'F'));
#ifndef ENABLE_SOUNDBAR
                    if (m->uuid[0] == UUID_CHARACTERISTIC_PROTOCOL_MODE)
                    {
                        /*  Select boot protocol mode  */
                        write_characteristic_8(m->handle, GATT_PROTOCOL_MODE_BOOT);
                    }
#else
                    if (m->uuid[0] == UUID_CSR_SOUNDBAR_REMOTE_CONTROL_CHARACTERISTIC_1)
                    {
                        RC_DEBUG_VERBOSE(("RC: UUID_CSR_SOUNDBAR_REMOTE_CONTROL_CHARACTERISTIC\n"));
                        if(m->properties == ATT_PROP_NOTIFY)
                        {
                            theSink.rundata->remoteControl.report_handle = m->handle;

                            RC_DEBUG(("RC: STATE is %d Report handle - %04X\n", theSink.rundata->remoteControl.state, theSink.rundata->remoteControl.report_handle));

                            /* Cache the HID report handle to PS. */
                            if(!theSink.rundata->hidConfig.hidRepHandle)
                            {
                                theSink.rundata->hidConfig.hidRepHandle = theSink.rundata->remoteControl.report_handle;
                                PsStore(PSKEY_HID_REMOTE_CONTROL_KEY_MAPS, (void*)&theSink.rundata->hidConfig, sizeof(hid_config_t));
                            }
                        }
                    }
#endif
                }
                break;

                case GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM:
                {
                    GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *m = (GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *) message;

                        RC_DEBUG_VERBOSE(("RC: GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM U=%04lX H=%04X M=%c\n", 
                                   m->uuid[0], m->handle, m->more_to_come ? 'T' : 'F'));

                        if (m->uuid[0] == UUID_CLIENT_CHARACTERISTIC_CONFIGURATION)
                        {
                            /*  Enable notification  */
                            RC_DEBUG_VERBOSE(("UUID_CLIENT_CHARACTERISTIC_CONFIGURATION : Enable notification\n"));
                            write_characteristic_16(m->handle, CLIENT_CONFIGURATION_NOTIFICATION);
                        }

                    if (!m->more_to_come)
                        set_speed_parameters(FALSE);

                }
                break;

                case GATT_WRITE_CHARACTERISTIC_VALUE_CFM:
                    break;

                case GATT_NOTIFICATION_IND:
                {
                    uint16 i;
                    uint16 rc_event = 0;
                    GATT_NOTIFICATION_IND_T *m = (GATT_NOTIFICATION_IND_T *) message;

#ifdef ENABLE_SOUNDBAR
                    if ((m->handle == theSink.rundata->remoteControl.report_handle) && (m->size_value >= GATT_LEN_HID_INPUT_REPORT))
#else
                    if (m->size_value == ATTR_LEN_HID_INPUT_REPORT)
#endif
                    {
                    for(i =0 ; i<m->size_value; i++)
                    {
                        RC_DEBUG((" %02X", m->value[i]));
                    }
                    RC_DEBUG(("\n"));
                    
                        if (m->value[3] == 0)
                        {
#ifdef ENABLE_SOUNDBAR
                            if (stateManagerGetState() == deviceLimbo)
                            {
                                if (m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidkeyStandbyResume)
                                {
                                    rc_event = EventPowerOn;
                                    RC_DEBUG(("RC: *** EventPowerOn ***\n "));
                                    MessageSend(&theSink.task, rc_event, NULL);
                                }
                            }
                            else
                            {
                                if (m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidkeyStandbyResume)
                                {
                                    /* Move to system to standby state. */
                                    rc_event = EventPowerOff;
                                    RC_DEBUG(("RC: *** EventPowerOff ***\n "));
                                }
                                /* Check for key events from the remote control */
                                else if (m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidkeyVolumeUp)
                                {
                                    rc_event = EventRCVolumeUp;
                                    RC_DEBUG(("RC: *** EventRCVolumeUp ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidkeyVolumeDown)
                                {
                                    rc_event = EventRCVolumeDown;
                                    RC_DEBUG(("RC: *** EventRCVolumeDown ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidkeySourceInput)
                                {
                                    rc_event = EventSwitchToNextAudioSource;
                                    RC_DEBUG(("RC: *** EventSwitchToNextAudioSource ***\n "));
                                }
#ifdef ENABLE_AVRCP
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpPlayPause)
                                {
                                    rc_event = EventAvrcpPlayPause;
                                    RC_DEBUG(("RC: *** EventAvrcpPlayPause ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpStop)
                                {
                                    rc_event = EventAvrcpStop;
                                    RC_DEBUG(("RC: *** EventAvrcpStop ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpSkipFrd)
                                {
                                    rc_event = EventAvrcpSkipForward;
                                    RC_DEBUG(("RC: *** EventAvrcpSkipForward ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpSkipBackwrd)
                                {
                                    rc_event = EventAvrcpSkipBackward;
                                    RC_DEBUG(("RC: *** EventAvrcpSkipBackward ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpNextGrp)
                                {
                                    rc_event = EventAvrcpNextGroup;
                                    RC_DEBUG(("RC: *** EventAvrcpNextGroup ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpPreviousGrp)
                                {
                                    rc_event = EventAvrcpPreviousGroup;
                                    RC_DEBUG(("RC: *** EventAvrcpPreviousGroup ***\n "));
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpRewindPressRelease)
                                {
                                    if(theSink.rundata->remoteControl.avrcpRwdButtonPress == FALSE)
                                    {
                                        rc_event = EventAvrcpRewindPress;
                                        theSink.rundata->remoteControl.avrcpRwdButtonPress = TRUE;
                                        RC_DEBUG(("RC: *** EventAvrcpRewindPress ***\n "));
                                    }
                                    else
                                    {
                                        rc_event = EventAvrcpRewindRelease;
                                        theSink.rundata->remoteControl.avrcpRwdButtonPress = FALSE;
                                        RC_DEBUG(("RC: *** EventAvrcpRewindRelease ***\n "));
                                    }
                                }
                                else if(m->value[GATT_NOTIFICATION_INDEX] == theSink.rundata->hidConfig.hidAvrcpFFPressRelease)
                                {
                                    if(theSink.rundata->remoteControl.avrcpFFButtonPress == FALSE)
                                    {
                                        rc_event = EventAvrcpFastForwardPress;
                                        theSink.rundata->remoteControl.avrcpFFButtonPress = TRUE;
                                        RC_DEBUG(("RC: *** EventAvrcpFastForwardPress ***\n "));
                                    }
                                    else
                                    {
                                        rc_event = EventAvrcpFastForwardRelease;
                                        theSink.rundata->remoteControl.avrcpFFButtonPress = FALSE;
                                        RC_DEBUG(("RC: *** EventAvrcpFastForwardRelease ***\n "));
                                    }
                                }
#endif
                                else
                                {
#endif
                                    switch (m->value[GATT_NOTIFICATION_INDEX])
                                    {
                                        case HID_KEY_UP_ARROW:
                                        case HID_KEY_VOLUME_UP:
                                            rc_event = EventVolumeUp;
                                            break;
                                            
                                        case HID_KEY_DOWN_ARROW:
                                        case HID_KEY_VOLUME_DOWN:
                                            rc_event = EventVolumeDown;
                                            break;
                                            
                                        case HID_KEY_B:
                                        case HID_KEY_BASS_BOOST:
                                            rc_event = EventBassBoostEnableDisableToggle;
                                            break;       
#ifdef ENABLE_AVRCP
                                        case HID_KEY_SPACE:
                                        case HID_KEY_PLAY_PAUSE:
                                            rc_event = EventAvrcpPlayPause;
                                            break;
                                            
                                        case HID_KEY_END:
                                        case HID_KEY_KEYPAD_END:
                                        case HID_KEY_STOP:
                                            rc_event = EventAvrcpStop;
                                            break;
                                            
                                        case HID_KEY_RIGHT_ARROW:
                                        case HID_KEY_NEXT_TRACK:
                                            rc_event = EventAvrcpSkipForward;
                                            break;
                                            
                                        case HID_KEY_LEFT_ARROW:
                                        case HID_KEY_PREVIOUS_TRACK:
                                            rc_event = EventAvrcpSkipBackward;
                                            break;
#endif
                                    }
                                }

                                if (rc_event)
                                    MessageSend(&theSink.task, rc_event, NULL);
                            }
                        }
#ifdef ENABLE_SOUNDBAR
                    }
                }
#endif
                break;

                default:
                RC_DEBUG(("RC: Un Handled message in hgs_connecting\n")); 
            }
        }
        break;

        case hgs_scan:
        {
               switch (id)
                {
    case RC_SCAN_STOP:
                    {
#ifndef ENABLE_SOUNDBAR
        RC_DEBUG(("RC: Disable Scan\n")); 
        ConnectionDmBleSetScanEnable(FALSE);
        theSink.rundata->remoteControl.state = hgs_idle;
#endif /* ENABLE_SOUNDBAR */
                    }
                    break;

                    default:
                    RC_DEBUG(("RC: Unhandled message in hgs_scan\n"));
                    break;
                }
        }
        break;
        
    default:
        RC_DEBUG(("RC: unhandled 0x%04X message in state %d\n", id, theSink.rundata->remoteControl.state));
        break;
    }
}

#ifdef ENABLE_SOUNDBAR
#ifdef ENABLE_AVRCP
/*************************************************************************
NAME
    rcResetAvrcpButtonPress
    
DESCRIPTION
    Reset AVRCP Rewind and Fast forward button presses.
*/
void rcResetAvrcpButtonPress(void)
{
    theSink.rundata->remoteControl.avrcpFFButtonPress = FALSE;
    theSink.rundata->remoteControl.avrcpRwdButtonPress = FALSE;
}
#endif
#endif

/*************************************************************************
NAME
    rcInitTask
    
DESCRIPTION
    Initialise the GATT library and specify the message receiver task
*/
#ifdef ENABLE_SOUNDBAR
void rcInitTask(uint16 devNameLen, const uint8 *devName)
#else
void rcInitTask(void)
#endif
{
#ifdef ENABLE_SOUNDBAR
    uint16 to_count = 0;
    uint16 from_count = 0;
    uint16 octet_h = 0;
    uint16 *packed_name = 0;
    uint16 packed_name_len = 0;
    uint16 *pDataBase = NULL;
    uint16 sizeGapSrvDb;
#endif
    RC_DEBUG(("RC: func rcInitTask\n"));

    theSink.rundata->remoteControl.task.handler = gatt_message_handler;
#ifdef ENABLE_SOUNDBAR
    if (devNameLen % 2)
        packed_name_len = devNameLen / 2 + 1;
    else
        packed_name_len = devNameLen / 2;
    
    packed_name = mallocPanic(packed_name_len);
    
    if (packed_name)
    {    
        /* must convert the name into a packed uint16 array (see PSKEY_LOCAL_DEVICE for format) */
        for (from_count = 0; from_count < devNameLen; from_count += 2)
        {
            octet_h = 0;
            if ((from_count + 1 ) < devNameLen)
            {
                octet_h = devName[from_count + 1] << 8;
            }     
            packed_name[to_count++] = octet_h | devName[from_count];            
        }

        /* Build GATT Data Base. */
        pDataBase = GattCreateGapSrvcDb(&sizeGapSrvDb, 
                                        DB_HANDLE,
                                        0,/*Refer developer.bluetooth.org . Setting 
                                            appearance to 'UNKNOWN'*/ 
                                        packed_name_len, 
                                        packed_name);
        freePanic(packed_name);
    }

    GattInit(&theSink.rundata->remoteControl.task, sizeGapSrvDb, pDataBase);
#else
    GattInit(&theSink.rundata->remoteControl.task, 0, NULL);
#endif
    theSink.rundata->remoteControl.state = hgs_starting;
}


/*************************************************************************
NAME
    rcConnect
    
DESCRIPTION
    Connect to a remote control in Human Interface Device form
*/
void rcConnect(void)
{
#ifndef ENABLE_SOUNDBAR
    uint8 pattern[] = {UUID_HUMAN_INTERFACE_DEVICE_SERVICE & 0xFF, UUID_HUMAN_INTERFACE_DEVICE_SERVICE >> 8};
#endif

    if (theSink.rundata->remoteControl.state == hgs_idle)
    {
        RC_DEBUG(("RC: scan\n"));
                
        theSink.rundata->remoteControl.state = hgs_scan;            
#ifndef ENABLE_SOUNDBAR
        ConnectionBleAddAdvertisingReportFilter(AD_TYPE_SERVICE_UUID_16BIT_LIST, 
                                                sizeof pattern, sizeof pattern, pattern);

        MessageCancelAll(&theSink.rundata->remoteControl.task, RC_SCAN_STOP);
        MessageSendLater(&theSink.rundata->remoteControl.task, RC_SCAN_STOP, NULL, RC_SCAN_TIMEOUT);
#endif
        RC_DEBUG(("RC: Enable Scan\n"));
        ConnectionDmBleSetScanEnable(TRUE);
    }
}


/*************************************************************************
NAME
    rcDisconnect
    
DESCRIPTION
    Connect to HID remote control
*/
void rcDisconnect(void)
{
#ifndef ENABLE_SOUNDBAR  
    MessageCancelAll(&theSink.rundata->remoteControl.task, RC_SCAN_STOP);
    ConnectionDmBleSetScanEnable(FALSE);
#endif

    RC_DEBUG(("RC: Disable Scan\n")); 
    ConnectionDmBleSetScanEnable(FALSE);
    
    if (theSink.rundata->remoteControl.cid)
        GattDisconnectRequest(theSink.rundata->remoteControl.cid);
}


/*************************************************************************
NAME
    rcHandleAdvertisingReport
    
DESCRIPTION
    Handle BLE advertising reports, checking for connectable keyboard
*/
void rcHandleAdvertisingReport(CL_DM_BLE_ADVERTISING_REPORT_IND_T *report)
{
#ifdef ENABLE_SOUNDBAR
    bool isSameBdAddr;
#endif
    if (theSink.rundata->remoteControl.state == hgs_scan)
    {
        RC_DEBUG_VERBOSE(("ad %u from %04x %02x %06lx rssi %d\n", 
                    report->size_advertising_data,
                    report->current_taddr.addr.nap,
                    report->current_taddr.addr.uap,
                    report->current_taddr.addr.lap,
                    report->rssi));

#ifdef ENABLE_SOUNDBAR        
        isSameBdAddr = BdaddrIsSame(&(theSink.rundata->remoteControl.bdAddr.addr), &(report->current_taddr.addr));
        RC_DEBUG_VERBOSE((" is same bd addr %d\n", isSameBdAddr));
#endif
        
        /* Filter distant (low RSSI) adverts */
        if ((report->rssi >= RC_MIN_RSSI)
#ifdef ENABLE_SOUNDBAR
 || isSameBdAddr /* Paired device. */
#endif
)
        {
            RC_DEBUG(("RC: found\n"));
#ifdef ENABLE_SOUNDBAR
            if ((BdaddrIsZero(&(theSink.rundata->remoteControl.bdAddr.addr))) || isSameBdAddr )
            {
#endif
                theSink.rundata->remoteControl.state = hgs_connecting;
#ifndef ENABLE_SOUNDBAR  
                MessageCancelAll(&theSink.rundata->remoteControl.task, RC_SCAN_STOP);
#endif
                RC_DEBUG(("RC: Disable Scan\n")); 
                ConnectionDmBleSetScanEnable(FALSE);
                GattConnectRequest(&theSink.rundata->remoteControl.task, &report->current_taddr, gatt_connection_ble_master_directed, TRUE);
#ifdef ENABLE_SOUNDBAR
            }
#endif
        }
    }
}


/*************************************************************************
NAME
    rcHandleSimplePairingComplete
    
DESCRIPTION
    Handle CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND
*/
void rcHandleSimplePairingComplete(CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T *ind)
{
#ifdef ENABLE_SOUNDBAR
    gatt_uuid_t uuid128[CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_UUID_SIZE] = 
                          {UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_1, UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_2, 
                           UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_3, UUID_CSR_SOUNDBAR_REMOTE_CONTROL_SERVICE_4};
#else
    gatt_uuid_t uuid = UUID_HUMAN_INTERFACE_DEVICE_SERVICE;
#endif

    RC_DEBUG(("RC: CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND: %d\n", ind->status));

    if (ind->status == success)
    {
        RC_DEBUG(("RC: STATE is hgs_discovery...\n"));

        /* Store the bd address of the paired device. */
#ifdef ENABLE_SOUNDBAR
        theSink.rundata->remoteControl.bdAddr = ind->taddr;
        if(theSink.rundata->remoteControl.state != hgs_discovery)
        {
            RC_DEBUG_VERBOSE(("GattDiscoverPrimaryServiceRequest \n"));
            GattDiscoverPrimaryServiceRequest(&theSink.rundata->remoteControl.task, theSink.rundata->remoteControl.cid, gatt_uuid128, uuid128);
        }
#else
        RC_DEBUG_VERBOSE(("GattDiscoverPrimaryServiceRequest \n"));
        GattDiscoverPrimaryServiceRequest(&theSink.rundata->remoteControl.task, theSink.rundata->remoteControl.cid, gatt_uuid16, &uuid);
#endif
    }
}
#ifdef ENABLE_SOUNDBAR
/*************************************************************************
NAME
    rcSetScan
    
DESCRIPTION
     Enable BLE Scanning.
*/
void rcSetScan()
{
    RC_DEBUG(("RC: func rcSetScan\n"));

    if(theSink.rundata->remoteControl.cid == 0)
    {
        RC_DEBUG(("RC: Enable Scanning....\n"));
        ConnectionDmBleSetScanEnable(TRUE);
        theSink.rundata->remoteControl.state = hgs_scan;
    }
}

/*************************************************************************
NAME
    rcStoreHidHandles

DESCRIPTION
     Store the hid report handle.
*/
void rcStoreHidHandles(uint16 handle)
{
    theSink.rundata->remoteControl.report_handle = theSink.rundata->hidConfig.hidRepHandle;
}

/*************************************************************************
NAME
    rcResetPairedDevice

DESCRIPTION
     Resets the LE HID paired device data base.
*/
void rcResetPairedDevice()
{
    RC_DEBUG(("RC: func rcResetPairedDevice\n"));

    /* Reset the stored bd address. */
    memset(&(theSink.rundata->remoteControl.bdAddr), 0 , sizeof(typed_bdaddr));

    /* Reset the report handle. */
    if (theSink.rundata->remoteControl.cid)
    {
        GattDisconnectRequest(theSink.rundata->remoteControl.cid); /* Disconnect gatt if it exist. */
    }
    theSink.rundata->remoteControl.report_handle = 0;
    theSink.rundata->hidConfig.hidRepHandle = 0;
    PsStore(PSKEY_HID_REMOTE_CONTROL_KEY_MAPS, (void*)&theSink.rundata->hidConfig, sizeof(hid_config_t));

    rcSetScan();
}
#endif

#else
static const int dummy;  /* ISO C forbids an empty source file */
#endif  /* def ENABLE_REMOTE */
