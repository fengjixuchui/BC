/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013

FILE NAME
    sink_usb.c        

DESCRIPTION
    Application level implementation of USB features

NOTES
    - Conditional on ENABLE_USB define
    - Ensure USB host interface and VM control of USB enabled in project/PS
*/

#include "sink_private.h"
#include "sink_debug.h"
#include "sink_powermanager.h"
#include "sink_usb.h"
#include "sink_usb_descriptors.h"
#include "sink_configmanager.h"
#include "sink_audio.h"
#include "sink_a2dp.h"
#include "sink_statemanager.h"
#include "sink_pio.h"
#include "sink_tones.h"
#include "sink_display.h"
#include "sink_audio_routing.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include <usb_device_class.h>
#include <power.h>
#include <panic.h>
#include <print.h>
#include <usb.h>
#include <file.h>
#include <stream.h>
#include <source.h>
#include <boot.h>
#include <string.h>
#include <charger.h>

#ifdef ENABLE_USB
#ifdef ENABLE_USB_AUDIO

#include <csr_a2dp_decoder_common_plugin.h>
#define USB_AUDIO_DISCONNECT_DELAY (500)

static const usb_plugin_info usb_plugins[] = 
{
    {SAMPLE_RATE_STEREO, NULL                , usb_plugin_stereo},
    {SAMPLE_RATE_CVC,    &usb_cvc_config     , usb_plugin_mono_nb},
    {SAMPLE_RATE_CVC_WB, &usb_cvc_wb_config  , usb_plugin_mono_wb}
};

#endif /* ENABLE_USB_AUDIO */

#ifdef DEBUG_USB
    #define USB_DEBUG(x) DEBUG(x)
#else
    #define USB_DEBUG(x) 
#endif

#define USB_CLASS_ENABLED(x)        ((bool)(theSink.usb.config.device_class & (x)))
#define USB_CLASS_DISABLE(x)        theSink.usb.config.device_class &= ~(x)
#define USB_CLASS_ENABLE(x)         theSink.usb.config.device_class |= (x)

#ifdef HAVE_VBAT_SEL
#define usbDeadBatteryAtBoot()      (ChargerGetBatteryStatusAtBoot() == CHARGER_BATTERY_DEAD)
#else
#define usbDeadBatteryAtBoot()      (TRUE) /* Assume dead battery until initial VBAT reading taken */
#endif
#define usbDeadBatteryProvision()   (theSink.usb.dead_battery && !theSink.usb.enumerated && !theSink.usb.deconfigured)

const char root_name[] = "usb_root";
const char fat_name[]  = "usb_fat";

/****************************************************************************
NAME 
    usbUpdateChargeCurrent
    
DESCRIPTION
    Set the charge current based on USB state
    
RETURNS
    void
*/ 
void usbUpdateChargeCurrent(void)
{
    /* Don't change anything if battery charging disabled */
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_BATTERY_CHARGING))
        powerManagerUpdateChargeCurrent();
}


/****************************************************************************
NAME 
    usbSetLowPowerMode
    
DESCRIPTION
    If delay is non zero queue a message to reset into low powered mode. If
    delay is zero do nothing.
    
RETURNS
    void
*/ 
static void usbSetLowPowerMode(uint8 delay)
{
    /* Only queue low power mode if not enumerated and attached to normal host/hub */
    if(!theSink.usb.enumerated && delay && (UsbAttachedStatus() == HOST_OR_HUB)) 
    {
        USB_DEBUG(("USB: Queue low power in %d sec\n", delay));
        MessageSendLater(&theSink.task, EventUsbLowPowerMode, 0, D_SEC(delay));
    }
}


/****************************************************************************
NAME 
    usbSetBootMode
    
DESCRIPTION
    Set the boot mode to default or low power
    
RETURNS
    void
*/ 
void usbSetBootMode(uint8 bootmode)
{
    /* Don't change anything if battery charging disabled */
    if(!USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_BATTERY_CHARGING))
        return;

    if(BootGetMode() != bootmode)
    {
        USB_DEBUG(("USB: Set Mode %d\n", bootmode));
        BootSetMode(bootmode);
    }
}


