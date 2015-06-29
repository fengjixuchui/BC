/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013

FILE NAME
    main.c        

DESCRIPTION
    This is main file for the application software for a sink device

NOTES

*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_init.h"
#include "sink_auth.h"
#include "sink_scan.h"
#include "sink_slc.h" 
#include "sink_inquiry.h"
#include "sink_devicemanager.h"
#include "sink_link_policy.h"
#include "sink_indicators.h"
#include "sink_dut.h" 
#include "sink_pio.h" 
#include "sink_multipoint.h" 
#include "sink_led_manager.h"
#include "sink_buttonmanager.h"
#include "sink_configmanager.h"
#include "sink_events.h"
#include "sink_statemanager.h"
#include "sink_states.h"
#include "sink_powermanager.h"
#include "sink_callmanager.h"
#include "sink_csr_features.h"
#include "sink_usb.h"
#include "sink_display.h"
#include "sink_speech_recognition.h"
#include "sink_a2dp.h"
#include "sink_config.h"
#include "sink_audio_routing.h"
#include "sink_remote_control.h"

#ifdef ISA1200_MOTOR_DRIVER
#include "ISA1200.h"
#endif

#ifdef MAX14521E_EL_RAMP_DRIVER
#include "EL_ramp.h"
#endif

#ifdef VREG_EN_WITH_PIO_SUPPORTED
#include "sink_buttons.h"
#endif

#ifdef MMA8452Q_SENSOR_SUPPORTED
#include "accelerator_system.h"
#endif

#ifdef PEDOMETER_SUPPORTED
#include "pedo.h"
#endif

#ifdef ENABLE_GAIA
#include "sink_gaia.h"
#endif
#ifdef ENABLE_PBAP
#include "sink_pbap.h"
#endif
#ifdef ENABLE_MAPC
#include "sink_mapc.h"
#endif
#ifdef ENABLE_AVRCP
#include "sink_avrcp.h"
#endif
#ifdef ENABLE_GATT
#include "sink_gatt.h"
#endif
#ifdef ENABLE_REMOTE
#include "sink_remote_control.h"
#endif
#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include "sink_volume.h"
#include "sink_tones.h"
#include "sink_tts.h" 

#include "sink_audio.h"
#include "sink_at_commands.h"
#include "vm.h"

#ifdef TEST_HARNESS
#include "test_sink.h"
#include "vm2host_connection.h"
#include "vm2host_hfp.h"
#include "vm2host_a2dp.h"
#include "vm2host_avrcp.h"
#endif

#include <bdaddr.h>
#include <connection.h>
#include <panic.h>
#include <ps.h>
#include <pio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#include <codec.h>
#include <boot.h>
#include <string.h>
#include <audio.h>
#include <sink.h>
#include <kalimba_standard_messages.h>
#include <audio_plugin_if.h>
#include <print.h>
#include <loader.h>

#ifdef ENABLE_FM
#include "sink_fm.h"
#include <fm_plugin_if.h>
#endif

#ifdef ENABLE_DISPLAY
#include <display.h>
#include <display_plugin_if.h>
#include <display_example_plugin.h>
#endif /* ENABLE_DISPLAY */

#ifdef ENABLE_GATT
#include <batt_rep.h>
#endif /* ENABLE_GATT */

#ifdef DEBUG_MAIN
#define MAIN_DEBUG(x) DEBUG(x)
    #define TRUE_OR_FALSE(x)  ((x) ? 'T':'F')   
#else
    #define MAIN_DEBUG(x) 
#endif

#ifdef MICBIAS_PIN8
#define MIC_EN	(1 << 8)
#endif
/* Single instance of the device state */
hsTaskData theSink;

#ifdef MMA8452Q_SENSOR_SUPPORTED
BIT_FIELD SystemFlag;                       /* system control flags*/
BIT_FIELD StreamMode;                /* stream mode control flags*/
uint8 functional_block;                      /* accelerometer function*/
uint8 value[6];                              /* working value result scratchpad*/
BIT_FIELD RegisterFlag;                     /* temporary accelerometer register variable*/
uint8 full_scale;                            /* current accelerometer full scale setting*/
int deviceID;

#endif

#ifdef PEDOMETER_SUPPORTED
int pedo_cnt = 0;
int old_pedo_cnt = 0;
uint8 debug_display_cnt = 0;
int16 data_accx[3];
int16 data_acc[3];
#endif

static void handleHFPStatusCFM ( hfp_lib_status pStatus ) ;

extern void _init(void);
static void sinkInitCodecTask( void ) ;

/*************************************************************************
NAME    
    handleCLMessage
    
DESCRIPTION
    Function to handle the CL Lib messages - these are independent of state

RETURNS

*/
static void handleCLMessage ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("CL = [%x]\n", id)) ;    
 
        /* Handle incoming message identified by its message ID */
    switch(id)
    {        
        case CL_INIT_CFM:
            MAIN_DEBUG(("CL_INIT_CFM [%d]\n" , ((CL_INIT_CFM_T*)message)->status ));
            if(((CL_INIT_CFM_T*)message)->status == success)
            {               
                /* Initialise the codec task */
                sinkInitCodecTask();
                
#ifdef ENABLE_GAIA                
                /* Initialise Gaia with a concurrent connection limit of 1 */
                GaiaInit(task, 1);
#endif

#ifdef ENABLE_SUBWOOFER
                /* Initialise the swat the library - Use library default service record and library default eSCO params - Library does not auto handles messages */
                SwatInit(&theSink.task, SW_MAX_REMOTE_DEVICES, swat_role_source, FALSE, FALSE, 0, 0, 0);
#endif
		
		#ifdef ISA1200_MOTOR_DRIVER
		ISA1200_Disable();
		#endif

		#ifdef MAX14521E_EL_RAMP_DRIVER
		EL_Ramp_Init();
		#endif
		
		#ifdef MMA8452Q_SENSOR_SUPPORTED
		SystemFlag.Byte = 0;
		StreamMode.Byte = 0;

		#ifdef STREAM_XYZ_INT_COUNT
		STREAM_FULLC = TRUE;
		#else
		STREAM_FULLG = TRUE;
		#endif
		
		value[0] = IIC_RegRead(WHO_AM_I_REG);
		if (value[0] == MMA8451Q_ID)
		{
			MAIN_DEBUG (("MMA8451Q: "));
			deviceID=1;
			OSMode_Normal=TRUE;
		}
		else if (value[0] == MMA8452Q_ID)
		{
			MAIN_DEBUG (("MMA8452Q: "));
			deviceID=2;
		} 
		else if (value[0] == MMA8453Q_ID) 
		{
			MAIN_DEBUG (("MMA8453Q: "));
			deviceID=3;
		}
		else
		{
			MAIN_DEBUG (("not identified"));
		}
	
		MMA845x_Init();

		#if 0
		MMA845x_Active();
		full_scale= FULL_SCALE_2G;
		
		#ifdef MMA8452Q_TERMINAL_SUPPORTED
		Print_ODR_HP();
		#endif

		/*
		**  Stream XYZ data via interrupts ICL n  ICH n IGL n IGH n (n=1 or 2)
		*/
		IIC_RegWrite(F_SETUP_REG, 0x00);
		
		/*
		**  LPF Data
		*/
		MAIN_DEBUG((" - Low Pass Filtered Data"));              
		MMA845x_Standby();
		IIC_RegWrite(XYZ_DATA_CFG_REG, (IIC_RegRead(XYZ_DATA_CFG_REG) & ~HPF_OUT_MASK));
		MMA845x_Active();
		

		IIC_RegWrite(CTRL_REG3, PP_OD_MASK);
				
		/*
		**  Activate sensor interrupts
		**  - Event set for new Z-axis data
		**  - all functions bypassed
		**  - enable Data Ready interrupt
		**  - route Data Ready to INT2
		*/
		InterruptsActive (0, INT_EN_DRDY_MASK, ~INT_CFG_DRDY_MASK);
		MessageSendLater( &theSink.task , EventXYZSamplingMode , 0 , DEFAULT_XYZ_SAMPLING_START) ;

		#ifdef PEDOMETER_SUPPORTED
		 pedometer_init();
		#endif
		#endif
		
		#endif		
            }
            else
            {
                Panic();
            }
        break;
        case CL_DM_WRITE_INQUIRY_MODE_CFM:
            /* Read the local name to put in our EIR data */
            ConnectionReadInquiryTx(&theSink.task);
        break;
        case CL_DM_READ_INQUIRY_TX_CFM:
            theSink.inquiry_tx = ((CL_DM_READ_INQUIRY_TX_CFM_T*)message)->tx_power;
            ConnectionReadLocalName(&theSink.task);
        break;
        case CL_DM_LOCAL_NAME_COMPLETE:
            MAIN_DEBUG(("CL_DM_LOCAL_NAME_COMPLETE\n"));
            /* Write EIR data and initialise the codec task */
            sinkWriteEirData((CL_DM_LOCAL_NAME_COMPLETE_T*)message);
#ifndef ENABLE_SOUNDBAR            
#ifdef ENABLE_GATT       
            if (theSink.features.gatt_enabled != GATT_DISABLED)
            {
                /* If Gatt enabled, initialise the Battery Reporter library with the local name */
                sinkGattInitBatteryReport(((CL_DM_LOCAL_NAME_COMPLETE_T *)message)->size_local_name, ((CL_DM_LOCAL_NAME_COMPLETE_T *)message)->local_name);
            }
#endif            
#endif /* ENABLE_SOUNDBAR */

#ifdef ENABLE_GATT
#ifdef ENABLE_REMOTE
#ifdef ENABLE_SOUNDBAR
            rcInitTask(((CL_DM_LOCAL_NAME_COMPLETE_T *)message)->size_local_name, ((CL_DM_LOCAL_NAME_COMPLETE_T *)message)->local_name);
#else
            rcInitTask();
#endif /* ENABLE_SOUNDBAR */
#endif /* ENABLE_REMOTE */
#endif /* ENABLE_GATT */

        break;
        case CL_SM_SEC_MODE_CONFIG_CFM:
            MAIN_DEBUG(("CL_SM_SEC_MODE_CONFIG_CFM\n"));
            /* Remember if debug keys are on or off */
            theSink.debug_keys_enabled = ((CL_SM_SEC_MODE_CONFIG_CFM_T*)message)->debug_keys;
        break;
        case CL_SM_PIN_CODE_IND:
            MAIN_DEBUG(("CL_SM_PIN_IND\n"));
            sinkHandlePinCodeInd((CL_SM_PIN_CODE_IND_T*) message);
        break;
        case CL_SM_USER_CONFIRMATION_REQ_IND:
            MAIN_DEBUG(("CL_SM_USER_CONFIRMATION_REQ_IND\n"));
            sinkHandleUserConfirmationInd((CL_SM_USER_CONFIRMATION_REQ_IND_T*) message);
        break;
        case CL_SM_USER_PASSKEY_REQ_IND:
            MAIN_DEBUG(("CL_SM_USER_PASSKEY_REQ_IND\n"));
            sinkHandleUserPasskeyInd((CL_SM_USER_PASSKEY_REQ_IND_T*) message);
        break;
        case CL_SM_USER_PASSKEY_NOTIFICATION_IND:
            MAIN_DEBUG(("CL_SM_USER_PASSKEY_NOTIFICATION_IND\n"));
            sinkHandleUserPasskeyNotificationInd((CL_SM_USER_PASSKEY_NOTIFICATION_IND_T*) message);
        break;
        case CL_SM_KEYPRESS_NOTIFICATION_IND:
        break;
        case CL_SM_REMOTE_IO_CAPABILITY_IND:
            MAIN_DEBUG(("CL_SM_IO_CAPABILITY_IND\n"));
            sinkHandleRemoteIoCapabilityInd((CL_SM_REMOTE_IO_CAPABILITY_IND_T*)message);
        break;
        case CL_SM_IO_CAPABILITY_REQ_IND:
            MAIN_DEBUG(("CL_SM_IO_CAPABILITY_REQ_IND\n"));
            sinkHandleIoCapabilityInd((CL_SM_IO_CAPABILITY_REQ_IND_T*) message);
        break;
        case CL_SM_AUTHORISE_IND:
            MAIN_DEBUG(("CL_SM_AUTHORISE_IND\n"));
            sinkHandleAuthoriseInd((CL_SM_AUTHORISE_IND_T*) message);
        break;            
        case CL_SM_AUTHENTICATE_CFM:
            MAIN_DEBUG(("CL_SM_AUTHENTICATE_CFM\n"));
            sinkHandleAuthenticateCfm((CL_SM_AUTHENTICATE_CFM_T*) message);
        break;           
#ifdef ENABLE_SUBWOOFER
        case CL_SM_GET_AUTH_DEVICE_CFM: /* This message should only be sent for subwoofer devices */
            MAIN_DEBUG(("CL_SM_GET_AUTH_DEVICE_CFM\n"));
            handleSubwooferGetAuthDevice((CL_SM_GET_AUTH_DEVICE_CFM_T*) message);
#endif
        break;
    
#ifdef ENABLE_PEER
        /* SDP messges */
        case CL_SDP_OPEN_SEARCH_CFM:
            MAIN_DEBUG(("CL_SDP_OPEN_SEARCH_CFM\n"));
            handleSdpOpenCfm((CL_SDP_OPEN_SEARCH_CFM_T *) message);
        break;
        case CL_SDP_CLOSE_SEARCH_CFM:
            MAIN_DEBUG(("CL_SDP_CLOSE_SEARCH_CFM\n"));
            handleSdpCloseCfm((CL_SDP_CLOSE_SEARCH_CFM_T *) message);
        break;
        case CL_SDP_SERVICE_SEARCH_CFM:
            MAIN_DEBUG(("CL_SDP_SERVICE_SEARCH_CFM\n"));
            HandleSdpServiceSearchCfm ((CL_SDP_SERVICE_SEARCH_CFM_T*) message);
        break;
        case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM    :
            MAIN_DEBUG(("CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM\n"));
            HandleSdpServiceSearchAttributeCfm ((CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T*) message);
        break;
#endif
        case CL_DM_REMOTE_FEATURES_CFM:
            MAIN_DEBUG(("HS : Supported Features\n")) ;
        break ;
        case CL_DM_INQUIRE_RESULT:
            MAIN_DEBUG(("HS : Inquiry Result\n"));
            inquiryHandleResult((CL_DM_INQUIRE_RESULT_T*)message);
        break;
        case CL_SM_GET_ATTRIBUTE_CFM:
            MAIN_DEBUG(("HS : CL_SM_GET_ATTRIBUTE_CFM Vol:%d \n",((CL_SM_GET_ATTRIBUTE_CFM_T *)(message))->psdata[0]));
        break;
        case CL_SM_GET_INDEXED_ATTRIBUTE_CFM:
            MAIN_DEBUG(("HS: CL_SM_GET_INDEXED_ATTRIBUTE_CFM[%d]\n" , ((CL_SM_GET_INDEXED_ATTRIBUTE_CFM_T*)message)->status)) ;
        break ;    
        
        case CL_DM_LOCAL_BD_ADDR_CFM:
            DutHandleLocalAddr((CL_DM_LOCAL_BD_ADDR_CFM_T *)message);
        break ;
#ifdef ENABLE_MAPC
        case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
            MAIN_DEBUG(("HS: CL_SM_GET_INDEXED_ATTRIBUTE_CFM[%d]\n" , ((CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T*)message)->status)) ;
            mapcHandleServiceSearchAttributeCfm( (CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T*) message);
#endif
        break ;
        case CL_DM_ROLE_CFM:
            linkPolicyHandleRoleCfm((CL_DM_ROLE_CFM_T *)message);
            return;
#ifdef ENABLE_REMOTE
        case CL_SM_BLE_IO_CAPABILITY_REQ_IND:
            {
                CL_SM_BLE_IO_CAPABILITY_REQ_IND_T *m = (CL_SM_BLE_IO_CAPABILITY_REQ_IND_T *) message;
#ifdef ENABLE_SOUNDBAR                
                ConnectionSmBleIoCapabilityResponse(&m->taddr, cl_sm_io_cap_no_input_no_output, KEY_DIST_RESPONDER_ENC_CENTRAL, FALSE, NULL);
#else                
                ConnectionSmBleIoCapabilityResponse(&m->taddr, cl_sm_io_cap_no_input_no_output, KEY_DIST_NONE, FALSE, NULL);
#endif
            }
            break;
            
        case CL_DM_BLE_ADVERTISING_REPORT_IND:
            rcHandleAdvertisingReport((CL_DM_BLE_ADVERTISING_REPORT_IND_T *) message);
            break;
            
        case CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND:
            rcHandleSimplePairingComplete((CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T *) message);
            break;
#endif

        case CL_SM_SET_TRUST_LEVEL_CFM:
            MAIN_DEBUG(("HS : CL_SM_SET_TRUST_LEVEL_CFM status %x\n",((CL_SM_SET_TRUST_LEVEL_CFM_T*)message)->status));
        break;
        case CL_DM_ACL_OPENED_IND:
            MAIN_DEBUG(("HS : ACL Opened\n"));
        break;
        case CL_DM_ACL_CLOSED_IND:
            MAIN_DEBUG(("HS : ACL Closed\n"));
#ifdef ENABLE_AVRCP
            if(theSink.features.avrcp_enabled)
            {                
                sinkAvrcpAclClosed(((CL_DM_ACL_CLOSED_IND_T *)message)->taddr.addr);    
            }
#endif  
        break;
            
            /*all unhandled connection lib messages end up here*/          
        default :
            MAIN_DEBUG(("Sink - Unhandled CL msg[%x]\n", id));
        break ;
    }
   
}

