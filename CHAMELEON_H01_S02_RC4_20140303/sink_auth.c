/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013

FILE NAME
    sink_auth.c        

DESCRIPTION
    This file contains the Authentication functionality for the Sink 
    Application

NOTES

*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_tts.h"
#include "sink_stateManager.h"
#include "sink_auth.h"

#include "sink_devicemanager.h"
#include "sink_debug.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include <ps.h>
#include <bdaddr.h>
#include <stdlib.h>
#include <sink.h>

#ifdef DEBUG_AUTH
    #define AUTH_DEBUG(x) DEBUG(x)    
#else
    #define AUTH_DEBUG(x) 
#endif   

/****************************************************************************
NAME    
    AuthCanSinkConnect 
    
DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanSinkConnect ( const bdaddr * bd_addr );

/****************************************************************************
NAME    
    AuthCanSinkPair 
    
DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanSinkPair ( void ) ;

/*************************************************************************
NAME    
     sinkHandlePinCodeInd
    
DESCRIPTION
     This function is called on receipt on an CL_PIN_CODE_IND message
     being recieved.  The Sink devices default pin code is sent back.

RETURNS
     
*/
void sinkHandlePinCodeInd(const CL_SM_PIN_CODE_IND_T* ind)
{
    uint16 pin_length = 0;
    uint8 pin[16];
    
    if ( AuthCanSinkPair() )
    {
	    
		AUTH_DEBUG(("auth: Can Pin\n")) ;
		
   		/* Do we have a fixed pin in PS, if not reject pairing */
    	if ((pin_length = PsFullRetrieve(PSKEY_FIXED_PIN, pin, 16)) == 0 || pin_length > 16)
   		{
   	    	/* Set length to 0 indicating we're rejecting the PIN request */
        	AUTH_DEBUG(("auth : failed to get pin\n")) ;
       		pin_length = 0; 
   		}	
        else if(theSink.features.VoicePromptPairing)
        {
            TTSPlayEvent(EventPinCodeRequest);
            TTSPlayNumString(pin_length, pin);
        }
	} 
    /* Respond to the PIN code request */
    ConnectionSmPinCodeResponse(&ind->taddr, pin_length, pin); 
}