/****************************************************************************
NAME 
    handleUsbMessage
    
DESCRIPTION
    Handle firmware USB messages
    
RETURNS
    void
*/ 
void handleUsbMessage(Task task, MessageId id, Message message) 
{
    USB_DEBUG(("USB: "));
    switch (id)
    {
        case MESSAGE_USB_ATTACHED:
        {
            USB_DEBUG(("MESSAGE_USB_ATTACHED\n"));
            usbUpdateChargeCurrent();
            audioHandleRouting(audio_source_none);
            usbSetLowPowerMode(theSink.usb.config.attach_timeout);
            if(theSink.usb.dead_battery)
                MessageSendLater(&theSink.task, EventUsbDeadBatteryTimeout, 0, D_MIN(45));
            break;
        }
        case MESSAGE_USB_DETACHED:
        {
            USB_DEBUG(("MESSAGE_USB_DETACHED\n"));
            theSink.usb.enumerated = FALSE;
            theSink.usb.suspended  = FALSE;
            theSink.usb.deconfigured = FALSE;
            usbUpdateChargeCurrent();
            audioHandleRouting(audio_source_none);
            MessageCancelAll(&theSink.task, EventUsbLowPowerMode);
            MessageCancelAll(&theSink.task, EventUsbDeadBatteryTimeout);
            break;
        }
        case MESSAGE_USB_ENUMERATED:
        {
            USB_DEBUG(("MESSAGE_USB_ENUMERATED\n"));
            if(!theSink.usb.enumerated)
            {
                theSink.usb.enumerated = TRUE;
                usbUpdateChargeCurrent();
                MessageCancelAll(&theSink.task, EventUsbLowPowerMode);
                MessageCancelAll(&theSink.task, EventUsbDeadBatteryTimeout);
            }
            break;
        }
        case MESSAGE_USB_SUSPENDED:
        {
            MessageUsbSuspended* ind = (MessageUsbSuspended*)message;
            USB_DEBUG(("MESSAGE_USB_SUSPENDED - %s\n", (ind->has_suspended ? "Suspend" : "Resume")));
            if(ind->has_suspended != theSink.usb.suspended)
            {
                theSink.usb.suspended = ind->has_suspended;
                usbUpdateChargeCurrent();
            }
            break;
        }
        case MESSAGE_USB_DECONFIGURED:
        {
            USB_DEBUG(("MESSAGE_USB_DECONFIGURED\n"));
            if(theSink.usb.enumerated)
            {
                theSink.usb.enumerated = FALSE;
                theSink.usb.deconfigured  = TRUE;
                usbUpdateChargeCurrent();
                usbSetLowPowerMode(theSink.usb.config.deconfigured_timeout);
            }
            break;
        }
        case MESSAGE_USB_ALT_INTERFACE:
        {
            uint16 interface_id;
            MessageUsbAltInterface* ind = (MessageUsbAltInterface*)message;

            USB_DEBUG(("MESSAGE_USB_ALT_INTERFACE %d %d\n", ind->interface, ind->altsetting));
            UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_MIC_INTERFACE_ID, &interface_id);
            if(interface_id == ind->interface)
            {
                theSink.usb.mic_active = (ind->altsetting ? TRUE : FALSE);
                USB_DEBUG(("USB: Mic ID %d active %d\n", interface_id, theSink.usb.mic_active));
            }
            UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_SPEAKER_INTERFACE_ID, &interface_id);
            if(interface_id == ind->interface)
            {
                theSink.usb.spkr_active = (ind->altsetting ? TRUE : FALSE);
                USB_DEBUG(("USB: Speaker ID %d active %d\n", interface_id, theSink.usb.spkr_active));
            }
#ifdef ENABLE_USB_AUDIO            
            /* check for changes in required audio routing */
            USB_DEBUG(("USB: MESSAGE_USB_ALT_INTERFACE checkAudioRouting\n"));
            MessageCancelFirst(&theSink.task, EventCheckAudioRouting);            
            MessageSendLater(&theSink.task, EventCheckAudioRouting, 0, USB_AUDIO_DISCONNECT_DELAY);            
#endif            
            break;
        }
        case USB_DEVICE_CLASS_MSG_AUDIO_LEVELS_IND:
        {
            USB_DEBUG(("USB_DEVICE_CLASS_MSG_AUDIO_LEVELS_IND\n"));
            usbAudioSetVolume();
            break;
        }
        default:
        {
            USB_DEBUG(("Unhandled USB message 0x%x\n", id));
            break;
        }
    }
}


