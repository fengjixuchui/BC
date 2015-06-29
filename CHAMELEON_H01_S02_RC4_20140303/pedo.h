/*********************************************************************
 
  (c) copyright Freescale Semiconductor Ltd. 2006
  ALL RIGHTS RESERVED
 
 *********************************************************************
 
 *********************************************************************

  File:				    Pedo.h
 
  Description:		    Pedometer algorithm header
  
  Date:        		 Nov 2006
  Author:			    Vincent Ko
  
 ********************************************************************/




void pedometer_init(void);
void pedometer(void* data);
int  pedometer_get_step(void);