/*************************************************************************
NAME    
     sinkHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS
     
*/
void sinkHandleUserConfirmationInd(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind)
{
	/* Can we pair? */
	if ( AuthCanSinkPair() && theSink.features.ManInTheMiddle)
    {
        theSink.confirmation = TRUE;
		AUTH_DEBUG(("auth: Can Confirm %ld\n",ind->numeric_value)) ;
		/* Should use text to speech here */
		theSink.confirmation_addr = mallocPanic(sizeof(typed_bdaddr));
		*theSink.confirmation_addr = ind->taddr;
        if(theSink.features.VoicePromptPairing)
        {
            TTSPlayEvent(EventConfirmationRequest);
            TTSPlayNumber(ind->numeric_value);
        }
	}
	else
    {
		/* Reject the Confirmation request */
		AUTH_DEBUG(("auth: Rejected Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(&ind->taddr, FALSE);
    }
}

/*************************************************************************
NAME    
     sinkHandleUserPasskeyInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_IND

RETURNS
     
*/
void sinkHandleUserPasskeyInd(const CL_SM_USER_PASSKEY_REQ_IND_T* ind)
{
	/* Reject the Passkey request */
	AUTH_DEBUG(("auth: Rejected Passkey Req\n")) ;
	ConnectionSmUserPasskeyResponse(&ind->taddr, TRUE, 0);
}


/*************************************************************************
NAME    
     sinkHandleUserPasskeyNotificationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS
     
*/
void sinkHandleUserPasskeyNotificationInd(const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind)
{
	AUTH_DEBUG(("Passkey: %ld \n", ind->passkey));
    if(theSink.features.ManInTheMiddle && theSink.features.VoicePromptPairing)
    {
        TTSPlayEvent(EventPasskeyDisplay);
        TTSPlayNumber(ind->passkey);
    }
	/* Should use text to speech here */
}

/*************************************************************************
NAME    
     sinkHandleIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS
     
*/
void sinkHandleIoCapabilityInd(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind)
{	
	/* If not pairable should reject */
	if(AuthCanSinkPair())
	{
		cl_sm_io_capability local_io_capability = theSink.features.ManInTheMiddle ? cl_sm_io_cap_display_yes_no : cl_sm_io_cap_no_input_no_output;
		
		AUTH_DEBUG(("auth: Sending IO Capability \n"));
		
		/* Send Response */
#if defined ENABLE_PEER && defined DISABLE_PEER_PDL
        if (theSink.inquiry.session == inquiry_session_peer)
        {   /* Do *not* bond with device */
            ConnectionSmIoCapabilityResponse(&ind->bd_addr,local_io_capability,theSink.features.ManInTheMiddle,FALSE,FALSE,0,0);
        }
        else
#endif
        {   /* Bond with device */
    		ConnectionSmIoCapabilityResponse(&ind->bd_addr,local_io_capability,theSink.features.ManInTheMiddle,TRUE,FALSE,0,0);
        }
	}
	else
	{
		AUTH_DEBUG(("auth: Rejecting IO Capability Req \n"));
		ConnectionSmIoCapabilityResponse(&ind->bd_addr, cl_sm_reject_request,FALSE,FALSE,FALSE,0,0);
	}
}

/*************************************************************************
NAME    
     sinkHandleRemoteIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND

RETURNS
     
*/
void sinkHandleRemoteIoCapabilityInd(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind)
{
	AUTH_DEBUG(("auth: Incoming Authentication Request\n"));
}

/****************************************************************************
NAME    
    sinkHandleAuthoriseInd
    
DESCRIPTION
    Request to authorise access to a particular service.

RETURNS
    void
*/
void sinkHandleAuthoriseInd(const CL_SM_AUTHORISE_IND_T *ind)
{
	
	bool lAuthorised = FALSE ;
	
	if ( AuthCanSinkConnect(&ind->bd_addr) )
	{
		lAuthorised = TRUE ;
	}
	
	AUTH_DEBUG(("auth: Authorised [%d]\n" , lAuthorised)) ;
	    
	/*complete the authentication with the authorised or not flag*/
    ConnectionSmAuthoriseResponse(&ind->bd_addr, ind->protocol_id, ind->channel, ind->incoming, lAuthorised);
}


/****************************************************************************
NAME    
    sinkHandleAuthenticateCfm
    
DESCRIPTION
    Indicates whether the authentication succeeded or not.

RETURNS
    void
*/
void sinkHandleAuthenticateCfm(const CL_SM_AUTHENTICATE_CFM_T *cfm)
{
#ifdef ENABLE_SUBWOOFER
    if (theSink.inquiry.action == rssi_subwoofer)
    {
        if ((cfm->status == auth_status_success) && (cfm->bonded))
        {
            /* Mark the subwoofer as a trusted device */
            deviceManagerMarkTrusted(&cfm->bd_addr);
            
            /* Store the subwoofers BDADDR to PS */
            storeSubwooferBdaddr(&cfm->bd_addr);
            
            /* Setup some default attributes for the subwoofer */
            deviceManagerStoreDefaultAttributes(&cfm->bd_addr, TRUE);
        }
        return;
    }
#endif
	/* Leave bondable mode if successful unless we got a debug key */
	if (cfm->status == auth_status_success && cfm->key_type != cl_sm_link_key_debug)
        if(theSink.inquiry.action != rssi_pairing)
            {
                /* Mark the device as trusted */
                deviceManagerMarkTrusted(&cfm->bd_addr);
                MessageSend (&theSink.task , EventPairingSuccessful , 0 );
            }

	/* Set up some default params and shuffle PDL */
	if(cfm->bonded)
	{
        deviceManagerStoreDefaultAttributes(&cfm->bd_addr, FALSE);
		deviceManagerSetPriority(&cfm->bd_addr);
	}
	
	/* Reset pairing info if we timed out on confirmation */
	AuthResetConfirmationFlags();
}


/****************************************************************************
NAME    
    AuthCanSinkPair 
    
DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanSinkPair ( void )
{
	bool lCanPair = FALSE ;
	
    if (theSink.features.SecurePairing)
    {
	    	/*if we are in pairing mode*/
		if ((stateManagerGetState() == deviceConnDiscoverable)||(theSink.inquiry.action == rssi_subwoofer))
		{
			lCanPair = TRUE ;
			AUTH_DEBUG(("auth: is ConnDisco\n")) ;
		}
    }
    else
    {
	    lCanPair = TRUE ;
    }
    return lCanPair ;
}