/****************************************************************************
NAME 
    usbFileInfo
    
DESCRIPTION
    Get file info (index and size) for a given file name.
    
RETURNS
    void
*/ 
static void usbFileInfo(const char* name, uint8 size_name, usb_file_info* info)
{
    Source source;
    info->index = FileFind(FILE_ROOT, name, size_name);
    source = StreamFileSource(info->index);
    info->size = SourceSize(source);
    SourceClose(source);
}


/****************************************************************************
NAME 
    usbFileName
    
DESCRIPTION
    Get file name from USB root
    
RETURNS
    void
*/ 
static void usbFileName(usb_file_info* root, usb_file_name_info* result)
{
    Source source = StreamFileSource(root->index);
    usb_file_name* file = (usb_file_name*)SourceMap(source);

    result->size = 0;

    if(file)
    {
        memmove(result->name, file->name, USB_NAME_SIZE);
        for(result->size = 0; result->size < USB_NAME_SIZE; result->size++)
            if(file->name[result->size] == ' ')
                break;
        *(result->name + result->size) = '.';
        result->size++;
        memmove(result->name + result->size, file->ext, USB_EXT_SIZE);
        result->size += USB_EXT_SIZE;
        SourceClose(source);
    }
#ifdef DEBUG_USB
    {
    uint8 count;
    USB_DEBUG(("USB: File Name "));
    for(count = 0; count < result->size; count++)
        USB_DEBUG(("%c", result->name[count]));
    USB_DEBUG(("\n"));
    }
#endif
}


/****************************************************************************
NAME 
    usbTimeCriticalInit
    
DESCRIPTION
    Initialise USB. This function is time critical and must be called from
    _init. This will fail if either Host Interface is not set to USB or
    VM control of USB is FALSE in PS. It may also fail if Transport in the
    project properties is not set to USB VM.
    
RETURNS
    void
*/ 
void usbTimeCriticalInit(void)
{
#ifdef ENABLE_USB_AUDIO
    const usb_plugin_info* plugin;
#endif
    usb_device_class_status status;
    usb_file_info root;
    usb_file_info file;
    usb_file_name_info file_name;

    USB_DEBUG(("USB: Time Critical\n"));

    /* Default to not configured or suspended */
    theSink.usb.ready = FALSE;
    theSink.usb.enumerated = FALSE;
    theSink.usb.suspended  = FALSE;
    theSink.usb.vbus_okay  = TRUE;
    theSink.usb.deconfigured = FALSE;

    /* Check if we booted with dead battery */
    usbSetVbatDead(usbDeadBatteryAtBoot());

    /* Get USB configuration */
    configManagerUsb();

    /* Abort if no device classes supported */
    if(!theSink.usb.config.device_class)
        return;

    usbFileInfo(root_name, sizeof(root_name)-1, &root);
    usbFileName(&root, &file_name);
    usbFileInfo(file_name.name, file_name.size, &file);

    /* If we can't find the help file don't enumerate mass storage */
    if(file.index == FILE_NONE || root.index == FILE_NONE) 
        USB_CLASS_DISABLE(USB_DEVICE_CLASS_TYPE_MASS_STORAGE);

#ifdef ENABLE_USB_AUDIO
    plugin = &usb_plugins[theSink.usb.config.plugin_type];
    USB_DEBUG(("USB: Audio Plugin %d\n", theSink.usb.config.plugin_index));
    UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_AUDIO_INTERFACE_DESCRIPTORS, 0, 0, (const uint8*)(plugin->usb_descriptors));
#else
    /* If audio not supported don't enumerate as mic or speaker */
    USB_CLASS_DISABLE(USB_DEVICE_CLASS_AUDIO);
#endif

    USB_DEBUG(("USB: Endpoint Setup [0x%04X] - ", theSink.usb.config.device_class));
    /* Attempt to enumerate - abort if failed */
    status = UsbDeviceClassEnumerate(&theSink.task, theSink.usb.config.device_class);

    if(status != usb_device_class_status_success)
    {
        USB_DEBUG(("Error %X\n", status));
        return;
    }

    USB_DEBUG(("Success\n"));
    /* Configure mass storage device */
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_MASS_STORAGE))
    {
        UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_MASS_STORAGE_FAT_DATA_AREA, file.index, file.size, 0);
        usbFileInfo(fat_name, sizeof(fat_name)-1, &file);
        UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_MASS_STORAGE_FAT_TABLE, file.index, file.size, 0);
        UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_MASS_STORAGE_FAT_ROOT_DIR, root.index, root.size, 0);
    }
}


