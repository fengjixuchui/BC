
#include "sink_private.h"
#include "sink_debug.h"
#include <i2c.h>
#include <ps.h>
#include <codec.h>
#include <pio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ISA1200.h"


#define WM8987L_Transfer(a) I2cTransfer ((SLAVE_ADDRESS_ISA1200LOW), (a), sizeof (a), NULL, 0)
#define WM8987L_Transfer2(a) I2cTransfer ((SLAVE_ADDRESS_ISA1200HIGH), (a), sizeof (a), NULL, 0)
#define WM8987L_TransferOK(a) (WM8987L_Transfer (a) == 1 + sizeof (a))
#define WM8987L_TransferOK2(a) (WM8987L_Transfer2 (a) == 1 + sizeof (a))
#ifdef DEBUG_WM8987L
#define WM8987L_DEBUG(x) DEBUG(x)
#else
#define WM8987L_DEBUG(x) 
#endif

#ifdef DEBUG_ISA1200L
#define DEBUG_ISA1200(x) DEBUG(x)
#else
#define DEBUG_ISA1200(x) 
#endif

#define LEN_PIN  (1<<10)
#define HEN_PIN (1<<11)
#define ISAEN_PIN (1<<15)

/*#define WM8987L_Read_I2C(a,b) I2cTransfer ((SLAVE_ADDRESS_S5M0065 | I2C_READ_MODE), (a), sizeof (a), b,  sizeof (b))*/
/*************************************************
 * Please use wait_time() function in boot
 * If you want to insert delay after kernel is active,
 * use sleep function that is supported by kernel.
 *************************************************/
static U32 g_initialized = INIT_NOT;
void wait_time(U32 time);
ISA1200_ERROR write_register(ISA1200_REG_ADDR addr, uint16 set_val);

void wait_time(U32 time) /* about 1us */
{


    volatile U32 ui_cnt0;
    volatile U32 ui_cnt1;

    if (INIT_NOT == g_initialized)
    {
        for (ui_cnt0 = 0; ui_cnt0 < time; ui_cnt0++)
        {
            /***************************************************
            // TODO: for port
            //     : make sure system clock
            ***************************************************/
            /* I assume there are three bubble in branch instruction.
            // So, it take time as 1us. */
            for (ui_cnt1 = 0; ui_cnt1 < (U32)(SYSTEM_CLOCK/1000000/4); ui_cnt1++)
                ; /* NULL */
        }
    }
    else
    {
        for (ui_cnt0 = 0; ui_cnt0 < 1000; ui_cnt0++)
        {
          /*  SAPA2_ERRMSG(("ERROR[SAPA2] : Do not use wiat_time() function after kernel is active.\n"));*/
        }
    }
}

void ISA1200_Enable(void)
{
	/*
	- isa_freq
		175Hz : 0xD6
		185Hz : 0xCA
	- isa_duty
		175Hz : 0xD6 일때 50% : 0x6B / 99% : 0xD6
		185Hz : 0xCA 일때 50% : 0x65 / 99% : 0xCA
		
	*/

	/*175hz 99% duty setting*/
	theSink.isa_freq = 0xD6; /*175hz*/
	theSink.isa_duty = 0x6B; /*duty 50%*/
	theSink.isa_dimlev = 0;
	
	PioSetDir32(ISAEN_PIN, ISAEN_PIN);	
	PioSetDir32(LEN_PIN, LEN_PIN);
	PioSetDir32(HEN_PIN, HEN_PIN);	
	
	PioSet32(ISAEN_PIN, ISAEN_PIN);
	PioSet32(HEN_PIN, HEN_PIN);
	wait_time(20);
	PioSet32(LEN_PIN, LEN_PIN);   
	
}

void ISA1200_Disable(void)
{
	PioSetDir32(ISAEN_PIN, ISAEN_PIN);	
	PioSetDir32(LEN_PIN, LEN_PIN);
	PioSetDir32(HEN_PIN, HEN_PIN);	
	
	PioSet32(ISAEN_PIN, 0);
	PioSet32(HEN_PIN, 0);
	wait_time(20);
	PioSet32(LEN_PIN, 0);  
	theSink.isa_dimlev = 0;
}

ISA1200_ERROR write_register(ISA1200_REG_ADDR addr, uint16 set_val)
{
	ISA1200_ERROR retval = WM8987L_NO_ERROR;

	U8 Transfer_Data[2];


	Transfer_Data[0] = addr; 
	Transfer_Data[1] = set_val; 

	wait_time(10); /*sdm070615 for test*/
	/***************************************************
	// TODO: for port
	//     : Do implementation of OEM i2c driver here.
	//     : SCL must be set under 100kHz (see s5m0065 spec.)
	***************************************************/

	/*  ret_len = I2cTransfer(uc_slave_addr, Transfer_Data, sizeof(Transfer_Data), NULL, 0);*/
	if (!WM8987L_TransferOK (Transfer_Data))
	{
		WM8987L_DEBUG(("**** WM8987L_TransferOK  is FAIL !!!!\n"));
		retval = WM8987L_ERROR_I2C_WRITE;
	}


	WM8987L_DEBUG(("write_register : 0x%x 0x%x \n",Transfer_Data[0],Transfer_Data[1]));

	return retval;
}

