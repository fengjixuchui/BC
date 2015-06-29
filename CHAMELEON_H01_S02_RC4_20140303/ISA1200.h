



#ifndef U8
#define U8 unsigned char
#endif

#ifndef U32
#define U32 unsigned int
#endif

#ifndef S8
#define S8 char
#endif


typedef enum
{
	WM8987L_NO_ERROR = 0,
	WM8987L_ERROR_I2C_WRITE,

	WM8987L_ERROR_I2C_READ,
	WM8987L_ERROR_INVALID_PATH,
	WM8987L_ERROR_OUT_OF_VOL_RANGE_SPK,
	WM8987L_ERROR_OUT_OF_VOL_RANGE_HP,

	WM8987L_ERROR_OUT_OF_VOL_VALUE_SPK,
	WM8987L_ERROR_OUT_OF_VOL_VALUE_HP,

	WM8987L_ERROR_OUT_OF_MIX_VOL_RANGE,

	WM8987L_ERROR_OUT_OF_EQ_MODE,
	WM8987L_ERROR_OUT_OF_EQ_BAND_NUM,
	WM8987L_ERROR_OUT_OF_EQ_LEVEL,
	WM8987L_ERROR_OUT_OF_EQ_VALUE,

	WM8987L_ERROR_NOT_SUPPORT_FS,
	WM8987L_ERROR_INVALID_CMD
} ISA1200_ERROR;


typedef enum
{
	ADDR_ISA1200_LDOCTRL = 0x00,

	ADDR_IN1_CTRL1,
	ADDR_IN1_CTRL2,
	ADDR_IN1_CTRL3,
	ADDR_IN1_CTRL4,
	ADDR_IN2_CTRL1,
	ADDR_IN2_CTRL2,
	ADDR_IN2_CTRL3,
	ADDR_IN2_CTRL4,

	ADDR_VOL_SET,
	ADDR_L_VOL_SPK,
	ADDR_R_VOL_SPK,
	ADDR_L_VOL_HP,
	ADDR_R_VOL_HP,
	ADDR_MUTE_CTRL,
	ADDR_PWM_CTRL,

	ADDR_SMARTBASS, /*  0x10*/
	ADDR_SIGROUTE,
	ADDR_MIXING,

	ADDR_EQ_MODE,
	ADDR_EQ1,
	ADDR_EQ2,
	ADDR_EQ3,
	ADDR_EQ4,
	ADDR_EQ5,

	ADDR_SPKFLAT1_QCB,
	ADDR_SPKFLAT1_FREQ,
	ADDR_SPKFLAT2_QCB,
	ADDR_SPKFLAT2_FREQ,
	ADDR_SPKFLAT3_QCB,
	ADDR_SPKFLAT3_FREQ,
	ADDR_SPKFLAT4_QCB,
	ADDR_SPKFLAT4_FREQ, /*  0x20 */
	ADDR_SPKFLAT5_QCB,
	ADDR_SPKFLAT5_FREQ,

	ADDR_S3D_PARAM,
	ADDR_S3D_LEVEL,

	ADDR_FAC_ADJ,
	ADDR_PLL_P2,  /* use when direct pll setting */
	ADDR_PLL_P1, /*  use when direct pll setting */
	ADDR_PLL_M2, /*  use when direct pll setting */
	ADDR_PLL_M1, /* use when direct pll setting */
	ADDR_PLL_CS, /* use when direct pll setting */

	ADDR_SLOPE_ADJ1,
	ADDR_SLOPE_ADJ2,
	ADDR_PEDEC_CTRL1,
	ADDR_PEDEC_CTRL2,

	ADDR_A_TEST,


	ADDR_ISA1200_INIT,                                      /* 0x30 */
	ADDR_ISA1200_MOTORTYPE,                          /* 0x31 */
	ADDR_ISA1200_EFFECT,                                 /* 0x32 */
	ADDR_ISA1200_WAVESIZE,                            /* 0x33 */
	ADDR_ISA1200_SIGNDIRECT,                         /* 0x34 */
	ADDR_ISA1200_PWMHIGH,                             /* 0x35 */
	ADDR_ISA1200_PWMPERIOD,                         /* 0x36 */
	ADDR_ISA1200_WAVEROTATIONRESERVE,     /* 0x37 */
	ADDR_ISA1200_WAVEREQ,                             /* 0x38 */
	ADDR_ISA1200_PLLDRV,                                 /* 0x39 */
	ADDR_ISA1200_PLLFEEDBECK,                       /* 0x3A */
	ADDR_ISA1200_EFFECTHIGHDUTY,                /* 0x3B */
	ADDR_ISA1200_EFFECTPERIOD,                     /* 0x3C */
	ADDR_ISA1200_EFFECTFRTIME,                     /* 0x3D */







	ADDR_SYS_RDY,
	ADDR_A_STATUS,

	WM8987L_ADDR_MAX
} ISA1200_REG_ADDR;

#define SLAVE_ADDRESS_ISA1200LOW 0x90
#define SLAVE_ADDRESS_ISA1200HIGH 0x92

#define SYSTEM_CLOCK 26000000 /* assumpt about 26MHz*/

#define INIT_NOT  (0x1F)
#define INIT_DONE (0xF1)

#define ISA1200_DUTY_PER_LEV	15

extern void ISA1200_Enable(void);
extern void ISA1200_Disable(void);
extern void ISA1200_Vibrator_On(void);
extern void ISA1200_Vibrator_Off(void);
extern void ISA1200_SetDuty(uint16 duty);

