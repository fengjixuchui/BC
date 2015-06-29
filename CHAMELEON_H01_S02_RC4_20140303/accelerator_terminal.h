/***********************************************************************************************\
* Freescale MMA8451,2,3Q Driver
*
* Filename: terminal.h
*
*
* (c) Copyright 20010, Freescale, Inc.  All rights reserved.
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
\***********************************************************************************************/
#ifndef _TERMINAL_H_
#define _TERMINAL_H_

/***********************************************************************************************\
* Public macros
\***********************************************************************************************/

/***********************************************************************************************\
* Public type definitions
\***********************************************************************************************/

/***********************************************************************************************\
* Public memory declarations
\***********************************************************************************************/

/***********************************************************************************************\
* Public prototypes
\***********************************************************************************************/

void TerminalInit (void);
void ProcessTerminal (void);
void OutputTerminal (uint8 BlockID, uint8 *ptr);
extern void Print_ODR_HP (void);


#endif  /* _TERMINAL_H_ */
