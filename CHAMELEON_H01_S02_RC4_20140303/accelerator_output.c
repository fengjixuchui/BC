/***********************************************************************************************\
* Freescale MMA8451,2,3Q Driver
*
* Filename: sci.c
*
*
* (c) Copyright 2010, Freescale, Inc.  All rights reserved.
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
\***********************************************************************************************/
#include "sink_private.h"
#include "sink_debug.h"
#include "accelerator_system.h"
#include "accelerator_output.h"

#ifdef DEBUG_WM8987L
#define WM8987L_DEBUG(x) DEBUG(x)
#else
#define WM8987L_DEBUG(x) 
#endif

#ifdef DEBUG_MMA8452Q_OUTPUTL
#define DEBUG_MMA8452Q_OUTPUT(x) DEBUG(x)
#else
#define DEBUG_MMA8452Q_OUTPUT(x) 
#endif

#ifdef MMA8452Q_TERMINAL_SUPPORTED
/***********************************************************************************************\
* Private macros
\***********************************************************************************************/

/*
**  The following macros are constants used in the fractional decimal conversion routine.
**
**    FRAC_2d1  =  2^-1  =  0.5
**    FRAC_2d2  =  2^-2  =  0.25    etc...
*/
#define FRAC_2d1              5000
#define FRAC_2d2              2500
#define FRAC_2d3              1250
#define FRAC_2d4               625
#define FRAC_2d5               313
#define FRAC_2d6               156
#define FRAC_2d7                78
#define FRAC_2d8                39
#define FRAC_2d9                20
#define FRAC_2d10               10
#define FRAC_2d11                5
#define FRAC_2d12                2


/***********************************************************************************************\
* Private type definitions
\***********************************************************************************************/

/***********************************************************************************************\
* Private prototypes
\***********************************************************************************************/



/***********************************************************************************************\
* Private memory declarations
\***********************************************************************************************/


/***********************************************************************************************\
* Public memory declarations
\***********************************************************************************************/



/***********************************************************************************************\
* Public functions
\***********************************************************************************************/


/*********************************************************\
* SCI single hex nibble character output
\*********************************************************/
void SCI_NibbOut(uint8 data)
{
	uint8 c;
	c = data + 0x30;
	if (c > 0x39)
		c += 0x07;

	DEBUG_MMA8452Q_OUTPUT(("%c", c))
}


/*********************************************************\
* SCI hex byte character output
\*********************************************************/
void SCI_ByteOut(uint8 data)
{
	uint8 c;
	c = data;
	c >>= 4;
	DEBUG_MMA8452Q_OUTPUT(("%c", c))
	c = data & 0x0F;
	DEBUG_MMA8452Q_OUTPUT(("%c", c))
}


/*********************************************************\
* SCI output signed 14-bit left-justified integer as decimal
*
* 
Example:   0xABCC = -1349
    	     0x5443  = +1349
*
\*********************************************************/
void SCI_s14dec_Out (tword data)
{
	uint8 a, b, c, d;
	uint16 r;
	
	data.Word = data.Word = (uint16)((0x00FF & (uint16)data.Byte.hi) << 8) | (data.Byte.lo);
	/*
	**  Determine sign and output
	*/
	if (data.Byte.hi > 0x7F)
	{
		DEBUG_MMA8452Q_OUTPUT (("-"));
		data.Word = ~data.Word + 1;
	}
	else
	{
		DEBUG_MMA8452Q_OUTPUT (("+"));
	}
	/*
	**  Calculate
	*/
	a = (uint8)((data.Word >>2) / 1000);
	r = (data.Word >>2) % 1000;
	b = (uint8)(r / 100);
	r %= 100;
	c = (uint8)(r / 10);
	d = (uint8)(r % 10);
	/*
	**  Format
	*/
	if (a == 0)
	{
		a = 0xF0;
		if (b == 0)
		{
			b = 0xF0;
			if (c == 0)
			{
				c = 0xF0;
			}
		}
	}
	/*
	**  Output result
	*/
	SCI_NibbOut (a);
	SCI_NibbOut (b);
	SCI_NibbOut (c);
	SCI_NibbOut (d);
}