void ISA1200_Vibrator_On(void)
{
	#if 0
	uint16 result=0;
	result = (uint16)write_register(ADDR_ISA1200_LDOCTRL, 0x80);						 
	result = (uint16)write_register(ADDR_ISA1200_LDOCTRL, 0x05);
	DEBUG_ISA1200(("********** ISA LDO CONTRO  RESULT  [%d]\n", result)) ;
	result = (uint16)write_register(ADDR_ISA1200_INIT, 0x91);  /* 98 OR 9C */
	DEBUG_ISA1200(("********** ISAINIT RESULT  [%d]\n", result)) ;
	result = (uint16)write_register(ADDR_ISA1200_MOTORTYPE, 0xC0); /* 80 EXTCLKSEL ONLY , 90 ADD PLL ENABLE */
	DEBUG_ISA1200(("********** ISAMOTOR TYPE PPL ENABLE  RESULT  [%d]\n", result)) ;
	result = (uint16)write_register(ADDR_ISA1200_WAVESIZE, 0x13);
	DEBUG_ISA1200(("********** ISA PPL WAVE HZ SIZE  [%d]\n", result)) ;
	
	result = (uint16)write_register(ADDR_ISA1200_PWMHIGH,  0x00/*theSink.isa_duty + theSink.isa_dimlev*/);   /* b0 is ok voltage duty */
	DEBUG_ISA1200(("********** ISA PWM HIGH  RESULT  [%d]\n", theSink.isa_dimlev)) ;
	result = (uint16)write_register(ADDR_ISA1200_PWMPERIOD,  theSink.isa_freq);
	DEBUG_ISA1200(("********** ISA PWM PERIOD  RESULT  [%d]\n", result)) ;
	#endif
}

void ISA1200_Vibrator_Off(void)
{
	uint16 result=0;

	result = (uint16)write_register(ADDR_ISA1200_INIT, 0x00);
	DEBUG_ISA1200(("ISAINIT POWER DOWN RESULT  [%d]\n", result)) ;	  
	wait_time(500);
}

void ISA1200_SetDuty(uint16 duty)
{	
	#if 1
	uint16 result=0;
	if(!duty)
	{
		result = (uint16)write_register(ADDR_ISA1200_LDOCTRL, 0x80);
		wait_time(100);
		result = (uint16)write_register(ADDR_ISA1200_LDOCTRL, 0x05);
		wait_time(150);
		DEBUG_ISA1200(("********** ISA LDO CONTRO  RESULT  [%d]\n", result)) ;
		result = (uint16)write_register(ADDR_ISA1200_INIT, 0x91);  /* 98 OR 9C */
		wait_time(150);
		DEBUG_ISA1200(("********** ISAINIT RESULT  [%d]\n", result)) ;
		result = (uint16)write_register(ADDR_ISA1200_MOTORTYPE, 0xC0); /* 80 EXTCLKSEL ONLY , 90 ADD PLL ENABLE */
		wait_time(150);
		DEBUG_ISA1200(("********** ISAMOTOR TYPE PPL ENABLE  RESULT  [%d]\n", result)) ;
		result = (uint16)write_register(ADDR_ISA1200_WAVESIZE, 0x13);
		wait_time(150);
		DEBUG_ISA1200(("********** ISA PPL WAVE HZ SIZE  [%d]\n", result));
		result = (uint16)write_register(ADDR_ISA1200_PWMHIGH,  theSink.isa_duty + duty);   /* b0 is ok voltage duty */
		wait_time(150);
		DEBUG_ISA1200(("********** ISA PWM HIGH  RESULT  [%d]\n", theSink.isa_duty + duty)) ;
		result = (uint16)write_register(ADDR_ISA1200_PWMPERIOD,  theSink.isa_freq);
		DEBUG_ISA1200(("********** ISA PWM PERIOD  RESULT  [%d]\n", result)) ;
	}
	else
	{
		#if 1
		result = (uint16)write_register(ADDR_ISA1200_PWMHIGH,  theSink.isa_duty + duty);   /* b0 is ok voltage duty */
		#endif
		DEBUG_ISA1200(("duty : %x\n",theSink.isa_duty + duty));
	}
	#else
	uint16 result=0;
	result = (uint16)write_register(ADDR_ISA1200_PWMHIGH,  theSink.isa_duty + duty);   /* b0 is ok voltage duty */
	DEBUG_ISA1200(("********** ISA PWM HIGH  RESULT  [%d]\n", duty)) ;
	#endif
}