/*************************************************************************
NAME    
    handleUEMessage
    
DESCRIPTION
    handles messages from the User Events

RETURNS
    
*/
static void handleUEMessage  ( Task task, MessageId id, Message message )
{
    /* Event state control is done by the config - we will be in the right state for the message
    therefore messages need only be passed to the relative handlers unless configurable */
    sinkState lState = stateManagerGetState() ;
    
    /*if we do not want the event received to be indicated then set this to FALSE*/
    bool lIndicateEvent = TRUE ;
        
    /* Deal with user generated Event specific actions*/
    switch ( id )
    {   
            /*these are the events that are not user generated and can occur at any time*/
        case EventOkBattery:
        case EventChargerDisconnected:
        case EventPairingFail:
        case EventLEDEventComplete:
        case EventChargeComplete:
        case EventChargeInProgress: 
        case EventLowBattery:
        case EventPowerOff:
        case EventLinkLoss:
        case EventLimboTimeout:
        case EventSLCConnected:
        case EventSLCConnectedAfterPowerOn:
        case EventPrimaryDeviceConnected:
        case EventSecondaryDeviceConnected:
        case EventError:
        case EventCancelLedIndication:
        case EventAutoSwitchOff:
        case EventReconnectFailed:
        case EventChargerGasGauge0:
        case EventChargerGasGauge1:
        case EventChargerGasGauge2:
        case EventChargerGasGauge3:     
        case EventRssiPairReminder:
        case EventRssiPairTimeout:
        case EventConnectableTimeout:
        case EventRefreshEncryption:
        case EventEndOfCall:
        case EventSCOLinkClose:
        case EventMissedCall:
        case EventUsbDeadBatteryTimeout:
        case EventSbcCodec:            
        case EventMp3Codec:
        case EventAacCodec:            
        case EventAptxCodec:
        case EventAptxLLCodec:            
        case EventFaststreamCodec:         
        case EventResetAvrcpMode:
	 case EventGaiaUser8:	/*EL_RAMP*/
        break;
        default:
                    /* If we have had an event then reset the timer - if it was the event then we will switch off anyway*/
            if (theSink.conf1->timeouts.AutoSwitchOffTime_s !=0)
            {
                /*MAIN_DEBUG(("HS: AUTOSent Ev[%x] Time[%d]\n",id , theSink.conf1->timeouts.AutoSwitchOffTime_s ));*/
                MessageCancelAll( task , EventAutoSwitchOff ) ;
                MessageSendLater( task , EventAutoSwitchOff , 0 , D_SEC(theSink.conf1->timeouts.AutoSwitchOffTime_s) ) ;                
            }
 
            /*cancel any missed call indicator on a user event (button press)*/
           MessageCancelAll(task , EventMissedCall ) ;
 
            /* check for led timeout/re-enable */

            LEDManagerCheckTimeoutState();
   
            /*  Swap the volume control messages if necessary  */
            if (((id == EventVolumeUp) || (id == EventVolumeDown)) && theSink.gVolButtonsInverted)
            {
                if (id == EventVolumeUp)
                    id = EventVolumeDown;
                
                else
                    id = EventVolumeUp;
            }
            
#ifdef ENABLE_GAIA
            gaiaReportUserEvent(id);
#endif
        break;
    }
     
/*    MAIN_DEBUG (( "HS : UE[%x]\n", id )); */
     
    /* The configurable Events*/
    switch ( id )
    {   
        case (EventToggleDebugKeys):
            MAIN_DEBUG(("HS: Toggle Debug Keys\n"));
            /* If the device has debug keys enabled then toggle on/off */
            ConnectionSmSecModeConfig(&theSink.task, cl_sm_wae_acl_none, !theSink.debug_keys_enabled, TRUE);
        break;
        case (EventPowerOn):
        case (EventPowerOnPanic):
            MAIN_DEBUG(("HS: Power On\n" )) ;
                                 
            /* if this init occurs and in limbo wait for the display init */
            if (stateManagerGetState() == deviceLimbo)
            {                                          
                displaySetState(TRUE);                    
                displayShowText(DISPLAYSTR_HELLO,  strlen(DISPLAYSTR_HELLO), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 10);
                displayUpdateVolume(theSink.features.DefaultVolume);  
#ifdef ENABLE_SUBWOOFER
                updateSwatVolume(theSink.features.DefaultVolume);
#endif
                
                /* update battery display */
                displayUpdateBatteryLevel(powerManagerIsChargerConnected());
            }
            
          
                                        
            /*we have received the power on event- we have fully powered on*/
            stateManagerPowerOn();   
            
#ifdef ENABLE_FM      
            /*Initialise FM data structure*/
            theSink.conf2->sink_fm_data.fmRxOn = FALSE;
#endif /* ENABLE_FM */

            /* set flag to indicate just powered up and may use different led pattern for connectable state
               if configured, flag is cleared on first successful connection */
            theSink.powerup_no_connection = TRUE;
            
            /* If critical temperature immediately power off */
            if(powerManagerIsVthmCritical())
                MessageSend(&theSink.task, EventPowerOff, 0);

            /* Take an initial battery reading (and power off if critical) */
            powerManagerReadVbat(battery_level_initial_reading);

            if(theSink.conf1->timeouts.EncryptionRefreshTimeout_m != 0)
                MessageSendLater(&theSink.task, EventRefreshEncryption, 0, D_MIN(theSink.conf1->timeouts.EncryptionRefreshTimeout_m));
            
            if ( theSink.features.DisablePowerOffAfterPowerOn )
            {
                theSink.PowerOffIsEnabled = FALSE ;
                DEBUG(("DIS[%x]\n" , theSink.conf1->timeouts.DisablePowerOffAfterPowerOnTime_s  )) ;
                MessageSendLater ( &theSink.task , EventEnablePowerOff , 0 , D_SEC ( theSink.conf1->timeouts.DisablePowerOffAfterPowerOnTime_s ) ) ;
            }
            else
            {
                theSink.PowerOffIsEnabled = TRUE ;
            }
#ifdef ENABLE_REMOTE
            rcConnect();
#endif

#ifdef ENABLE_SUBWOOFER
            /* check to see if there is a paired subwoofer device, if not kick off an inquiry scan */
            MessageSend(&theSink.task, EventSubwooferCheckPairing, 0);
#endif
        break ;          
        case (EventPowerOff):    
            MAIN_DEBUG(("HS: PowerOff - En[%c]\n" , ((theSink.PowerOffIsEnabled) ? 'T':'F') )) ;
			
		#ifdef MAX14521E_EL_RAMP_DRIVER
		if(EL_Ramp_GetStatus())
		{
			EL_Ramp_Off();
			EL_Ramp_Disable();
			MAIN_DEBUG (( "HS : EL_RAMP disabled [%x]\n", id ));     
			MessageCancelAll (&theSink.task, EventGaiaUser6);
		}
		#endif

		MessageCancelAll (&theSink.task, EventXYZSamplingMode);
		MMA845x_Standby();
		
            /* don't indicate event if already in limbo state */
            if(lState == deviceLimbo) lIndicateEvent = FALSE ;
                
            if ( theSink.PowerOffIsEnabled || powerManagerIsVbatCritical() || powerManagerIsVthmCritical())
            {
                stateManagerEnterLimboState();
                AuthResetConfirmationFlags();
                
                VolumeSetMicrophoneGain(hfp_invalid_link, VOLUME_MUTE_OFF);
                sinkClearQueueudEvent();                
    
                if(theSink.conf1->timeouts.EncryptionRefreshTimeout_m != 0)
                    MessageCancelAll ( &theSink.task, EventRefreshEncryption) ;
                
                MessageCancelAll ( &theSink.task , EventPairingFail) ;
#ifdef ENABLE_AVRCP
                /* cancel any queued ff or rw requests */
                MessageCancelAll (&theSink.task, EventAvrcpFastForwardPress);
                MessageCancelAll (&theSink.task, EventAvrcpRewindPress);
#if defined (ENABLE_REMOTE) && defined (ENABLE_SOUNDBAR)
                rcResetAvrcpButtonPress();
#endif
#endif
#ifdef ENABLE_SPEECH_RECOGNITION
                /* if speech recognition is in tuning mode stop it */
                if(theSink.csr_speech_recognition_tuning_active)
                {
                    speechRecognitionStop();
                    theSink.csr_speech_recognition_tuning_active = FALSE;                   
                }
#endif                
#ifdef ENABLE_REMOTE
                rcDisconnect();
#endif
#ifdef ENABLE_FM
                if (theSink.conf2->sink_fm_data.fmRxOn)
                {               
                    MessageSend(&theSink.task, EventFmOff, 0);                    
                }
#endif
#ifdef ENABLE_SUBWOOFER
                /* Disconnect a subwoofer if one is connected (Event can't be generated at this point so call handler directly) */
                handleEventSubwooferDisconnect();
#endif
                /* keep the display on if charging */
                if (powerManagerIsChargerConnected() && (stateManagerGetState() == deviceLimbo) )
                {
                    displaySetState(TRUE);
                    displayShowText(DISPLAYSTR_CHARGING,  strlen(DISPLAYSTR_CHARGING), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 0);
                    displayUpdateVolume(0); 
                    displayUpdateBatteryLevel(TRUE);
                }          
                else
                {
                    displaySetState(FALSE);
                }
#ifdef ENABLE_SOUNDBAR
                /* set the active routed source back to none */
                audioSwitchToAudioSource(audio_source_none);    
#endif
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
            
        
        break ;
        case (EventInitateVoiceDial):
            MAIN_DEBUG(("HS: InitVoiceDial [%d]\n", theSink.VoiceRecognitionIsActive));            
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
            if ( theSink.PowerOffIsEnabled )
            {
                
                if (theSink.VoiceRecognitionIsActive)
                {
                    sinkCancelVoiceDial(hfp_primary_link) ;
                    lIndicateEvent = FALSE ;
                    /* replumb any existing audio connections */                    
                    audioHandleRouting(audio_source_none);
                }
                else
                {         
                    sinkInitiateVoiceDial (hfp_primary_link) ;
                }            
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventInitateVoiceDial_AG2):
            MAIN_DEBUG(("HS: InitVoiceDial AG2[%d]\n", theSink.VoiceRecognitionIsActive));            
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
            if ( theSink.PowerOffIsEnabled )
            {
                
                if (theSink.VoiceRecognitionIsActive)
                {
                    sinkCancelVoiceDial(hfp_secondary_link) ;                    
                    lIndicateEvent = FALSE ;
                    /* replumb any existing audio connections */                    
                    audioHandleRouting(audio_source_none);
                }
                else
                {                
                    sinkInitiateVoiceDial(hfp_secondary_link) ;
                }            
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventLastNumberRedial):
            MAIN_DEBUG(("HS: LNR\n" )) ;
            
            if ( theSink.PowerOffIsEnabled )
            {
                if (theSink.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theSink.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theSink.task , EventInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 1 */
                        sinkInitiateLNR(hfp_primary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 1 */
                    sinkInitiateLNR(hfp_primary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;   
        case (EventLastNumberRedial_AG2):
            MAIN_DEBUG(("HS: LNR AG2\n" )) ;
            if ( theSink.PowerOffIsEnabled )
            {
                if (theSink.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theSink.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theSink.task , EventInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 2 */
                        sinkInitiateLNR(hfp_secondary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 2 */
                   sinkInitiateLNR(hfp_secondary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;  
        case (EventAnswer):
            MAIN_DEBUG(("HS: Answer\n" )) ;
            /* don't indicate event if not in incoming call state as answer event is used
               for some of the multipoint three way calling operations which generate unwanted
               tones */
            if(stateManagerGetState() != deviceIncomingCallEstablish) lIndicateEvent = FALSE ;

            /* Call the HFP lib function, this will determine the AT cmd to send
               depending on whether the profile instance is HSP or HFP compliant. */ 
            sinkAnswerOrRejectCall( TRUE );      
        break ;   
        case (EventReject):
            MAIN_DEBUG(("HS: Reject\n" )) ;
            /* Reject incoming call - only valid for instances of HFP */ 
            sinkAnswerOrRejectCall( FALSE );
        break ;
        case (EventCancelEnd):
            MAIN_DEBUG(("HS: CancelEnd\n" )) ;
            /* Terminate the current ongoing call process */
            sinkHangUpCall();

        break ;
        case (EventTransferToggle):
            MAIN_DEBUG(("HS: Transfer\n" )) ;
            sinkTransferToggle(id);
        break ;
        case EventCheckForAudioTransfer :
            MAIN_DEBUG(("HS: Check Aud Tx\n")) ;    
            sinkCheckForAudioTransfer();
            break ;
        case (EventToggleMute):
            MAIN_DEBUG(("EventToggleMute\n")) ;
            VolumeToggleMute();
        break ;
            
        case EventMuteOn :
            MAIN_DEBUG(("EventMuteOn\n")) ;
            VolumeSetMicrophoneGain(hfp_invalid_link, VOLUME_MUTE_ON);
        break ;
        case EventMuteOff:
            MAIN_DEBUG(("EventMuteOff\n")) ;
            VolumeSetMicrophoneGain(hfp_invalid_link, VOLUME_MUTE_OFF);                    
        break;
        
        case (EventVolumeUp):      
            MAIN_DEBUG(("EventVolumeUp\n")) ;
            VolumeCheck(increase_volume);			
            break;
            
        case (EventVolumeDown):     
            MAIN_DEBUG(("EventVolumeDown\n")) ;
            VolumeCheck(decrease_volume);      
            break;
            
        case (EventEnterPairingEmptyPDL):        
        case (EventEnterPairing):
            MAIN_DEBUG(("HS: EnterPair [%d]\n" , lState )) ;
#ifdef ENABLE_SUBWOOFER
            /* Ensure the subwoofer device is not over-written by marking it as the most recently used device */
            markSubwooferMostRecentDevice();
#endif
            /*go into pairing mode*/ 
            if (( lState != deviceLimbo) && (lState != deviceConnDiscoverable ))
            {
                theSink.inquiry.session = inquiry_session_normal;
                stateManagerEnterConnDiscoverableState( TRUE );                
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventPairingFail):
            /*we have failed to pair in the alloted time - return to the connectable state*/
            MAIN_DEBUG(("HS: Pairing Fail\n")) ;
            if (lState != deviceTestMode)
            {
                switch (theSink.features.PowerDownOnDiscoTimeout)
                {
                    case PAIRTIMEOUT_POWER_OFF:
                    {
#ifdef ENABLE_FM
                        if (!IsSinkFmRxOn())
#endif
                        {
                        MessageSend ( task , EventPowerOff , 0) ;
                        }
                    }
                        break;
                    case PAIRTIMEOUT_POWER_OFF_IF_NO_PDL:
                        /* Power off if no entries in PDL list */
                        if (ConnectionTrustedDeviceListSize() == 0)
                        {
#ifdef ENABLE_FM
                            if (!IsSinkFmRxOn())
#endif
                            {
                            MessageSend ( task , EventPowerOff , 0) ;
                        }
                        }
                        else
                        {
                            stateManagerEnterConnectableState(TRUE); 
                        }
                        break;
                    case PAIRTIMEOUT_CONNECTABLE:
                    default:
                        stateManagerEnterConnectableState(TRUE);          
                }
            }
            /* have attempted to connect following a power on and failed so clear the power up connection flag */                
            theSink.powerup_no_connection = FALSE;

        break ;                        
        case ( EventPairingSuccessful):
            MAIN_DEBUG(("HS: Pairing Successful\n")) ;
            if (lState == deviceConnDiscoverable)
                stateManagerEnterConnectableState(FALSE);
        break ;
        case ( EventConfirmationAccept ):
            MAIN_DEBUG(("HS: Pairing Correct Res\n" )) ;
            sinkPairingAcceptRes();
        break;
        case ( EventConfirmationReject ):
            MAIN_DEBUG(("HS: Pairing Reject Res\n" )) ;
            sinkPairingRejectRes();
        break;
        case ( EventEstablishSLC ) :
                /* Make sure we're not using the Panic action */
                theSink.panic_reconnect = FALSE;
                /* Fall through */
        case ( EventEstablishSLCOnPanic ):
                            
#ifdef ENABLE_SUBWOOFER
            /* if performing a subwoofer inquiry scan then cancel the SLC connect request
               this will resume after the inquiry scan has completed */
            if(theSink.inquiry.action == rssi_subwoofer)
            {    
                lIndicateEvent = FALSE;
                break;
            }   
#endif                
            /* check we are not already connecting before starting */
            {
                MAIN_DEBUG(("EventEstablishSLC\n")) ;
                
                slcEstablishSLCRequest() ;
                
                /* don't indicate the event at first power up if the use different event at power on
                   feature bit is enabled, this enables the establish slc event to be used for the second manual
                   connection request */
                if(stateManagerGetState() == deviceConnectable)
                {
                    /* send message to do indicate a start of paging process when in connectable state */
                    MessageSend(&theSink.task, EventStartPagingInConnState ,0);  
                }   
            }  
        break ;
        case ( EventRssiPair ):
            MAIN_DEBUG(("HS: RSSI Pair\n"));
            lIndicateEvent = inquiryPair( inquiry_session_normal, TRUE );
        break;
        case ( EventRssiResume ):
            MAIN_DEBUG(("HS: RSSI Resume\n"));
            inquiryResume();
        break;
        case ( EventRssiPairReminder ):
            MAIN_DEBUG(("HS: RSSI Pair Reminder\n"));
            if (stateManagerGetState() != deviceLimbo )
                MessageSendLater(&theSink.task, EventRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));
            else
                lIndicateEvent = FALSE;

        break;
        case ( EventRssiPairTimeout ):
            MAIN_DEBUG(("HS: RSSI Pair Timeout\n"));
            inquiryTimeout();
        break;
        case ( EventRefreshEncryption ):
            MAIN_DEBUG(("HS: Refresh Encryption\n"));
            {
                uint8 k;
                Sink sink;
                Sink audioSink;
                /* For each profile */
                for(k=0;k<MAX_PROFILES;k++)
                {
                    MAIN_DEBUG(("Profile %d: ",k));
                    /* If profile is connected */
                    if((HfpLinkGetSlcSink((k + 1), &sink))&&(sink))
                    {
                        /* If profile has no valid SCO sink associated with it */
                        HfpLinkGetAudioSink((k + hfp_primary_link), &audioSink);
                        if(!SinkIsValid(audioSink))
                        {
                            MAIN_DEBUG(("Key Refreshed\n"));
                            /* Refresh the encryption key */
                            ConnectionSmEncryptionKeyRefreshSink(sink);
                        }
#ifdef DEBUG_MAIN
                        else
                        {
                            MAIN_DEBUG(("Key Not Refreshed, SCO Active\n"));
                        }
                    }
                    else
                    {
                        MAIN_DEBUG(("Key Not Refreshed, SLC Not Active\n"));
#endif
                    }
                }
                MessageSendLater(&theSink.task, EventRefreshEncryption, 0, D_MIN(theSink.conf1->timeouts.EncryptionRefreshTimeout_m));
            }
        break;
        
        /* 60 second timer has triggered to disable connectable mode in multipoint
            connection mode */
        case ( EventConnectableTimeout ) :
#ifdef ENABLE_SUBWOOFER     
            if(!SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
            {
                MAIN_DEBUG(("SM: disable Connectable Cancelled due to lack of subwoofer\n" ));
                break;
            }        
#endif        
            /* only disable connectable mode if at least one hfp instance is connected */
            if(theSink.no_of_profiles_connected)
            {
                MAIN_DEBUG(("SM: disable Connectable \n" ));
                /* disable connectability */
                sinkDisableConnectable();
            }
        break;
        
        case ( EventLEDEventComplete ) :
            /*the message is a ptr to the event we have completed*/
            MAIN_DEBUG(("HS : LEDEvCmp[%x]\n" ,  (( LMEndMessage_t *)message)->Event  )) ;
            
            
            switch ( (( LMEndMessage_t *)message)->Event )
            {
                case (EventResetPairedDeviceList) :
                {      /*then the reset has been completed*/
                    MessageSend(&theSink.task , EventResetComplete , 0 ) ;
   
                        /*power cycle if required*/
                    if ((theSink.features.PowerOffAfterPDLReset )&&
                        (stateManagerGetState() > deviceLimbo )) 
                    {
                        MAIN_DEBUG(("HS: Reboot After Reset\n")) ;
#ifdef ENABLE_FM
                        if (!IsSinkFmRxOn())
#endif
                        {
                        MessageSend ( &theSink.task , EventPowerOff , 0 ) ;
                    }
                }
                }
                break ;            
                
                case EventPowerOff:
                {
                    /*allows a reset of the device for those designs which keep the chip permanently powered on*/
                    if (theSink.features.ResetAfterPowerOffComplete)
                    {
                        configManagerDefragCheck();
                        /* try to set the same boot mode; this triggers the target to reboot.*/
                        BootSetMode(BootGetMode());
                    }

                    if(powerManagerIsVthmCritical())
                        stateManagerUpdateLimboState();
                }
                break ;
                
                default: 
                break ;
            }
            
            if (theSink.features.QueueLEDEvents )
            {
                    /*if there is a queueud event*/
                if (theSink.theLEDTask->Queue.Event1)
                {
                    MAIN_DEBUG(("HS : Play Q'd Ev [%x]\n", (EVENTS_MESSAGE_BASE + theSink.theLEDTask->Queue.Event1)  ));
                    MAIN_DEBUG(("HS : Queue [%x][%x][%x][%x]\n", theSink.theLEDTask->Queue.Event1,
                                                              theSink.theLEDTask->Queue.Event2,
                                                              theSink.theLEDTask->Queue.Event3,
                                                              theSink.theLEDTask->Queue.Event4
                                                                    
                                                                ));
                
                    LEDManagerIndicateEvent ( (EVENTS_MESSAGE_BASE + theSink.theLEDTask->Queue.Event1) ) ;
    
                        /*shuffle the queue*/
                    theSink.theLEDTask->Queue.Event1 = theSink.theLEDTask->Queue.Event2 ;
                    theSink.theLEDTask->Queue.Event2 = theSink.theLEDTask->Queue.Event3 ;
                    theSink.theLEDTask->Queue.Event3 = theSink.theLEDTask->Queue.Event4 ;
                    theSink.theLEDTask->Queue.Event4 = 0x00 ;
                }   
                else
                {
                    /* restart state indication */
                    LEDManagerIndicateState ( stateManagerGetState () ) ;
                }
            }
            else
                LEDManagerIndicateState ( stateManagerGetState () ) ;
                
        break ;   
        case (EventAutoSwitchOff):
            MAIN_DEBUG(("HS: Auto S Off[%d] sec elapsed\n" , theSink.conf1->timeouts.AutoSwitchOffTime_s )) ;
            switch ( lState )
            {   
                case deviceLimbo:
                case deviceConnectable:
                case deviceConnDiscoverable:
                    if ( (!usbIsAttached())
#ifdef ENABLE_FM
                         || (!IsSinkFmRxOn())
#endif
                        )
                        {
                        MessageSend (task, EventPowerOff , 0);
                        }
                    break;
                case deviceConnected:
                    if(deviceManagerNumConnectedDevs() == deviceManagerNumConnectedPeerDevs())
                    {   /* Only connected to peer devices, so allow auto-switchoff to occur */
#ifdef ENABLE_FM
                        if (!IsSinkFmRxOn())
#endif
                        {
                        MessageSend (task, EventPowerOff , 0);
                    }
                    }
                    break;
                case deviceOutgoingCallEstablish:   
                case deviceIncomingCallEstablish:   
                case deviceActiveCallSCO:            
                case deviceActiveCallNoSCO:             
                case deviceTestMode:
                    break ;
                default:
                    MAIN_DEBUG(("HS : UE ?s [%d]\n", lState));
                    break ;
            }
        break;
        case (EventChargerConnected):
        {
            MAIN_DEBUG(("HS: Charger Connected\n"));
            powerManagerChargerConnected();
            if ( lState == deviceLimbo )
            { 
                stateManagerUpdateLimboState();
            }
            
            /* indicate battery charging on the display */
            displayUpdateBatteryLevel(TRUE);  
        }
        break;
        case (EventChargerDisconnected):
        {
            MAIN_DEBUG(("HS: Charger Disconnected\n"));
            powerManagerChargerDisconnected();
 
            /* if in limbo state, schedule a power off event */
            if (lState == deviceLimbo )
            {
                /* cancel existing limbo timeout and rescheduled another limbo timeout */
                MessageCancelAll ( &theSink.task , EventLimboTimeout ) ;
                MessageSendLater ( &theSink.task , EventLimboTimeout , 0 , D_SEC(theSink.conf1->timeouts.LimboTimeout_s) ) ;

                /* turn off the display if in limbo and no longer charging */
                displaySetState(FALSE);
            }            
            else
            {
                /* update battery display */
                displayUpdateBatteryLevel(FALSE); 
            }
        }
        break;
        case (EventResetPairedDeviceList):
            {
                MAIN_DEBUG(("HS: --Reset PDL--")) ;                
                if ( stateManagerIsConnected () )
                {
                   /*disconnect any connected HFP profiles*/
                   sinkDisconnectAllSlc();
                   /*disconnect any connected A2DP/AVRCP profiles*/
                   disconnectAllA2dpAvrcp();
                }                
#ifdef ENABLE_SOUNDBAR
#ifdef ENABLE_SUBWOOFER
                /* Also delete the subwoofers Bluetooth address; don't need to delete the link key as deviceManagerRemoveAllDevices() will do this */
                deleteSubwooferPairing(FALSE);
#endif
#ifdef ENABLE_REMOTE
                /* Reset the LE HID data base. */
                rcResetPairedDevice();
#endif
#endif /* ENABLE_SOUNDBAR */
                
                deviceManagerRemoveAllDevices();
                
                if(INQUIRY_ON_PDL_RESET)
                    MessageSend(&theSink.task, EventRssiPair, 0);
            }
        break ;
        case ( EventLimboTimeout ):
            {
                /*we have received a power on timeout - shutdown*/
                MAIN_DEBUG(("HS: EvLimbo TIMEOUT\n")) ;
                if (lState != deviceTestMode)
                {
                    stateManagerUpdateLimboState();
                }
            }    
        break ;
        case EventSLCDisconnected: 
                MAIN_DEBUG(("HS: EvSLCDisconnect\n")) ;
            {
                theSink.VoiceRecognitionIsActive = FALSE ;
                MessageCancelAll ( &theSink.task , EventNetworkOrServiceNotPresent ) ;
                
#ifdef ENABLE_GATT
                sinkGattUpdateBatteryReportDisconnection();      
#endif                 
            }
        break ;
        case (EventLinkLoss):
            MAIN_DEBUG(("HS: Link Loss\n")) ;
            {
                /* should the device have been powered off prior to a linkloss event being
                   generated, this can happen if a link loss has occurred within 5 seconds
                   of power off, ensure the device does not attempt to reconnet from limbo mode */
                if(stateManagerGetState()== deviceLimbo)
                    lIndicateEvent = FALSE;
            }
        break ;
        case (EventMuteReminder) :        
            MAIN_DEBUG(("HS: Mute Remind\n")) ;
            /*arrange the next mute reminder tone*/
            MessageSendLater( &theSink.task , EventMuteReminder , 0 ,D_SEC(theSink.conf1->timeouts.MuteRemindTime_s ) )  ;            

            /* only play the mute reminder tone if AG currently having its audio routed is in mute state */ 
            if(!VolumePlayMuteToneQuery())
                lIndicateEvent = FALSE;
        break;

        case EventBatteryLevelRequest:
          MAIN_DEBUG(("EventBatteryLevelRequest\n")) ;
          powerManagerReadVbat(battery_level_user);
        break;

        case EventCriticalBattery:
            DEBUG(("HS: EventCriticalBattery\n")) ;
        break;

        case EventLowBattery:
            DEBUG(("HS: EventLowBattery\n")) ;
        break; 
        
        case EventGasGauge0 :
        case EventGasGauge1 :
        case EventGasGauge2 :
        case EventGasGauge3 :
            DEBUG(("HS: EventGasGauge%d\n", id - EventGasGauge0)) ;
        break ;

        case EventOkBattery:
            MAIN_DEBUG(("HS: EventOkBattery\n")) ;
        break;   

        case EventChargeInProgress:
            MAIN_DEBUG(("HS: EventChargeInProgress\n")) ;
        break;

        case EventChargeComplete:  
            MAIN_DEBUG(("HS: EventChargeComplete\n")) ;
        break;

        case EventChargeDisabled:
            MAIN_DEBUG(("HS: EventChargeDisabled\n")) ;            
        break;

        case EventEnterDUTState :
        {
            MAIN_DEBUG(("EnterDUTState \n")) ;
            stateManagerEnterTestModeState();                
        }
        break;        
        case EventEnterDutMode :
        {
            MAIN_DEBUG(("Enter DUT Mode \n")) ;            
            if (lState !=deviceTestMode)
            {
                MessageSend( task , EventEnterDUTState, 0 ) ;
            }
            enterDutMode () ;               
        }
        break;        
        case EventEnterTXContTestMode :
        {
            MAIN_DEBUG(("Enter TX Cont \n")) ;        
            if (lState !=deviceTestMode)
            {
                MessageSend( task , EventEnterDUTState , 0 ) ;
            }            
            enterTxContinuousTestMode() ;
        }
        break ;     
        case EventVolumeOrientationNormal:
                theSink.gVolButtonsInverted = FALSE ;               
                MAIN_DEBUG(("HS: VOL ORIENT NORMAL [%d]\n", theSink.gVolButtonsInverted)) ;
                    /*write this to the PSKEY*/                
                /* also include the led disable state as well as orientation, write this to the PSKEY*/ 
                /* also include the selected tts language  */
                configManagerWriteSessionData () ;                          
        break;
        case EventVolumeOrientationInvert:       
               theSink.gVolButtonsInverted = TRUE ;
               MAIN_DEBUG(("HS: VOL ORIENT INVERT[%d]\n", theSink.gVolButtonsInverted)) ;               
               /* also include the led disable state as well as orientation, write this to the PSKEY*/                
               /* also include the selected tts language  */
               configManagerWriteSessionData () ;           
        break;        
        case EventToggleVolume:                
                theSink.gVolButtonsInverted ^=1 ;    
                MAIN_DEBUG(("HS: Toggle Volume Orientation[%d]\n", theSink.gVolButtonsInverted)) ;                
        break ;        
        case EventNetworkOrServiceNotPresent:
            {       /*only bother to repeat this indication if it is not 0*/
                if ( theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s )
                {       /*make sure only ever one in the system*/
                    MessageCancelAll( task , EventNetworkOrServiceNotPresent) ;
                    MessageSendLater  ( task , 
                                        EventNetworkOrServiceNotPresent ,
                                        0 , 
                                        D_SEC(theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s) ) ;
                }                                    
                MAIN_DEBUG(("HS: NO NETWORK [%d]\n", theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s )) ;
            }                                
        break ;
        case EventNetworkOrServicePresent:
            {
                MessageCancelAll ( task , EventNetworkOrServiceNotPresent ) ;                
                MAIN_DEBUG(("HS: YES NETWORK\n")) ;
            }   
        break ;
        case EventEnableDisableLeds  :   
            MAIN_DEBUG(("HS: Toggle EN_DIS LEDS ")) ;
            MAIN_DEBUG(("HS: Tog Was[%c]\n" , theSink.theLEDTask->gLEDSEnabled ? 'T' : 'F')) ;
            
            LedManagerToggleLEDS();
            MAIN_DEBUG(("HS: Tog Now[%c]\n" , theSink.theLEDTask->gLEDSEnabled ? 'T' : 'F')) ;            
       
            break ;        
        case EventEnableLEDS:
            MAIN_DEBUG(("HS: Enable LEDS\n")) ;
            LedManagerEnableLEDS ( ) ;
                /* also include the led disable state as well as orientation, write this to the PSKEY*/                
            configManagerWriteSessionData ( ) ;             
            break ;
        case EventDisableLEDS:
            MAIN_DEBUG(("HS: Disable LEDS\n")) ;            
            LedManagerDisableLEDS ( ) ;
            
                /* also include the led disable state as well as orientation, write this to the PSKEY*/                
            configManagerWriteSessionData ( ) ;            
            break ;
        case EventCancelLedIndication:
            MAIN_DEBUG(("HS: Disable LED indication\n")) ;        
            LedManagerResetLEDIndications ( ) ;
            break ;
        case EventCallAnswered:
            MAIN_DEBUG(("HS: EventCallAnswered\n")) ;
        break;
        case EventSLCConnected:
        case EventSLCConnectedAfterPowerOn:
            
            MAIN_DEBUG(("HS: EventSLCConnected\n")) ;
            /*if there is a queued event - we might want to know*/                
            sinkRecallQueuedEvent();
            
#ifdef ENABLE_GATT
            sinkGattUpdateBatteryReportConnection();      
#endif            
            
        break;            
        case EventPrimaryDeviceConnected:
        case EventSecondaryDeviceConnected:
            /*used for indication purposes only*/
            MAIN_DEBUG(("HS:Device Connected [%c]\n " , (id - EventPrimaryDeviceConnected)? 'S' : 'P'  )); 
        break;
        case EventVLongTimer:
        case EventLongTimer:
           if (lState == deviceLimbo)
           {
               lIndicateEvent = FALSE ;
           }
        break ;
            /*these events have no required action directly associated with them  */
             /*they are received here so that LED patterns and Tones can be assigned*/
        case EventSCOLinkOpen :        
            MAIN_DEBUG(("EventScoLinkOpen\n")) ;
        break ;
        case EventSCOLinkClose:
            MAIN_DEBUG(("EventScoLinkClose\n")) ;
        break ;
        case EventEndOfCall :        
            MAIN_DEBUG(("EventEndOfCall\n")) ;
#ifdef ENABLE_DISPLAY
            displayShowSimpleText(DISPLAYSTR_CLEAR,1);
            displayShowSimpleText(DISPLAYSTR_CLEAR,2);
#endif            
        break;    
        case EventResetComplete:        
            MAIN_DEBUG(("EventResetComplete\n")) ;
        break ;
        case EventError:        
            MAIN_DEBUG(("EventError\n")) ;
        break;
        case EventReconnectFailed:        
            MAIN_DEBUG(("EventReconnectFailed\n")) ;
        break;
        
#ifdef THREE_WAY_CALLING        
        case EventThreeWayReleaseAllHeld:       
            MAIN_DEBUG(("HS3 : RELEASE ALL\n"));          
            /* release the held call */
            MpReleaseAllHeld();
        break;
        case EventThreeWayAcceptWaitingReleaseActive:    
            MAIN_DEBUG(("HS3 : ACCEPT & RELEASE\n"));
            MpAcceptWaitingReleaseActive();
        break ;
        case EventThreeWayAcceptWaitingHoldActive  :
            MAIN_DEBUG(("HS3 : ACCEPT & HOLD\n"));
            /* three way calling not available in multipoint usage */
            MpAcceptWaitingHoldActive();
        break ;
        case EventThreeWayAddHeldTo3Way  :
            MAIN_DEBUG(("HS3 : ADD HELD to 3WAY\n"));
            /* check to see if a conference call can be created, more than one call must be on the same AG */            
            MpHandleConferenceCall(TRUE);
        break ;
        case EventThreeWayConnect2Disconnect:  
            MAIN_DEBUG(("HS3 : EXPLICIT TRANSFER\n"));
            /* check to see if a conference call can be created, more than one call must be on the same AG */            
            MpHandleConferenceCall(FALSE);
        break ;
#endif      
        case (EventEnablePowerOff):
        {
            MAIN_DEBUG(("HS: EventEnablePowerOff \n")) ;
            theSink.PowerOffIsEnabled = TRUE ;
        }
        break;        
        case EventPlaceIncomingCallOnHold:
            sinkPlaceIncomingCallOnHold();
        break ;
        
        case EventAcceptHeldIncomingCall:
            sinkAcceptHeldIncomingCall();
        break ;
        case EventRejectHeldIncomingCall:
            sinkRejectHeldIncomingCall();
        break;
        
        case EventEnterDFUMode:       
        {
            MAIN_DEBUG(("EventEnterDFUMode\n")) ;
            BootSetMode(BOOTMODE_DFU);
        }
        break;

        case EventEnterServiceMode:
        {
            MAIN_DEBUG(("Enter Service Mode \n")) ;            

            enterServiceMode();
        }
        break ;
        case EventServiceModeEntered:
        {
            MAIN_DEBUG(("Service Mode!!!\n")) ; 
        }
        break;

        case EventAudioMessage1:
        case EventAudioMessage2:
        case EventAudioMessage3:
        case EventAudioMessage4:
        {
            if (theSink.routed_audio)
            {
                uint16 * lParam = mallocPanic( sizeof(uint16)) ;
                *lParam = (id -  EventAudioMessage1) ; /*0,1,2,3*/
                if(!AudioSetMode ( AUDIO_MODE_CONNECTED , (void *) lParam) )
                    freePanic(lParam);
            }
        }
        break ;
        
        case EventUpdateStoredNumber:
            sinkUpdateStoredNumber();
        break;
        
        case EventDialStoredNumber:
            MAIN_DEBUG(("EventDialStoredNumber\n"));
            sinkDialStoredNumber();
        
        break;
        case EventRestoreDefaults:
            MAIN_DEBUG(("EventRestoreDefaults\n"));
            configManagerRestoreDefaults();
                
        break;
        
        case EventTone1:
        case EventTone2:
            MAIN_DEBUG(("HS: EventTone[%d]\n" , (id - EventTone1 + 1) )) ;
        break;
        
        case EventSelectTTSLanguageMode:
            if(theSink.tts_enabled) 
            {
                MAIN_DEBUG(("EventSelectTTSLanguageModes\n"));
                TTSSelectTTSLanguageMode();
            }
            else
            {
                lIndicateEvent = FALSE ;
            } 
        break;
        
        case EventConfirmTTSLanguage:
            if(theSink.tts_enabled) 
            {
                /* Store TTS language in PS */
                configManagerWriteSessionData () ;
            }
        break;

        /* enable multipoint functionality */
        case EventEnableMultipoint:
            MAIN_DEBUG(("EventEnableMultipoint\n"));
            /* enable multipoint operation */
            configManagerEnableMultipoint(TRUE);
            /* and store in PS for reading at next power up */
            configManagerWriteSessionData () ;
        break;
        
        /* disable multipoint functionality */
        case EventDisableMultipoint:
            MAIN_DEBUG(("EventDisableMultipoint\n"));
            /* disable multipoint operation */
            configManagerEnableMultipoint(FALSE);
            /* and store in PS for reading at next power up */
            configManagerWriteSessionData () ;           
        break;
        
        /* disabled leds have been re-enabled by means of a button press or a specific event */
        case EventResetLEDTimeout:
            MAIN_DEBUG(("EventResetLEDTimeout\n"));
            LEDManagerIndicateState ( lState ) ;     
            theSink.theLEDTask->gLEDSStateTimeout = FALSE ;               
        break;
        /* starting paging whilst in connectable state */
        case EventStartPagingInConnState:
            MAIN_DEBUG(("EventStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theSink.paging_in_progress = TRUE;
        break;
        
        /* paging stopped whilst in connectable state */
        case EventStopPagingInConnState:
            MAIN_DEBUG(("EventStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theSink.paging_in_progress = FALSE;
        break;
        
        /* continue the slc connection procedure, will attempt connection
           to next available device */
        case EventContinueSlcConnectRequest:
            /* don't continue connecting if in pairing mode */
            if(stateManagerGetState() != deviceConnDiscoverable)
            {
                MAIN_DEBUG(("EventContinueSlcConnectRequest\n"));
                /* attempt next connection */
                slcContinueEstablishSLCRequest();
            }
        break;
        
        /* indication of call waiting when using two AG's in multipoint mode */
        case EventMultipointCallWaiting:
            MAIN_DEBUG(("EventMultipointCallWaiting\n"));
        break;
                   
        /* kick off a check the role of the device and make changes if appropriate by requesting a role indication */
        case EventCheckRole:
            linkPolicyCheckRoles();
        break;

        case EventMissedCall:
        {
            if(theSink.conf1->timeouts.MissedCallIndicateTime_s != 0)
            { 
                MessageCancelAll(task , EventMissedCall ) ;
                     
                theSink.MissedCallIndicated -= 1;               
                if(theSink.MissedCallIndicated != 0)
                {
                    MessageSendLater( &theSink.task , EventMissedCall , 0 , D_SEC(theSink.conf1->timeouts.MissedCallIndicateTime_s) ) ;
                }
            }   
        }
        break;
      
#ifdef ENABLE_PBAP                  
        case EventPbapDialMch:
        {         
            /* pbap dial from missed call history */
            MAIN_DEBUG(("EventPbapDialMch\n"));  
            
            if ( theSink.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theSink.features.LNRCancelsVoiceDialIfActive   && 
                    theSink.VoiceRecognitionIsActive)
                {
                    MessageSend(&theSink.task , EventInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {                   
                    pbapDialPhoneBook(pbap_mch);
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;      
        
        case EventPbapDialIch:
        {
            /* pbap dial from incoming call history */
            MAIN_DEBUG(("EventPbapDialIch\n"));
            
            if ( theSink.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theSink.features.LNRCancelsVoiceDialIfActive   && 
                    theSink.VoiceRecognitionIsActive)
                {
                    MessageSend(&theSink.task , EventInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {                   
                    pbapDialPhoneBook(pbap_ich);
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;
        
        case EventEstablishPbap:
        {
            MAIN_DEBUG(("EventEstablishPbap\n"));
            
            /* Connect to the primary and secondary hfp link devices */
            theSink.pbapc_data.pbap_command = pbapc_action_idle;
             
            pbapConnect( hfp_primary_link );
            pbapConnect( hfp_secondary_link );
        }
        break;
            
        case EventPbapSetPhonebook:
        {
            MAIN_DEBUG(("EventPbapSetPhonebook, active pb is [%d]\n", theSink.pbapc_data.pbap_active_pb));

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_command = pbapc_setting_phonebook;
            
            if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
            {
                pbapConnect( hfp_primary_link );
            }
            else
            {
                /* Set the link to active state */
                linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
            }
            
            lIndicateEvent = FALSE ;
        }
        break;
        
        case EventPbapBrowseEntry:
        {
            MAIN_DEBUG(("EventPbapBrowseEntry\n"));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                /* If Pbap profile does not connected, connect it first */
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                    theSink.pbapc_data.pbap_browsing_start_flag = 1;
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));
                    
                    if(theSink.pbapc_data.pbap_browsing_start_flag == 0)
                    {
                        theSink.pbapc_data.pbap_browsing_start_flag = 1;
                        PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
                    }
                    else
                    {
                        MessageSend ( &theSink.task , PBAPC_APP_PULL_VCARD_ENTRY , 0 ) ;
                    }
                }
                
                theSink.pbapc_data.pbap_command = pbapc_browsing_entry;
            }
                
            lIndicateEvent = FALSE ;
        }
        break;
        
        case EventPbapBrowseList:
        {
            MAIN_DEBUG(("EventPbapBrowseList\n"));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {         
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
                }
                
                theSink.pbapc_data.pbap_command = pbapc_browsing_list;
            }
                
            lIndicateEvent = FALSE ;
        }
        break;
        
        case EventPbapDownloadPhonebook:
        {
            MAIN_DEBUG(("EventPbapDownloadPhonebook\n"));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    MessageSend(&theSink.task , PBAPC_APP_PULL_PHONE_BOOK , 0 ) ;
                }
                
                theSink.pbapc_data.pbap_command = pbapc_downloading;
            }
                
            lIndicateEvent = FALSE ;
        }
        break;
        
        case EventPbapGetPhonebookSize:
        {
            MAIN_DEBUG(("EventPbapGetPhonebookSize"));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    MessageSend(&theSink.task , PBAPC_APP_PHONE_BOOK_SIZE , 0 ) ;
                }
                
                theSink.pbapc_data.pbap_command = pbapc_phonebooksize;
            }
                
            lIndicateEvent = FALSE ;
        }
        break;        
        
        case EventPbapSelectPhonebookObject:
        {
            MAIN_DEBUG(("EventPbapSelectPhonebookObject\n"));  

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_browsing_start_flag = 0;
            
            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                theSink.pbapc_data.pbap_active_pb += 1;
                
                if(theSink.pbapc_data.pbap_active_pb > pbap_cch)
                {
                    theSink.pbapc_data.pbap_active_pb = pbap_pb;
                }
            } 

            lIndicateEvent = FALSE ;
        }    
        break; 
        
        case EventPbapBrowseComplete:
        {
            MAIN_DEBUG(("EventPbapBrowseComplete\n"));
        
            /* Set the link policy based on the HFP or A2DP state */
            linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link)); 
            
            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_browsing_start_flag = 0;
            lIndicateEvent = FALSE ;
            
        }
        break;        
        
        
#endif        
        
#ifdef WBS_TEST
        /* TEST EVENTS for WBS testing */
        case EventSetWbsCodecs:
            if(theSink.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theSink.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), FALSE);
            }
            else
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
                theSink.RenegotiateSco = 1;
                HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , FALSE);           
            }
            
        break;
    
        case EventSetWbsCodecsSendBAC:
            if(theSink.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theSink.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), TRUE);
            }
           else
           {
               MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
               theSink.RenegotiateSco = 1;
               HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , TRUE);           
           }
           break;
 
         case EventOverrideResponse:
                   
           if(theSink.FailAudioNegotiation)
           {
               MAIN_DEBUG(("HS : Fail Neg = off\n")) ;
               theSink.FailAudioNegotiation = 0;
           }
           else
           {
               MAIN_DEBUG(("HS : Fail Neg = on\n")) ;
               theSink.FailAudioNegotiation = 1;
           }
       break; 

#endif
    
       case EventCreateAudioConnection:
           MAIN_DEBUG(("HS : Create Audio Connection\n")) ;
           
           CreateAudioConnection();
       break;

#ifdef ENABLE_MAPC
        case EventMapcMsgNotification:
            /* Generate a tone or TTS */
            MAIN_DEBUG(("HS : EventMapcMsgNotification\n")) ;
        break;
        case EventMapcMnsSuccess:
            /* Generate a tone to indicate the mns service success */
            MAIN_DEBUG(("HS : EventMapcMnsSuccess\n")) ;
        break;
        case EventMapcMnsFailed:
            /* Generate a tone to indicate the mns service failed */
            MAIN_DEBUG(("HS : EventMapcMnsFailed\n")) ;
        break;
#endif
            
       case EventEnableIntelligentPowerManagement:
           MAIN_DEBUG(("HS : Enable LBIPM\n")) ;           
            /* enable LBIPM operation */
           theSink.lbipmEnable = 1;          
           /* send plugin current power level */           
           AudioSetPower(powerManagerGetLBIPM());
            /* and store in PS for reading at next power up */
           configManagerWriteSessionData () ;     
       break;
       
       case EventDisableIntelligentPowerManagement:
           MAIN_DEBUG(("HS : Disable LBIPM\n")) ;           
            /* disable LBIPM operation */
           theSink.lbipmEnable = 0;
           /* notify the plugin Low power mode is no longer required */           
           AudioSetPower(powerManagerGetLBIPM());
            /* and store in PS for reading at next power up */
           configManagerWriteSessionData () ; 
       break;
       
       case EventToggleIntelligentPowerManagement:
           MAIN_DEBUG(("HS : Toggle LBIPM\n")) ;
           if(theSink.lbipmEnable)
           {
               MessageSend( &theSink.task , EventDisableIntelligentPowerManagement , 0 ) ;
           }
           else
           {
               MessageSend( &theSink.task , EventEnableIntelligentPowerManagement , 0 ) ;
           }

       break; 
       
        case EventUsbPlayPause:
           MAIN_DEBUG(("HS : EventUsbPlayPause\n")) ;  
           usbPlayPause();
        break;
        case EventUsbStop:
           MAIN_DEBUG(("HS : EventUsbStop\n")) ;  
           usbStop();
        break;
        case EventUsbFwd:
           MAIN_DEBUG(("HS : EventUsbFwd\n")) ;  
           usbFwd();
        break;
        case EventUsbBck:
           MAIN_DEBUG(("HS : EventUsbBck\n")) ;  
           usbBck();
        break;
        case EventUsbMute:
           MAIN_DEBUG(("HS : EventUsbMute")) ;  
           usbMute(); 
        break;
        case EventUsbLowPowerMode:
            /* USB low power mode */
            usbSetBootMode(BOOTMODE_USB_LOW_POWER);
        break;
        case EventUsbDeadBatteryTimeout:
            usbSetVbatDead(FALSE);
        break;

        case EventWiredAudioConnected:
            /* Update audio routing */
            audioHandleRouting(audio_source_none);
        break;
        case EventWiredAudioDisconnected:
            /* Update audio routing */
            audioHandleRouting(audio_source_none);
        break;
            

#ifdef ENABLE_AVRCP   

       case EventAvrcpPlayPause:
            MAIN_DEBUG(("HS : EventAvrcpPlayPause\n")) ;  
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventAvrcpRewindPress);
#if defined (ENABLE_REMOTE) && defined (ENABLE_SOUNDBAR)
            rcResetAvrcpButtonPress();
#endif
#ifdef ENABLE_PEER
            /* Prioritise control to any Peer source device */
            if ( !controlA2DPPeer(EventAvrcpPlayPause) )
#endif
            {
                sinkAvrcpPlayPause();
            }
       break;

      case EventAvrcpPlay:
            MAIN_DEBUG(("HS : EventAvrcpPlay\n")) ;  
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventAvrcpRewindPress);
#if defined (ENABLE_REMOTE) && defined (ENABLE_SOUNDBAR)
            rcResetAvrcpButtonPress();
#endif
#ifdef ENABLE_PEER
            /* Prioritise control to any Peer source device */
            if ( !controlA2DPPeer(EventAvrcpPlay) )
#endif
            {
                sinkAvrcpPlay();
            }
       break;

       case EventAvrcpPause:
            MAIN_DEBUG(("HS : EventAvrcpPause\n")) ;  
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventAvrcpRewindPress);
#if defined (ENABLE_REMOTE) && defined (ENABLE_SOUNDBAR)
            rcResetAvrcpButtonPress();
#endif
#ifdef ENABLE_PEER
            /* Prioritise control to any Peer source device */
            if ( !controlA2DPPeer(EventAvrcpPause) )
#endif
            {
                sinkAvrcpPause();
            }
       break;
            
       case EventAvrcpStop:
            MAIN_DEBUG(("HS : EventAvrcpStop\n")) ; 
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventAvrcpRewindPress);
#if defined (ENABLE_REMOTE) && defined (ENABLE_SOUNDBAR)
            rcResetAvrcpButtonPress();