/****************************************************************************
NAME 
    usbInit
    
DESCRIPTION
    Initialisation done once the main loop is up and running. Determines USB
    attach status etc.
    
RETURNS
    void
*/ 
void usbInit(void)
{
    USB_DEBUG(("USB: Init\n"));

    /* Abort if no device classes supported */
    if(!theSink.usb.config.device_class)
        return;
    /* If battery charging enabled set the charge current */
    usbUpdateChargeCurrent();
#ifdef ENABLE_USB_AUDIO
    /* Pass NULL USB mic Sink until the plugin handles USB mic */
    theSink.a2dp_link_data->a2dp_audio_connect_params.usb_params = NULL; 
#endif
    /* Schedule reset to low power mode if attached */
    usbSetLowPowerMode(theSink.usb.config.attach_timeout);
    /* Check for audio */
    theSink.usb.ready = TRUE;
    audioHandleRouting(audio_source_none);
    PioSetPio(theSink.conf1->PIOIO.pio_outputs.PowerOnPIO, pio_drive, TRUE);
}


/****************************************************************************
NAME 
    usbSetVbusLevel
    
DESCRIPTION
    Set whether VBUS is above or below threshold
    
RETURNS
    void
*/ 
void usbSetVbusLevel(voltage_reading vbus) 
{
    USB_DEBUG(("USB: VBUS %dmV [%d]\n", vbus.voltage, vbus.level));
    theSink.usb.vbus_okay = vbus.level;
}


/****************************************************************************
NAME 
    usbSetDeadBattery
    
DESCRIPTION
    Set whether VBAT is below the dead battery threshold
    
RETURNS
    void
*/ 
void usbSetVbatDead(bool dead)
{
    USB_DEBUG(("USB: VBAT %s\n", dead ? "Dead" : "Okay"));
    theSink.usb.dead_battery = dead;
    if(!dead) MessageCancelAll(&theSink.task, EventUsbDeadBatteryTimeout);
}


/****************************************************************************
NAME 
    usbGetChargeCurrent
    
DESCRIPTION
    Get USB charger limits
    
RETURNS
    void
*/ 
sink_charge_current* usbGetChargeCurrent(void) 
{
    /* USB charging not enabled - no limits */
    if(!USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_BATTERY_CHARGING))
        return NULL;

    USB_DEBUG(("USB: Status "));

    /* Set charge current */
    switch(UsbAttachedStatus())
    {
        case HOST_OR_HUB:
            USB_DEBUG(("Host/Hub "));
            if(theSink.usb.suspended)
            {
                USB_DEBUG(("Suspended (Battery %s)\n", usbDeadBatteryProvision() ? "Dead" : "Okay"));
                if(usbDeadBatteryProvision())
                    return &theSink.usb.config.i_susp_db;
                else
                    return &theSink.usb.config.i_susp;
            }
            else if(powerManagerIsChargerFullCurrent())
            {
                USB_DEBUG(("%sEnumerated (Chg Full)\n", theSink.usb.enumerated ? "" : "Not "));
                if(!theSink.usb.enumerated)
                    return &theSink.usb.config.i_att;
                else
                    return &theSink.usb.config.i_conn;
            }
            else
            {
                USB_DEBUG(("%sEnumerated (Chg Partial)\n", theSink.usb.enumerated ? "" : "Not "));
                if(!theSink.usb.enumerated)
                    return &theSink.usb.config.i_att_trickle;
                else
                    return &theSink.usb.config.i_conn_trickle;
            }
#ifdef HAVE_FULL_USB_CHARGER_DETECTION
        case DEDICATED_CHARGER:
            USB_DEBUG(("Dedicated Charger Port%s\n", theSink.usb.vbus_okay ? "" : " Limited"));
            if(theSink.usb.vbus_okay)
                return &theSink.usb.config.i_dchg;
            else
                return &theSink.usb.config.i_lim;

        case HOST_OR_HUB_CHARGER:
        case CHARGING_PORT:
            USB_DEBUG(("Charger Port%s\n", theSink.usb.vbus_okay ? "" : " Limited"));
            if(theSink.usb.vbus_okay)
                return &theSink.usb.config.i_chg;
            else
                return &theSink.usb.config.i_lim;
#endif
        case DETACHED:
        default:
            USB_DEBUG(("Detached\n"));
            if(powerManagerIsChargerConnected())
                return &theSink.usb.config.i_disc;
            else
                return NULL;
    }
}


