/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013

FILE NAME
    sink_auth.h
    
DESCRIPTION
    
*/

#include "sink_private.h"

#ifndef _SINK_AUTH_PRIVATE_H_
#define _SINK_AUTH_PRIVATE_H_

/*************************************************************************
NAME    
     sinkHandlePinCodeInd
    
DESCRIPTION
     This function is called on receipt on an CL_PIN_CODE_IND message
     being recieved.  The sink device default pin code is sent back.

RETURNS
     
*/
void sinkHandlePinCodeInd(const CL_SM_PIN_CODE_IND_T* ind);


/*************************************************************************
NAME    
     sinkHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS
     
*/
void sinkHandleUserConfirmationInd(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind);
		
		
/*************************************************************************
NAME    
     sinkHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS
     
*/
void sinkHandleUserPasskeyInd(const CL_SM_USER_PASSKEY_REQ_IND_T* ind);


/*************************************************************************
NAME    
     sinkHandleUserPasskeyNotificationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS
     
*/
void sinkHandleUserPasskeyNotificationInd(const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind);
		

/*************************************************************************
NAME    
     sinkHandleIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS
     
*/
void sinkHandleIoCapabilityInd(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind);
		
		
/*************************************************************************
NAME    
     sinkHandleRemoteIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND

RETURNS
     
*/
void sinkHandleRemoteIoCapabilityInd(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind);


/****************************************************************************
NAME    
    sinkHandleAuthoriseInd
    
DESCRIPTION
    Request to authorise access to a particular service.

RETURNS
    void
*/
void sinkHandleAuthoriseInd(const CL_SM_AUTHORISE_IND_T *ind);


/****************************************************************************
NAME    
    sinkHandleAuthenticateCfm
    
DESCRIPTION
    Indicates whether the authentication succeeded or not.

RETURNS
    void
*/
void sinkHandleAuthenticateCfm(const CL_SM_AUTHENTICATE_CFM_T *cfm);

/****************************************************************************
NAME    
    sinkPairingAcceptRes 
    
DESCRIPTION
    Respond correctly to a pairing info request ind (pin code, passkey, etc...)

RETURNS
    void
*/
void sinkPairingAcceptRes( void );

/****************************************************************************
NAME    
    sinkPairingRejectRes 
    
DESCRIPTION
    Respond reject to a pairing info request ind (pin code, passkey, etc...)

RETURNS
    void
*/
void sinkPairingRejectRes( void );

/****************************************************************************
NAME    
    AuthResetConfirmationFlags
    
DESCRIPTION
    Helper function to reset the confirmation flags and associated BT address

RETURNS
     
*/

void AuthResetConfirmationFlags ( void ) ;

#endif /* _SINK_AUTH_PRIVATE_H_ */