/****************************************************************************
NAME    
    AuthCanSinkConnect 
    
DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanSinkConnect ( const bdaddr * bd_addr )
{
	bool lCanConnect = FALSE ;
    uint8 NoOfDevices = deviceManagerNumConnectedDevs();
    
    /* if device is already connected via a different profile allow this next profile to connect */
    if(deviceManagerProfilesConnected(bd_addr))
    {
    	AUTH_DEBUG(("auth: already connected, CanConnect = TRUE\n")) ;
        lCanConnect = TRUE;
    }
    /* this bdaddr is not already connected, therefore this is a new device, ensure it is allowed 
       to connect, if not reject it */
    else
    {
        /* when multipoint is turned off, only allow one device to connect */
        if(((!theSink.MultipointEnable)&&(!NoOfDevices))||
           ((theSink.MultipointEnable)&&(NoOfDevices < MAX_MULTIPOINT_CONNECTIONS)))
        {
            /* is secure pairing enabled? */
            if (theSink.features.SecurePairing)
            {
    	        /* If page scan is enabled (i.e. we are either connectable/discoverable or 
    	    	connected in multi point) */
    	    	if ( theSink.page_scan_enabled )
    	    	{
    	    		lCanConnect = TRUE ;
    	    		AUTH_DEBUG(("auth: is connectable\n")) ;
    	    	}		
            }
            /* no secure pairing */
            else
            {
            	AUTH_DEBUG(("auth: MP CanConnect = TRUE\n")) ;
    	        lCanConnect = TRUE ;
            }
        }
    }
  
    AUTH_DEBUG(("auth:  CanConnect = %d\n",lCanConnect)) ;
  
    return lCanConnect ;
}

/****************************************************************************
NAME    
    sinkPairingAcceptRes 
    
DESCRIPTION
    Respond correctly to a pairing info request ind

RETURNS
    void
*/
void sinkPairingAcceptRes( void )
{		
    if(AuthCanSinkPair() && theSink.confirmation)
	{
		AUTH_DEBUG(("auth: Accepted Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(theSink.confirmation_addr, TRUE);
     }
	else
     {
		AUTH_DEBUG(("auth: Invalid state for confirmation\n"));
     }
}

/****************************************************************************
NAME    
    sinkPairingRejectRes 
    
DESCRIPTION
    Respond reject to a pairing info request ind

RETURNS
    void
*/
void sinkPairingRejectRes( void )
{
	if(AuthCanSinkPair() && theSink.confirmation)
	{	
		AUTH_DEBUG(("auth: Rejected Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(theSink.confirmation_addr, FALSE);
	}
	else
	{
		AUTH_DEBUG(("auth: Invalid state for confirmation\n"));
	}
}

/****************************************************************************
NAME    
    AuthResetConfirmationFlags
    
DESCRIPTION
    Helper function to reset the confirmations flag and associated BT address

RETURNS
     
*/

void AuthResetConfirmationFlags ( void )
{
	AUTH_DEBUG(("auth: Reset Confirmation Flags\n"));
	if(theSink.confirmation)
	{
		AUTH_DEBUG(("auth: Free Confirmation Addr\n"));
		freePanic(theSink.confirmation_addr);
	}
	theSink.confirmation_addr = NULL;
	theSink.confirmation = FALSE;
}

