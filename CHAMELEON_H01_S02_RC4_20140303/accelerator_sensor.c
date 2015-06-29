
#include "sink_private.h"
#include "sink_debug.h"
#include <i2c.h>
#include <ps.h>
#include <codec.h>
#include <pio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "accelerator_system.h"
#include "sink_buttons.h"

#define WM8987L_Write(a) I2cTransfer ((SLAVE_ADDRESS_MMA8452Q_WR), (a), sizeof(a), NULL, 0)
#define WM8987L_WriteOK(a) (WM8987L_Write (a) == 1 + sizeof(a))

#define WM8987L_Read(a,b,n) I2cTransfer ((SLAVE_ADDRESS_MMA8452Q_RD), (a), 1, (b), n)
#define WM8987L_ReadOK(a,b) (WM8987L_Read (a,b) == 1 + 1)


#ifdef DEBUG_WM8987L
#define WM8987L_DEBUG(x) DEBUG(x)
#else
#define WM8987L_DEBUG(x) 
#endif

#ifdef DEBUG_MMA8452QL
#define DEBUG_MMA8452Q(x) DEBUG(x)
#else
#define DEBUG_MMA8452Q(x) 
#endif



static U32 g_initialized = INIT_NOT;
void wait_time3(U32 time); /* about 1us */

void IIC_RegWrite(uint8 reg,uint8 val)
{
	uint8 Transfer_Data[2];
	uint8 ret;
	Transfer_Data[0] = reg;
	Transfer_Data[1] = val;

	wait_time3(10); /*sdm070615 for test*/
	ret = WM8987L_Write(Transfer_Data);
  	if(ret == (sizeof(Transfer_Data) + 1))
  	{
  		DEBUG_MMA8452Q(("********** WM8987L_WriteOK : %x -> REG:%x, %x\n",	ret, Transfer_Data[0], Transfer_Data[1])) ;
  	}
	else
	{
		DEBUG_MMA8452Q(("********** WM8987L_WriteFail!! : %x -> REG:%x, %x\n", ret, Transfer_Data[0], Transfer_Data[1])) ;
	}
	
	
}


/*********************************************************
* IIC Read Register
*********************************************************/
uint8 IIC_RegRead(uint8 reg)
{
	uint8 b;
	uint8 Transfer_Data[2];
	uint8 Recv_Data[2];
	uint16 ret;
	Transfer_Data[0] = reg;
	Transfer_Data[1] = 0;
	ret = WM8987L_Read(Transfer_Data, Recv_Data, 1);
	DEBUG_MMA8452Q(("********** WM8987L_ReadOK-1 REG:%x,%x RES %x\n", Transfer_Data[0], Transfer_Data[1], Recv_Data[0])) ;	
  	b = Recv_Data[0];
	return b;
}

#if 0
/*********************************************************\
* IIC Write Multiple Registers
\*********************************************************/
void IIC_RegWriteN(uint8 reg1, uint8 N, uint8 *array)
{
	IICC_TX = 1;                                  // Transmit Mode
	IIC_Start();                                  // Send Start
	IIC_CycleWrite(address);                      // Send IIC "Write" Address
	IIC_CycleWrite(reg1);                         // Send Register
	while (N>0)                                   // Send N Values
	{
	IIC_CycleWrite(*array);
	array++;
	N--;
	}
	IIC_Stop();                                   // Send Stop
}
#endif

/*********************************************************\
* IIC Read Multiple Registers
\*********************************************************/
void IIC_RegReadN(uint8 reg1, uint8 N, uint8 *array)
{
	uint8 Transfer_Data[2];
	uint16 ret;
	Transfer_Data[0] = reg1;
	Transfer_Data[1] = 0;
	ret = WM8987L_Read(Transfer_Data, array, N);
	DEBUG_MMA8452Q(("********** IIC_RegReadN-1 %x, %x, %x, %x, %x, %x\n", array[0], array[1], array[2], array[3], array[4], array[5])) ;
}



void wait_time3(U32 time) /* about 1us */
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





void MMA845x_Init(void)
{
	DEBUG_MMA8452Q(("********** Sensor DeviceID  [%x]\n", deviceID)) ;

	MMA845x_Standby();
	/*
	**  Configure sensor for:
	**    - Sleep Mode Poll Rate of 50Hz (20ms)
	**    - System Output Data Rate of 200Hz (5ms)
	**    - Full Scale of +/-2g
	*/
	IIC_RegWrite(CTRL_REG1, ASLP_RATE_20MS+DATA_RATE_10MS);
	IIC_RegWrite(XYZ_DATA_CFG_REG, FULL_SCALE_2G);
}

void MMA845x_Active(void)
{
	IIC_RegWrite(CTRL_REG1, (IIC_RegRead(CTRL_REG1) | ACTIVE_MASK));
}

void MMA845x_Standby(void)
{
	uint8 n;
	/*
	**  Read current value of System Control 1 Register.
	**  Put sensor into Standby Mode.
	**  Return with previous value of System Control 1 Register.
	*/
	n = IIC_RegRead(CTRL_REG1);
	IIC_RegWrite(CTRL_REG1, n & ~ACTIVE_MASK);
}

/*********************************************************\
**  Activate sensor interrupts
\*********************************************************/
void InterruptsActive (uint8 ctrl_reg3, uint8 ctrl_reg4, uint8 ctrl_reg5)
{
	MMA845x_Standby();
	IIC_RegWrite(CTRL_REG3, (IIC_RegRead(CTRL_REG3) | ctrl_reg3));
	IIC_RegWrite(CTRL_REG4, ctrl_reg4);
	IIC_RegWrite(CTRL_REG5, ctrl_reg5);
	MMA845x_Active();
}


