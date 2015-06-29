
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
	ADDR_MAX14521E_DEVICE_ID = 0x00,
	ADDR_MAX14521E_POWER_MODE,
	ADDR_MAX14521E_OUTPUT_FREQ,
	ADDR_MAX14521E_SLOPE_SHAPE,
	ADDR_MAX14521E_BOOST_CONV_FREQ,
	ADDR_MAX14521E_AUDIO_EFFECTS,
	ADDR_MAX14521E_EL1_PEAK_VOLTAGE,
	ADDR_MAX14521E_EL2_PEAK_VOLTAGE,
	ADDR_MAX14521E_EL3_PEAK_VOLTAGE,
	ADDR_MAX14521E_EL4_PEAK_VOLTAGE,
	ADDR_MAX14521E_EL_SEND,

	MAX14521E_ADDR_MAX
} MAX14521E_REG_ADDR;

#define SLAVE_ADDRESS_MAX14521E_WR 0xF0
#define SLAVE_ADDRESS_MAX14521E_RD 0xF1
#define SYSTEM_CLOCK 26000000 /* assumpt about 26MHz*/

#define INIT_NOT  (0x1F)
#define INIT_DONE (0xF1)

#define DEFAULT_PATTERN_INTERVAL	100

#define PATTERN_0		0
#define PATTERN_1		1
#define PATTERN_2		2
#define PATTERN_3		3
#define PATTERN_4		4

#define EL_SET_PIN  	(9)
#define EL_SET_PIN_MASK	((uint32)1 << EL_SET_PIN)


extern void EL_Ramp_Init(void);
extern void EL_Ramp_Enable(void);
extern void EL_Ramp_Disable(void);
extern void EL_Ramp_On(void);
extern void EL_Ramp_Off(void);
extern uint8 EL_Ramp_GetStatus(void);