#endif
#ifdef ENABLE_PEER
            /* Prioritise control to any Peer source device */
            if ( !controlA2DPPeer(EventAvrcpStop) )
#endif
            {
                sinkAvrcpStop();
            }
       break;
            
       case EventAvrcpSkipForward:
           MAIN_DEBUG(("HS : EventAvrcpSkipForward\n")) ;  
           sinkAvrcpSkipForward();
       break;
 
       case EventEnterBootMode2:
            MAIN_DEBUG(("Reboot into different bootmode [2]\n")) ;
           BootSetMode(BOOTMODE_CUSTOM) ;
       break ;
      
       case EventAvrcpSkipBackward:
           MAIN_DEBUG(("HS : EventAvrcpSkipBackward\n")) ; 
           sinkAvrcpSkipBackward();
       break;
            
       case EventAvrcpFastForwardPress:
           MAIN_DEBUG(("HS : EventAvrcpFastForwardPress\n")) ;  
           sinkAvrcpFastForwardPress();
           /* rescehdule a repeat of this message every 1.5 seconds */
           MessageSendLater( &theSink.task , EventAvrcpFastForwardPress , 0 , AVRCP_FF_REW_REPEAT_INTERVAL) ;
       break;
            
       case EventAvrcpFastForwardRelease:
           MAIN_DEBUG(("HS : EventAvrcpFastForwardRelease\n")) ;
           /* cancel any queued FF repeat requests */
           MessageCancelAll (&theSink.task, EventAvrcpFastForwardPress);
           sinkAvrcpFastForwardRelease();
       break;
            
       case EventAvrcpRewindPress:
           MAIN_DEBUG(("HS : EventAvrcpRewindPress\n")) ; 
           /* rescehdule a repeat of this message every 1.8 seconds */
           MessageSendLater( &theSink.task , EventAvrcpRewindPress , 0 , AVRCP_FF_REW_REPEAT_INTERVAL) ;
           sinkAvrcpRewindPress();
       break;
            
       case EventAvrcpRewindRelease:
           MAIN_DEBUG(("HS : EventAvrcpRewindRelease\n")) ; 
           /* cancel any queued FF repeat requests */
           MessageCancelAll (&theSink.task, EventAvrcpRewindPress);
           sinkAvrcpRewindRelease();
       break;
       
       case EventAvrcpToggleActive:
           MAIN_DEBUG(("HS : EventAvrcpToggleActive\n"));
           if (sinkAvrcpGetNumberConnections() > 1)
               sinkAvrcpToggleActiveConnection();
           else
               lIndicateEvent = FALSE;
       break;
       
       case EventAvrcpNextGroup:
           MAIN_DEBUG(("HS : EventAvrcpNextGroup\n"));
           sinkAvrcpNextGroup();
       break;
       
       case EventAvrcpPreviousGroup:
           MAIN_DEBUG(("HS : EventAvrcpPreviousGroup\n"));
           sinkAvrcpPreviousGroup();
       break;

       case EventAvrcpShuffleOff:
           MAIN_DEBUG(("HS : EventAvrcpShuffleOff\n"));
           sinkAvrcpShuffle(AVRCP_SHUFFLE_OFF);
        break;
        
       case EventAvrcpShuffleAllTrack:
           MAIN_DEBUG(("HS : EventAvrcpShuffleAllTrack\n"));
           sinkAvrcpShuffle(AVRCP_SHUFFLE_ALL_TRACK);
        break;
        
       case EventAvrcpShuffleGroup:
           MAIN_DEBUG(("HS : EventAvrcpShuffleGroup\n"));
           sinkAvrcpShuffle(AVRCP_SHUFFLE_GROUP);
        break;

       case EventAvrcpRepeatOff:
           MAIN_DEBUG(("HS : EventAvrcpRepeatOff\n"));
           sinkAvrcpRepeat(AVRCP_REPEAT_OFF);
        break;
        
       case EventAvrcpRepeatSingleTrack:
           MAIN_DEBUG(("HS : EventAvrcpRepeatSingleTrack\n"));
           sinkAvrcpRepeat(AVRCP_REPEAT_SINGLE_TRACK);
        break;        
            
       case EventAvrcpRepeatAllTrack:
           MAIN_DEBUG(("HS : EventAvrcpRepeatAllTrack\n"));           
           sinkAvrcpRepeat(AVRCP_REPEAT_ALL_TRACK);
        break;
        
       case EventAvrcpRepeatGroup:
           MAIN_DEBUG(("HS : EventAvrcpRepeatGroup\n"));
           sinkAvrcpRepeat(AVRCP_REPEAT_GROUP);
        break;
        