/*********************************************************\
* SCI output signed 12-bit left-justified integer as decimal
*
* Example:  0x23D0  =  "+ 573"
*           0xDC30  =  "- 573"
*
\*********************************************************/
void SCI_s12dec_Out (tword data)
{
	uint8 a, b, c, d;
	uint16 r;

	data.Word = data.Word = (uint16)((0x00FF & (uint16)data.Byte.hi) << 8) | (data.Byte.lo);
	/*
	**  Determine sign and output
	*/
	if (data.Byte.hi > 0x7F)
	{
		DEBUG_MMA8452Q_OUTPUT (("-"));
		data.Word = ~data.Word + 1;
	}
	else
	{
		DEBUG_MMA8452Q_OUTPUT (("+"));
	}
	/*
	**  Calculate
	*/
	a = (uint8)((data.Word >>4) / 1000);
	r = (data.Word >>4) % 1000;
	b = (uint8)(r / 100);
	r %= 100;
	c = (uint8)(r / 10);
	d = (uint8)(r % 10);
	/*
	**  Format
	*/
	if (a == 0)
	{
		a = 0xF0;
		if (b == 0)
		{
			b = 0xF0;
			if (c == 0)
			{
				c = 0xF0;
			}
		}
	}
	/*
	**  Output result
	*/
	SCI_NibbOut (a);
	SCI_NibbOut (b);
	SCI_NibbOut (c);
	SCI_NibbOut (d);
}


/*********************************************************\
* SCI output signed 10-bit left-justified integer as decimal
*
* 
Example:   
    	     
*
\*********************************************************/
void SCI_s10dec_Out (tword data)
{
	uint8 a, b, c;
	uint16 r;

	data.Word = data.Word = (uint16)((0x00FF & (uint16)data.Byte.hi) << 8) | (data.Byte.lo);
	/*
	**  Determine sign and output
	*/
	if (data.Byte.hi > 0x7F)
	{
		DEBUG_MMA8452Q_OUTPUT (("-"));
		data.Word = ~data.Word + 1;
	}
	else
	{
		DEBUG_MMA8452Q_OUTPUT (("+"));
	}
	/*
	**  Calculate
	*/
	a = (uint8)((data.Word >>6) / 100);
	r = (data.Word >>6) % 100;
	b = (uint8)(r / 10);
	c = (uint8)(r % 10);
	/*
	**  Format
	*/
	if (a == 0)
	{
		a = 0xF0;
		if (b == 0)
		{
			b = 0xF0;
		}
	}
	/*
	**  Output result
	*/
	SCI_NibbOut (a);
	SCI_NibbOut (b);
	SCI_NibbOut (c);
}




/*********************************************************\
* SCI output signed 8-bit left-justified integer as decimal
*
* Example:  0x5400  =  "+ 84"
*           0xAB00  =  "- 84"
*
\*********************************************************/
void SCI_s8dec_Out (tword data)
{
	uint8 a, b, c;
	uint16 r;

	data.Word = data.Word = (uint16)((0x00FF & (uint16)data.Byte.hi) << 8) | (data.Byte.lo);
	/*
	**  Determine sign and output
	*/
	if (data.Byte.hi > 0x7F)
	{
		DEBUG_MMA8452Q_OUTPUT (("-"));
		data.Word = ~data.Word + 1;
	}
	else
	{
		DEBUG_MMA8452Q_OUTPUT (("+"));
	}
	/*
	**  Calculate
	*/
	a = (uint8)((data.Word >>8) / 100); /*Shift the data over since it is MSB only*/
	r = (data.Word >>8) % 100;
	b = (uint8)(r / 10);
	c = (uint8)(r % 10);
	/*
	**  Format
	*/
	if (a == 0)
	{
		a = 0xF0;
		if (b == 0)
		{
			b = 0xF0;
		}
	}
	/*
	**  Output result
	*/
	SCI_NibbOut (a);
	SCI_NibbOut (b);
	SCI_NibbOut (c);
}



