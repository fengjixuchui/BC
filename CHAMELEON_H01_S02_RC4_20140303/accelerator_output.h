/***********************************************************************************************
* Freescale MMA8451,2,3Q Driver
*
* Filename: sci.h
*
*
* (c) Copyright 2010, Freescale, Inc.  All rights reserved.
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
***********************************************************************************************/
#ifndef _SCI_H_
#define _SCI_H_


#include "sink_private.h"
#include "sink_debug.h"
#include "accelerator_system.h"

#ifdef MMA8452Q_TERMINAL_SUPPORTED
/***********************************************************************************************
* Public macros
***********************************************************************************************/

#define ASCII_BS      0x08
#define ASCII_LF      0x0A
#define ASCII_CR      0x0D
#define ASCII_DEL     0x7F

/***********************************************************************************************
* Public type definitions
***********************************************************************************************/
/***********************************************************************************************
**
**  Variable type definition: tword
*/
#if 1
typedef struct
{
  uint16 Word;
  struct
  {
    uint8 hi;
    uint8 lo;
  } Byte;
} tword;
#endif
/***********************************************************************************************
* Public memory declarations
***********************************************************************************************/


/***********************************************************************************************
* Public prototypes
***********************************************************************************************/

void SCI_NibbOut(uint8 data);
void SCI_ByteOut(uint8 data);
void SCI_s14dec_Out (tword data);
void SCI_s12dec_Out (tword data);
void SCI_s10dec_Out(tword data);
void SCI_s8dec_Out(tword data);
void SCI_s14frac_Out (tword data);
#endif

#endif  /* _SCI_H_ */
