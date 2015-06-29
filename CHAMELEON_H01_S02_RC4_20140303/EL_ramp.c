
#include "sink_private.h"
#include "sink_debug.h"
#include <i2c.h>
#include <ps.h>
#include <codec.h>
#include <pio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "EL_ramp.h"
#include "sink_buttons.h"

#define WM8987L_Transfer(a) I2cTransfer ((SLAVE_ADDRESS_MAX14521E_WR), (a), sizeof (a), NULL, 0)
#define WM8987L_TransferOK(a) (WM8987L_Transfer (a) == 1 + sizeof (a))

#ifdef DEBUG_WM8987L
#define WM8987L_DEBUG(x) DEBUG(x)
#else
#define WM8987L_DEBUG(x) 
#endif

#ifdef DEBUG_MAX14521EL
#define DEBUG_MAX14521E(x) DEBUG(x)
#else
#define DEBUG_MAX14521E(x) 
#endif


#ifdef MAX14521E_EL_RAMP_DRIVER

static U32 g_initialized = INIT_NOT;


void wait_time2(U32 time); /* about 1us */
uint8 write_register2(MAX14521E_REG_ADDR addr, uint16 set_val);

void wait_time2(U32 time) /* about 1us */
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

uint8 write_register2(MAX14521E_REG_ADDR addr, uint16 set_val)
{
	uint8 retval = TRUE;

	uint8 Transfer_Data[2];


	Transfer_Data[0] = addr; 
	Transfer_Data[1] = set_val; 

	wait_time2(10); /*sdm070615 for test*/
	/***************************************************
	// TODO: for port
	//     : Do implementation of OEM i2c driver here.
	//     : SCL must be set under 100kHz (see s5m0065 spec.)
	***************************************************/

	/*  ret_len = I2cTransfer(uc_slave_addr, Transfer_Data, sizeof(Transfer_Data), NULL, 0);*/
	if (!WM8987L_TransferOK (Transfer_Data))
	{
		retval = FALSE;
	}

	WM8987L_DEBUG(("write_register : 0x%x 0x%x \n",Transfer_Data[0],Transfer_Data[1]));

	return TRUE;
}


void EL_Ramp_Init(void)
{
	theSink.el_enable = FALSE;
	theSink.el_pattern_state = PATTERN_0;
	theSink.el_pattern_on = 0;
	theSink.el_pattern_interval = 0;
	PioSetDir32(EL_SET_PIN_MASK, EL_SET_PIN_MASK);	
	PioSet32(EL_SET_PIN_MASK, EL_SET_PIN_MASK);
}

void EL_Ramp_Enable(void)
{
	theSink.el_enable = TRUE;
	theSink.el_pattern_state = PATTERN_0;
	theSink.el_pattern_on = 0;
	theSink.el_pattern_interval = 0;
	PioSetDir32(EL_SET_PIN_MASK, EL_SET_PIN_MASK);	
	PioSet32(EL_SET_PIN_MASK, 0);

}

void EL_Ramp_Disable(void)
{
	theSink.el_enable = FALSE;
	theSink.el_pattern_state = PATTERN_0;
	theSink.el_pattern_on = 0;
	theSink.el_pattern_interval = 0;
	PioSetDir32(EL_SET_PIN_MASK, EL_SET_PIN_MASK);	
	PioSet32(EL_SET_PIN_MASK, EL_SET_PIN_MASK);
}

uint8 EL_Ramp_GetStatus(void)
{
	return theSink.el_enable;
}

void EL_Ramp_On(void)
{
	uint16 result=0;
	result = (uint16)write_register2(ADDR_MAX14521E_DEVICE_ID, 0xB2);						 
	result = (uint16)write_register2(ADDR_MAX14521E_POWER_MODE, 0x01);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_POWER_MODE  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_OUTPUT_FREQ, 0xD6);  
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_OUTPUT_FREQ  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_SLOPE_SHAPE, 0x01); 
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_SLOPE_SHAPE  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_BOOST_CONV_FREQ, 0x00);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_BOOST_CONV_FREQ  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_AUDIO_EFFECTS,  0xC1);   
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_AUDIO_EFFECTS  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_EL1_PEAK_VOLTAGE,  0x1e);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_EL1_PEAK_VOLTAGE  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_EL2_PEAK_VOLTAGE,  0x00);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_EL2_PEAK_VOLTAGE  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_EL3_PEAK_VOLTAGE,  0x00);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_EL3_PEAK_VOLTAGE  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_EL4_PEAK_VOLTAGE,  0x00);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_EL4_PEAK_VOLTAGE  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_EL_SEND,  0x00);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_EL_SEND  [%d]\n", result)) ;
	
	/*
	result = (uint16)I2cTransfer ((SLAVE_ADDRESS_MAX14521E_WR), ADDR_MAX14521E_EL_SEND, 1, NULL, 0);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_EL_SEND  [%d]\n", result)) ;
	*/
}

void EL_Ramp_Off(void)
{
	uint16 result=0;
	result = (uint16)write_register2(ADDR_MAX14521E_POWER_MODE, 0x00);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_POWER_MODE  [%d]\n", result)) ;
	result = (uint16)write_register2(ADDR_MAX14521E_EL_SEND,  0x00);
	DEBUG_MAX14521E(("********** ADDR_MAX14521E_EL_SEND  [%d]\n", result)) ;
}
#endif