/*********************************************************\
* SCI output signed 14-bit left-justified integer as fraction (5 decimal places)
*
* Example:  
*           
*
\*********************************************************/
void SCI_s14frac_Out (tword data)
{
	BIT_FIELD value;
	uint16 result;
	uint8 a, b, c, d;
	uint16 r;

	data.Word = data.Word = (uint16)((0x00FF & (uint16)data.Byte.hi) << 8) | (data.Byte.lo);
	/*
	**  Determine sign and output
	*/
	if (data.Byte.hi > 0x7F)
	{
		DEBUG_MMA8452Q_OUTPUT (("-"));

		data.Word &= 0xFFFC;
		data.Word = ~data.Word + 1;
	}
	else
	{
		DEBUG_MMA8452Q_OUTPUT (("+"));
	}
	/*
	**  Determine integer value and output
	*/
	if (full_scale == FULL_SCALE_2G)
	{
		SCI_NibbOut((data.Byte.hi & 0x40) >>6);
		data.Word = data.Word <<2;
	} 
	else if (full_scale == FULL_SCALE_4G)
	{
		SCI_NibbOut((data.Byte.hi & 0x60) >>5);
		data.Word = data.Word <<3;
	}
	else
	{
		SCI_NibbOut((data.Byte.hi & 0x70) >>4);
		data.Word = data.Word <<4;
	}   
	DEBUG_MMA8452Q_OUTPUT (("."));
	/*
	**  Determine mantissa value
	*/
	result = 0;
	#if 0
	value.Byte = data.Byte.hi;
	if (value.Bit._7 == 1)
		result += FRAC_2d1;
	if (value.Bit._6 == 1)
		result += FRAC_2d2;
	if (value.Bit._5 == 1)
		result += FRAC_2d3;
	if (value.Bit._4 == 1)
		result += FRAC_2d4;

	data.Word = data.Word <<4;
	value.Byte = data.Byte.hi;

	if (value.Bit._7 == 1)
		result += FRAC_2d5;
	if (value.Bit._6 == 1)
		result += FRAC_2d6;
	if (value.Bit._5 == 1)
		result += FRAC_2d7;
	if (value.Bit._4 == 1)
		result += FRAC_2d8;
	if (value.Bit._3 ==1)
		result += FRAC_2d9;
	if (value.Bit._2 ==1)
		result += FRAC_2d10;

	if(value.Bit._1 ==1)
		result += FRAC_2d11;
	if(value.Bit._0 ==1)
		result += FRAC_2d12;
	#else
	value.Byte = data.Byte.hi;
	if (value.Byte & (1 << 7))
		result += FRAC_2d1;
	if (value.Byte & (1 << 6))
		result += FRAC_2d2;
	if (value.Byte & (1 << 5))
		result += FRAC_2d3;
	if (value.Byte & (1 << 4))
		result += FRAC_2d4;

	data.Word = data.Word <<4;
	value.Byte = data.Byte.hi;

	if (value.Byte & (1 << 7))
		result += FRAC_2d5;
	if (value.Byte & (1 << 6))
		result += FRAC_2d6;
	if (value.Byte & (1 << 5))
		result += FRAC_2d7;
	if (value.Byte & (1 << 4))
		result += FRAC_2d8;
	if (value.Byte & (1 << 3))
		result += FRAC_2d9;
	if (value.Byte & (1 << 2))
		result += FRAC_2d10;

	if(value.Byte & (1 << 1))
		result += FRAC_2d11;
	if(value.Byte & (1 << 0))
		result += FRAC_2d12;
	#endif
	/*
	**  Convert mantissa value to 4 decimal places
	*/
	r = result % 1000;
	a = (uint8)(result / 1000);
	b = (uint8)(r / 100);
	r %= 100;
	c = (uint8)(r / 10);
	d = (uint8)(r%10);

	/*
	**  Output mantissa
	*/
	SCI_NibbOut (a);
	SCI_NibbOut (b);
	SCI_NibbOut (c);
	SCI_NibbOut (d);
	DEBUG_MMA8452Q_OUTPUT(("g\n"));
}
#endif