#endif       
       
        case EventSwitchAudioMode:           
        {
            /* If A2DP in use and muted set mute */
            AUDIO_MODE_T mode = VolumeCheckA2dpMute() ? AUDIO_MODE_MUTE_SPEAKER : AUDIO_MODE_CONNECTED;
            /* If USB in use set current USB mode */
            usbAudioGetMode(&mode);
            /* cycle through EQ modes */
            theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = A2DP_MUSIC_PROCESSING_FULL_NEXT_EQ_BANK;
            MAIN_DEBUG(("HS : EventSwitchAudioMode %x\n", theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing ));
            AudioSetMode(mode, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        }
        break;              
       
       case EventToggleButtonLocking:
            MAIN_DEBUG(("HS : EventToggleButtonLocking (%d)\n",theSink.buttons_locked));
            if (theSink.buttons_locked)
            {                
                MessageSend( &theSink.task , EventButtonLockingOff , 0 ) ;
            }
            else
            {                
                MessageSend( &theSink.task , EventButtonLockingOn , 0 ) ;
            }
        break;
        
        case EventButtonLockingOn:
            MAIN_DEBUG(("HS : EventButtonLockingOn\n"));
            theSink.buttons_locked = TRUE;            
        break;
        
        case EventButtonLockingOff:
            MAIN_DEBUG(("HS : EventButtonLockingOff\n"));
            theSink.buttons_locked = FALSE;          
        break;
        
        case EventDisableVoicePrompts:
            MAIN_DEBUG(("HS : EventDisableVoicePrompts"));
            /* disable voice prompts */

            /* Play the disable voice prompt before disabling voice prompts */
            if(theSink.tts_enabled == TRUE) /* Check if tts is already enabled */
            {
                TonesPlayEvent( id );
            }

            theSink.tts_enabled = FALSE;
            /* write enable state to pskey user 12 */
            configManagerWriteSessionData () ;                          
        break;
                
        case EventEnableVoicePrompts:
            MAIN_DEBUG(("HS : EventEnableVoicePrompts"));
            /* enable voice prompts */
            theSink.tts_enabled = TRUE;            
            /* write enable state to pskey user 12 */
            configManagerWriteSessionData () ;                          
        break;

        case EventAudioTestMode:
            MAIN_DEBUG(("HS : EventAudioTestMode\n"));        
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventEnterDUTState, 0);
            }      
            enterAudioTestMode();
        break;
        
        case EventToneTestMode:
            MAIN_DEBUG(("HS : EventToneTestMode\n")); 
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventEnterDUTState, 0);
            }      
            enterToneTestMode();
        break;
        
        case EventKeyTestMode:
            MAIN_DEBUG(("HS : EventKeyTestMode\n")); 
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventEnterDUTState, 0);
            }      
            enterKeyTestMode();
        break;

