/***********************************************************************************************
* Freescale MMA8451,2,3Q Driver
*
* Filename: terminal.c
*
*
* (c) Copyright 2010, Freescale, Inc.  All rights reserved.
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
***********************************************************************************************/
#include "sink_private.h"
#include "sink_debug.h"
#include "accelerator_system.h"

/***********************************************************************************************
* Private macros
***********************************************************************************************/

/***********************************************************************************************
* Private type definitions
***********************************************************************************************/
#ifdef DEBUG_MMA8452QL
#define DEBUG_MMA8452Q(x) DEBUG(x)
#else
#define DEBUG_MMA8452Q(x) 
#endif

#ifdef MMA8452Q_TERMINAL_SUPPORTED
/***********************************************************************************************
* Private prototypes
***********************************************************************************************/

uint8 ProcessHexInput (uint8 *in, uint8 *out);
void CopyXYZ (uint8 *ptr);
void CopyXYZ8 (uint8 *ptr);
void PrintXYZdec14 (void);
void PrintXYZdec12 (void);
void PrintXYZdec10(void);
void PrintXYZdec8 (void);
void PrintXYZfrac (void);
void ClearDataFlash (void);
void PrintBuffering (void);
void PrintFIFO (void);
void ReadDataFlashXYZ (void);
void WriteFlashBuffer (void);

/***********************************************************************************************
* Private memory declarations
***********************************************************************************************/
BIT_FIELD StreamMode;                       /* stream mode control flags*/

extern BIT_FIELD RegisterFlag;
extern uint8 value[];                        /* working value result scratchpad*/
static tword x_value;                       /* 16-bit X accelerometer value*/
static tword y_value;                       /* 16-bit Y accelerometer value*/
static tword z_value;                       /* 16-bit Z accelerometer value*/


extern uint8 functional_block;               /* accelerometer function*/
                                        
const char* ODRvalues [] = 
{
	"800", "400", "200", "100", "50", "12.5", "6.25", "1.563"
};

const char* HPvaluesN [] =
{
	"16", "8", "4", "2",  /*800*/
	"16", "8", "4", "2",  /*400*/
	"8", "4", "2", "1",   /*200*/
	"4", "2", "1", "0.5",   /*100*/
	"2", "1", "0.5", "0.25", /*50*/
	"2", "1", "0.5", "0.25", /*12.5*/
	"2", "1", "0.5", "0.25", /*6.25*/
	"2", "1", "0.5", "0.25"
};

const char* HPvaluesLNLP [] =
{
	"16", "8", "4", "2",
	"16", "8", "4", "2",
	"8", "4", "2", "1",
	"4", "2", "1", "0.5",
	"2", "1", "0.5", "0.25",
	"0.5", "0.25", "0.125", "0.063",
	"0.25", "0.125", "0.063", "0.031",
	"0.063", "0.031", "0.016", "0.008"
};

const char* HPvaluesHiRes [] =
{
	"16", "8", "4", "2"
};

const char* HPvaluesLP [] =
{
	"16", "8", "4", "2",
	"8", "4", "2", "1",
	"4", "2", "1", "0.5",
	"2", "1", "0.5", "0.25",
	"1", "0.5", "0.25", "0.125",
	"0.25", "0.125", "0.063", "0.031",
	"0.125", "0.063", "0.031", "0.016",
	"0.031", "0.016", "0.008", "0.004" 
};

const char* GRange [] =
{
	"2g", "4g", "8g"
};


