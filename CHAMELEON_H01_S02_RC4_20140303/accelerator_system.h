/***********************************************************************************************
* Freescale MMA8451,2,3Q Driver
*
* Filename: system.h
*
*
* (c) Copyright 2010, Freescale, Inc.  All rights reserved.
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
***********************************************************************************************/
#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "sink_private.h"
#include "accelerator_sensor.h"              /*MMA845xQ macros*/
#include "accelerator_terminal.h"            /*Terminal interface macros*/
#include "accelerator_output.h"                  /* SCI macros*/
/***********************************************************************************************
* Public type definitions
***********************************************************************************************/

/***********************************************************************************************
**
**  Variable type definition: BIT_FIELD
*/


typedef union
{
	uint8 Byte;
	struct {
	unsigned _0:1;
	unsigned _1:1;
	unsigned _2:1;
	unsigned _3:1;
	unsigned _4:1;
	unsigned _5:1;
	unsigned _6:1;
	unsigned _7:1;
	} Bit;
} BIT_FIELD;

#if 0
/***********************************************************************************************
**
**  Variable type definition: tword
*/
typedef union
{
  unsigned short Word;
  struct
  {
    unsigned hi:8;
    unsigned lo:8;
  } Byte;
} tword;
#endif

/***********************************************************************************************
**
**  Variable type definition: tfifo_xyz
*/
typedef struct
{
  uint8 x_msb;
  uint8 x_lsb;
  uint8 y_msb;
  uint8 y_lsb;
  uint8 z_msb;
  uint8 z_lsb;
} tfifo_xyz;


/***********************************************************************************************
**
**  Variable type definition: tfifo_sample
*/
typedef union
{
  uint8 Byte[6];
  struct
  {
    tfifo_xyz XYZ;
  } Sample;
} tfifo_sample;

#define FIFO_BUFFER_SIZE          32


/***********************************************************************************************
* Project includes
***********************************************************************************************/



/***********************************************************************************************
* Public macros
***********************************************************************************************/


/***********************************************************************************************
**
**  System control bit flags
*/
#define PROMPT_WAIT                 (SystemFlag.Bit._7)
#define SCI_INPUT_READY             (SystemFlag.Bit._6)
#define INPUT_ERROR                 (SystemFlag.Bit._5)
#define POLL_ACTIVE                 (SystemFlag.Bit._4)
#define ODR_400                     (SystemFlag.Bit._3)


/***********************************************************************************************
**
**  Stream mode control bit flags
*/
#define STREAM_FULLC                  (StreamMode.Bit._7)
#define STREAM_FULLG                  (StreamMode.Bit._6)
#define XYZ_STREAM                    (StreamMode.Bit._5)
#define INT_STREAM                    (StreamMode.Bit._4)
#define OSMode_Normal                 (StreamMode.Bit._3)
#define OSMode_LNLP                   (StreamMode.Bit._2)
#define OSMode_LP                     (StreamMode.Bit._1)
#define OSMode_HiRes                  (StreamMode.Bit._0)
#define STREAM_FULLC_MASK             (0x80)
#define STREAM_FULLG_MASK             (0x40)
#define STREAM_XYZ_MASK               (0x20)
#define STREAM_INT_MASK               (0x10)


/***********************************************************************************************
**
**  Accelerometer functions
*/
enum
{
  FBID_FULL_XYZ_SAMPLE,          /*  2 =  XYZ Sample Registers (0x01 - 0x06)*/
  FBID_8_XYZ_SAMPLE,
  FBID_FIFO,                     /* 10 = FIFO*/
  FBID_MAX
};

/***********************************************************************************************
* Public memory declarations
***********************************************************************************************/

extern BIT_FIELD SystemFlag;                /* system control flags*/
extern uint8 full_scale;                     /* current accelerometer full scale setting*/
extern int deviceID;

/***********************************************************************************************
* Public prototypes
***********************************************************************************************/

extern void InterruptsActive (uint8 ctrl_reg3, uint8 ctrl_reg4, uint8 ctrl_reg5);

#endif  /* _SYSTEM_H_ */
