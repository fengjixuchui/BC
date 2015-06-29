/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2013

FILE NAME
    sink_configmanager.h
    
DESCRIPTION
    Configuration manager for the device - resoponsible for extracting user information out of the 
    PSKEYs and initialising the configurable nature of the device components
    
*/
#ifndef SINK_POWER_MANAGER_H
#define SINK_POWER_MANAGER_H

#include <csrtypes.h>
#include <power.h>
#include "sink_debug.h"


#define PSKEY_CHARGER_CALC_VTERM 0x25b6

typedef enum
{
    battery_level_automatic         = (1<<0),
    battery_level_user              = (1<<1),
    battery_level_initial_reading   = (1<<2)
} battery_level_source;

typedef enum
{
    vsel_chg,
    vsel_bat
} sink_vsel;

typedef struct
{
    unsigned            charge:1;
    power_boost_enable  boost:2;
    sink_vsel        vsel:1;
    unsigned            power_off:1;
    unsigned            disable_leds:1;
    unsigned            unused:2;
    unsigned            current:8;
} sink_charge_current;

typedef enum
{
    vterm_default,  /* Use the default termination voltage */
    vterm_voltage,  /* Use voltage as specified in mV (BC7) */
    vterm_trim      /* Reduce trim by specified amount (pre-BC7) */
} sink_vterm;

typedef struct
{
    sink_vterm   type:4;   
    unsigned        trim:4;
    unsigned        voltage:8;
} sink_charge_termination;

typedef struct
{
    sink_charge_current      current;
    sink_charge_termination  termination;
} sink_charge_setting;

typedef struct
{
    unsigned                unused:5;
    battery_level_source    sources:3;
    unsigned                event:8;        /* Event to generate */
} sink_battery_setting;

typedef struct
{
    sink_battery_setting bat_events[POWER_MAX_VBAT_REGIONS];
    sink_charge_setting  chg_settings[POWER_MAX_VTHM_REGIONS];
} sink_power_settings;

typedef struct
{
    power_config            config;
    sink_power_settings  settings;
} sink_power_config;

typedef struct
{
    uint16 max_battery_v;
    uint16 min_battery_v;
} sink_battery_limits;


/****************************************************************************
NAME    
    powerManagerConfig
    
DESCRIPTION
      Configure power management
    
RETURNS
    void
*/
void powerManagerConfig(const power_config * config);


/****************************************************************************
NAME    
    powerManagerChargerConnected
    
DESCRIPTION
      This function is called when the charger is plugged into the device
    
RETURNS
    void
*/
void powerManagerChargerConnected(void);


/****************************************************************************
NAME    
    powerManagerChargerDisconnected
    
DESCRIPTION
      This function is called when the charger is unplugged from the device
    
RETURNS
    void
*/
void powerManagerChargerDisconnected(void);


/*************************************************************************
NAME    
    handlePowerMessage
    
DESCRIPTION
    handles the Battery/Charger Monitoring Messages

RETURNS
    
*/
void handlePowerMessage( Task task, MessageId id, Message message );


/****************************************************************************
NAME
    powerManagerIsVbatLow

DESCRIPTION
      Call this function to check the low battery warning state

RETURNS
    TRUE or FALSE
*/
#define powerManagerIsVbatLow() (theSink.battery_state == POWER_BATT_LOW)


/****************************************************************************
NAME
    powerManagerIsVbatCritical

DESCRIPTION
      Call this function to check the critical battery warning state

RETURNS
    TRUE or FALSE
*/
#define powerManagerIsVbatCritical() (theSink.battery_state == POWER_BATT_CRITICAL)


/****************************************************************************
NAME    
    powerManagerIsChargerConnected
    
DESCRIPTION
    This function is called by applications to check whether the charger has 
    been plugged into the device
    
RETURNS
    bool
*/
bool powerManagerIsChargerConnected(void);


/****************************************************************************
NAME    
    powerManagerIsChargerDisabled
    
DESCRIPTION
    This function is called by applications to check whether the charger has 
    been disabled
    
RETURNS
    bool
*/
bool powerManagerIsChargerDisabled(void);


/****************************************************************************
NAME    
    powerManagerIsChargerFullCurrent
    
DESCRIPTION
    This function is called by applications to check whether the charger is
    in (potentially) drawing full configured current.
    
RETURNS
    bool
*/
bool powerManagerIsChargerFullCurrent(void);


/****************************************************************************
NAME
    powerManagerIsVthmCritical
    
DESCRIPTION
    This function is called by applications to check whether the battery 
    temperature has reached critical level where the device must be powered
    off
    
RETURNS
    bool
*/
bool powerManagerIsVthmCritical(void);


/****************************************************************************
NAME    
    powerManagerReadVbat
    
DESCRIPTION
      Call this function to take an immediate battery reading and sent to AG.
    
RETURNS
    void
*/
void powerManagerReadVbat(battery_level_source source);


/****************************************************************************
NAME    
    powerManagerGetLBIPM
    
DESCRIPTION
      Returns the Power level to use for Low Battery Intelligent Power Management (LBIPM)
    Note will always return high level if this feature is disabled.
    
RETURNS
    void
*/
power_battery_level powerManagerGetLBIPM( void );


/****************************************************************************
NAME    
    powerManagerChargerSetup
    
DESCRIPTION
    Update the charger settings based on USB limits and latest temperature
    reading. Updates current, trim, boost charge settings and enables or 
    disables the charger. On BC7 this can also be used to select whether the
    chip draws power from VBAT or VBYP. 
    
RETURNS
    void
*/
void powerManagerChargerSetup(voltage_reading* vthm);


/****************************************************************************
NAME    
    powerManagerChargerSetup
    
DESCRIPTION
    Update the charger settings based on latest reading of VTHM
    
RETURNS
    void
*/
void powerManagerUpdateChargeCurrent(void);


/****************************************************************************
NAME    
    powerManagerCheckPanic
    
DESCRIPTION
    Check if the last reset was caused by Panic/ESD
    
RETURNS
    void
*/
#ifdef INSTALL_PANIC_CHECK
void powerManagerCheckPanic(void);
#else
#define powerManagerCheckPanic() theSink.panic_reconnect = theSink.features.ReconnectOnPanic
#endif

#endif /* SINK_POWER_MANAGER_H */