void Print_ODR_HP (void)
{
	DEBUG_MMA8452Q(("ODR = "));
	
	if ((IIC_RegRead(CTRL_REG2) & MODS_MASK)==0) {

		OSMode_Normal=TRUE;
		OSMode_LNLP=FALSE;
		OSMode_HiRes=FALSE;
		OSMode_LP=FALSE;
	} else if ((IIC_RegRead(CTRL_REG2) & MODS_MASK)==1) {

		OSMode_Normal=FALSE;
		OSMode_LNLP=TRUE;
		OSMode_HiRes=FALSE;
		OSMode_LP=FALSE;
	} else if ((IIC_RegRead(CTRL_REG2) & MODS_MASK)==2) {

		OSMode_Normal=FALSE;
		OSMode_LNLP=FALSE;
		OSMode_HiRes=TRUE;
		OSMode_LP=FALSE;  
	} else if ((IIC_RegRead(CTRL_REG2) & MODS_MASK)==3) {
		OSMode_Normal=FALSE;
		OSMode_LNLP=FALSE;
		OSMode_HiRes=FALSE;
		OSMode_LP=TRUE;
	}
	value[2] = IIC_RegRead(CTRL_REG1);
	value[0] = value[2] & DR_MASK;
	value[0] >>= 1; /* move DR value over to the left by 1*/

	value[1] = value[0] + (IIC_RegRead(HP_FILTER_CUTOFF_REG) & SEL_MASK);

	value[0] >>= 2;  /* move DR value over to the left by 2 more*/

	DEBUG_MMA8452Q((ODRvalues[value[0]]));
	DEBUG_MMA8452Q(("Hz   "));
	if (value[0] == 0 || value[0]==1)   /*Checking for 400Hz ODR or 800Hz ODR*/
	{
		ODR_400 = TRUE;
	}
	else
	{
		ODR_400 = FALSE;
	}


	DEBUG_MMA8452Q(("HP = "));

	if (OSMode_Normal==TRUE) {

		DEBUG_MMA8452Q((HPvaluesN[value[1]]));
	}
	else if (OSMode_LNLP==TRUE)
	{
		DEBUG_MMA8452Q((HPvaluesLNLP[value[1]]));
	}
	else if (OSMode_HiRes==TRUE)
	{
		value[1]=value[1]&0x03;
		DEBUG_MMA8452Q((HPvaluesHiRes[value[1]]));
	} 
	else if (OSMode_LP==TRUE){
		DEBUG_MMA8452Q((HPvaluesLP[value[1]]));
	}


	DEBUG_MMA8452Q(("Hz   ")); 
	DEBUG_MMA8452Q(("OS Mode = "));
	if (OSMode_Normal==TRUE) 
	{
		DEBUG_MMA8452Q(("Normal   "));
	} 
	else if (OSMode_LNLP==TRUE)
	{
		DEBUG_MMA8452Q(("Low Noise Low Power   "));
	}
	else if (OSMode_HiRes==TRUE){
		DEBUG_MMA8452Q(("Hi Resolution   "));
	} 
	else if (OSMode_LP==TRUE){
		DEBUG_MMA8452Q(("Low Power   "));
	}

	DEBUG_MMA8452Q(("G-Range = "));
	DEBUG_MMA8452Q((GRange[full_scale]));
	DEBUG_MMA8452Q(("\n"));
}

/*********************************************************\
*
\*********************************************************/
void CopyXYZ (uint8 *ptr)
{
  x_value.Byte.hi = *ptr++;
  x_value.Byte.lo = *ptr++;
  y_value.Byte.hi = *ptr++;
  y_value.Byte.lo = *ptr++;
  z_value.Byte.hi = *ptr++; 
  z_value.Byte.lo = *ptr;
       
}

void CopyXYZ8 (uint8 *ptr) 
{
  x_value.Byte.hi = *ptr++;
  x_value.Byte.lo = 0;
  y_value.Byte.hi = *ptr++;
  y_value.Byte.lo = 0;
  z_value.Byte.hi = *ptr; 
  z_value.Byte.lo = 0;
}
  


/*********************************************************\
*
\*********************************************************/
void PrintXYZdec14 (void)
{
  DEBUG_MMA8452Q(("X= "));
  SCI_s14dec_Out(x_value);
  DEBUG_MMA8452Q(("  Y= "));
  SCI_s14dec_Out(y_value);
  DEBUG_MMA8452Q(("  Z= "));
  SCI_s14dec_Out(z_value);
}
/*********************************************************\
*
\*********************************************************/
 void PrintXYZdec12 (void)
{
  DEBUG_MMA8452Q(("X= "));
  SCI_s12dec_Out(x_value);
  DEBUG_MMA8452Q(("  Y= "));
  SCI_s12dec_Out(y_value);
  DEBUG_MMA8452Q(("  Z= "));
  SCI_s12dec_Out(z_value);
}