#ifdef ENABLE_SPEECH_RECOGNITION
        case EventSpeechRecognitionStart:
        {
        
            if ( speechRecognitionIsEnabled() )
                speechRecognitionStart() ;
            else
                lIndicateEvent = FALSE; 
        }
        break ;    
        case EventSpeechRecognitionStop:
        {
            if(speechRecognitionIsEnabled() ) 
                speechRecognitionStop() ;    
            else
                lIndicateEvent = FALSE; 
        }    
        break ;
        /* to tune the Speech Recognition using the UFE generate this event */
        case EventSpeechRecognitionTuningStart:
        {
            /* ensure speech recognition is enabled */
            if ( speechRecognitionIsEnabled() )
            {
                /* ensure not already in tuning mode */
                if(!theSink.csr_speech_recognition_tuning_active)
                {
                    theSink.csr_speech_recognition_tuning_active = TRUE;
                    speechRecognitionStart() ;
                }
            }
        } 
        break;       
        
        case EventSpeechRecognitionTuningYes:
        break;
        
        case EventSpeechRecognitionTuningNo:
        break;

#endif

        case EventTestDefrag:
            MAIN_DEBUG(("HS : EventTestDefrag\n")); 
            configManagerFillPs();
        break;

        case EventStreamEstablish:
            MAIN_DEBUG(("HS : EventStreamEstablish[%u]\n", ((EVENT_STREAM_ESTABLISH_T *)message)->priority)); 
            connectA2dpStream( ((EVENT_STREAM_ESTABLISH_T *)message)->priority, 0 );
        break;
        
#ifdef ENABLE_GATT
        case EventA2dpConnected:
            MAIN_DEBUG(("HS : EventA2dpConnected\n"));
            sinkGattUpdateBatteryReportConnection();      
        break;
#endif 
        case EventUpdateAttributes:
            deviceManagerDelayedUpdateAttributes((EVENT_UPDATE_ATTRIBUTES_T*)message);
        break;
        
        case EventPeerSessionConnDisc:
            MAIN_DEBUG(("HS: PeerSessionConnDisc [%d]\n" , lState )) ;
            /*go into pairing mode*/ 
            if ( lState != deviceLimbo)
            {
                /* ensure there is only one device connected to allow peer dev to connect */
                if(deviceManagerNumConnectedDevs() < MAX_A2DP_CONNECTIONS)
                {
                    theSink.inquiry.session = inquiry_session_peer;
                    stateManagerEnterConnDiscoverableState( FALSE );                
                }
                /* no free connections, indicate an error condition */
                else
                {
                    lIndicateEvent = FALSE;
                    MessageSend ( &theSink.task , EventError , 0 ) ;               
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        
        case ( EventPeerSessionInquire ):
            MAIN_DEBUG(("HS: PeerSessionInquire\n"));
            
            /* ensure there is only one device connected to allow peer dev to connect */
            if(deviceManagerNumConnectedDevs() < MAX_A2DP_CONNECTIONS)
            {
                lIndicateEvent = inquiryPair( inquiry_session_peer, FALSE );
            }
            /* no free connections, indicate an error condition */
            else
            {
                lIndicateEvent = FALSE;
                MessageSend ( &theSink.task , EventError , 0 ) ;               
            }
                
        break;
        
        case EventPeerSessionEnd:
        {
#ifdef PEER_SCATTERNET_DEBUG   /* Scatternet debugging only */
            uint16 i;
            for_all_a2dp(i)
            {
                if (theSink.a2dp_link_data)
                {
                    theSink.a2dp_link_data->invert_ag_role[i] = !theSink.a2dp_link_data->invert_ag_role[i];
                    MAIN_DEBUG(("HS: invert_ag_role[%u] = %u\n",i,theSink.a2dp_link_data->invert_ag_role[i]));
                    
                    if (theSink.a2dp_link_data->connected[i] && (theSink.a2dp_link_data->peer_device[i] != remote_device_peer))
                    {
                        linkPolicyUseA2dpSettings( theSink.a2dp_link_data->device_id[i], 
                                                   theSink.a2dp_link_data->stream_id[i], 
                                                   A2dpSignallingGetSink(theSink.a2dp_link_data->device_id[i]) );
                    }
                }
            }
#else   /* Normal operation */
            MAIN_DEBUG(("HS: EventPeerSessionEnd\n"));
            lIndicateEvent = disconnectAllA2dpPeerDevices();
#endif    
        }
        break;
        
        case EventSwapMediaChannel:
            /* attempt to swap media channels, don't indicate event if not successful */
            if(!audioSwapMediaChannel())
                lIndicateEvent = FALSE;
        break;

        /* bass boost enable disable toggle */
        case EventBassBoostEnableDisableToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_BASS_BOOST_BYPASS)
            {
                /* disable bass boost */
                sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,FALSE);       
            }
            else
            {
                /* enable bass boost */
                sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,TRUE);       
            }
        break;
        
        /* bass boost enable indication */
        case EventBassBoostOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable bass boost */
            sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,TRUE);
        break;
                
        /* bass boost disable indication */
        case EventBassBoostOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable bass boost */
            sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,FALSE);
        break;

        /* 3D enhancement enable disable toggle */              
        case Event3DEnhancementEnableDisableToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_SPATIAL_BYPASS)
            {
                /* disable 3d */
                sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,FALSE);
            }
            else
            {
                /* enable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,TRUE);
            }
        break;
        
        /* 3D enhancement enable indication */
        case Event3DEnhancementOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,TRUE);
        break;
        
        /* 3D enhancement disable indication */
        case Event3DEnhancementOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,FALSE);
        break;     
        
        /* check whether the Audio Amplifier drive can be turned off after playing
           a tone or voice prompt */
        case EventCheckAudioAmpDrive:
            /* cancel any pending messages */
            MessageCancelAll( &theSink.task , EventCheckAudioAmpDrive);
            /* when the device is no longer routing audio tot he speaker then
               turn off the audio amplifier */
            if((!IsAudioBusy()) && (!theSink.routed_audio))
            {
                MAIN_DEBUG (( "HS : EventCheckAudioAmpDrive turn off amp\n" ));     
                PioSetPio ( theSink.conf1->PIOIO.pio_outputs.DeviceAudioActivePIO , pio_drive, FALSE) ;
            }
            /* audio is still busy, check again later */            
            else
            {
                MAIN_DEBUG (( "HS : EventCheckAudioAmpDrive still busy, reschedule\n" ));     
                MessageSendLater(&theSink.task , EventCheckAudioAmpDrive, 0, CHECK_AUDIO_AMP_PIO_DRIVE_DELAY);    
            }
        break;

        /* external microphone has been connected */
        case EventExternalMicConnected:
            theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_FITTED;
            /* if routing audio update the mic source for dsp apps that support it */
            if(theSink.routed_audio)            
                AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        break;
        
        /* external microphone has been disconnected */
        case EventExternalMicDisconnected:
            theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_NOT_FITTED;
            /* if routing audio update the mic source for dsp apps that support it */
            if(theSink.routed_audio)            
                AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
       break;

       /* event to enable the simple speech recognition functionality, persistant over power cycle */
       case EventEnableSSR:
            theSink.ssr_enabled = TRUE;
       break;
       
       /* event to disable the simple speech recognition functionality, persistant over power cycle */
       case EventDisableSSR:
            theSink.ssr_enabled = FALSE;
       break;

       /* NFC tag detected, determine action based on current connection state */
       case EventNFCTagDetected:
            /* if not connected to an AG, go straight into pairing mode */
            if(stateManagerGetState() < deviceConnected)
                stateManagerEnterConnDiscoverableState( TRUE );
            /* otherwise see if audio is on the device and attempt
               to transfer audio if not present */
            else
               sinkCheckForAudioTransfer(); 
       break;               