/****************************************************************************
NAME 
    usbIsAttached
    
DESCRIPTION
    Determine if USB is attached
    
RETURNS
    TRUE if USB is attached, FALSE otherwise
*/ 
bool usbIsAttached(void)
{
    /* If not detached return TRUE */
    return (UsbAttachedStatus() != DETACHED);
}


#ifdef ENABLE_USB_AUDIO


/****************************************************************************
NAME 
    usbSendHidEvent
    
DESCRIPTION
    Send HID event over USB
    
RETURNS
    void
*/ 
void usbSendHidEvent(usb_device_class_event event)
{
    USB_DEBUG(("USB: HID Event 0x%X\n", event));
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_HID_CONSUMER_TRANSPORT_CONTROL))
        UsbDeviceClassSendEvent(event);
}


/****************************************************************************
NAME 
    usbAudioIsAttached
    
DESCRIPTION
    Determine if USB Audio is attached
    
RETURNS
    TRUE if USB Audio is attached, FALSE otherwise
*/ 
bool usbAudioIsAttached(void)
{
    /* If USB detached or not ready - no audio */
    if(!usbIsAttached() || !theSink.usb.ready) 
        return FALSE;
    /* If USB attached and always on - audio */
    if(theSink.usb.config.audio_always_on)
        return TRUE;
    /* If mic and speaker both inactive - no audio */
    if(!theSink.usb.mic_active && !theSink.usb.spkr_active) 
        return FALSE;
    /* Mic or speaker active - audio */
    return TRUE;
}


/****************************************************************************
NAME 
    usbGetVolume
    
DESCRIPTION
    Extract USB volume setting from USB lib levels
    
RETURNS
    Volume to pass to csr_usb_audio_plugin
*/ 
static uint16 usbGetVolume(AUDIO_MODE_T* mode)
{
    uint16 result;
    bool mic_muted;
    bool spk_muted = FALSE;

    /* Get vol settings from USB lib */
    usb_device_class_audio_levels levels;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_LEVELS, (uint16*)&levels);

    /* limit check volume levels returned */
    if((levels.out_l_vol < 0)||(levels.out_l_vol > VOLUME_A2DP_MAX_LEVEL))
        levels.out_l_vol = VOLUME_A2DP_MAX_LEVEL;
    if((levels.out_r_vol < 0)||(levels.out_r_vol > VOLUME_A2DP_MAX_LEVEL))
        levels.out_r_vol = VOLUME_A2DP_MAX_LEVEL;

    USB_DEBUG(("USB: Gain L %X R %X\n", levels.out_l_vol, levels.out_r_vol));
    USB_DEBUG(("USB: Mute M %X S %X\n", levels.in_mute, levels.out_mute));

    if(theSink.usb.config.plugin_type == usb_plugin_stereo)
    {
        /* Use A2DP gain mappings */
        uint8 l_vol = theSink.conf1->gVolMaps[ levels.out_l_vol ].A2dpGain;
        uint8 r_vol = theSink.conf1->gVolMaps[ levels.out_r_vol ].A2dpGain;
        /* Convert A2DP gain setting to mute/gain */
        spk_muted = (r_vol == 0 && l_vol == 0);
        if(r_vol > 0) r_vol --;
        if(l_vol > 0) l_vol --;
        /* Pack result */
        result = ((l_vol << 8) | r_vol);
        displayUpdateVolume((levels.out_l_vol + levels.out_r_vol + 1) / 2);
#ifdef ENABLE_SUBWOOFER
        updateSwatVolume((levels.out_l_vol + levels.out_r_vol + 1) / 2);
#endif
    }
    
    else
    {
        /* Use HFP gain mappings */
        result = theSink.conf1->gVolMaps[ levels.out_l_vol ].VolGain;
        displayUpdateVolume(levels.out_l_vol);
#ifdef ENABLE_SUBWOOFER
        updateSwatVolume(levels.out_l_vol);
#endif
    }

    /* Mute if muted by host or not supported */
    mic_muted = levels.in_mute  || !USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_AUDIO_MICROPHONE);
    spk_muted = spk_muted || levels.out_mute || !USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_AUDIO_SPEAKER);

    if(mode)
    {
        if(mic_muted && spk_muted)
            *mode = AUDIO_MODE_MUTE_BOTH;
        else if(mic_muted)
            *mode = AUDIO_MODE_MUTE_MIC;
        else if(spk_muted)
            *mode = AUDIO_MODE_MUTE_SPEAKER;
        else
            *mode = AUDIO_MODE_CONNECTED;
    }

    return result;
}