/*********************************************************\
*
\*********************************************************/
 void PrintXYZdec10 (void)
{
  DEBUG_MMA8452Q(("X= "));
  SCI_s10dec_Out(x_value);
  DEBUG_MMA8452Q(("  Y= "));
  SCI_s10dec_Out(y_value);
  DEBUG_MMA8452Q(("  Z= "));
  SCI_s10dec_Out(z_value);
}

/*********************************************************\
*
\*********************************************************/
void PrintXYZdec8 (void)
{
  DEBUG_MMA8452Q(("X= "));
  SCI_s8dec_Out(x_value);
  DEBUG_MMA8452Q(("  Y= "));
  SCI_s8dec_Out(y_value);
  DEBUG_MMA8452Q(("  Z= "));
  SCI_s8dec_Out(z_value);
}


/*********************************************************\
*
\*********************************************************/
void PrintXYZfrac (void)
{
  DEBUG_MMA8452Q(("X= "));
  SCI_s14frac_Out(x_value);
  DEBUG_MMA8452Q(("  Y= "));
  SCI_s14frac_Out(y_value);
  DEBUG_MMA8452Q(("  Z= "));
  SCI_s14frac_Out(z_value);
}

/*********************************************************\
**  Terminal Output
\*********************************************************/
void OutputTerminal (uint8 BlockID, uint8 *ptr)
{
	switch (BlockID)
	{
		
		case FBID_FULL_XYZ_SAMPLE:
			/*
			**  Full Resolution XYZ Sample Registers (0x00 - 0x06)
			*/      

			CopyXYZ(ptr); 

			/*
			**  If the ODR = 400Hz, buffer the samples in the Data Flash.
			*/
			if (ODR_400 == TRUE) 
			{
#if 0
				/*
				**  Provide a visual indication that we're buffering and write to Flash
				*/
				PrintBuffering();
				WriteFlashBuffer();
#endif
			}
			else
			{
				/*
				**  Output formats are:
				**    - Stream XYZ data as signed counts
				**    - Stream XYZ data as signed g's
				*/
				if (STREAM_FULLC == 1)
				{

					switch (deviceID)
					{
						case 1:
							PrintXYZdec14();
						break;
						case 2:
							PrintXYZdec12();
						break;
						case 3:
							PrintXYZdec10();
						break;
					}

				}
				else
				{
					PrintXYZfrac();
				}

				DEBUG_MMA8452Q(("\n"));
			}

		break;

		
		case FBID_FIFO:
#if 0
		/*
		**  FIFO
		**
		**  If the ODR = 400Hz, buffer the samples in the Data Flash.
		*/
		if (ODR_400 == TRUE)
		{
		/*
		**  Provide a visual indication that we're buffering
		*/
		PrintBuffering();
		/*
		**  Save the samples in the FIFO buffer into the Data Flash
		*/
		value[0] = RegisterFlag.Byte & F_CNT_MASK;
		for (value[1]=0; value[1]!=value[0]; value[1]++)
		{
		while (DATAFLASH_Busy() == TRUE);
		/*
		**  Save sample to Data Flash
		*/
		DATAFLASH_WriteEnableLatch();
		SPI_SS_SELECT;
		SPI_ChrShift(0x02);
		SPI_ChrShift(address_in[0]);
		SPI_ChrShift(address_in[1]);
		SPI_ChrShift(address_in[2]);
		SPI_ChrShift(fifo_data[value[1]].Sample.XYZ.x_msb);
		SPI_ChrShift(fifo_data[value[1]].Sample.XYZ.x_lsb);
		SPI_ChrShift(fifo_data[value[1]].Sample.XYZ.y_msb);
		SPI_ChrShift(fifo_data[value[1]].Sample.XYZ.y_lsb);
		SPI_ChrShift(fifo_data[value[1]].Sample.XYZ.z_msb);
		SPI_ChrShift(fifo_data[value[1]].Sample.XYZ.z_lsb);
		SPI_SS_DESELECT;
		/*
		**  Adjust Data Flash address pointer
		*/
		if (address_in[2] > 245)
		{
		address_in[2] = 0;
		address_in[1]++;
		}
		else
		{
		address_in[2] += 6;
		}
		}
		}
		else
		{
		PrintFIFO();
		}
#endif
		break;


		default:
		break;
	}
}

#endif