#ifdef ENABLE_AVRCP
       case EventResetAvrcpMode:
            {  
                uint16 index = *(uint16 *) message;
                lIndicateEvent = FALSE ;
                theSink.avrcp_link_data->link_active[index] =  FALSE;
            }
       break;
#endif       

       /* check whether the current routed audio is still the correct one and
          change sources if appropriate */
       case EventCheckAudioRouting:
            /* check audio routing */
            audioHandleRouting(audio_source_none);    
            /* don't indicate event as may be generated by USB prior to configuration
               being loaded */
            lIndicateEvent = FALSE;
       break;
               
#ifdef ENABLE_FM
       case EventFmOn:
        {
            /* enable the FM hardware if not already enabled */           
            MAIN_DEBUG(("HS : EventFmOn\n"));
            /* FM not available unless device is powered on */
            if(stateManagerGetState()!=deviceLimbo)
            {
                /* ensure fm is not already enabled */
                if(!theSink.conf2->sink_fm_data.fmRxOn)
                {
                    /* enable FM hardware */
                    sinkFmInit(FM_ENABLE_RX);
                }
                else
                {
                    MAIN_DEBUG (( "HS : Cannot enable FM as already enabled\n"));
                    lIndicateEvent=FALSE;
                }
            }
        }
        break;

       case EventFmOff:
            MAIN_DEBUG(("HS : EventFmOff\n"));
            
            if (theSink.conf2->sink_fm_data.fmRxOn)
            {
                sinkFmRxAudioDisconnect();
                sinkFmRxPowerOff();
                theSink.conf2->sink_fm_data.fmRxOn=FALSE;
                
#ifdef ENABLE_DISPLAY   
                displayShowSimpleText(DISPLAYSTR_CLEAR,1);
#endif           
            }
            else
            {
                MAIN_DEBUG (( "HS : FM already stopped \n"));
                lIndicateEvent=FALSE;
            }
        break;

       case EventFmRxTuneUp:
            MAIN_DEBUG(("HS : EventFmRxTuneUp\n")); 

            if (theSink.conf2->sink_fm_data.fmRxOn)
            {
                sinkFmRxTuneUp();
            }            
            else
            {
                MAIN_DEBUG (( "HS : Cannot tune up FM as audio is busy or FM is not ON \n"));
                lIndicateEvent=FALSE;
            }

        break;

       case EventFmRxTuneDown:
            MAIN_DEBUG(("HS : EventFmRxTuneDown\n")); 
            if (theSink.conf2->sink_fm_data.fmRxOn)
            {
                sinkFmRxTuneDown();
            }
            else
            {
                MAIN_DEBUG (( "HS : Cannot tune down FM as audio is busy or FM is not ON \n"));
                lIndicateEvent=FALSE;
            }
        break;
            
       case EventFmRxStore:
            MAIN_DEBUG(("HS : EventFmRxStore\n")); 
            if (theSink.conf2->sink_fm_data.fmRxOn)
            {
                sinkFmRxStoreFreq(theSink.conf2->sink_fm_data.fmRxTunedFreq);
            }            
            else
            {
                MAIN_DEBUG (( "HS : Cannot store FM freq as FM is not ON \n"));
                lIndicateEvent=FALSE;
            }
        break;

       case EventFmRxTuneToStore:
            MAIN_DEBUG(("HS : EventFmRxTuneToStore\n")); 
            if (theSink.conf2->sink_fm_data.fmRxOn)
            {
                sinkFmRxTuneToStore(theSink.conf2->sink_fm_data.fmRxTunedFreq);
            }
            else
            {
                MAIN_DEBUG (( "HS : Cannot playstore FM as FM is not ON \n"));
                lIndicateEvent=FALSE;
            }

        break;
        
       case EventFmRxErase:
            MAIN_DEBUG(("HS : EventFmRxErase\n")); 
            if (theSink.conf2->sink_fm_data.fmRxOn)
            {
                sinkFmRxEraseFreq(theSink.conf2->sink_fm_data.fmRxTunedFreq);
            }
            else
            {
                MAIN_DEBUG (( "HS : Cannot erase FM freq as FM is not ON \n"));
                lIndicateEvent=FALSE;
            }

        break;
#endif /* ENABLE_FM */
       
               
/* when using manual audio source routing, these events can be used to switch
   between audio sources */
#ifdef ENABLE_SOUNDBAR

       /* manually switch to wired audio source */
       case EventSelectWiredAudioSource:
            if (stateManagerGetState() != deviceLimbo)
                audioSwitchToAudioSource(audio_source_WIRED);
       break;
       
       /* manually switch to USB audio source */
       case EventSelectUSBAudioSource:
            if (stateManagerGetState() != deviceLimbo)
                audioSwitchToAudioSource(audio_source_USB);
       break;
       
       /* manually switch to AG1 audio source */
       case EventSelectAG1AudioSource:
            if (stateManagerGetState() != deviceLimbo)
                audioSwitchToAudioSource(audio_source_AG1);
       break;
       
       /* manually switch to AG2 audio source */
       case EventSelectAG2AudioSource:
            if (stateManagerGetState() != deviceLimbo)
                audioSwitchToAudioSource(audio_source_AG2);
       break;
       
       /* manually switch to AG2 audio source */
       case EventSelectFMAudioSource:
            if (stateManagerGetState() != deviceLimbo)
                audioSwitchToAudioSource(audio_source_FM);
       break;
        
       /* manually switch to no audio source - disconnect any audio currently routed */
       case EventSelectNoAudioSource:
            if (stateManagerGetState() != deviceLimbo)
                audioSwitchToAudioSource(audio_source_none);
       break;

       /* Toggle between wired source and bluetooth source */
       case EventSwitchToNextAudioSource:  
            MAIN_DEBUG (( "HS : EventToggleSource for Soundbar\n"));     
            /* only allow source switching when powered on */
            if (stateManagerGetState() != deviceLimbo)
            {
                audioSwitchToNextAudioSource();
            }
            else
            {
                /* don't indicate event if not powered on */
                lIndicateEvent = FALSE;
            }
       break;


       case EventRCVolumeDown:
            MAIN_DEBUG (( "HS : EventRCVolumeDown for Soundbar\n"));
            MessageSendLater(&theSink.task, EventVolumeDown, 0,200);
       break;
       
       case EventRCVolumeUp:
            MAIN_DEBUG (( "HS : EventRCVolumeUp for Soundbar\n"));
            MessageSendLater(&theSink.task, EventVolumeUp, 0,200);
       break;
#endif /* ENABLE_SOUNDBAR */
       
#ifdef ENABLE_SUBWOOFER
       case EventSubwooferStartInquiry:
            handleEventSubwooferStartInquiry();
       break;
       
       case EventSubwooferCheckPairing:
            handleEventSubwooferCheckPairing();
       break;
       
       case EventSubwooferOpenLLMedia:
            /* open a Low Latency media connection */
            handleEventSubwooferOpenLLMedia();
       break;
       
       case EventSubwooferOpenStdMedia:
            /* open a standard latency media connection */
            handleEventSubwooferOpenStdMedia();
       break;
       
       case EventSubwooferVolumeUp:
            handleEventSubwooferVolumeUp();
       break;
       
       case EventSubwooferVolumeDown:
            handleEventSubwooferVolumeDown();
       break;
       
       case EventSubwooferCloseMedia:
            handleEventSubwooferCloseMedia();
       break;
       
       case EventSubwooferStartStreaming:
            handleEventSubwooferStartStreaming();
       break;
       
       case EventSubwooferSuspendStreaming:
            handleEventSubwooferSuspendStreaming();
       break;
       
       case EventSubwooferDeletePairing:
            handleEventSubwooferDeletePairing();
       break;
#endif

       case EventEnterDriverlessDFUMode:
            LoaderModeEnter();
       break;
	   
	#ifdef MAX14521E_EL_RAMP_DRIVER
	case EventELPatternMode:	/*EL_RAMP Pattern interval*/
		if(EL_Ramp_GetStatus())
		{
			if(theSink.el_pattern_state == PATTERN_0)
			{
				EL_Ramp_On();
			}
			else
			{
				if(theSink.el_pattern_on)
				{
					EL_Ramp_On();
				}
				else
				{
					EL_Ramp_Off();
				}
				theSink.el_pattern_on ^= 1;
				MessageCancelAll (&theSink.task, EventELPatternMode);
				MessageSendLater( &theSink.task , EventELPatternMode , 0 , theSink.el_pattern_state * DEFAULT_PATTERN_INTERVAL) ;
			}
					
			/*MessageCancelAll (&theSink.task, EventGaiaUser6);*/
			
		}
	break;
	
	case EventGaiaUser7:	/*EL_RAMP Pattern*/
		if(EL_Ramp_GetStatus())
		{
			if(theSink.el_pattern_state >= PATTERN_4)
				theSink.el_pattern_state = PATTERN_0;
			else
				theSink.el_pattern_state++;
			
			MessageCancelAll (&theSink.task, EventELPatternMode);
			MessageSendLater( &theSink.task , EventELPatternMode , 0 , DEFAULT_PATTERN_INTERVAL) ;
		}
	break;
	
	case EventGaiaUser8:	/*EL_RAMP*/
		if(EL_Ramp_GetStatus())
		{
			EL_Ramp_Off();
			EL_Ramp_Disable();
			MAIN_DEBUG (( "HS : EL_RAMP disabled [%x]\n", id ));     
			MessageCancelAll (&theSink.task, EventELPatternMode);

			
			MessageCancelAll (&theSink.task, EventXYZSamplingMode);
			MMA845x_Standby();
		}
		else
		{
			EL_Ramp_Enable();
			EL_Ramp_On();
			MAIN_DEBUG (( "HS : EL_RAMP enabled [%x]\n", id ));     

			MMA845x_Active();
			full_scale= FULL_SCALE_2G;
			MessageSendLater( &theSink.task , EventXYZSamplingMode , 0 , DEFAULT_XYZ_SAMPLING_INTERVAL) ;
			#ifdef PEDOMETER_SUPPORTED
			 pedometer_init();
			#endif
		}
	break;
	#endif

	case EventXYZSamplingMode:	
	{
		RegisterFlag.Byte = IIC_RegRead(STATUS_00_REG);

		
		if ((RegisterFlag.Byte & ZYXDR_BIT))
		{
			#if 0
			/*
			**  Read the  XYZ sample data
			*/
			if (deviceID>3)
			{
				IIC_RegReadN(OUT_X_MSB_REG, 3, &value[0]);
			} 
			else 
			{
				IIC_RegReadN(OUT_X_MSB_REG, 6, &value[0]);
			}

			/*
			**  Output results
			*/
			#endif
			#if 0
			if(RegisterFlag.Byte & 1)
			{
				data_acc[0] = ((int16)((IIC_RegRead(OUT_X_MSB_REG)<<8) | IIC_RegRead(OUT_X_LSB_REG)))>>4;
			}
				
			if(RegisterFlag.Byte & 2)
			{
				data_acc[1] = ((int16)((IIC_RegRead(OUT_Y_MSB_REG)<<8) | IIC_RegRead(OUT_Y_LSB_REG)))>>4;
			}
				
			if(RegisterFlag.Byte & 4)
			{
				data_acc[2] = ((int16)((IIC_RegRead(OUT_Z_MSB_REG)<<8) | IIC_RegRead(OUT_Z_LSB_REG)))>>4;
			}
			#endif
			if(RegisterFlag.Byte & 1)
			{
				data_acc[0] = (int16)IIC_RegRead(OUT_X_MSB_REG);
			}
				
			if(RegisterFlag.Byte & 2)
			{
				data_acc[1] = (int16)IIC_RegRead(OUT_Y_MSB_REG);
			}
				
			if(RegisterFlag.Byte & 4)
			{
				data_acc[2] = (int16)IIC_RegRead(OUT_Z_MSB_REG);
			}
		}
		#ifdef PEDOMETER_SUPPORTED

		 data_acc[0] *= -1;
	     data_acc[1] *= -1;
		 data_accx[0] = data_acc[0]; 
		 data_accx[1] = data_acc[1]; 
		 data_accx[2] = data_acc[2];

		 pedometer(data_accx);
		 pedo_cnt = pedometer_get_step();

		 if(debug_display_cnt++ == 30)
		 {
		 	debug_display_cnt = 0;
			#ifdef MMA8452Q_TERMINAL_SUPPORTED
			OutputTerminal (FBID_FULL_XYZ_SAMPLE, &value[0]);
			#endif	
		 	MAIN_DEBUG(( "HS : pedo_cnt [%d], x:%d, y : %d, z : %d \n", pedo_cnt, data_acc[0], data_acc[1], data_acc[2]));

			if(old_pedo_cnt != pedo_cnt)
			{
				PsStore(35, &pedo_cnt, sizeof(pedo_cnt));
				if(theSink.el_pattern_state == PATTERN_0)
					EL_Ramp_On();
			}
			else
			{
				if(theSink.el_pattern_state == PATTERN_0)
					EL_Ramp_Off();
			}
			old_pedo_cnt = pedo_cnt;
			#if 0
			#define gSteps                   gPedoPtr->steps
			#define gEdgeType                gPedoPtr->edge_type
			#define gLastType                gPedoPtr->last_type
			#define gIntervalThres           gPedoPtr->interval_thres
			#define gAmpThres                gPedoPtr->amp_thres
			#define buffer                   gPedoPtr->filter
			#define gStepsElapsed            gPedoPtr->steps_elapsed
			#define gLastValue               gPedoPtr->last_value
			#define gLastPeakValue           gPedoPtr->last_peakvalue
			#define gLastValleyValue         gPedoPtr->last_valleyvalue
			#endif
		 } 
		#endif
		
		MessageCancelAll (&theSink.task, EventXYZSamplingMode);
		MessageSendLater( &theSink.task , EventXYZSamplingMode , 0 , DEFAULT_XYZ_SAMPLING_INTERVAL) ;
	}
	break;


       default :
            MAIN_DEBUG (( "HS : UE unhandled!! [%x]\n", id ));     
        break ;  
        
    }   
    
        /* Inform theevent indications that we have received a user event*/
        /* For all events except the end event notification as this will just end up here again calling itself...*/
    if ( lIndicateEvent )
    {
        if ( id != EventLEDEventComplete )
        {
            LEDManagerIndicateEvent ( id ) ;
        }           
        
        TonesPlayEvent ( id ) ;
        
        ATCommandPlayEvent ( id ) ;
    }
    