/****************************************************************************
NAME 
    usbAudioSinkMatch
    
DESCRIPTION
    Compare sink to the USB audio sink
    
RETURNS
    TRUE if sink matches USB audio sink, otherwise FALSE
*/ 
bool usbAudioSinkMatch(Sink sink)
{
    Source usb_source = NULL;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SOURCE, (uint16*)&usb_source);

    USB_DEBUG(("USB: usbAudioSinkMatch sink %x = %x, enabled = %x\n", (uint16)sink , (uint16)StreamSinkFromSource(usb_source), USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) ));

    return (USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) && sink && (sink == StreamSinkFromSource(usb_source)));
}


/****************************************************************************
NAME 
    usbAudioGetPluginInfo
    
DESCRIPTION
    Get USB plugin info for current config
    
RETURNS
    TRUE if successful, otherwise FALSE
*/ 
static const usb_plugin_info* usbAudioGetPluginInfo(Task* task, usb_plugin_type type, uint8 index)
{
    switch(type)
    {
        case usb_plugin_stereo:
            *task = getA2dpPlugin(index);
            
            audioControlLowPowerCodecs (FALSE) ;
        break;
        case usb_plugin_mono_nb:
            *task = audioHfpGetPlugin(hfp_wbs_codec_mask_cvsd, index);
            
            audioControlLowPowerCodecs (TRUE) ;
        break;
        case usb_plugin_mono_wb:
            *task = audioHfpGetPlugin(hfp_wbs_codec_mask_msbc, index);
            
             audioControlLowPowerCodecs (TRUE) ;
        break;
        default:
            *task = NULL;
        break;
    }

    return &usb_plugins[type];
}


/****************************************************************************
NAME 
    usbAudioRoute
    
DESCRIPTION
    Connect USB audio stream
    
RETURNS
    void
*/ 
void usbAudioRoute(void)
{
    AudioPluginFeatures features;
    Sink sink;
    Source source;
    uint16 sampleFreq;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SOURCE, (uint16*)(&source));
    /* Note: UsbDeviceClassGetValue uses uint16 which limits max value of sample frequency to 64k (uint 16 has range 0->65536) */
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_SAMPLE_FREQ, &sampleFreq);
    sink = StreamSinkFromSource(source);
    /* determine additional features applicable for this audio plugin */
    features.stereo = (AUDIO_PLUGIN_FORCE_STEREO || theSink.features.stereo);
    features.use_i2s_output = theSink.features.UseI2SOutputCapability;

    USB_DEBUG(("USB: Audio "));
    /* Check Audio configured (sink will be NULL if VM USB not enabled) */
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) && sink)
    {
        USB_DEBUG(("Configured "));
        if(usbAudioIsAttached())
        {
            USB_DEBUG(("Attached\n"));
            if(theSink.routed_audio != sink)
            {
                Task plugin;
                AUDIO_MODE_T mode;
                uint16 volume = usbGetVolume(&mode);
                const usb_plugin_info* plugin_info = usbAudioGetPluginInfo(&plugin, theSink.usb.config.plugin_type, theSink.usb.config.plugin_index);

                theSink.routed_audio = sink;
                UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SINK, (uint16*)(&theSink.cvc_params.usb_params.usb_sink));
                /* Make sure we're using correct parameters for USB */
                theSink.a2dp_link_data->a2dp_audio_connect_params.mode_params = &theSink.a2dp_link_data->a2dp_audio_mode_params;

                USB_DEBUG(("USB: Connect 0x%X 0x%X", (uint16)sink, (uint16)(theSink.cvc_params.usb_params.usb_sink)));

#ifdef ENABLE_SUBWOOFER
            /* set the sub woofer link type prior to passing to audio connect */
            theSink.a2dp_link_data->a2dp_audio_connect_params.sub_woofer_type  = AUDIO_SUB_WOOFER_NONE;  
            theSink.a2dp_link_data->a2dp_audio_connect_params.sub_sink  = NULL;  
            /* bits inverted in dsp plugin */                
            sinkAudioSetEnhancement(MUSIC_CONFIG_SUB_WOOFER_BYPASS,TRUE);
#else
            /* no subwoofer support, set the sub woofer bypass bit in music config message sent o dsp */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SUB_WOOFER_BYPASS,FALSE);
#endif          

                /* use a2dp connect parameters */
                /* sample frequency is not fixed so read this from usb library */
                if(plugin_info->plugin_type == usb_plugin_stereo)
                    AudioConnect(plugin, sink, AUDIO_SINK_USB, theSink.codec_task, volume, sampleFreq, features, mode, 0, powerManagerGetLBIPM(), &theSink.a2dp_link_data->a2dp_audio_connect_params, &theSink.task);
                /* all other plugins use cvc connect parameters */                
                else
                    AudioConnect(plugin, sink, AUDIO_SINK_USB, theSink.codec_task, volume, sampleFreq, features, mode, 0, powerManagerGetLBIPM(), &theSink.cvc_params, &theSink.task);
                
            }
        }
    }
    USB_DEBUG(("\n"));
}