#ifdef ENABLE_GAIA
    gaiaReportEvent(id);
#endif
    
#ifdef TEST_HARNESS 
    vm2host_send_event(id);
#endif
    
#ifdef DEBUG_MALLOC
    printf("MAIN: Event [%x] Available SLOTS:[%d]\n" ,id, VmGetAvailableAllocations() ) ;
#endif          
    
}


/*************************************************************************
NAME    
    handleHFPMessage

DESCRIPTION
    handles the messages from the user events

RETURNS

*/
static void handleHFPMessage  ( Task task, MessageId id, Message message )
{   
    MAIN_DEBUG(("HFP = [%x]\n", id)) ;   
    
    switch(id)
    {
        /* -- Handsfree Profile Library Messages -- */
    case HFP_INIT_CFM:              
        {
            /* Init configuration that is required now */
            uint32 ClassOfDevice = 0 ;

            InitEarlyUserFeatures();       
            MAIN_DEBUG(("HFP_INIT_CFM - enable streaming[%x]\n", theSink.features.EnableA2dpStreaming)) ;   
        
            PsFullRetrieve( 0x0003, &ClassOfDevice , sizeof(uint32) );    
                
            if (ClassOfDevice == 0 )
            {
                if(theSink.features.EnableA2dpStreaming)
                {
#ifdef ENABLE_SOUNDBAR                    
                    ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_COD_RENDER | AV_MAJOR_DEVICE_CLASS | AV_MINOR_SPEAKER);  
#else
                    ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_COD_RENDER | AV_MAJOR_DEVICE_CLASS | AV_MINOR_HEADPHONES);        
#endif                    
                }
                else
                {
                    ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS | AV_MINOR_HEADSET);    
                }
        
            }
            else /*use the value from the PSKEY*/
            {
                ConnectionWriteClassOfDevice( ClassOfDevice ) ;
            }
            
            if  ( stateManagerGetState() == deviceLimbo ) 
            {
                if ( ((HFP_INIT_CFM_T*)message)->status == hfp_success )
                    sinkInitComplete( (HFP_INIT_CFM_T*)message );
                else
                    Panic();                
            }        
        }
        
    break;
 
    case HFP_SLC_CONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_CONNECT_IND\n"));
        if (stateManagerGetState() != deviceLimbo)
        {   
            sinkHandleSlcConnectInd((HFP_SLC_CONNECT_IND_T *) message);
        }
    break;

    case HFP_SLC_CONNECT_CFM:
        MAIN_DEBUG(("HFP_SLC_CONNECT_CFM [%x]\n", ((HFP_SLC_CONNECT_CFM_T *) message)->status ));
        if (stateManagerGetState() == deviceLimbo)
        {
            if ( ((HFP_SLC_CONNECT_CFM_T *) message)->status == hfp_success )
            {
                /*A connection has been made and we are now logically off*/
                sinkDisconnectAllSlc();   
            }
        }
        else
        {
            sinkHandleSlcConnectCfm((HFP_SLC_CONNECT_CFM_T *) message);
        }
                            
        break;
        
    case HFP_SLC_LINK_LOSS_IND:
        MAIN_DEBUG(("HFP_SLC_LINK_LOSS_IND\n"));
        slcHandleLinkLossInd((HFP_SLC_LINK_LOSS_IND_T*)message);
    break;
    
    case HFP_SLC_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_DISCONNECT_IND\n"));
        MAIN_DEBUG(("Handle Disconnect\n"));
        sinkHandleSlcDisconnectInd((HFP_SLC_DISCONNECT_IND_T *) message);
    break;
    case HFP_SERVICE_IND:
        MAIN_DEBUG(("HFP_SERVICE_IND [%x]\n" , ((HFP_SERVICE_IND_T*)message)->service  ));
        indicatorsHandleServiceInd ( ((HFP_SERVICE_IND_T*)message) ) ;       
    break;
    /* indication of call status information, sent whenever a change in call status 
       occurs within the hfp lib */
    case HFP_CALL_STATE_IND:
        /* the Call Handler will perform device state changes and be
           used to determine multipoint functionality */
        /* don't process call indications if in limbo mode */
        if(stateManagerGetState()!= deviceLimbo)
            sinkHandleCallInd((HFP_CALL_STATE_IND_T*)message);            
    break;

    case HFP_RING_IND:
        MAIN_DEBUG(("HFP_RING_IND\n"));     
        sinkHandleRingInd((HFP_RING_IND_T *)message);
    break;
    case HFP_VOICE_TAG_NUMBER_IND:
        MAIN_DEBUG(("HFP_VOICE_TAG_NUMBER_IND\n"));
        sinkWriteStoredNumber((HFP_VOICE_TAG_NUMBER_IND_T*)message);
    break;
    case HFP_DIAL_LAST_NUMBER_CFM:
        MAIN_DEBUG(("HFP_LAST_NUMBER_REDIAL_CFM\n"));       
        handleHFPStatusCFM (((HFP_DIAL_LAST_NUMBER_CFM_T*)message)->status ) ;
    break;      
    case HFP_DIAL_NUMBER_CFM:
        MAIN_DEBUG(("HFP_DIAL_NUMBER_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_NUMBER_CFM_T *) message)->status));
        handleHFPStatusCFM (((HFP_DIAL_NUMBER_CFM_T*)message)->status ) ;    
    break;
    case HFP_DIAL_MEMORY_CFM:
        MAIN_DEBUG(("HFP_DIAL_MEMORY_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_MEMORY_CFM_T *) message)->status));        
    break ;     
    case HFP_CALL_ANSWER_CFM:
        MAIN_DEBUG(("HFP_ANSWER_CALL_CFM\n"));
    break;
    case HFP_CALL_TERMINATE_CFM:
        MAIN_DEBUG(("HFP_TERMINATE_CALL_CFM %d\n", stateManagerGetState()));       
    break;
    case HFP_VOICE_RECOGNITION_IND:
        MAIN_DEBUG(("HS: HFP_VOICE_RECOGNITION_IND_T [%c]\n" ,TRUE_OR_FALSE( ((HFP_VOICE_RECOGNITION_IND_T* )message)->enable) )) ;            
            /*update the state of the voice dialling on the back of the indication*/
        theSink.VoiceRecognitionIsActive = ((HFP_VOICE_RECOGNITION_IND_T* ) message)->enable ;            
    break;
    case HFP_VOICE_RECOGNITION_ENABLE_CFM:
        MAIN_DEBUG(("HFP_VOICE_RECOGNITION_ENABLE_CFM s[%d] w[%d]i", (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) , theSink.VoiceRecognitionIsActive));

            /*if the cfm is in error then we did not succeed - toggle */
        if  ( (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) )
            theSink.VoiceRecognitionIsActive = 0 ;
            
        MAIN_DEBUG(("[%d]\n", theSink.VoiceRecognitionIsActive));
        
        handleHFPStatusCFM (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) ;            
    break;
    case HFP_CALLER_ID_ENABLE_CFM:
        MAIN_DEBUG(("HFP_CALLER_ID_ENABLE_CFM\n"));
    break;
    case HFP_VOLUME_SYNC_SPEAKER_GAIN_IND:
    {
        HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *) message;

        MAIN_DEBUG(("HFP_VOLUME_SYNC_SPEAKER_GAIN_IND %d\n", ind->volume_gain));        

        VolumeHandleSpeakerGainInd(ind);
    }
    break;
    case HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND:
    {
        HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T*)message;
        MAIN_DEBUG(("HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND %d\n", ind->mic_gain));
        if(theSink.features.EnableSyncMuteMicrophones)
        {
            VolumeSetMicrophoneGainCheckMute(ind->priority, ind->mic_gain);
        }
    }
    
    break;
    
    case HFP_CALLER_ID_IND:
        {
            HFP_CALLER_ID_IND_T *ind = (HFP_CALLER_ID_IND_T *) message;
 
            /* ensure this is not a HSP profile */
            DEBUG(("HFP_CALLER_ID_IND number %s", ind->caller_info + ind->offset_number));
            DEBUG((" name %s\n", ind->caller_info + ind->offset_name));
            
            /* Show name or number on display */
            if (ind->size_name)
                displayShowSimpleText((char *) ind->caller_info + ind->offset_name, 1);
            
            else
                displayShowSimpleText((char *) ind->caller_info + ind->offset_number, 1);
                
            /* Attempt to play caller name */
            if(!TTSPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
            {
                /* Caller name not present or not supported, try to play number */
                TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
            }
        }
    
    break;           
    
    case HFP_UNRECOGNISED_AT_CMD_IND:
    {
        sinkHandleUnrecognisedATCmd( (HFP_UNRECOGNISED_AT_CMD_IND_T*)message ) ;
    }
    break ;

    case HFP_HS_BUTTON_PRESS_CFM:
        {
            MAIN_DEBUG(("HFP_HS_BUTTON_PRESS_CFM\n")) ;
        }
    break ;
     /*****************************************************************/

#ifdef THREE_WAY_CALLING    
    case HFP_CALL_WAITING_ENABLE_CFM :
            MAIN_DEBUG(("HS3 : HFP_CALL_WAITING_ENABLE_CFM_T [%c]\n", (((HFP_CALL_WAITING_ENABLE_CFM_T * )message)->status == hfp_success) ?'T':'F' )) ;
    break ;    
    case HFP_CALL_WAITING_IND:
        {
            /* pass the indication to the multipoint handler which will determine if the call waiting tone needs
               to be played, this will depend upon whether the indication has come from the AG with
               the currently routed audio */
            mpHandleCallWaitingInd((HFP_CALL_WAITING_IND_T *)message);
        }
    break;

#endif  
    case HFP_SUBSCRIBER_NUMBERS_CFM:
        MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBERS_CFM [%c]\n" , (((HFP_SUBSCRIBER_NUMBERS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' )) ;
    break ;
    case HFP_SUBSCRIBER_NUMBER_IND:
#ifdef DEBUG_MAIN            
    {
        uint16 i=0;
            
        MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBER_IND [%d]\n" , ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->service )) ;
        for (i=0;i< ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->size_number ; i++)
        {
            MAIN_DEBUG(("%c", ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->number[i])) ;
        }
        MAIN_DEBUG(("\n")) ;
    } 
#endif
    break ;
    case HFP_CURRENT_CALLS_CFM:
        MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_CFM [%c]\n", (((HFP_CURRENT_CALLS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' )) ;
    break ;
    case HFP_CURRENT_CALLS_IND:
        MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_IND id[%d] mult[%d] status[%d]\n" ,
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->call_idx , 
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->multiparty  , 
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->status)) ;
    break;
    case HFP_AUDIO_CONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_CONNECT_IND\n")) ;
        audioHandleSyncConnectInd( (HFP_AUDIO_CONNECT_IND_T *)message ) ;
    break ;
    case HFP_AUDIO_CONNECT_CFM:
        MAIN_DEBUG(("HFP_AUDIO_CONNECT_CFM[%x][%x][%s%s%s] r[%d]t[%d]\n", ((HFP_AUDIO_CONNECT_CFM_T *)message)->status ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->audio_sink ,
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_sco) ? "SCO" : "" )      ,  
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_esco) ? "eSCO" : "" )    ,
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_unknown) ? "unk?" : "" ) ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->rx_bandwidth ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->tx_bandwidth 
                                                      )) ;
        /* should the device receive a sco connect cfm in limbo state */
        if (stateManagerGetState() == deviceLimbo)
        {
            /* confirm that it connected successfully before disconnecting it */
            if (((HFP_AUDIO_CONNECT_CFM_T *)message)->status == hfp_audio_connect_no_hfp_link)
            {
                MAIN_DEBUG(("HFP_AUDIO_CONNECT_CFM in limbo state, disconnect it\n" ));
                ConnectionSyncDisconnect(((HFP_AUDIO_CONNECT_CFM_T *)message)->audio_sink, hci_error_oetc_user);
            }
        }
        /* not in limbo state, process sco connect indication */
        else
        {      
            audioHandleSyncConnectCfm((HFP_AUDIO_CONNECT_CFM_T *)message);            
        }               
    break ;
    case HFP_AUDIO_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_DISCONNECT_IND [%x]\n", ((HFP_AUDIO_DISCONNECT_IND_T *)message)->status)) ;
        audioHandleSyncDisconnectInd ((HFP_AUDIO_DISCONNECT_IND_T *)message) ;
    break ;
    case HFP_SIGNAL_IND:
        MAIN_DEBUG(("HS: HFP_SIGNAL_IND [%d]\n", ((HFP_SIGNAL_IND_T* )message)->signal )) ; 
    break ;
    case HFP_ROAM_IND:
        MAIN_DEBUG(("HS: HFP_ROAM_IND [%d]\n", ((HFP_ROAM_IND_T* )message)->roam )) ;
    break; 
    case HFP_BATTCHG_IND:     
        MAIN_DEBUG(("HS: HFP_BATTCHG_IND [%d]\n", ((HFP_BATTCHG_IND_T* )message)->battchg )) ;
    break;
    
/*******************************************************************/
    
    case HFP_CSR_FEATURES_TEXT_IND:
        csr2csrHandleTxtInd () ;
    break ;
    
    case HFP_CSR_FEATURES_NEW_SMS_IND:
       csr2csrHandleSmsInd () ;   
    break ;
    
    case HFP_CSR_FEATURES_GET_SMS_CFM:
       csr2csrHandleSmsCfm() ;
    break ;
    
    case HFP_CSR_FEATURES_BATTERY_LEVEL_REQUEST_IND:
       csr2csrHandleAgBatteryRequestInd() ;
    break ;
    
/*******************************************************************/
    
/*******************************************************************/

    /*******************************/
    
    default :
        MAIN_DEBUG(("HS :  HFP ? [%x]\n",id)) ;
    break ;
    }
}

/*************************************************************************
NAME    
    handleCodecMessage
    
DESCRIPTION
    handles the codec Messages

RETURNS
    
*/
static void handleCodecMessage  ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("CODEC MSG received [%x]\n", id)) ;
      
    if (id == CODEC_INIT_CFM ) 
    {       /* The codec is now initialised */
    
        if ( ((CODEC_INIT_CFM_T*)message)->status == codec_success) 
        {
            MAIN_DEBUG(("CODEC_INIT_CFM\n"));   
            sinkHfpInit();
            theSink.codec_task = ((CODEC_INIT_CFM_T*)message)->codecTask ;                   
        }
        else
        {
            Panic();
        }
    }
}

/* Handle any audio plugin messages */
static void handleAudioPluginMessage( Task task, MessageId id, Message message )
{
    switch (id)
    {        
        case AUDIO_PLUGIN_DSP_IND:
            /* Clock mismatch rate, sent from the DSP via the a2dp decoder common plugin? */
            if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE)
            {
                handleA2DPStoreClockMismatchRate(((AUDIO_PLUGIN_DSP_IND_T*)message)->value[0]);
            }
            /* Current EQ bank, sent from the DSP via the a2dp decoder common plugin? */
            else if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == A2DP_MUSIC_MSG_CUR_EQ_BANK)
            {
                handleA2DPStoreCurrentEqBank(((AUDIO_PLUGIN_DSP_IND_T*)message)->value[0]);
            }
            /* Current enhancements, sent from the DSP via the a2dp decoder common plugin? */
            else if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == A2DP_MUSIC_MSG_ENHANCEMENTS)
            {
                handleA2DPStoreEnhancements(((~((AUDIO_PLUGIN_DSP_IND_T*)message)->value[1]) & (MUSIC_CONFIG_SUB_WOOFER_BYPASS|MUSIC_CONFIG_BASS_BOOST_BYPASS|MUSIC_CONFIG_SPATIAL_BYPASS)));
            }
        break;
            
        /* indication that the DSP is ready to accept data ensuring no audio samples are disposed of */    
        case AUDIO_PLUGIN_DSP_READY_FOR_DATA:
            /* ensure dsp is up and running */
            if(((AUDIO_PLUGIN_DSP_READY_FOR_DATA_T*)message)->dsp_status == DSP_RUNNING)
                MAIN_DEBUG(("HS :  DSP ready for data\n")) ;
            
#ifdef ENABLE_SUBWOOFER     
            /* configure the subwoofer type when the dsp is up and running */
            if(SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_STANDARD)            
                AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_L2CAP, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));    
            else if(SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_LOW_LATENCY)                            
                AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_ESCO, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));    
#endif            
        break;
      
        default:
            MAIN_DEBUG(("HS :  AUDIO ? [%x]\n",id)) ;
        break ;           
    }   
}

#ifdef ENABLE_DISPLAY
/* Handle any display plugin messages */
static void handleDisplayPluginMessage( Task task, MessageId id, Message message )
{
    switch (id)
    {        
    case DISPLAY_PLUGIN_INIT_IND:
        {
            DISPLAY_PLUGIN_INIT_IND_T *m = (DISPLAY_PLUGIN_INIT_IND_T *) message;
            MAIN_DEBUG(("HS :  DISPLAY INIT: %u\n", m->result));
            if (m->result)
            {
                if (powerManagerIsChargerConnected() && (stateManagerGetState() == deviceLimbo) )
                {
                    /* indicate charging if in limbo */
                    displaySetState(TRUE);
                    displayShowText(DISPLAYSTR_CHARGING,  strlen(DISPLAYSTR_CHARGING), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 0);
                    displayUpdateVolume(0); 
                    displayUpdateBatteryLevel(TRUE);  
                }
                else if (stateManagerGetState() != deviceLimbo)
                {          
                    /* if this init occurs and not in limbo, turn the display on */
                    displaySetState(TRUE);                    
                    displayShowText(DISPLAYSTR_HELLO,  strlen(DISPLAYSTR_HELLO), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 10);
                    displayUpdateVolume(theSink.features.DefaultVolume);  
                    /* update battery display */
                    displayUpdateBatteryLevel(FALSE); 
                }                             
            }
        }
        break;
        
    default:
        MAIN_DEBUG(("HS :  DISPLAY ? [%x]\n",id)) ;
        break ;           
    }   
}
#endif /* ENABLE_DISPLAY */

#ifdef ENABLE_FM
/* Handle any FM plugin messages */
static void handleFmPluginMessage(Task task, MessageId id, Message message)
{
    switch (id)
    {        
        /* received when the FM hardware has been initialised and tuned to the
           last used frequency */
        case FM_PLUGIN_INIT_IND:
        {
            FM_PLUGIN_INIT_IND_T *m = (FM_PLUGIN_INIT_IND_T*) message;
            MAIN_DEBUG(("HS: FM INIT: %d\n", m->result));

            if (m->result)
            {
                /*Set vol to default during init, FM volume is scaled 0 to 3F*/                
                theSink.conf2->sink_fm_data.fmVol = theSink.features.DefaultVolume;
                /* set the fm receiver hardware to default volume level (0x3F) */
                sinkFmRxUpdateVolume(theSink.conf2->sink_fm_data.fmVol);
                /* set flag to indicate FM audio is now available */
                theSink.conf2->sink_fm_data.fmRxOn=TRUE;
                /* connect the FM audio if no other audio sources are avilable */
                audioHandleRouting(audio_source_none);
            }
        }
        break;

        /* received when tuning is complete, the frequency tunes to is returned
           within the message, this is stored in persistant store */
        case FM_PLUGIN_TUNE_COMPLETE_IND:
        {
            FM_PLUGIN_TUNE_COMPLETE_IND_T *m = (FM_PLUGIN_TUNE_COMPLETE_IND_T*) message;
            MAIN_DEBUG(("HS: FM_PLUGIN_TUNE_COMPLETE_IND: %d\n", m->result));

            if (m->result)
            {            
                /* valid the returned frequency and store for later writing to ps session data */
                if (m->tuned_freq!=0x0000) 
                {
                    theSink.conf2->sink_fm_data.fmRxTunedFreq=m->tuned_freq;

                    /*Display new frequency, clear older display*/
#ifdef ENABLE_DISPLAY  
                    {                            
                        /*If the freq is stored in the Ps key, add special char for user to identify as favourite station */
                        uint8 index=0;
                        fm_display_type type=FM_SHOW_STATION;

                        for (index=0;index<FM_MAX_PRESET_STATIONS;index++)                   
                        {
                            if (theSink.conf2->sink_fm_data.fmStoredFreq.freq[index]==m->tuned_freq)
                            {
                                type=FM_ADD_FAV_STATION;
                                break;
                            }
                        }

                        /*Display frequency*/
                        sinkFmDisplayFreq(m->tuned_freq, type);
                    }
#endif                  
                }
                MAIN_DEBUG(("FM RX currently tuned to freq (0x%x) (%d) \n", theSink.conf2->sink_fm_data.fmRxTunedFreq, 
                                                                            theSink.conf2->sink_fm_data.fmRxTunedFreq));
            }
        }
        break;

        
        default:
        MAIN_DEBUG(("HS :  FM unhandled msg [%x]\n",id)) ;
        break ;           
    }   
}
#endif /* ENABLE_FM */


/*************************************************************************
NAME    
    app_handler
    
DESCRIPTION
    This is the main message handler for the Sink Application.  All
    messages pass through this handler to the subsequent handlers.

RETURNS

*/
static void app_handler(Task task, MessageId id, Message message)
{
    /*MAIN_DEBUG(("MSG [%x][%x][%x]\n", (int)task , (int)id , (int)&message)) ;*/
    
    /* determine the message type based on base and offset */
    if ( ( id >= EVENTS_MESSAGE_BASE ) && ( id <= EVENTS_LAST_EVENT ) )
    {
        handleUEMessage(task, id,  message);          
    }
    else  if ( (id >= CL_MESSAGE_BASE) && (id <= CL_MESSAGE_TOP) )
    {
        handleCLMessage(task, id,  message);        
    #ifdef TEST_HARNESS 
        vm2host_connection(task, id, message);
    #endif 
    }
    else if ( (id >= HFP_MESSAGE_BASE ) && (id <= HFP_MESSAGE_TOP) )
    {     
        handleHFPMessage(task, id,  message);     
    #ifdef TEST_HARNESS 
        vm2host_hfp(task, id, message);
    #endif 
    }    
    else if ( (id >= CODEC_MESSAGE_BASE ) && (id <= CODEC_MESSAGE_TOP) )
    {     
        handleCodecMessage (task, id, message) ;     
    }
    else if ( (id >= POWER_MESSAGE_BASE ) && (id <= POWER_MESSAGE_TOP) )
    {     
        handlePowerMessage (task, id, message) ;     
    }
#ifdef ENABLE_PBAP    
    else if ( ((id >= PBAPC_MESSAGE_BASE ) && (id <= PBAPC_MESSAGE_TOP)) ||
              ((id >= PBAPC_APP_MSG_BASE ) && (id <= PBAPC_APP_MSG_TOP)) )
    {     
        handlePbapMessages (task, id, message) ;     
    }
#endif
#ifdef ENABLE_MAPC 
    else if ( ((id >= MAPC_MESSAGE_BASE )    && (id <= MAPC_API_MESSAGE_END)) ||
              ((id >= MAPC_APP_MESSAGE_BASE) && (id <= MAPC_APP_MESSAGE_TOP)) )
    {     
        handleMapcMessages (task, id, message) ;     
    }    
#endif
#ifdef ENABLE_AVRCP
    else if ( (id >= AVRCP_INIT_CFM ) && (id <= AVRCP_MESSAGE_TOP) )
    {     
        sinkAvrcpHandleMessage (task, id, message) ;     
    #ifdef TEST_HARNESS 
        vm2host_avrcp(task, id, message);
    #endif 
    }
#endif
    
#ifdef CVC_PRODTEST
    else if (id == MESSAGE_FROM_KALIMBA)
    {
        cvcProductionTestKalimbaMessage (task, id, message);
    }
#endif    
    else if ( (id >= A2DP_MESSAGE_BASE ) && (id <= A2DP_MESSAGE_TOP) )
    {     
        handleA2DPMessage(task, id,  message);
    #ifdef TEST_HARNESS 
        vm2host_a2dp(task, id, message);
    #endif 
        return;
    }
    else if ( (id >= AUDIO_UPSTREAM_MESSAGE_BASE ) && (id <= AUDIO_UPSTREAM_MESSAGE_TOP) )
    {     
        handleAudioPluginMessage(task, id,  message);
        return;
    }    
    else if( ((id >= MESSAGE_USB_ENUMERATED) && (id <= MESSAGE_USB_SUSPENDED)) || 
             ((id >= MESSAGE_USB_DECONFIGURED) && (id <= MESSAGE_USB_DETACHED)) ||
             ((id >= USB_DEVICE_CLASS_MSG_BASE) && (id <= USB_DEVICE_CLASS_MSG_TOP)) )
    {
        handleUsbMessage(task, id, message);
        return;
    }
#ifdef ENABLE_GAIA
    else if ((id >= GAIA_MESSAGE_BASE) && (id < GAIA_MESSAGE_TOP))
    {
        handleGaiaMessage(task, id, message);
        return;
    }
#endif
#ifdef ENABLE_DISPLAY    
    else if ( (id >= DISPLAY_UPSTREAM_MESSAGE_BASE ) && (id <= DISPLAY_UPSTREAM_MESSAGE_TOP) )
    {     
        handleDisplayPluginMessage(task, id,  message);
        return;
    }      
#endif   /* ENABLE DISPLAY */
#ifdef ENABLE_GATT   
    else if ( (id >= BATT_REP_BASE ) && (id <= BATT_REP_MESSAGE_TOP) )
    {     
        handleGattBatteryReportMessage(task, id, message);
        return;
    }      
#endif /* ENABLE_GATT */    
#ifdef ENABLE_SUBWOOFER
    else if ( (id >= SWAT_MESSAGE_BASE) && (id <= SWAT_MESSAGE_TOP) )
    {
        handleSwatMessage(task, id, message);
        return;
    }
#endif /* ENABLE_SUBWOOFER */
#ifdef ENABLE_FM  
    else if ( (id >= FM_UPSTREAM_MESSAGE_BASE ) && (id <= FM_UPSTREAM_MESSAGE_TOP) )
    {     
        handleFmPluginMessage(task, id, message);
        return;
    }      
#endif /* ENABLE_FM */    
    else 
    { 
        MAIN_DEBUG(("MSGTYPE ? [%x]\n", id)) ;
    }       
}

/* Time critical initialisation */
void _init(void)
{
    /* Set the application task */
    theSink.task.handler = app_handler;
    /* Get and store the current config id */
    set_config_id ( PSKEY_CONFIGURATION_ID ) ;
    /* set flag to indicate that configuration is being read, use to prevent use of variables
       prior to completion of initialisation */
    theSink.SinkInitialising = TRUE;
    /* Map in any PIOs required */
    configManagerPioMap();
    /* Time critical USB setup */
    usbTimeCriticalInit();    
	
}


/* The Sink Application starts here...*/
int main(void)
{
    DEBUG (("Main [%s]\n",__TIME__));
        
    /* Initialise memory required early */
    configManagerInitMemory();

    /* Retrieve device state prior to reset */
    theSink.rundata->old_state = BootGetPreservedWord();

    /* initialise memory for the led manager */
    LedManagerMemoryInit();
    LEDManagerInit( ) ;

    /* Initialise device state */
    AuthResetConfirmationFlags();
    
    /*the internal regs must be latched on (smps and LDO)*/
    PioSetPowerPin ( TRUE ) ;
	
	#ifdef MICBIAS_PIN8
	PioSetDir32(MIC_EN, MIC_EN);	
	PioSet32(MIC_EN, 1);
	#endif

    switch (BootGetMode() )
    {
#ifdef CVC_PRODTEST 
        case BOOTMODE_CVC_PRODTEST:
            /*run the cvc prod test code and dont start the applicaiton */
            cvcProductionTestEnter() ;
        break ;
#endif        
        case BOOTMODE_DFU:
            /*do nothing special for the DFU boot mode, 
            This mode expects to have the appropriate host interfface enabled 
            Don't start the application */
        break ;
        
        case BOOTMODE_DEFAULT:
        case BOOTMODE_CUSTOM:         
        case BOOTMODE_USB_LOW_POWER:  
        case BOOTMODE_ALT_FSTAB:  
        default:
        {
            /*the above are application boot modes so kick of the app init routines*/
            const msg_filter MsgFilter = {msg_group_acl};
            uint16 PdlNumberOfDevices = 0 ;

            /* the number of paired devices can be restricted using pskey user 40,
               a number between 1 and 8 is allowed */
            PsRetrieve(PSKEY_PAIRED_DEVICE_LIST_SIZE, &PdlNumberOfDevices , sizeof(uint16) );
          
            DEBUG (("PDLSize[%d]\n" , PdlNumberOfDevices ));

            /* Initialise the Connection Library with the options */
            ConnectionInitEx2(&theSink.task , &MsgFilter , PdlNumberOfDevices );

            #ifdef TEST_HARNESS
                test_init();
            #endif
        }
        break ;
    }
        /* Start the message scheduler loop */
    MessageLoop();
        /* Never get here...*/
    return 0;  
}



#ifdef DEBUG_MALLOC 

#include "vm.h"
void * MallocPANIC ( const char file[], int line , size_t pSize )
{    
    static uint16 lSize = 0 ;
    static uint16 lCalls = 0 ;
    void * lResult;
 
    lCalls++ ;
    lSize += pSize ;    
    printf("+%s,l[%d]c[%d] t[%d] a[%d] s[%d]",file , line ,lCalls, lSize , (uint16)VmGetAvailableAllocations(), pSize ); 
                
    lResult = malloc ( pSize ) ;
     
    printf("@[0x%x]\n", (uint16)lResult);
    
        /*and panic if the malloc fails*/
    if ( lResult == NULL )
    {
        printf("MA : !\n") ;
        Panic() ;
    }
    
    return lResult ; 
                
}

void FreePANIC ( const char file[], int line, void * ptr ) 
{
    static uint16 lCalls = 0 ;    
    lCalls++ ; 
    printf("-%s,l[%d]c[%d] a[%d] @[0x%x]\n",file , line ,lCalls, (uint16)VmGetAvailableAllocations()-1, (uint16)ptr); 
    /* panic if attempting to free a null pointer*/
    if ( ptr == NULL )
    {
        printf("MF : !\n") ;
        Panic() ;
    }
    free( ptr ) ;    
}
#endif

/*************************************************************************
NAME    
    sinkInitCodecTask
    
DESCRIPTION
    Initialises the codec task

RETURNS

*/
static void sinkInitCodecTask ( void ) 
{
    /* The Connection Library has been successfully initialised, 
       initialise the HFP library to instantiate an instance of both
       the HFP and the HSP */
                   
    /*CodecInitWolfson (&theSink.rundata->codec, &theSink.task, wolfson_init_params *init) */
        
    /*init the codec task*/     	
    CodecInitCsrInternal (&theSink.rundata->codec, &theSink.task) ;
}
/*************************************************************************
NAME    
    handleHFPStatusCFM
    
DESCRIPTION
    Handles a status response from the HFP and sends an error message if one was received

RETURNS

*/
static void handleHFPStatusCFM ( hfp_lib_status pStatus ) 
{
    if (pStatus != hfp_success )
    {
        MAIN_DEBUG(("HS: HFP CFM Err [%d]\n" , pStatus)) ;
        MessageSend ( &theSink.task , EventError , 0 ) ;
#ifdef ENABLE_PBAP
        if(theSink.pbapc_data.pbap_command == pbapc_dialling)
        {
            MessageSend ( &theSink.task , EventPbapDialFail , 0 ) ; 
        } 
#endif        
    }
    else
    {
         MAIN_DEBUG(("HS: HFP CFM Success [%d]\n" , pStatus)) ;
    }

#ifdef ENABLE_PBAP
    theSink.pbapc_data.pbap_command = pbapc_action_idle;
#endif
}