/****************************************************************************
NAME 
    usbAudioDisconnect
    
DESCRIPTION
    Disconnect USB audio stream
    
RETURNS
    void
*/ 
void usbAudioDisconnect(void)
{
    if(usbAudioSinkMatch(theSink.routed_audio))
    {
        USB_DEBUG(("USB: Disconnect 0x%X\n", (uint16)theSink.routed_audio));
        AudioDisconnect();
        theSink.routed_audio = 0;
        /* If speaker is in use then pause */
        if(theSink.usb.spkr_active)
            UsbDeviceClassSendEvent(USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_PAUSE);
    }
}


/****************************************************************************
NAME 
    usbAudioSetVolume
    
DESCRIPTION
    Set USB audio volume
    
RETURNS
    void
*/ 
void usbAudioSetVolume(void)
{
    if(usbAudioSinkMatch(theSink.routed_audio) && USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO))
    {
        Task plugin;
        AUDIO_MODE_T mode;
        uint16 volume = usbGetVolume(&mode);
        
        /* get usb plugin being used */
        usbAudioGetPluginInfo(&plugin, theSink.usb.config.plugin_type, theSink.usb.config.plugin_index);      
        /* Set volume if USB audio connected */
        AudioSetVolume(volume, TonesGetToneVolume(FALSE), plugin);
        /* use a2dp connect parameters */
        AudioSetMode(mode, &theSink.a2dp_link_data->a2dp_audio_mode_params);
    }
}


/****************************************************************************
NAME 
    usbAudioGetMode
    
DESCRIPTION
    Get the current USB audio mode if USB in use
    
RETURNS
    void
*/ 
void usbAudioGetMode(AUDIO_MODE_T* mode)
{
    if(usbAudioSinkMatch(theSink.routed_audio) && (theSink.usb.config.plugin_type == usb_plugin_stereo))
    {
        (void)usbGetVolume(mode);
    }
}

/****************************************************************************
NAME 
    usbGetAudioSink
    
DESCRIPTION
    check USB state and return sink if available
    
RETURNS
   sink if available, otherwise 0
*/ 
Sink usbGetAudioSink(void)
{
    Source usb_source = NULL;
    Sink sink = NULL;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SOURCE, (uint16*)&usb_source);
    
    /* if the usb lead is attached and the speaker is active, try to obtain the audio sink */
    if((usbAudioIsAttached())&&(theSink.usb.spkr_active))
    {
        /* attempt to obtain USB audio sink */
        sink = StreamSinkFromSource(usb_source);
        USB_DEBUG(("USB: usbGetAudioSink sink %x, enabled = %x\n", (uint16)sink , USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) ));
    }
    /* USB not attached */
    else
        USB_DEBUG(("USB: usbGetAudioSink sink %x, enabled = %x, speaker active = %x\n", (uint16)sink , USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO), theSink.usb.spkr_active ));
    
    return sink;
}

#endif /* ENABLE_USB_AUDIO */
#endif /* ENABLE_USB */
