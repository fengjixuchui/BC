/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2010-2013
Part of ADK 2.5.1

FILE NAME
    sink_gaia.c

DESCRIPTION
    Interface with the Generic Application Interface Architecture library

NOTES

*/

#ifdef ENABLE_GAIA
#include "sink_gaia.h"
#include <ps.h>
#include <vm.h>
#include <memory.h>
#include <led.h>
#include <csr_voice_prompts_plugin.h>
#include <csr_speech_recognition_plugin.h>
#include <boot.h>
#include "sink_config.h"
#include "sink_configmanager.h"
#include "sink_leds.h"
#include "sink_led_manager.h"
#include "sink_tones.h"
#include "sink_tts.h"
#include "sink_buttons.h"
#include "sink_volume.h"
#include "sink_speech_recognition.h"
#include "sink_device_id.h"


/*  Gaia-global data stored in app-allocated structure */
#define gaia_data theSink.rundata->gaia_data

#ifdef CVC_PRODTEST
    #define CVC_PRODTEST_PASS           0x0001
    #define CVC_PRODTEST_FAIL           0x0002
#endif

/*  Map feature numbers to bit positions and widths in PSKEY_FEATURE_BLOCK
 *  Note that the bitfields don't cross word boundaries
*/
static const gaia_feature_map_t gaia_feature_map[] = {
    { 0, 15, 1},  /*   0 ReconnectOnPanic:1 */
    { 0, 14, 1},  /*   1 OverideFilterPermanentlyOn:1 */
    { 0, 13, 1},  /*   2 MuteSpeakerAndMic:1 */
    { 0, 12, 1},  /*   3 PlayTonesAtFixedVolume:1 */
    { 0, 11, 1},  /*   4 PowerOffAfterPDLReset:1 */
    { 0, 10, 1},  /*   5 RemainDiscoverableAtAllTimes:1 */
    { 0,  9, 1},  /*   6 DisablePowerOffAfterPowerOn:1 */
    { 0,  8, 1},  /*   7 AutoAnswerOnConnect:1 */
    { 0,  7, 1},  /*   8 EnterPairingModeOnFailureToConnect:1 */
    { 0,  6, 2},  /*   9 unused2:2 */
    { 0,  4, 1},  /*  10 AdjustVolumeWhilstMuted:1 */
    { 0,  3, 1},  /*  11 VolumeChangeCausesUnMute:1 */
    { 0,  2, 1},  /*  12 PowerOffOnlyIfVRegEnLow:1 */
    { 0,  1, 1},  /*  13 unused3:1 */
    { 0,  0, 1},  /*  14 pair_mode_en:1 */
    { 1, 15, 1},  /*  15 GoConnectableButtonPress:1 */
    { 1, 14, 1},  /*  16 DisableTTSTerminate:1 */
    { 1, 13, 1},  /*  17 AutoReconnectPowerOn:1 */
    { 1, 12, 1},  /*  18 speech_rec_enabled:1 */
    { 1, 11, 1},  /*  19 SeparateLNRButtons:1 */
    { 1, 10, 1},  /*  20 SeparateVDButtons:1 */
    { 1,  9, 2},  /*  21 gatt_enabled:2 */
    { 1,  7, 2},  /*  22 PowerDownOnDiscoTimeout:2 */
    { 1,  5, 2},  /*  23 ActionOnCallTransfer:2 */
    { 1,  3, 2},  /*  24 LedTimeMultiplier:2 */
    { 1,  1, 2},  /*  25 ActionOnPowerOn:2 */
    { 2, 15, 4},  /*  26 DiscoIfPDLLessThan:4 */
    { 2, 11, 1},  /*  27 DoNotDiscoDuringLinkLoss:1 */
    { 2, 10, 1},  /*  28 ManInTheMiddle:1 */
    { 2,  9, 1},  /*  29 UseDiffConnectedEventAtPowerOn:1 */
    { 2,  8, 1},  /*  30 EncryptOnSLCEstablishment:1 */
    { 2,  7, 1},  /*  31 UseLowPowerAudioCodecs:1 */
    { 2,  6, 1},  /*  32 PlayLocalVolumeTone:1 */
    { 2,  5, 1},  /*  33 SecurePairing:1 */
    { 2,  4, 1},  /*  34 unused4:1 */
    { 2,  3, 1},  /*  35 QueueVolumeTones:1 */
    { 2,  2, 1},  /*  36 QueueEventTones:1 */
    { 2,  1, 1},  /*  37 QueueLEDEvents:1 */
    { 2,  0, 1},  /*  38 MuteToneFixedVolume:1 */
    { 3, 15, 1},  /*  39 ResetLEDEnableStateAfterReset:1 */
    { 3, 14, 1},  /*  40 ResetAfterPowerOffComplete:1 */
    { 3, 13, 1},  /*  41 AutoPowerOnAfterInitialisation:1 */
    { 3, 12, 1},  /*  42 DisableRoleSwitching:1 */
    { 3, 11, 1},  /*  43 audio_sco:1 */
    { 3, 10, 3},  /*  44 audio_plugin:3 */
    { 3,  7, 4},  /*  45 DefaultVolume:4 */
    { 3,  3, 1},  /*  46 IgnoreButtonPressAfterLedEnable:1 */
    { 3,  2, 1},  /*  47 LNRCancelsVoiceDialIfActive:1 */
    { 3,  1, 1},  /*  48 GoConnectableDuringLinkLoss:1 */
    { 3,  0, 1},  /*  49 stereo:1 */
    { 4, 15, 1},  /*  50 ChargerTerminationLEDOveride:1 */
    { 4, 14, 5},  /*  51 FixedToneVolumeLevel:5 */
    { 4,  9, 1},  /*  52 unused:1 */
    { 4,  8, 1},  /*  53 ForceEV3S1ForSco2:1 */
    { 4,  7, 1},  /*  54 VoicePromptPairing:1 */
    { 4,  6, 1},  /*  55 avrcp_enabled:1 */
    { 4,  5, 2},  /*  56 PairIfPDLLessThan:2 */
    { 4,  3, 1},  /*  57 EnableSyncMuteMicrophones:1 */
    { 4,  2, 2},  /*  58 ActionOnPanicReset:2 */
    { 4,  0, 1},  /*  59 VoicePromptNumbers:1 */
    { 5, 15, 4},  /*  60 DefaultA2dpVolLevel:4 */
    { 5, 11, 1},  /*  61 pbap_enabled:1 */
    { 5, 10, 1},  /*  62 EnableA2dpStreaming:1 */
    { 5,  9, 5},  /*  63 A2dpOptionalCodecsEnabled:5 */
    { 5,  4, 1},  /*  64 EnableA2dpMediaOpenOnConnection:1 */
    { 5,  3, 1},  /*  65 AssumeAutoSuspendOnCall:1 */
    { 5,  2, 3},  /*  66 ReconnectLastAttempts:3 */
};


/*  Masks for bitfield widths 0 to 8  */
static const uint16 width_mask[] =
    {0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF};


/*  Sizes expected by ConfigRetrieve() and whether fixed  */
static const gaia_config_entry_size_t fixed_entry_size[] = {
    {1, sizeof (sink_power_config)},  /*  0 PSKEY_BATTERY_CONFIG */
    {1, sizeof (button_config_type)},  /*  1 PSKEY_BUTTON_CONFIG */
    {0, sizeof (button_pattern_config_type)}, /*  2 PSKEY_BUTTON_PATTERN_CONFIG */
    {0, 1}, /*  3 PSKEY_AT_COMMANDS */
    {1, sizeof (pio_config_type)}, /*  4 PSKEY_PIO_BLOCK */
    {1, sizeof (HFP_features_type)}, /*  5 PSKEY_ADDITIONAL_HFP_SUPPORTED_FEATURES */
    {1, sizeof (Timeouts_t)}, /*  6 PSKEY_TIMEOUTS */
    {1, 1}, /*  7 PSKEY_TRI_COL_LEDS */
    {1, 4}, /*  8 PSKEY_DEVICE_ID */
    {1, sizeof (lengths_config_type)}, /*  9 PSKEY_LENGTHS */
    {1, BM_NUM_BUTTON_TRANSLATIONS * sizeof(button_translation_type)}, /* 10 PSKEY_BUTTON_TRANSLATION */
    {0, sizeof(tts_config_type)}, /* 11 PSKEY_TTS */
    {1, sizeof(session_data_type)}, /* 12 PSKEY_VOLUME_ORIENTATION */
    {1, sizeof(radio_config_type)}, /* 13 PSKEY_RADIO_CONFIG */
    {1, sizeof (subrate_t)}, /* 14 PSKEY_SSR_PARAMS */
    {1, sizeof (feature_config_type)}, /* 15 PSKEY_FEATURE_BLOCK */
    {1, VOL_NUM_VOL_SETTINGS * sizeof (VolMapping_t)}, /* 16 PSKEY_SPEAKER_GAIN_MAPPING */
    {1, sizeof (hfp_init_params)}, /* 17 PSKEY_HFP_INIT */
    {0, sizeof (LEDFilter_t)}, /* 18 PSKEY_LED_FILTERS */
    {0, 1}, /* 19 PSKEY_CONFIG_TONES */
    {0, sizeof (LEDPattern_t)}, /* 20 PSKEY_LED_STATES */
    {1, sizeof (vp_config_type)}, /* 21 PSKEY_VOICE_PROMPTS */
    {0, sizeof (LEDPattern_t)}, /* 22 PSKEY_LED_EVENTS */
    {1, BM_EVENTS_PER_PS_BLOCK * sizeof (event_config_type)}, /* 23 PSKEY_EVENTS_A */
    {1, BM_EVENTS_PER_PS_BLOCK * sizeof (event_config_type)}, /* 24 PSKEY_EVENTS_B */
    {1, BM_EVENTS_PER_PS_BLOCK * sizeof (event_config_type)}, /* 25 PSKEY_EVENTS_C */
    {0, sizeof (tone_config_type)}, /* 26 PSKEY_TONES */
    {1, sizeof (rssi_pairing_t)}, /* 27 PSKEY_RSSI_PAIRING */
    {1, sizeof (usb_config)} /* 28 PSKEY_USB_CONFIG */
};
        

/*************************************************************************
NAME    
    gaia_send_packet
    
DESCRIPTION
    Build and Send a Gaia protocol packet
   
*/ 
static void gaia_send_packet(uint16 vendor_id, uint16 command_id, uint16 status,
                          uint16 payload_length, uint8 *payload)
{
    uint16 packet_length;
    uint8 *packet;
    uint8 flags = GaiaTransportGetFlags(gaia_data.gaia_transport);
    
    packet_length = GAIA_HEADER_SIZE + payload_length + 2;
    packet = mallocPanic(packet_length);
    
    if (packet)
    {
        packet_length = GaiaBuildResponse(packet, flags,
                                          vendor_id, command_id, 
                                          status, payload_length, payload);
        
        GaiaSendPacket(gaia_data.gaia_transport, packet_length, packet);
    }
}


/*************************************************************************
NAME    
    gaia_send_response
    
DESCRIPTION
    Build and Send a Gaia acknowledgement packet
   
*/ 
static void gaia_send_response(uint16 vendor_id, uint16 command_id, uint16 status,
                          uint16 payload_length, uint8 *payload)
{
    gaia_send_packet(vendor_id, command_id | GAIA_ACK_MASK, status,
                     payload_length, payload);
}


/*************************************************************************
NAME    
    gaia_send_response_16
    
DESCRIPTION
    Build and Send a Gaia acknowledgement packet from a uint16[] payload
   
*/ 
static void gaia_send_response_16(uint16 command_id, uint16 status,
                          uint16 payload_length, uint16 *payload)
{
    uint16 packet_length;
    uint8 *packet;
    uint8 flags = GaiaTransportGetFlags(gaia_data.gaia_transport);
    
    packet_length = GAIA_HEADER_SIZE + 2 * payload_length + 2;
    packet = mallocPanic(packet_length);
    
    if (packet)
    {
        packet_length = GaiaBuildResponse16(packet, flags,
                                          GAIA_VENDOR_CSR, command_id | GAIA_ACK_MASK, 
                                          status, payload_length, payload);
        
        GaiaSendPacket(gaia_data.gaia_transport, packet_length, packet);
    }
}


/*************************************************************************
NAME
    gaia_send_success
    gaia_send_success_payload
    gaia_send_invalid_parameter
    gaia_send_insufficient_resources
    
DESCRIPTION
    Convenience macros for common responses
*/
#define gaia_send_success(command_id) \
    gaia_send_response(GAIA_VENDOR_CSR, command_id, GAIA_STATUS_SUCCESS, 0, NULL)
                
#define gaia_send_success_payload(command_id, payload_len, payload) \
    gaia_send_response(GAIA_VENDOR_CSR, command_id, GAIA_STATUS_SUCCESS, payload_len, (uint8 *) payload)
                
#define gaia_send_invalid_parameter(command_id) \
    gaia_send_response(GAIA_VENDOR_CSR, command_id, GAIA_STATUS_INVALID_PARAMETER, 0, NULL)
    
#define gaia_send_insufficient_resources(command_id) \
    gaia_send_response(GAIA_VENDOR_CSR, command_id, GAIA_STATUS_INSUFFICIENT_RESOURCES, 0, NULL)


#ifdef DEBUG_GAIA
static void dump(char *caption, uint16 *data, uint16 data_len)
{
    DEBUG(("%s: %d:", caption, data_len));
    while (data_len--)
        DEBUG((" %04x", *data++));
    DEBUG(("\n"));
}
#endif
    
    
/*************************************************************************
NAME    
    gaia_send_notification
    
DESCRIPTION
    Send a Gaia notification packet
   
*/ 
static void gaia_send_notification(uint8 event, uint16 payload_length, uint8 *payload)
{
    gaia_send_packet(GAIA_VENDOR_CSR, GAIA_EVENT_NOTIFICATION, event, payload_length, payload);
}


/*************************************************************************
NAME
    wpack
    
DESCRIPTION
    Pack an array of 2n uint8s into an array of n uint16s
*/
static void wpack(uint16 *dest, uint8 *src, uint16 n)
{
    while (n--)
    {
        *dest = *src++ << 8;
        *dest++ |= *src++;
    }
}

    
/*************************************************************************
NAME
    dwunpack
    
DESCRIPTION
    Unpack a uint32 into an array of 4 uint8s
*/
static void dwunpack(uint8 *dest, uint32 src)
{
    *dest++ = src >> 24;
    *dest++ = (src >> 16) & 0xFF;
    *dest++ = (src >> 8) & 0xFF;
    *dest = src & 0xFF;
}

    
/*************************************************************************
NAME
    pitch_length_tone
    
DESCRIPTION
    Pack a pitch + length pair into a ringtone note
*/
static ringtone_note pitch_length_tone(uint8 pitch, uint8 length)
{
    return (ringtone_note) 
            ((pitch << RINGTONE_SEQ_NOTE_PITCH_POS) 
            | (length << RINGTONE_SEQ_NOTE_LENGTH_POS));
}


/*************************************************************************
NAME
    get_config_lengths
    
DESCRIPTION
    Load the lengths structure
*/
static void get_config_lengths(lengths_config_type *lengths)
{
    ConfigRetrieve(theSink.config_id, PSKEY_LENGTHS, lengths, sizeof (lengths_config_type));
}


/*************************************************************************
NAME
    send_app_message
    
DESCRIPTION
    Send a message to the main application
*/
static void send_app_message(uint16 message)
{
    MessageSend(&theSink.task, message, NULL);
}


/*************************************************************************
NAME
    set_abs_eq_bank
    
DESCRIPTION
    Select a Music Manager equaliser bank by absolute index
*/
static void set_abs_eq_bank(uint16 bank)
{
    bank += A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0;
    
    GAIA_DEBUG(("G: EQ %u -> %u\n",
                theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing,
                bank));
    
    if(bank != theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing)
    {
        AUDIO_MODE_T mode = VolumeCheckA2dpMute() ? AUDIO_MODE_MUTE_SPEAKER : AUDIO_MODE_CONNECTED;
        usbAudioGetMode(&mode);
        
        theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = bank;
        configManagerWriteSessionData() ; 
        AudioSetMode(mode, &theSink.a2dp_link_data->a2dp_audio_mode_params);
    }
}


/*************************************************************************
NAME    
    gaia_change_volume
    
DESCRIPTION
    Respond to GAIA_COMMAND_CHANGE_VOLUME request by sending the
    appropriate event message to the sink device task.  The device inverts
    the message meanings if it thinks the buttons are inverted so we have
    to double bluff it.
    
    direction 0 means volume up, 1 means down.
*/    
static void gaia_change_volume(uint8 direction)
{
    uint16 message = 0;
    uint8 status;
    
    if (theSink.gVolButtonsInverted)
        direction ^= 1;
    
    if (direction == 0)
    {
        message = EventVolumeUp;
        status = GAIA_STATUS_SUCCESS;
    }
    
    else if (direction == 1)
    {
        message = EventVolumeDown;
        status = GAIA_STATUS_SUCCESS;
    }
    
    else
        status = GAIA_STATUS_INVALID_PARAMETER;
    
    if (status == GAIA_STATUS_SUCCESS)
        send_app_message(message);
    
    gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_CHANGE_VOLUME, status, 0, NULL);  
}


/*************************************************************************
NAME    
    gaia_alert_leds
    
DESCRIPTION
    Act on a GAIA_COMMAND_ALERT_LEDS request

*/    
static void gaia_alert_leds(uint8 *payload)
{
    uint16 time;
    LEDPattern_t *pattern;
    
    uint16 idx = 0;
    
    while ((idx < theSink.theLEDTask->gEventPatternsAllocated) 
        && (theSink.theLEDTask->gEventPatterns[idx].StateOrEvent != EventGaiaAlertLEDs - EVENTS_MESSAGE_BASE))
        ++idx;
    
    GAIA_DEBUG(("G: al: %d/%d\n", idx, theSink.theLEDTask->gEventPatternsAllocated));

    if (idx == theSink.theLEDTask->gEventPatternsAllocated)
    {
        gaia_send_invalid_parameter(GAIA_COMMAND_ALERT_LEDS);
        return;
    }
    
    pattern = &theSink.theLEDTask->gEventPatterns[idx];

    time = (payload[0] << 8) | payload[1];
    pattern->OnTime = (time >> theSink.features.LedTimeMultiplier) / 10;
    
    time = (payload[2] << 8) | payload[3];
    pattern->OffTime = (time >> theSink.features.LedTimeMultiplier) / 10;
    
    time = (payload[4] << 8) | payload[5];
    pattern->RepeatTime = (time >> theSink.features.LedTimeMultiplier) / 50;
    
    pattern->DimTime = payload[7];
    pattern->NumFlashes = payload[8];
    pattern->TimeOut = payload[9];
    pattern->LED_A = payload[10];
    pattern->LED_B = payload[11];
    pattern->OverideDisable = FALSE;
    pattern->Colour = payload[12];

    LEDManagerIndicateEvent(EventGaiaAlertLEDs);
    gaia_send_success(GAIA_COMMAND_ALERT_LEDS);    
}


/*************************************************************************
NAME    
    gaia_alert_tone
    
DESCRIPTION
    Act on a GAIA_COMMAND_ALERT_TONE request
    
    The command payload holds tempo, volume, timbre and decay values for
    the whole sequence followed by pitch and duration values for each note.
    
NOTE
    Since we can't be sure when the tone will start playing, let alone
    when it will finish, we can't free the storage.
*/    
static void gaia_alert_tone(uint8 length, uint8 *tones)
{
    AudioPluginFeatures features;

    /* determine additional features applicable for this audio plugin */
    features.stereo = theSink.features.stereo;
    features.use_i2s_output = theSink.features.UseI2SOutputCapability;

/*  Must be at least 4 octets and an even number in total  */
    if ((length < 4) || (length & 1))
        gaia_send_invalid_parameter(GAIA_COMMAND_ALERT_TONE);
    
/*  If there are no notes then we're done  */
    else if (length == 4)
        gaia_send_success(GAIA_COMMAND_ALERT_TONE);
        
    else
    {
        if (gaia_data.alert_tone == NULL)
            gaia_data.alert_tone = mallocPanic(GAIA_TONE_BUFFER_SIZE);
        
        if ((gaia_data.alert_tone == NULL) || (length > GAIA_TONE_MAX_LENGTH))
            gaia_send_insufficient_resources(GAIA_COMMAND_ALERT_TONE);
        
        else
        {
            uint16 idx;
            uint16 volume;
            ringtone_note *t = gaia_data.alert_tone;
    
            GAIA_DEBUG(("G: at: %d %d %d %d\n", tones[0], tones[1], tones[2], tones[3]));
            
            AudioStopTone();
            
            
        /*  Set up tempo, volume, timbre and decay  */
            *t++ = RINGTONE_TEMPO(tones[0] * 4);
            volume = tones[1];
            *t++ = (ringtone_note) (RINGTONE_SEQ_TIMBRE | tones[2]);
            *t++ = RINGTONE_DECAY(tones[3]);
                
            
        /*  Set up note-duration pairs  */
            for (idx = 4; idx < length; idx += 2)
            {
                GAIA_DEBUG((" - %d %d\n", tones[idx], tones[idx + 1]));
                *t++ = pitch_length_tone(tones[idx], tones[idx + 1]);
            }
                
        /*  Mark the end  */
            *t = RINGTONE_END;
            
            AudioPlayTone(gaia_data.alert_tone, FALSE, theSink.codec_task, volume, features);   
            gaia_send_success(GAIA_COMMAND_ALERT_TONE);    
        }
    }
}


/*************************************************************************
NAME    
    gaia_alert_event
    
DESCRIPTION
    Act on a GAIA_COMMAND_ALERT_EVENT request by indicating the associated
    event

*/    
static void gaia_alert_event(uint8 alert)
{
    uint16 event = EVENTS_MESSAGE_BASE + alert;
    
    GAIA_DEBUG(("G: ae: %02X %04X\n", alert, event));
    
    if (event <= EVENTS_LAST_EVENT)
    {
        if (event != EventLEDEventComplete)
            LEDManagerIndicateEvent(event);
    
        TonesPlayEvent(event);
        
        gaia_send_success(GAIA_COMMAND_ALERT_EVENT);
    }
    
    else
        gaia_send_invalid_parameter(GAIA_COMMAND_ALERT_EVENT);
}


/*************************************************************************
NAME    
    gaia_alert_voice
    
DESCRIPTION
    Act on a GAIA_COMMAND_ALERT_VOICE request
    Play the indicated voice prompt
    We have already established that theSink.no_tts_languages != 0

*/    
static void gaia_alert_voice(uint8 payload_length, uint8 *payload)
{
    lengths_config_type lengths;
    TaskData *plugin = NULL;
    uint16 nr_prompts;
    uint16 tts_id = (payload[0] << 8) | payload[1];
 
    get_config_lengths(&lengths);

#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION
    nr_prompts = lengths.no_vp / theSink.no_tts_languages;
#else
    nr_prompts = lengths.no_vp;
#endif
    
    GAIA_DEBUG(("G: ti: %d/%d + %d\n", tts_id, nr_prompts, payload_length - 2));

    if (tts_id < nr_prompts)
    {
#ifdef FULL_TEXT_TO_SPEECH
        plugin = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;   
#endif
 
#ifdef ENABLE_VOICE_PROMPTS
        plugin = (TaskData *) &csr_voice_prompts_plugin;
#endif 
   
        TTSPlay(plugin, tts_id, payload + 2, payload_length - 2, TRUE, FALSE);
        gaia_send_success(GAIA_COMMAND_ALERT_VOICE);
    }
    
    else
        gaia_send_invalid_parameter(GAIA_COMMAND_ALERT_VOICE);        
}


/*************************************************************************
NAME
    gaia_set_led_config
    
DESCRIPTION
    Respond to GAIA_COMMAND_SET_LED_CONFIGURATION request
    
    State and Event config requests are 16 octets long and look like
    +--------+--------+-----------------+-----------------+-----------------+
    |  TYPE  | STATE/ |   LED ON TIME   |    OFF TIME     |   REPEAT TIME   |
    +--------+-EVENT--+--------+--------+--------+--------+--------+--------+ ...
    
        +-----------------+--------+--------+--------+--------+--------+--------+
        |    DIM TIME     |FLASHES |TIMEOUT | LED_A  | LED_B  |OVERRIDE| COLOUR |
    ... +--------+--------+--------+--------+--------+--------+DISABLE-+--------+
    
    
    Filter config requests are 6 or 7 octets depending on the filter action and look like
    +--------+--------+--------+--------+--------+--------+--------+
    |  TYPE  | INDEX  | EVENT  | ACTION |OVERRIDE| PARAM1 |[PARAM2]|
    +--------+--------+--------+--------+DISABLE-+--------+--------+ 
    
*/    
static void gaia_set_led_config(uint16 request_length, uint8 *request)
{
    lengths_config_type lengths;
    uint16 no_entries;
    uint16 max_entries;
    uint16 ps_key;
    uint16 config_length;
    uint16 index;
    uint8 expect_length;
    
    get_config_lengths(&lengths);
    
    switch (request[0])
    {
    case GAIA_LED_CONFIGURATION_FILTER:        
        no_entries = lengths.no_led_filter;
        max_entries = MAX_LED_FILTERS;
        ps_key = PSKEY_LED_FILTERS;
        
        switch (request[3])
        {
        case GAIA_LED_FILTER_CANCEL:
        case GAIA_LED_FILTER_OVERRIDE:
        case GAIA_LED_FILTER_COLOUR:
            expect_length = 6;
            break;
            
        case GAIA_LED_FILTER_SPEED:
        case GAIA_LED_FILTER_FOLLOW:
            expect_length = 7;
            break;
            
        default:
            expect_length = 0;
            break;
        }
        
        break;

    case GAIA_LED_CONFIGURATION_STATE:
        no_entries = lengths.no_led_states;
        max_entries = MAX_LED_STATES;
        ps_key = PSKEY_LED_STATES;
        expect_length = 16;
        break;

    case GAIA_LED_CONFIGURATION_EVENT:
        no_entries = lengths.no_led_events;
        max_entries = MAX_LED_EVENTS;
        ps_key = PSKEY_LED_EVENTS;
        expect_length = 16;     
        break;
        
    default:
        no_entries = 0;
        max_entries = 0;
        ps_key = 0;
        expect_length = 0;
        break;
    }
    
    GAIA_DEBUG(("G: lc: t=%d id=%d n=%d k=%d r=%d x=%d\n", 
                 request[0], request[1], no_entries, ps_key, request_length, expect_length));
            
    if (request_length != expect_length)
    {
        gaia_send_invalid_parameter(GAIA_COMMAND_SET_LED_CONFIGURATION);
        return;
    }
   
        
    if (request[0] == GAIA_LED_CONFIGURATION_FILTER)
    {
        LEDFilter_t *config = NULL;
        bool changed = FALSE;
    
        index = request[1] - 1;
        
        if (index > no_entries)
        {
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_LED_CONFIGURATION);
            return;
        }
            
        if (index == no_entries)
            config_length = (no_entries + 1) * sizeof (LEDFilter_t);
        
        else
            config_length = no_entries * sizeof (LEDFilter_t);
        
        config = mallocPanic(config_length);
        if (config == NULL)
        {
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_LED_CONFIGURATION);
            return;
        }
            
        ConfigRetrieve(theSink.config_id, ps_key, config, config_length);
        
        changed = (index == no_entries) || (config[index].Event != request[2]);
        
        switch (request[3])
        {
        case GAIA_LED_FILTER_CANCEL:
            if (changed || (config[index].FilterToCancel != request[5]))
            {
                memset(&config[index], 0, sizeof (LEDFilter_t));
                config[index].FilterToCancel = request[5];
                changed = TRUE;
            }
            break;
            
        case GAIA_LED_FILTER_SPEED:
            if (changed 
                || (config[index].SpeedAction != request[5])
                || (config[index].Speed != request[6]))
            {
                memset(&config[index], 0, sizeof (LEDFilter_t));
                config[index].SpeedAction = request[5];
                config[index].Speed = request[6];
                changed = TRUE;
            }
            break;
            
        case GAIA_LED_FILTER_OVERRIDE:
            if (changed || (config[index].OverideLEDActive != request[5]))
            {
                memset(&config[index], 0, sizeof (LEDFilter_t));
                config[index].OverideLEDActive = request[5];
                changed = TRUE;
            }
            break;
            
        case GAIA_LED_FILTER_COLOUR:
            if (changed || (config[index].Colour != request[5]))
            {
                memset(&config[index], 0, sizeof (LEDFilter_t));
                config[index].Colour = request[5];
                changed = TRUE;
            }
            break;
            
        case GAIA_LED_FILTER_FOLLOW:
            if (changed 
                || (config[index].FollowerLEDActive != request[5])
                || (config[index].FollowerLEDDelay != request[6]))
            {
                memset(&config[index], 0, sizeof (LEDFilter_t));
                config[index].FollowerLEDActive = request[5];
                config[index].FollowerLEDDelay = request[6];
                changed = TRUE;
            }
            break;
            
        default:
            expect_length = 0;
            break;
        }
        
        if (changed)
        {
            config[index].Event = request[2];
            config[index].IsFilterActive = TRUE;
                    
            config_length = PsStore(ps_key, config, config_length);
            
            if ((config_length > 0) && (no_entries != lengths.no_led_filter))
            {
                lengths.no_led_filter = no_entries;
                config_length = PsStore(PSKEY_LENGTHS, &lengths, sizeof lengths);
                if (config_length == 0)
                {
                    DEBUG(("GAIA: PsStore lengths failed\n"));
                    Panic();
                }
            }
        }
                    
        freePanic(config);
    }
    
    else
    {
        LEDPattern_t *config = NULL;
        bool changed = FALSE;
        uint16 on_time = 0;
        uint16 off_time = 0;
        uint16 repeat_time = 0;
    
        config_length = no_entries * sizeof (LEDPattern_t);        
        config = mallocPanic(config_length + sizeof (LEDPattern_t));
        if (config == NULL)
        {
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_LED_CONFIGURATION);
            return;
        }
            
        ConfigRetrieve(theSink.config_id, ps_key, config, config_length);

        index = 0;
        while ((index < no_entries) && (config[index].StateOrEvent != request[1]))
            ++index;
                    
        GAIA_DEBUG(("G: idx %d/%d/%d\n", index, no_entries, max_entries));
        
        if (index == max_entries)
        {
            freePanic(config);
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_LED_CONFIGURATION);
            return;
        }
        
        if (index == no_entries)
        {
            GAIA_DEBUG(("G: + %d %d at %d\n", request[0], request[1], index));
            changed = TRUE;
            ++no_entries;
            config_length += sizeof (LEDPattern_t);
        }
        
        else
            GAIA_DEBUG(("G: = %d %d at %d\n", request[0], request[1], index));

        
        on_time = ((uint16) (request[2] << 8) | request[3]) / 10;
        off_time = ((uint16) (request[4] << 8) | request[5]) / 10;
        repeat_time = ((uint16) (request[6] << 8) | request[7]) / 50;
    
        if (changed
            || config[index].OnTime != on_time
            || config[index].OffTime != off_time
            || config[index].RepeatTime != repeat_time
            || config[index].DimTime != request[9]
            || config[index].NumFlashes != request[10]
            || config[index].TimeOut != request[11]
            || config[index].LED_A != request[12]
            || config[index].LED_B != request[13]
            || config[index].OverideDisable != request[14]
            || config[index].Colour != request[15])
        {
            memset(&config[index], 0, sizeof (LEDPattern_t));
            
            config[index].StateOrEvent = request[1];
            config[index].OnTime = on_time;
            config[index].OffTime = off_time;
            config[index].RepeatTime = repeat_time;
            config[index].DimTime = request[9];
            config[index].NumFlashes = request[10];
            config[index].TimeOut = request[11];
            config[index].LED_A = request[12];
            config[index].LED_B = request[13];
            config[index].OverideDisable = request[14];
            config[index].Colour = request[15];
            
            config_length = PsStore(ps_key, config, config_length);
            if (config_length > 0)
            {
                if (request[0] == GAIA_LED_CONFIGURATION_STATE)
                {
                    changed = no_entries != lengths.no_led_states;
                    lengths.no_led_states = no_entries;
                }
                
                else
                {
                    changed = no_entries != lengths.no_led_events;
                    lengths.no_led_events = no_entries;
                }
                
                if (changed)
                {
                    config_length = PsStore(PSKEY_LENGTHS, &lengths, sizeof lengths);
                    if (config_length == 0)
                    {
                        DEBUG(("GAIA: PsStore lengths failed\n"));
                        Panic();
                    }
                }

            }
        }
                    
        freePanic(config);
    }
    
    if (config_length == 0)
        gaia_send_insufficient_resources(GAIA_COMMAND_SET_LED_CONFIGURATION);

    else
    {
        uint8 payload[2];
        payload[0] = request[0];    /* type */
        payload[1] = request[1];    /* index  */
        gaia_send_success_payload(GAIA_COMMAND_SET_LED_CONFIGURATION, 2, payload);
    }
}

    
/*************************************************************************
NAME
    gaia_send_led_config
    
DESCRIPTION
    Respond to GAIA_COMMAND_GET_LED_CONFIGURATION request for the
    configuration of a given state, event or filter
*/       
#define filter_config ((LEDFilter_t *)config)
#define pattern_config ((LEDPattern_t *)config)
static void gaia_send_led_config(uint8 type, uint8 index)
{
    lengths_config_type lengths;
    void *config;
    uint16 config_length;
    uint16 key;
    uint16 no_entries;
    uint16 time;          
    uint16 payload_len;
    uint8 payload[16];
    
    
    get_config_lengths(&lengths);
    
    switch (type)
    {
    case GAIA_LED_CONFIGURATION_STATE:
        no_entries = lengths.no_led_states;
        config_length = no_entries * sizeof (LEDPattern_t);
        key = PSKEY_LED_STATES;
        break;
        
    case GAIA_LED_CONFIGURATION_EVENT:
        no_entries = lengths.no_led_events;
        config_length = no_entries * sizeof (LEDPattern_t);
        key = PSKEY_LED_EVENTS;
        break;
        
    case GAIA_LED_CONFIGURATION_FILTER:
        no_entries = lengths.no_led_filter;
        config_length = no_entries * sizeof (LEDFilter_t);
        key = PSKEY_LED_FILTERS;
        break;
        
    default:
        no_entries = 0;
        config_length = 0;
        key = 0;
        break;
    }
    
    if (no_entries == 0)
    {
        gaia_send_invalid_parameter(GAIA_COMMAND_GET_LED_CONFIGURATION);
        return;
    }
        
    config = mallocPanic(config_length);
    if (config == NULL)
    {
        gaia_send_insufficient_resources(GAIA_COMMAND_GET_LED_CONFIGURATION);
        return;
    }
    
    ConfigRetrieve(theSink.config_id, key, config, config_length);
    
    payload[0] = type;
    payload[1] = index;
            
    if (type == GAIA_LED_CONFIGURATION_FILTER)
    {
    /*  Filter numbers are 1-based  */
        if ((index < 1) || (index > no_entries))
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_LED_CONFIGURATION);
    
        else
        {
            --index;

            payload[2] = filter_config[index].Event;
            payload[4] = filter_config[index].OverideDisable;
            
            if (filter_config[index].FilterToCancel != 0)
            {
                payload[3] = GAIA_LED_FILTER_CANCEL;
                payload[5] = filter_config[index].FilterToCancel;
                payload_len = 6;
            }
            
            else if (filter_config[index].Speed != 0)
            {
                payload[3] = GAIA_LED_FILTER_SPEED;
                payload[5] = filter_config[index].SpeedAction;
                payload[6] = filter_config[index].Speed;
                payload_len = 7;
            }
            
            else if (filter_config[index].OverideLEDActive != 0)
            {
                payload[3] = GAIA_LED_FILTER_OVERRIDE;
                payload[5] = filter_config[index].OverideLEDActive;
                payload_len = 6;
            }
            
            else if (filter_config[index].Colour != 0)
            {
                payload[3] = GAIA_LED_FILTER_COLOUR;
                payload[5] = filter_config[index].Colour;
                payload_len = 6;
            }
            
            else if (filter_config[index].FollowerLEDActive != 0)
            {
                payload[3] = GAIA_LED_FILTER_FOLLOW;
                payload[5] = filter_config[index].FollowerLEDActive;
                payload[6] = filter_config[index].FollowerLEDDelay;
                payload_len = 7;
            }
            
            else
            /*  Can't infer the filter action  */
                payload_len = 0;
            
            
            if (payload_len == 0)
                gaia_send_insufficient_resources(GAIA_COMMAND_GET_LED_CONFIGURATION);
            
            else
                gaia_send_success_payload(GAIA_COMMAND_GET_LED_CONFIGURATION, payload_len, payload);
        }
    }
    
    else
    {
        uint16 event = index;
        
        index = 0;
        while ((index < no_entries) && (pattern_config[index].StateOrEvent != event))
            ++index;
        
        if (index == no_entries)
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_LED_CONFIGURATION);
        
        else
        {
            GAIA_DEBUG(("G: e: %d %d at %d\n", type, event, index));
            time = pattern_config[index].OnTime * 10;
            payload[2] = time >> 8;
            payload[3] = time & 0xFF;
            
            time = pattern_config[index].OffTime * 10;
            payload[4] = time >> 8;
            payload[5] = time & 0xFF;
            
            time = pattern_config[index].RepeatTime * 50;
            payload[6] = time >> 8;
            payload[7] = time & 0xFF;
            
            payload[8] = 0;
            payload[9] = pattern_config[index].DimTime;
            
            payload[10] = pattern_config[index].NumFlashes;
            payload[11] = pattern_config[index].TimeOut;
            payload[12] = pattern_config[index].LED_A;
            payload[13] = pattern_config[index].LED_B;
            payload[14] = pattern_config[index].OverideDisable;
            payload[15] = pattern_config[index].Colour;            
            
            payload_len = 16;
            gaia_send_success_payload(GAIA_COMMAND_GET_LED_CONFIGURATION, payload_len, payload);
        }
    }
        
    freePanic(config);
}
#undef pattern_config
#undef filter_config


/*************************************************************************
NAME
    gaia_retrieve_tone_config
    
DESCRIPTION
    Get the tone configuration from PS or const
    Returns the number of tones configured, 0 on failure
*/       
static uint16 gaia_retrieve_tone_config(tone_config_type **config)
{
    lengths_config_type lengths;
    uint16 config_length;
    
    get_config_lengths(&lengths);
    
    GAIA_DEBUG(("G: rtc: %d tones\n", lengths.no_tones));
    
    config_length = lengths.no_tones * sizeof(tone_config_type);
    
    *config = mallocPanic(config_length);
    
    if (*config == NULL)
    {
        GAIA_DEBUG(("G: rtc: no space\n"));
        return 0;
    }
    
    ConfigRetrieve(theSink.config_id, PSKEY_TONES, *config, config_length);
        
    return lengths.no_tones;
}
    
    
/*************************************************************************
NAME
    gaia_set_tone_config
    
DESCRIPTION
    Set the tone associated with a given event.  We update PS, not the
    running device configuration.
*/   
static void gaia_set_tone_config(uint8 event, uint8 tone)
{
    lengths_config_type lengths;
    uint16 no_tones;

    get_config_lengths(&lengths);
    
    no_tones = lengths.no_tones;    
    GAIA_DEBUG(("G: stc: %d tones\n", no_tones));
    
    if (no_tones == 0)
        gaia_send_insufficient_resources(GAIA_COMMAND_SET_TONE_CONFIGURATION);
    
    else
    {
        tone_config_type *config;
        uint16 config_length;
        
        config_length = no_tones * sizeof (tone_config_type);

    /*  Include space for a new event in case we add one  */
        config = mallocPanic(config_length + sizeof (tone_config_type));
        
        if (config == NULL)
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_TONE_CONFIGURATION);

        else
        {
            uint16 index = 0;
            
            ConfigRetrieve(theSink.config_id, PSKEY_TONES, config, config_length);
            config[no_tones].tone = TONE_NOT_DEFINED;

            while ((index < no_tones) && (config[index].event != event))
                ++index;
           
            if (index == no_tones)
            {
                ++no_tones;
                config_length += sizeof (tone_config_type);
            }
            
            config[index].event = event;
            
            if (config[index].tone != tone)
            {
                config[index].tone = tone;        
                config_length = PsStore(PSKEY_TONES, config, config_length);
            }
            
            if (config_length == 0)
                gaia_send_insufficient_resources(GAIA_COMMAND_SET_TONE_CONFIGURATION);
                
            else
            {
                if (no_tones != lengths.no_tones)
                {
                    lengths.no_tones = no_tones;
                    config_length = PsStore(PSKEY_LENGTHS, &lengths, sizeof lengths);
                }
            
                if (config_length == 0)
                {
                    DEBUG(("GAIA: PsStore lengths failed\n"));
                    Panic();
                }
                
                else
                {
                    uint8 payload[2];
                    
                    payload[0] = event;
                    payload[1] = tone;
                    gaia_send_success_payload(GAIA_COMMAND_SET_TONE_CONFIGURATION, 2, payload);
                }
            }
            
            freePanic(config);
        }
    }
}
    
    
/*************************************************************************
NAME
    gaia_send_tone_config
    
DESCRIPTION
    Respond to GAIA_COMMAND_GET_TONE_CONFIGURATION request for a single
    event-tone pair
*/       
static void gaia_send_tone_config(uint8 event)
{
    tone_config_type *config;
    uint16 no_tones;
    uint16 index = 0;
    
    no_tones = gaia_retrieve_tone_config(&config);
    
    if (no_tones == 0)
        gaia_send_insufficient_resources(GAIA_COMMAND_SET_TONE_CONFIGURATION);
        
    else
    {
        while ((index < no_tones) && (config[index].event != event))
            ++index;
        
        if (index == no_tones)
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_TONE_CONFIGURATION);
        
        else
        {
            uint8 payload[2];
            payload[0] = config[index].event;
            payload[1] = config[index].tone;
            gaia_send_success_payload(GAIA_COMMAND_GET_TONE_CONFIGURATION, 2, payload);
        }
    }
    
    if (config != NULL)
        freePanic(config);
}
    

/*************************************************************************
NAME
    gaia_send_all_tone_configs
    
DESCRIPTION
    Respond to GAIA_COMMAND_GET_TONE_CONFIGURATION with all tone configs
*/       
static void gaia_send_all_tone_configs(void)
{
    tone_config_type *tones = NULL;
    uint16 no_tones;
    
    no_tones = gaia_retrieve_tone_config(&tones);
    GAIA_DEBUG(("G: atc: gaia_retrieve_tone_config: %d\n", no_tones));
    
    if (no_tones == 0)
        gaia_send_insufficient_resources(GAIA_COMMAND_GET_TONE_CONFIGURATION);
        
    else
    {
        gaia_send_response_16(GAIA_COMMAND_GET_TONE_CONFIGURATION, 
                               GAIA_STATUS_SUCCESS, no_tones, (uint16 *) tones);
    }
    
    if (tones != NULL)
        freePanic(tones);
}

    
/*************************************************************************
NAME
    gaia_retrieve_feature_block
    
DESCRIPTION
    Return a pointer to malloc'd feature block, NULL on failure
*/
static feature_config_type *gaia_retrieve_feature_block(void)
{
    feature_config_type *feature_block = mallocPanic(sizeof (feature_config_type));
    
    if (feature_block != NULL)
    {
        ConfigRetrieve(theSink.config_id, PSKEY_FEATURE_BLOCK, feature_block, sizeof (feature_config_type)); 
    }

    return feature_block;
}


/*************************************************************************
NAME
    gaia_set_feature_config
    
DESCRIPTION
    GAIA_COMMAND_SET_FEATURE_CONFIGURATION
*/
static void gaia_set_feature_config(uint8 feature, uint8 value)
{
    feature_config_type *feature_block = gaia_retrieve_feature_block();
    uint16 status;
    uint8 payload[2];
    
    payload[0] = feature;
    payload[1] = value;

    if (feature_block == NULL)
        status = GAIA_STATUS_INSUFFICIENT_RESOURCES;
        
    else if (feature >= sizeof gaia_feature_map)
        status = GAIA_STATUS_INVALID_PARAMETER;
    
    else
    {
        gaia_feature_map_t map = gaia_feature_map[feature];
        uint16 mask = width_mask[map.size];

        GAIA_DEBUG(("G: map: %d (%d %d %d)\n", feature, map.word, map.posn, map.size));

        if ((map.size == 0) || (value & ~mask))
            status = GAIA_STATUS_INVALID_PARAMETER;
        
        else
        {
            uint16 *vector = (uint16 *) feature_block;
            vector[map.word] = (vector[map.word] & ~(mask << map.posn)) | (value << map.posn);
            status = GAIA_STATUS_SUCCESS;
        }
        
        if (status == GAIA_STATUS_SUCCESS)
            if (PsStore(PSKEY_FEATURE_BLOCK, feature_block, sizeof (feature_config_type)) == 0)
                status = GAIA_STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (feature_block)
        freePanic(feature_block);
    
    gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_SET_FEATURE_CONFIGURATION, 
                     status, sizeof payload, payload);
}


/*************************************************************************
NAME
    gaia_send_feature_config
    
DESCRIPTION
    GAIA_COMMAND_GET_FEATURE_CONFIGURATION
*/
static void gaia_send_feature_config(uint8 feature)
{
    feature_config_type *features = gaia_retrieve_feature_block();
    uint16 status;
    uint8 payload[2];


    payload[0] = feature;
    payload[1] = 0;
    
    if (features == NULL)
        status = GAIA_STATUS_INSUFFICIENT_RESOURCES;
        
    else if (feature >= sizeof gaia_feature_map)
        status = GAIA_STATUS_INVALID_PARAMETER;
    
    else
    {
        gaia_feature_map_t map = gaia_feature_map[feature];

        GAIA_DEBUG(("G: ftr: %d (%d %d %d)\n", feature, map.word, map.posn, map.size));

        if (map.size == 0)
            status = GAIA_STATUS_INVALID_PARAMETER;
        
        else
        {
            uint16 *vector = (uint16 *) features;
            payload[1] = (vector[map.word] >> map.posn) & width_mask[map.size];
            status = GAIA_STATUS_SUCCESS;
        }
    }
    
    
    if (features)
        freePanic(features);

    gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_FEATURE_CONFIGURATION, status, 2, payload);
}


/*************************************************************************
NAME
    gaia_retrieve_event_config
    
DESCRIPTION
    Attempt to retrieve the indexed event configuration
    Return pointer to config block on success
    On failure, send an appropriate Gaia response and return NULL
*/
static event_config_type *gaia_retrieve_event_config(uint16 command_id, uint8 index)
{
    event_config_type *config;
    uint16 key;
    
    if (index >= BM_EVENTS_PER_PS_BLOCK * 3  /* BM_NUM_BLOCKS */)
    {
        gaia_send_invalid_parameter(command_id);
        return NULL;
    }
    
    config = mallocPanic(BM_EVENTS_PER_PS_BLOCK * sizeof (event_config_type));
    if (config == NULL)
    {
        gaia_send_insufficient_resources(command_id);
        return NULL;
    }
        
    
    if (index < BM_EVENTS_PER_PS_BLOCK)
        key = PSKEY_EVENTS_A;
    
    else if (index < 2 * BM_EVENTS_PER_PS_BLOCK)
    {
        key = PSKEY_EVENTS_B;
        index -= BM_EVENTS_PER_PS_BLOCK;
    }
    
    else
    {
        key = PSKEY_EVENTS_C;
        index -= 2 * BM_EVENTS_PER_PS_BLOCK;
    }
    
    
    ConfigRetrieve(theSink.config_id, key, config, BM_EVENTS_PER_PS_BLOCK * sizeof (event_config_type));            
    return config;
}


/*************************************************************************
NAME
    gaia_set_user_event_config
    
DESCRIPTION
    Handle GAIA_COMMAND_SET_USER_EVENT_CONFIG to configure the indexed
    user event.  The payload contains eight octets:
      o  The event index
      o  The event ID
      o  The event type
      o  The PIO mask (three octets, including chg & vreg)
      o  The state mask (two octets)
*/
static void gaia_set_user_event_config(uint8 *payload)
{
    event_config_type *config;
    uint16 index = payload[0];
    
    config = gaia_retrieve_event_config(GAIA_COMMAND_SET_USER_EVENT_CONFIGURATION,index);
    if (config)
    {
        uint16 key;
        
        GAIA_DEBUG(("G: ue: %02X: ", index));
        
        if (index < BM_EVENTS_PER_PS_BLOCK)
            key = PSKEY_EVENTS_A;
        
        else if (index < 2 * BM_EVENTS_PER_PS_BLOCK)
        {
            key = PSKEY_EVENTS_B;
            index -= BM_EVENTS_PER_PS_BLOCK;
        }

        else
        {
            key = PSKEY_EVENTS_C;
            index -= 2 * BM_EVENTS_PER_PS_BLOCK;
        }

        config[index].event = payload[1];
        config[index].type = payload[2];
        
        config[index].pio_mask = (payload[4] << 8) | (payload[5]);
        
        config[index].state_mask = (payload[3] << 12)
                                   | (payload[6] << 8)
                                   | (payload[7]);
    
        GAIA_DEBUG(("- %02X %02X %04X %04X\n",
                    config[index].event, 
                    config[index].type, 
                    config[index].pio_mask, 
                    config[index].state_mask));
    
        if (PsStore(key, config, BM_EVENTS_PER_PS_BLOCK * sizeof (event_config_type)))
            gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_SET_USER_EVENT_CONFIGURATION, 
                            GAIA_STATUS_SUCCESS, 8, payload);
        
        else
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_USER_EVENT_CONFIGURATION);
        
        freePanic(config);
    }
}


/*************************************************************************
NAME
    gaia_send_user_event_config
    
DESCRIPTION
    Respond to GAIA_COMMAND_GET_USER_EVENT_CONFIG to retrieve the indexed
    user event configuration.  The response packet contains the command
    status followed by
      o  The event index
      o  The event ID
      o  The event type
      o  The PIO mask (three octets, including chg & vreg)
      o  The state mask (two octets)
*/
static void gaia_send_user_event_config(uint8 index)
{
    event_config_type *config;
    
    config = gaia_retrieve_event_config(GAIA_COMMAND_GET_USER_EVENT_CONFIGURATION, index);
    if (config)
    {
        uint8 payload[8];
    
        GAIA_DEBUG(("G: ue: %02X: ", index));
            
        payload[0] = index;
        
        index %= BM_EVENTS_PER_PS_BLOCK;
        
        GAIA_DEBUG(("- %02X %02X %04X %04X\n",
                    config[index].event, 
                    config[index].type, 
                    config[index].pio_mask, 
                    config[index].state_mask));
        
        payload[1] = config[index].event;
        payload[2] = config[index].type;
        payload[3] = config[index].state_mask >> 12;
        payload[4] = config[index].pio_mask >> 8;
        payload[5] = config[index].pio_mask & 0xFF;
        payload[6] = (config[index].state_mask >> 8) & 0x0F;
        payload[7] = config[index].state_mask & 0xFF;
        
        gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_USER_EVENT_CONFIGURATION, 
                         GAIA_STATUS_SUCCESS, 8, payload);
        freePanic(config);
    }
}

 
/*************************************************************************
NAME
    gaia_send_default_volumes
    
DESCRIPTION
    Respond to GAIA_COMMAND_GET_DEFAULT_VOLUME
    Three octets after the status represent tones, speech and music volumes
*/
static void gaia_send_default_volumes(void)
{
    feature_config_type *feature_block = gaia_retrieve_feature_block();
    
    if (feature_block == NULL)
        gaia_send_insufficient_resources(GAIA_COMMAND_GET_DEFAULT_VOLUME);
    
    else
    {
        uint8 payload[3];
        
        payload[0] = feature_block->FixedToneVolumeLevel;
        payload[1] = feature_block->DefaultVolume;
        payload[2] = feature_block->DefaultA2dpVolLevel;
        gaia_send_success_payload(GAIA_COMMAND_GET_DEFAULT_VOLUME, 3, payload);
        
        freePanic(feature_block);
    }
}


/*************************************************************************
NAME
    gaia_set_default_volumes
    
DESCRIPTION
    Respond to GAIA_COMMAND_SET_DEFAULT_VOLUME
    The three payload octets represent tones, speech and music volumes
*/
static void gaia_set_default_volumes(uint8 volumes[])
{
    if ((volumes[0] > 22)  /* should be def or const for MAX_SPEAKER_GAIN_INDEX */
        || (volumes[1] >= VOL_NUM_VOL_SETTINGS)
        || (volumes[2] >= VOL_NUM_VOL_SETTINGS))
        gaia_send_invalid_parameter(GAIA_COMMAND_SET_DEFAULT_VOLUME);
   
    else
    {
        feature_config_type *feature_block = gaia_retrieve_feature_block();
        
        if (feature_block == NULL)
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_DEFAULT_VOLUME);
        
        else
        {
            feature_block->FixedToneVolumeLevel = volumes[0];
            feature_block->DefaultVolume = volumes[1];
            feature_block->DefaultA2dpVolLevel = volumes[2];
        
            if (PsStore(PSKEY_FEATURE_BLOCK, feature_block, sizeof (feature_config_type)) > 0)
                gaia_send_success(GAIA_COMMAND_SET_DEFAULT_VOLUME);
        
            else
                gaia_send_insufficient_resources(GAIA_COMMAND_SET_DEFAULT_VOLUME);
        
            freePanic(feature_block);
        }
    }
}


/*************************************************************************
NAME
    gaia_send_config_id
    
DESCRIPTION
    Handle a GAIA_COMMAND_GET_CONFIGURATION_ID command
*/
static void gaia_send_config_id(void)
{
    uint8 payload[2];
    
    payload[0] = theSink.config_id >> 8;
    payload[1] = theSink.config_id & 0xFF;
    
    gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_CONFIGURATION_ID, 
                     GAIA_STATUS_SUCCESS, 2, payload);
}


/*************************************************************************
NAME
    gaia_factory_reset_config
    
DESCRIPTION
    Perform a 'factory reset' by deleting PS keys which override defaults
    and setting the configuration ID.  It is possible to store an
    invalid id; we rely on set_config_id() to deal with that.
*/
static void gaia_factory_reset_config(uint16 config_id)
{
    uint16 key;
    
    for (key = 0; key < sizeof (config_type); ++key)
        PsStore(key, NULL, 0);
    
    if (PsStore(PSKEY_CONFIGURATION_ID, &config_id, 1))
        gaia_send_success(GAIA_COMMAND_FACTORY_DEFAULT_RESET);    
    else
        gaia_send_insufficient_resources(GAIA_COMMAND_FACTORY_DEFAULT_RESET);
    
    /* update the config id */
    set_config_id ( PSKEY_CONFIGURATION_ID ) ;
}


/*************************************************************************
NAME
    gaia_set_tts_config
    
DESCRIPTION
    Set the TTS configuration requested by a
    GAIA_COMMAND_SET_VOICE_PROMPT_CONFIGURATION command
*/
static void gaia_set_tts_config(uint8 *payload)
{
    lengths_config_type lengths;
    tts_config_type *config;
    uint16 config_len;
    uint16 no_tts;
    uint8 event = payload[0];
    uint8 status = GAIA_STATUS_SUCCESS;
    
    get_config_lengths(&lengths);
    
    GAIA_DEBUG(("G: tts: %d %02X\n", lengths.no_tts, event));
    
    no_tts = lengths.no_tts;
    config_len = no_tts * sizeof (tts_config_type);
    config = mallocPanic(config_len + sizeof (tts_config_type));
    
    if (config == NULL)
        status = GAIA_STATUS_INSUFFICIENT_RESOURCES;
    
    else
    {
        uint16 idx = 0;
        
        ConfigRetrieve(theSink.config_id, PSKEY_TTS, config, config_len);
        
        while ((idx < lengths.no_tts) && (config[idx].event != event))
            ++idx;
    
        if (idx == lengths.no_tts)
        {
            ++no_tts;
            config_len += sizeof (tts_config_type);
        }
            
        config[idx].event = event;
        config[idx].tts_id = payload[1];
        config[idx].sco_block = payload[2];
        config[idx].state_mask = payload[3] << 8;
        config[idx].state_mask |= payload[4];
        
        if (PsStore(PSKEY_TTS, config, config_len) == 0)
            status = GAIA_STATUS_INSUFFICIENT_RESOURCES;
        
        else if (no_tts > lengths.no_tts)
        {
            lengths.no_tts = no_tts;
            if (PsStore(PSKEY_LENGTHS, &lengths, sizeof lengths) == 0)
            {
                DEBUG(("GAIA: PsStore lengths failed\n"));
                Panic();
            }
        }
        
        freePanic(config);
    }

    gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_SET_VOICE_PROMPT_CONFIGURATION, 
                     status, 0, NULL);
}


/*************************************************************************
NAME
    gaia_send_tts_config
    
DESCRIPTION
    Send the TTS configuration requested by a
    GAIA_COMMAND_GET_VOICE_PROMPT_CONFIGURATION command. The configuration
    for an event is sent in five octets representing
        o  event
        o  tts_id (the voice prompt number)
        o  sco_block (TRUE meaning 'do not play if SCO is present')
        o  state_mask, upper 6 bits (states in which prompt will play)
        o  state_mask, lower 8 bits
*/
static void gaia_send_tts_config(uint8 event)
{
    lengths_config_type lengths;
    uint16 payload_len = 0;
    uint8 payload[5];
    uint8 status;
    
    get_config_lengths(&lengths);
    
    GAIA_DEBUG(("G: tts: %d %02X\n", lengths.no_tts, event));
    
    if (lengths.no_tts == 0)
        status = GAIA_STATUS_INVALID_PARAMETER;
    
    else
    {
        uint16 config_len = lengths.no_tts * sizeof (tts_config_type);
        tts_config_type *config = mallocPanic(config_len);
        
        if (config == NULL)
            status = GAIA_STATUS_INSUFFICIENT_RESOURCES;
        
        else
        {
            uint16 idx = 0;
            
            ConfigRetrieve(theSink.config_id, PSKEY_TTS, config, config_len);
            
            while ((idx < lengths.no_tts) && (config[idx].event != event))
                ++idx;
                        
            if (idx == lengths.no_tts)
                status = GAIA_STATUS_INVALID_PARAMETER;
            
            else
            {
                payload[0] = event;
                payload[1] = config[idx].tts_id;
                payload[2] = config[idx].sco_block;
                payload[3] = config[idx].state_mask >> 8;
                payload[4] = config[idx].state_mask & 0xFF;
                
                payload_len = 5;
                status = GAIA_STATUS_SUCCESS;
            }
            
            freePanic(config);
        }
    }
    
    gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_VOICE_PROMPT_CONFIGURATION, 
                     status, payload_len, payload);
}


/*************************************************************************
NAME
    gaia_set_timer_config
    
DESCRIPTION
    Handle GAIA_COMMAND_SET_TIMER_CONFIGURATION
    For simplicity we just pack the payload into a Timeouts_t structure,
    which works so long as nobody minds EncryptionRefreshTimeout_m being
    in minutes
*/
static void gaia_set_timer_config(uint8 payload_len, uint8 *payload)
{
    uint16 config_length = sizeof (Timeouts_t);
    
    if (payload_len != (2 * config_length))
        gaia_send_invalid_parameter(GAIA_COMMAND_SET_TIMER_CONFIGURATION);
    
    else
    {
        uint16 *config = mallocPanic(config_length);
        
        if (config == NULL)
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_TIMER_CONFIGURATION);
        
        else
        {
            wpack(config, payload, config_length);
            
            if (PsStore(PSKEY_TIMEOUTS, config, config_length))
                gaia_send_success(GAIA_COMMAND_SET_TIMER_CONFIGURATION);
            
            else
                gaia_send_insufficient_resources(GAIA_COMMAND_SET_TIMER_CONFIGURATION);
            
            freePanic(config);
        }
    }
}


/*************************************************************************
NAME
    gaia_send_timer_config
    
DESCRIPTION
    Handle GAIA_COMMAND_GET_TIMER_CONFIGURATION
    For simplicity we just bundle up the Timeouts_t structure, which works
    so long as nobody minds EncryptionRefreshTimeout_m being in minutes
*/
static void gaia_send_timer_config(void)
{
    uint16 config_length = sizeof (Timeouts_t);
    Timeouts_t *timeouts = mallocPanic(config_length);
    
    if (timeouts == NULL)
        gaia_send_insufficient_resources(GAIA_COMMAND_GET_TIMER_CONFIGURATION);
    
    else
    {
        ConfigRetrieve(theSink.config_id, PSKEY_TIMEOUTS, 
                       timeouts, config_length);
        
        gaia_send_response_16(GAIA_COMMAND_GET_TIMER_CONFIGURATION, 
                               GAIA_STATUS_SUCCESS, config_length, (uint16 *) timeouts);
        freePanic(timeouts);
    }
}


/*************************************************************************
NAME
    gaia_set_audio_gain_config
    
DESCRIPTION
    Handle GAIA_COMMAND_SET_AUDIO_GAIN_CONFIGURATION
    The command payload contains five octets for each volume mapping,
    corresponding to IncVol, DecVol, Tone, A2dpGain and VolGain
*/
static void gaia_set_audio_gain_config(uint8 payload_len, uint8 *payload)
{
    if (payload_len != (VOL_NUM_VOL_SETTINGS * 5))
        gaia_send_invalid_parameter(GAIA_COMMAND_SET_AUDIO_GAIN_CONFIGURATION);
    
    else
    {
        uint16 config_len = VOL_NUM_VOL_SETTINGS * sizeof (VolMapping_t);
        VolMapping_t *config = mallocPanic(config_len);
        
        if (config == NULL)
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_AUDIO_GAIN_CONFIGURATION);
            
        else
        {
            const uint8 max_vol = 15;
            const uint8 max_tone = 161;  /* WAV_RING_TONE_TOP - 1, not quite right  */
            const uint8 max_gain = 22;
            
            uint16 idx;
            uint8* data = payload;
            bool data_error = FALSE;
            
        
            for (idx = 0; !data_error && (idx < VOL_NUM_VOL_SETTINGS); ++idx)
            {
                if (*data > max_vol)
                    data_error = TRUE;
                
                config[idx].IncVol = *data++;
                
                if (*data > max_vol)
                    data_error = TRUE;
                
                config[idx].DecVol = *data++;
                
                if (*data > max_tone)
                    data_error = TRUE;

                config[idx].Tone = *data++;
                
                if (*data > max_gain)
                    data_error = TRUE;
                
                config[idx].A2dpGain = *data++;

                if (*data > max_gain)
                    data_error = TRUE;
                
                config[idx].VolGain = *data++;
            }
            
            if (data_error)
                gaia_send_invalid_parameter(GAIA_COMMAND_SET_AUDIO_GAIN_CONFIGURATION);
            
            else if (PsStore(PSKEY_SPEAKER_GAIN_MAPPING, config, config_len) == 0)
                gaia_send_insufficient_resources(GAIA_COMMAND_SET_AUDIO_GAIN_CONFIGURATION);
            
            else
                gaia_send_success(GAIA_COMMAND_SET_AUDIO_GAIN_CONFIGURATION);

            freePanic(config);
        }
    }  
}

        
/*************************************************************************
NAME
    gaia_send_audio_gain_config
    
DESCRIPTION
    Handle GAIA_COMMAND_GET_AUDIO_GAIN_CONFIGURATION
    The response contains five octets for each volume mapping, corresponding
    to IncVol, DecVol, Tone, A2dpGain and VolGain
*/
static void gaia_send_audio_gain_config(void)
{
    uint16 payload_len = VOL_NUM_VOL_SETTINGS * 5;
    uint16 config_length = VOL_NUM_VOL_SETTINGS * sizeof (VolMapping_t);
    VolMapping_t *payload = mallocPanic(payload_len);
    
/*  assert(sizeof (VolMapping_t) <= 5);  */
    
    if (payload == NULL)
        gaia_send_insufficient_resources(GAIA_COMMAND_GET_AUDIO_GAIN_CONFIGURATION);
    
    else
    {
        uint16 idx = VOL_NUM_VOL_SETTINGS;
        uint8 *data = (uint8 *) payload + payload_len;
        
        ConfigRetrieve(theSink.config_id, PSKEY_SPEAKER_GAIN_MAPPING, 
                       payload, config_length);
        
        while (idx--)
        {
            *--data = payload[idx].VolGain;            
            *--data = payload[idx].A2dpGain;
            *--data = payload[idx].Tone;
            *--data = payload[idx].DecVol;
            *--data = payload[idx].IncVol;
        }
        
        gaia_send_success_payload(GAIA_COMMAND_GET_AUDIO_GAIN_CONFIGURATION, payload_len, payload);
        freePanic(payload);
    }  
}


/*************************************************************************
NAME
    gaia_set_user_tone_config
    
DESCRIPTION
    Handle GAIA_COMMAND_SET_USER_TONE_CONFIGURATION command
    The command payload contains a tone number (1 to
    MAX_NUM_VARIABLE_TONES) and note data for the indicated tone.
    
    Note data consists of tempo, volume, timbre and decay values for
    the whole sequence followed by pitch and length values for each note.
    
    If the indicated tone number is not in use we add it, otherwise we
    delete the existing data, compact the free space and add the new tone
    data at the end of the list.
*/
static void gaia_set_user_tone_config(uint8 payload_len, uint8 *payload)
{
    lengths_config_type lengths;
    uint16 config_len;
    uint16 *note_data;
    uint16 tone_len;
    uint16 idx;
    uint16 old;
    uint8 tone_idx;
    
    if ((payload_len < 5) || (payload[0] < 1) || (payload[0] > MAX_NUM_VARIABLE_TONES))
    {
        gaia_send_invalid_parameter(GAIA_COMMAND_SET_USER_TONE_CONFIGURATION);
        return;
    }
    
    note_data = mallocPanic(GAIA_TONE_BUFFER_SIZE);
    
    if (note_data == NULL)
    {
        gaia_send_insufficient_resources(GAIA_COMMAND_SET_USER_TONE_CONFIGURATION);
        return;
    }
            
    get_config_lengths(&lengths);
    
    config_len = lengths.userTonesLength;
    if (config_len == 0)
    {
        /* No tones; create empty list  */
        config_len = MAX_NUM_VARIABLE_TONES;
        for (idx = 0; idx < config_len; ++idx)
            note_data[idx] = 0;    
    }
    
    else
        ConfigRetrieve(theSink.config_id, PSKEY_CONFIG_TONES, note_data, config_len);
    
#ifdef DEBUG_GAIA
    dump("before", note_data, config_len);
#endif
    
    tone_idx = payload[0] - 1;

    old = note_data[tone_idx];
    /* If the index in use, purge it  */
    if (old)
    {
        /*  Find the end  */
        idx = old;
        while (note_data[idx] != (uint16) RINGTONE_END)
            ++idx;
        
        tone_len = idx - old + 1;

        /* Adjust any offsets into purged space  */
        for (idx = 0; idx < MAX_NUM_VARIABLE_TONES; ++idx)
            if (note_data[idx] > old)
                note_data[idx] -= tone_len;
        
        /* Close the gap  */
        config_len -= tone_len;
        for (idx = old; idx < config_len; ++idx)
            note_data[idx] = note_data[idx + tone_len];
    }
        
#ifdef DEBUG_GAIA
    dump("during", note_data, config_len);
#endif
        
    tone_len = (payload_len - 4) / 2 + 1;
    
    if ((config_len + tone_len) > GAIA_TONE_BUFFER_SIZE)  /* TODO: test this sooner! */
        gaia_send_insufficient_resources(GAIA_COMMAND_SET_USER_TONE_CONFIGURATION);
    
    else
    {
        /* Copy new tone into free space  */
        note_data[tone_idx] = config_len;
        
        if (payload[1] != 0)
            note_data[config_len++] = (uint16) RINGTONE_TEMPO(payload[1] * 4);
        
        if (payload[2] != 0)
            note_data[config_len++] = (uint16) RINGTONE_VOLUME(payload[2]);
        
        if (payload[3] != 0)
            note_data[config_len++] = (uint16) RINGTONE_SEQ_TIMBRE | payload[3];
        
        if (payload[4] != 0)
            note_data[config_len++] = (uint16) RINGTONE_DECAY(payload[4]);
        
        for (idx = 5; idx < payload_len; idx += 2)
            note_data[config_len++] = (uint16) pitch_length_tone(payload[idx], payload[idx + 1]);
        
        note_data[config_len++] = (uint16) RINGTONE_END;
        
#ifdef DEBUG_GAIA
        dump("after ", note_data, config_len);
#endif

        /*  Write back to persistent store  */
        if (PsStore(PSKEY_CONFIG_TONES, note_data, config_len) == 0)
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_USER_TONE_CONFIGURATION);
        
        else
        {
            /*  Adjust the lengths PS key if necessary */
            if (config_len != lengths.userTonesLength)
            {
                lengths.userTonesLength = config_len;
                if (PsStore(PSKEY_LENGTHS, &lengths, sizeof lengths) == 0)
                {
                    DEBUG(("GAIA: PsStore lengths failed\n"));
                    Panic();
                }
            }
            
            gaia_send_success(GAIA_COMMAND_SET_USER_TONE_CONFIGURATION);
        }
    }
        
    freePanic(note_data);
}


/*************************************************************************
NAME
    gaia_send_user_tone_config
    
DESCRIPTION
    Act on a GAIA_COMMAND_GET_USER_TONE_CONFIGURATION command to send the
    note data for the indicated user-defined tone configuration.

    Note data consists of tempo, volume, timbre and decay values for
    the whole sequence followed by pitch and length values for each note.
    
    At most one of each tempo, volume, timbre and decay value is used and
    the tempo value is scaled, so if the configuration was set other than
    by Gaia it might not be accurately represented.
    
    We expect sizeof (ringtone_note) to be 1; see ringtone_if.h
*/
static void gaia_send_user_tone_config(uint8 id)
{
    lengths_config_type lengths;
    uint16 config_len;
    uint16 *note_data = NULL;

    get_config_lengths(&lengths);

    if ((id < 1) || (id > MAX_NUM_VARIABLE_TONES) || (lengths.userTonesLength == 0))
    {
        gaia_send_invalid_parameter(GAIA_COMMAND_GET_USER_TONE_CONFIGURATION);
        return;
    }
    
    config_len = lengths.userTonesLength;
    note_data = mallocPanic(config_len);
    
    if (note_data == NULL)
        gaia_send_insufficient_resources(GAIA_COMMAND_GET_USER_TONE_CONFIGURATION);
    
    else
    {
        ConfigRetrieve(theSink.config_id, PSKEY_CONFIG_TONES, note_data, config_len);
        
        if (note_data[id - 1] == 0)
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_USER_TONE_CONFIGURATION);
        
        else
        {
            uint16 *start = note_data + note_data[id - 1];
            uint16 *find = start;
            uint16 tone_length = 0;
            uint8 payload_length = 0;
            uint8 *payload = NULL;
            
            note_data[0] = 30; /* default tempo (30 * 4 = 120) */
            note_data[1] = RINGTONE_SEQ_VOLUME_MASK; /* default volume (max) */
            note_data[2] = ringtone_tone_sine;  /* default timbre (sine) */
            note_data[3] = 0x20;  /* default decay rate */
            
        /*  Find the tempo, volume, timbre and decay  */
            while (*find != (uint16) RINGTONE_END)
            {
                if (*find & RINGTONE_SEQ_CONTROL_MASK)
                {
                    switch (*find & RINGTONE_SEQ_CONTROL_COMMAND_MASK)
                    {
                    case RINGTONE_SEQ_TEMPO:
                        note_data[0] = ((*find & RINGTONE_SEQ_TEMPO_MASK) + 2) / 4;
                        break;
                        
                    case RINGTONE_SEQ_VOLUME:
                        note_data[1] = *find & RINGTONE_SEQ_VOLUME_MASK;
                        break;
                        
                    case RINGTONE_SEQ_TIMBRE:
                        note_data[2] = *find & RINGTONE_SEQ_TIMBRE_MASK;
                        break;
                        
                    case RINGTONE_SEQ_DECAY:
                        note_data[3] = *find & RINGTONE_SEQ_DECAY_RATE_MASK;
                        break;
                    }
                }
                
                else
                    ++tone_length;
                
                ++find;
            }
            
            payload_length = 2 * tone_length + 4;
            payload = mallocPanic(payload_length);
            
            if (payload == NULL)
                gaia_send_insufficient_resources(GAIA_COMMAND_GET_USER_TONE_CONFIGURATION);
            
            else
            {
                payload[0] = note_data[0];
                payload[1] = note_data[1];
                payload[2] = note_data[2];
                payload[3] = note_data[3];
                
                find = start;
                payload_length = 4;
                
            /*  Find the pitch and length of each note  */
                while (*find != (uint16) RINGTONE_END)
                {
                    if ((*find & RINGTONE_SEQ_CONTROL_MASK) == 0)
                    {
                        payload[payload_length++] = (*find & RINGTONE_SEQ_NOTE_PITCH_MASK) >> RINGTONE_SEQ_NOTE_PITCH_POS;
                        payload[payload_length++] = *find & RINGTONE_SEQ_NOTE_LENGTH_MASK;
                    }
                    
                    ++find;
                }
                
                gaia_send_success_payload(GAIA_COMMAND_GET_USER_TONE_CONFIGURATION,
                                          payload_length, payload);
                
                freePanic(payload);
            }
        }
        
        freePanic(note_data);
    }
}
        

/*************************************************************************
NAME
    gaia_retrieve_power_config
    
DESCRIPTION
    Attempt to retrieve the power configuration
    Return pointer to config block on success, NULL on failure
*/
static sink_power_config *gaia_retrieve_power_config(void)
{
    sink_power_config *config = mallocPanic(sizeof (sink_power_config));
    
    if (config)
        ConfigRetrieve(theSink.config_id, PSKEY_BATTERY_CONFIG, 
                       config, sizeof (sink_power_config));
    return config;
}


/*************************************************************************
NAME
    int_voltage
    
DESCRIPTION
    Convert two octets of millivolts to an internal, scaled voltage
*/
static uint8 int_voltage(uint8 *in)
{
    uint16 mv = (in[0] << 8) | in[1];
    return (mv + POWER_VSCALE / 2) / POWER_VSCALE;
}


/*************************************************************************
NAME
    ext_voltage
    
DESCRIPTION
    Convert an internal, scaled voltage to two octets of millivolts
*/
static void ext_voltage(uint8 *out, uint8 in)
{
    uint16 mv = POWER_VSCALE * in;
    out[0] = mv >> 8;
    out[1] = mv & 0xFF;
}


/*************************************************************************
NAME
    gaia_set_power_config
    
DESCRIPTION
    Handle GAIA_COMMAND_SET_POWER_CONFIGURATION
*/
static void gaia_set_power_config(uint8 *payload)
{
    sink_power_config *power = gaia_retrieve_power_config();
    
    if (power)
    {
    /*  Erase power_config, keep sink_power_settings  */
        memset(&power->config, 0, sizeof (power_config));
        
        power->config.vref.adc.source = payload[0];
        power->config.vref.adc.period_chg = payload[1];
        power->config.vref.adc.period_no_chg = payload[2];
        
        power->config.vbat.adc.source = payload[3];
        power->config.vbat.adc.period_chg = payload[4];
        power->config.vbat.adc.period_no_chg = payload[5];
        power->config.vbat.limits[0].limit = int_voltage(payload + 6);
        power->config.vbat.limits[1].limit = int_voltage(payload + 8);
        power->config.vbat.limits[2].limit = int_voltage(payload + 10);
        power->config.vbat.limits[3].limit = int_voltage(payload + 12);
        power->config.vbat.limits[4].limit = int_voltage(payload + 14);
        power->config.vbat.limits[5].limit = POWER_VBAT_LIMIT_END;
        
        power->config.vthm.adc.source = payload[16];
        power->config.vthm.adc.period_chg = payload[17];
        power->config.vthm.adc.period_no_chg = payload[18];
        power->config.vthm.limits[0] = (payload[19] << 8) | payload[20];
        power->config.vthm.limits[1] = (payload[21] << 8) | payload[22];
        power->config.vthm.limits[2] = POWER_VTHM_LIMIT_END;
        
        power->config.vchg.adc.source = payload[23];
        power->config.vchg.adc.period_chg = payload[24];
        power->config.vchg.adc.period_no_chg = payload[25];
        power->config.vchg.limit = int_voltage(payload + 26);

        if (PsStore(PSKEY_BATTERY_CONFIG, power, sizeof (sink_power_config)))
            gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_SET_POWER_CONFIGURATION, GAIA_STATUS_SUCCESS, 19, payload);
        
        else
            gaia_send_insufficient_resources(GAIA_COMMAND_SET_POWER_CONFIGURATION);
            
        freePanic(power);
    }
    
    else
        gaia_send_insufficient_resources(GAIA_COMMAND_SET_POWER_CONFIGURATION);
}


/*************************************************************************
NAME
    gaia_send_power_config
    
DESCRIPTION
    Handle GAIA_COMMAND_GET_POWER_CONFIGURATION
    Returns GAIA_CONFIGURATION_LENGTH_POWER (28) data octets:
     0      Vref ADC source
     1      Vref monitoring period, on charge
     2      Vref monitoring period, off charge
      
     3      Vbat ADC source
     4      Vbat monitoring period, on charge
     5      Vbat monitoring period, off charge
     6..15  Five battery thresholds (mV, two octets each)
      
    16      Vthm ADC source
    17      Vthm monitoring period, on charge
    18      Vthm monitoring period, off charge
    19..22  Two thermistor thresholds (mV, two octets each)
      
    23      Vchg ADC source
    24      Vchg monitoring period, on charge
    25      Vchg monitoring period, off charge
    26, 27  Vchg limit (mV, two octets)
*/
static void gaia_send_power_config(void)
{
    sink_power_config *power = gaia_retrieve_power_config();
    
    if (power)
    {
        uint8 payload[GAIA_CONFIGURATION_LENGTH_POWER];
        
        payload[0] = power->config.vref.adc.source;
        payload[1] = power->config.vref.adc.period_chg;
        payload[2] = power->config.vref.adc.period_no_chg;
        
        payload[3] = power->config.vbat.adc.source;
        payload[4] = power->config.vbat.adc.period_chg;
        payload[5] = power->config.vbat.adc.period_no_chg;
        
        ext_voltage(payload + 6, power->config.vbat.limits[0].limit);
        ext_voltage(payload + 8, power->config.vbat.limits[1].limit);
        ext_voltage(payload + 10, power->config.vbat.limits[2].limit);
        ext_voltage(payload + 12, power->config.vbat.limits[3].limit);
        ext_voltage(payload + 14, power->config.vbat.limits[4].limit);
        
        payload[16] = power->config.vthm.adc.source;
        payload[17] = power->config.vthm.adc.period_chg;
        payload[18] = power->config.vthm.adc.period_no_chg;
        payload[19] = power->config.vthm.limits[0] >> 8;
        payload[20] = power->config.vthm.limits[0] & 0xFF;
        payload[21] = power->config.vthm.limits[1] >> 8;
        payload[22] = power->config.vthm.limits[1] & 0xFF;
        
        payload[23] = power->config.vchg.adc.source;
        payload[24] = power->config.vchg.adc.period_chg;
        payload[25] = power->config.vchg.adc.period_no_chg;
        ext_voltage(payload + 26, power->config.vchg.limit);
        
        
        gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_POWER_CONFIGURATION, GAIA_STATUS_SUCCESS, 
                         GAIA_CONFIGURATION_LENGTH_POWER, payload);
        freePanic(power);
    }
    
    else
        gaia_send_insufficient_resources(GAIA_COMMAND_GET_POWER_CONFIGURATION);
}


/*************************************************************************
NAME
    gaia_send_application_version
    
DESCRIPTION
    Handle GAIA_COMMAND_GET_APPLICATION_VERSION by sending the Device ID
    setting
*/
static void gaia_send_application_version(void)
{    
#ifdef DEVICE_ID_CONST
/*  Device ID is defined in sink_device_id.h  */
    uint16 payload[] = {DEVICE_ID_VENDOR_ID_SOURCE,
                        DEVICE_ID_VENDOR_ID,
                        DEVICE_ID_PRODUCT_ID,
                        DEVICE_ID_BCD_VERSION};
    
    gaia_send_response_16(GAIA_COMMAND_GET_APPLICATION_VERSION, 
                         GAIA_STATUS_SUCCESS, sizeof payload, payload);
#else
/*  Read Device ID from PS, irrespective of DEVICE_ID_PSKEY  */
    uint16 payload[8];
    uint16 payload_length;
    
    payload_length = PsRetrieve(PSKEY_DEVICE_ID, payload, sizeof payload);

    gaia_send_response_16(GAIA_COMMAND_GET_APPLICATION_VERSION, 
                         GAIA_STATUS_SUCCESS, payload_length, payload);    
#endif
}
  

/*************************************************************************
NAME
    gaia_set_feature
    
DESCRIPTION
    Handle GAIA_COMMAND_SET_FEATURE
*/
static void gaia_set_feature(uint8 payload_length, uint8 *payload)
{
    if ((payload_length == 0) || (payload[0] > GAIA_MAX_FEATURE_BIT))
        gaia_send_invalid_parameter(GAIA_COMMAND_SET_FEATURE);
    
    else
        gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_SET_FEATURE, GAIA_STATUS_NOT_AUTHENTICATED, 1, payload);
}


/*************************************************************************
NAME
    gaia_get_feature
    
DESCRIPTION
    Handle GAIA_COMMAND_GET_FEATURE
*/
static void gaia_get_feature(uint8 payload_length, uint8 *payload)
{
    if ((payload_length != 1) || (payload[0] > GAIA_MAX_FEATURE_BIT))
        gaia_send_invalid_parameter(GAIA_COMMAND_SET_FEATURE);
    
    else
    {
        uint8 response[2];
        response[0] = payload[0];
        response[1] = 0x00;
        gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_FEATURE, GAIA_STATUS_SUCCESS, 2, response);
    }
}


/*************************************************************************
NAME
    gaia_register_notification
    
DESCRIPTION
    Handle GAIA_COMMAND_REGISTER_NOTIFICATION
*/
static void gaia_register_notification(uint8 payload_length, uint8 *payload)
{
    if (payload_length == 0)
        gaia_send_invalid_parameter(GAIA_COMMAND_REGISTER_NOTIFICATION);
    
    else
    {
        uint8 status = GAIA_STATUS_INVALID_PARAMETER;
        
        GAIA_DEBUG(("G: reg: %02X %d\n", payload[0], payload_length));
        switch (payload[0])
        {
        case GAIA_EVENT_PIO_CHANGED:
            if (payload_length == 5)
            {
                uint32 mask;
                
                mask = payload[1];
                mask = (mask << 8) | payload[2];
                mask = (mask << 8) | payload[3];
                mask = (mask << 8) | payload[4];
                
                gaia_data.pio_change_mask = mask;
                
                GAIA_DEBUG(("G: pm: %08lX\n", mask));
                status = GAIA_STATUS_SUCCESS;
            }

            break;
                
            
        case GAIA_EVENT_BATTERY_CHARGED:
            gaia_data.notify_battery_charged = TRUE;
            status = GAIA_STATUS_SUCCESS;
            break;
            
            
        case GAIA_EVENT_CHARGER_CONNECTION:
            gaia_data.notify_charger_connection = TRUE;
            status = GAIA_STATUS_SUCCESS;
            break;
            
            
        case GAIA_EVENT_USER_ACTION:
            gaia_data.notify_ui_event = TRUE;
            status = GAIA_STATUS_SUCCESS;
            break;
            
            
#ifdef ENABLE_SPEECH_RECOGNITION
        case GAIA_EVENT_SPEECH_RECOGNITION:
            gaia_data.notify_speech_rec = TRUE;
            status = GAIA_STATUS_SUCCESS;
            break;
#endif
        }
        
        gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_REGISTER_NOTIFICATION, status, 1, payload);
    } 
}


/*************************************************************************
NAME
    gaia_get_notification
    
DESCRIPTION
    Handle GAIA_COMMAND_GET_NOTIFICATION
    Respond with the current notification setting (enabled/disabled status
    and notification-specific data)
*/
static void gaia_get_notification(uint8 payload_length, uint8 *payload)
{
    uint16 response_len = 0;
    uint8 response[6];
    uint8 status = GAIA_STATUS_INVALID_PARAMETER;
    
    if (payload_length > 0)
    {
        response[0] = payload[0];
        response_len = 1;
        
        switch (payload[0])
        {
        case GAIA_EVENT_PIO_CHANGED:
            response[1] = gaia_data.pio_change_mask != 0;
            dwunpack(response + 2, gaia_data.pio_change_mask);     
            response_len = 6;
            status = GAIA_STATUS_SUCCESS;
            break;
            
        case GAIA_EVENT_BATTERY_CHARGED:
            response[1] = gaia_data.notify_battery_charged;
            response_len = 2;
            status = GAIA_STATUS_SUCCESS;
            break;
            
        case GAIA_EVENT_CHARGER_CONNECTION:
            response[1] = gaia_data.notify_charger_connection;
            response_len = 2;
            status = GAIA_STATUS_SUCCESS;
            break;
            
        case GAIA_EVENT_USER_ACTION:
            response[1] = gaia_data.notify_ui_event;
            response_len = 2;
            status = GAIA_STATUS_SUCCESS;
            break;
            
#ifdef ENABLE_SPEECH_RECOGNITION
        case GAIA_EVENT_SPEECH_RECOGNITION:
            response[1] = gaia_data.notify_speech_rec;
            response_len = 2;
            status = GAIA_STATUS_SUCCESS;
            break;
#endif
        }
    }
    
    gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_NOTIFICATION, status, response_len, response);
}


/*************************************************************************
NAME
    gaia_cancel_notification
    
DESCRIPTION
    Handle GAIA_COMMAND_CANCEL_NOTIFICATION
*/
static void gaia_cancel_notification(uint8 payload_length, uint8 *payload)
{
    if (payload_length != 1)
        gaia_send_invalid_parameter(GAIA_COMMAND_CANCEL_NOTIFICATION);
    
    else
    {
        uint8 status;
        
        switch (payload[0])
        {
        case GAIA_EVENT_PIO_CHANGED:
            gaia_data.pio_change_mask = 0UL;
            status = GAIA_STATUS_SUCCESS;
            break;
            
        case GAIA_EVENT_BATTERY_CHARGED:
            gaia_data.notify_battery_charged = FALSE;
            status = GAIA_STATUS_SUCCESS;
            break;
            
        case GAIA_EVENT_CHARGER_CONNECTION:
            gaia_data.notify_charger_connection = FALSE;
            status = GAIA_STATUS_SUCCESS;
            break;
            
        case GAIA_EVENT_USER_ACTION:
            gaia_data.notify_ui_event = FALSE;
            status = GAIA_STATUS_SUCCESS;
            break;
            
#ifdef ENABLE_SPEECH_RECOGNITION            
        case GAIA_EVENT_SPEECH_RECOGNITION:
            gaia_data.notify_speech_rec = FALSE;
            status = GAIA_STATUS_SUCCESS;
            break;
#endif
            
        default:
            status = GAIA_STATUS_INVALID_PARAMETER;
            break;
        }
        
        gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_CANCEL_NOTIFICATION, status, 1, payload);
    } 
}


/*************************************************************************
NAME
    gaia_handle_configuration_command
    
DESCRIPTION
    Handle a Gaia configuration command or return FALSE if we can't
*/
static bool gaia_handle_configuration_command(Task task, GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    switch (command->command_id)
    {
    case GAIA_COMMAND_SET_LED_CONFIGURATION:
        if (command->size_payload < 6)
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_LED_CONFIGURATION);

        else
            gaia_set_led_config(command->size_payload, command->payload);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_LED_CONFIGURATION:
        if (command->size_payload == 2)
            gaia_send_led_config(command->payload[0], command->payload[1]);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_LED_CONFIGURATION);
        
        return TRUE;
        
                
    case GAIA_COMMAND_SET_TONE_CONFIGURATION:
        if (command->size_payload == 2)
            gaia_set_tone_config(command->payload[0], command->payload[1]);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_TONE_CONFIGURATION);
        
        return TRUE;

        
    case GAIA_COMMAND_GET_TONE_CONFIGURATION:
        if (command->size_payload == 0)
            gaia_send_all_tone_configs();
        
        else if (command->size_payload == 1)
            gaia_send_tone_config(command->payload[0]);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_TONE_CONFIGURATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_SET_DEFAULT_VOLUME:
        if (command->size_payload == 3)
            gaia_set_default_volumes(command->payload);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_DEFAULT_VOLUME);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_DEFAULT_VOLUME:
        gaia_send_default_volumes();
        return TRUE;
        
            
    case GAIA_COMMAND_FACTORY_DEFAULT_RESET:
        if (command->size_payload == 1)
            gaia_factory_reset_config(command->payload[0]);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_FACTORY_DEFAULT_RESET);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_CONFIGURATION_ID:
        gaia_send_config_id();
        return TRUE;
        
        
    case GAIA_COMMAND_SET_VOICE_PROMPT_CONFIGURATION:
        if (command->size_payload == 4)
            gaia_set_tts_config(command->payload);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_VOICE_PROMPT_CONFIGURATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_VOICE_PROMPT_CONFIGURATION:
        if (command->size_payload == 1)
            gaia_send_tts_config(command->payload[0]);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_VOICE_PROMPT_CONFIGURATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_SET_FEATURE_CONFIGURATION:
        if (command->size_payload == 2)
            gaia_set_feature_config(command->payload[0], command->payload[1]);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_FEATURE_CONFIGURATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_FEATURE_CONFIGURATION:
        if (command->size_payload == 1)
            gaia_send_feature_config(command->payload[0]);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_FEATURE_CONFIGURATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_SET_USER_EVENT_CONFIGURATION:
        if (command->size_payload == 8)
            gaia_set_user_event_config(command->payload);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_USER_EVENT_CONFIGURATION);
        
        return TRUE;

        
    case GAIA_COMMAND_GET_USER_EVENT_CONFIGURATION:
        if (command->size_payload == 1)
            gaia_send_user_event_config(command->payload[0]);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_USER_EVENT_CONFIGURATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_SET_TIMER_CONFIGURATION:
        gaia_set_timer_config(command->size_payload, command->payload);
        return TRUE;
        
            
    case GAIA_COMMAND_GET_TIMER_CONFIGURATION:
        gaia_send_timer_config();
        return TRUE;
        
            
    case GAIA_COMMAND_SET_AUDIO_GAIN_CONFIGURATION:
        if (command->size_payload != 5 * VOL_NUM_VOL_SETTINGS)
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_LED_CONFIGURATION);

        else
            gaia_set_audio_gain_config(command->size_payload, command->payload);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_AUDIO_GAIN_CONFIGURATION:
        gaia_send_audio_gain_config();
        return TRUE;
        
        
    case GAIA_COMMAND_SET_POWER_CONFIGURATION:
        if (command->size_payload == GAIA_CONFIGURATION_LENGTH_POWER)
            gaia_set_power_config(command->payload);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_POWER_CONFIGURATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_POWER_CONFIGURATION:
        gaia_send_power_config();
        return TRUE;
        
        
    case GAIA_COMMAND_SET_USER_TONE_CONFIGURATION:
        gaia_set_user_tone_config(command->size_payload, command->payload);
        return TRUE;
        
        
    case GAIA_COMMAND_GET_USER_TONE_CONFIGURATION:
        if (command->size_payload == 1)
            gaia_send_user_tone_config(command->payload[0]);

        else
            gaia_send_invalid_parameter(GAIA_COMMAND_GET_USER_TONE_CONFIGURATION);
        
        return TRUE;

#ifdef ENABLE_SQIFVP
    case GAIA_COMMAND_GET_MOUNTED_PARTITIONS:
        {
            uint8 response[1];   
            response[0] = theSink.rundata->partitions_mounted;
            gaia_send_success_payload(GAIA_COMMAND_GET_MOUNTED_PARTITIONS, 1, response);
            
            /*reread available partitions*/
            configManagerSqifPartitionsInit();
            return TRUE;          
        }
#endif  
        
    default:
        return FALSE;
    }
}
    

/*************************************************************************
NAME
    gaia_handle_control_command
    
DESCRIPTION
    Handle a Gaia control command or return FALSE if we can't
*/
static bool gaia_handle_control_command(Task task, GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint8 response[1];
    
    switch (command->command_id)
    {
    case GAIA_COMMAND_CHANGE_VOLUME:
        if (command->size_payload == 1)
            gaia_change_volume(command->payload[0]);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_CHANGE_VOLUME);
        
        return TRUE;

        
    case GAIA_COMMAND_ALERT_LEDS:
        if (command->size_payload == 13)
            gaia_alert_leds(command->payload);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_ALERT_LEDS);
        
        return TRUE;
        
        
    case GAIA_COMMAND_ALERT_TONE:
        gaia_alert_tone(command->size_payload, command->payload);
        return TRUE;
        
        
    case GAIA_COMMAND_ALERT_EVENT:
        if (command->size_payload == 1)
            gaia_alert_event(command->payload[0]);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_ALERT_EVENT);
        
        return TRUE;
        
        
    case GAIA_COMMAND_ALERT_VOICE:
#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION
        if (theSink.no_tts_languages == 0)
            return FALSE;
#endif                
        if (command->size_payload >= 2)
            gaia_alert_voice(command->size_payload, command->payload);
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_ALERT_VOICE);
        
        return TRUE;
        
        
    case GAIA_COMMAND_POWER_OFF:
        send_app_message(EventPowerOff);
        gaia_send_success(GAIA_COMMAND_POWER_OFF);
        return TRUE;

    
    case GAIA_COMMAND_SET_VOLUME_ORIENTATION:
        if ((command->size_payload == 1) && (command->payload[0] == 0))
        {
            if (theSink.gVolButtonsInverted)
            {
                theSink.gVolButtonsInverted = FALSE;
                configManagerWriteSessionData();
            }
            
            gaia_send_success(GAIA_COMMAND_SET_VOLUME_ORIENTATION);
        }
        
        else if ((command->size_payload == 1) && (command->payload[0] == 1))
        {
            if (!theSink.gVolButtonsInverted)
            {
                theSink.gVolButtonsInverted = TRUE;
                configManagerWriteSessionData();
            }
            
            gaia_send_success(GAIA_COMMAND_SET_VOLUME_ORIENTATION);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_VOLUME_ORIENTATION);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_VOLUME_ORIENTATION:
        response[0] = theSink.gVolButtonsInverted;
        gaia_send_success_payload(GAIA_COMMAND_GET_VOLUME_ORIENTATION, 1, response);
        return TRUE;
    
    
    case GAIA_COMMAND_SET_LED_CONTROL:
        if ((command->size_payload == 1) && (command->payload[0] == 0))
        {
            GAIA_DEBUG(("G: GAIA_COMMAND_SET_LED_CONTROL: 0\n")); 
            LedManagerDisableLEDS();
        /*  LedsIndicateNoState();  */
        /*  configManagerWriteSessionData();  */
            LedConfigure(LED_0, LED_ENABLE, FALSE);
            LedConfigure(LED_1, LED_ENABLE, FALSE);
            gaia_send_success(GAIA_COMMAND_SET_LED_CONTROL);
        }
        
        else if ((command->size_payload == 1) && (command->payload[0] == 1))
        {
            GAIA_DEBUG(("G: GAIA_COMMAND_SET_LED_CONTROL: 1\n")); 
            LedManagerEnableLEDS();
        /*  configManagerWriteSessionData();  */
            gaia_send_success(GAIA_COMMAND_SET_LED_CONTROL);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_LED_CONTROL);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_LED_CONTROL:
        GAIA_DEBUG(("G: GAIA_COMMAND_GET_LED_CONTROL\n")); 
        response[0] = theSink.theLEDTask->gLEDSEnabled;
        gaia_send_success_payload(GAIA_COMMAND_GET_LED_CONTROL, 1, response);
        return TRUE;
        
    case GAIA_COMMAND_PLAY_TONE:
        if (command->size_payload == 1) 
        {
            TonesPlayTone(command->payload[0], TRUE, FALSE);
            gaia_send_success(GAIA_COMMAND_PLAY_TONE);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_PLAY_TONE);

        return TRUE;
        
        
    case GAIA_COMMAND_SET_VOICE_PROMPT_CONTROL:
        if ((command->size_payload == 1) && ((command->payload[0] == 0) || (command->payload[0] == 1)))
        {
            GAIA_DEBUG(("G: GAIA_COMMAND_SET_VOICE_PROMPT_CONTROL: %d\n", command->payload[0])); 
            if (theSink.tts_enabled != command->payload[0])
            {
                theSink.tts_enabled = command->payload[0];
                configManagerWriteSessionData();
            }
            
            gaia_send_success(GAIA_COMMAND_SET_VOICE_PROMPT_CONTROL);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_VOICE_PROMPT_CONTROL);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_VOICE_PROMPT_CONTROL:
        GAIA_DEBUG(("G: GAIA_COMMAND_GET_VOICE_PROMPT_CONTROL\n")); 
        response[0] = theSink.tts_enabled;
        gaia_send_success_payload(GAIA_COMMAND_GET_VOICE_PROMPT_CONTROL, 1, response);
        return TRUE;
        
        
#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION
    case GAIA_COMMAND_CHANGE_TTS_LANGUAGE:
        if (theSink.no_tts_languages == 0)
            return FALSE;
        
        send_app_message(EventSelectTTSLanguageMode);
        gaia_send_success(GAIA_COMMAND_CHANGE_TTS_LANGUAGE);
        return TRUE;
        
        
    case GAIA_COMMAND_SET_TTS_LANGUAGE:
        if (theSink.no_tts_languages == 0)
            return FALSE;
        
        if ((command->size_payload == 1) && (command->payload[0] < theSink.no_tts_languages))
        {
            theSink.tts_language = command->payload[0];
            configManagerWriteSessionData();
            gaia_send_success(GAIA_COMMAND_SET_TTS_LANGUAGE);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_TTS_LANGUAGE);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_TTS_LANGUAGE:
        if (theSink.no_tts_languages == 0)
            return FALSE;
        
        response[0] = theSink.tts_language;
        gaia_send_success_payload(GAIA_COMMAND_GET_TTS_LANGUAGE, 1, response);
        return TRUE;
#endif
        
#ifdef ENABLE_SPEECH_RECOGNITION
    case GAIA_COMMAND_SET_SPEECH_RECOGNITION_CONTROL:
        if ((command->size_payload == 1) && (command->payload[0] < 2))
        {
            send_app_message(command->payload[0] ? EventEnableSSR : EventDisableSSR);
            gaia_send_success(GAIA_COMMAND_SET_SPEECH_RECOGNITION_CONTROL);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_SPEECH_RECOGNITION_CONTROL);
        
        return TRUE;
        
    case GAIA_COMMAND_GET_SPEECH_RECOGNITION_CONTROL:
        response[0] = theSink.ssr_enabled;
        gaia_send_success_payload(GAIA_COMMAND_GET_SPEECH_RECOGNITION_CONTROL, 1, response);
        return TRUE;
        
    case GAIA_COMMAND_START_SPEECH_RECOGNITION:
        speechRecognitionStart();
        gaia_send_success(GAIA_COMMAND_START_SPEECH_RECOGNITION);
        return TRUE;
#endif
        
    case GAIA_COMMAND_SWITCH_EQ_CONTROL:
        send_app_message(EventSwitchAudioMode);
        gaia_send_success(GAIA_COMMAND_SWITCH_EQ_CONTROL);
        return TRUE;
        
        
    case GAIA_COMMAND_SET_EQ_CONTROL:
        if ((command->size_payload == 1) && (command->payload[0] <= A2DP_MUSIC_MAX_EQ_BANK))
        {
            set_abs_eq_bank(command->payload[0]);
            gaia_send_success(GAIA_COMMAND_SET_EQ_CONTROL);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_EQ_CONTROL);
        
        return TRUE;

                        
    case GAIA_COMMAND_GET_EQ_CONTROL:
        response[0] = theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing;
        if (response[0] < A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0)
            gaia_send_insufficient_resources(GAIA_COMMAND_GET_EQ_CONTROL);

        else
        {
            response[0] -= A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0;
            gaia_send_success_payload(GAIA_COMMAND_GET_EQ_CONTROL, 1, response);
        }
        return TRUE;
        
        
    case GAIA_COMMAND_SET_BASS_BOOST_CONTROL:
        if ((command->size_payload == 1) && (command->payload[0] < 2))
        {
            send_app_message(command->payload[0] ? EventBassBoostOn : EventBassBoostOff);
            gaia_send_success(GAIA_COMMAND_SET_BASS_BOOST_CONTROL);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_BASS_BOOST_CONTROL);
        
        return TRUE;
        
        
    case GAIA_COMMAND_GET_BASS_BOOST_CONTROL:
        response[0] = theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_BASS_BOOST_BYPASS ?
                      0 : 1;
        
        gaia_send_success_payload(GAIA_COMMAND_GET_BASS_BOOST_CONTROL, 1, response);
        return TRUE;
        
        
    case GAIA_COMMAND_TOGGLE_BASS_BOOST_CONTROL:
        send_app_message(EventBassBoostEnableDisableToggle);
        gaia_send_success(GAIA_COMMAND_TOGGLE_BASS_BOOST_CONTROL);
        return TRUE;
            
        
    case GAIA_COMMAND_SET_3D_ENHANCEMENT_CONTROL:
        if ((command->size_payload == 1) && (command->payload[0] < 2))
        {
            send_app_message(command->payload[0] ? Event3DEnhancementOn : Event3DEnhancementOff);
            gaia_send_success(GAIA_COMMAND_SET_3D_ENHANCEMENT_CONTROL);
        }
        
        else
            gaia_send_invalid_parameter(GAIA_COMMAND_SET_3D_ENHANCEMENT_CONTROL);
        
        return TRUE;

        
    case GAIA_COMMAND_GET_3D_ENHANCEMENT_CONTROL:
        response[0] = theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_SPATIAL_BYPASS ?
                      0 : 1;
        
        gaia_send_success_payload(GAIA_COMMAND_GET_3D_ENHANCEMENT_CONTROL, 1, response);
        return TRUE;
        
        
    case GAIA_COMMAND_TOGGLE_3D_ENHANCEMENT_CONTROL:
        send_app_message(Event3DEnhancementEnableDisableToggle);
        gaia_send_success(GAIA_COMMAND_TOGGLE_3D_ENHANCEMENT_CONTROL);
        return TRUE;     
        
    default:
        return FALSE;
    }
}


/*************************************************************************
NAME
    gaia_handle_status_command
    
DESCRIPTION
    Handle a Gaia polled status command or return FALSE if we can't
*/
static bool gaia_handle_status_command(Task task, GAIA_UNHANDLED_COMMAND_IND_T *command)
{       
    switch (command->command_id)
    {
    case GAIA_COMMAND_GET_APPLICATION_VERSION:
        gaia_send_application_version();
        return TRUE;
                   
    default:
        return FALSE;
    }
}


/*************************************************************************
NAME
    gaia_handle_feature_command
    
DESCRIPTION
    Handle a Gaia feature command or return FALSE if we can't
*/
static bool gaia_handle_feature_command(Task task, GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    switch (command->command_id)
    {
    case GAIA_COMMAND_SET_FEATURE:
        gaia_set_feature(command->size_payload, command->payload);
        return TRUE;
        
    case GAIA_COMMAND_GET_FEATURE:
        gaia_get_feature(command->size_payload, command->payload);
        return TRUE;
        
    default:
        return FALSE;
    }
}


/*************************************************************************
NAME
    gaia_handle_notification_command
    
DESCRIPTION
    Handle a Gaia notification command or return FALSE if we can't
    Notification acknowledgements are swallowed
*/
static bool gaia_handle_notification_command(Task task, GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if (command->command_id & GAIA_ACK_MASK)
    {
        GAIA_DEBUG(("G: ACK\n")); 
        return TRUE;
    }
    
    switch (command->command_id)
    {
    case GAIA_COMMAND_REGISTER_NOTIFICATION:
        GAIA_DEBUG(("G: GAIA_COMMAND_REGISTER_NOTIFICATION\n"));
        gaia_register_notification(command->size_payload, command->payload);
        return TRUE;
        
    case GAIA_COMMAND_GET_NOTIFICATION:
        GAIA_DEBUG(("G: GAIA_COMMAND_GET_NOTIFICATION\n"));
        gaia_get_notification(command->size_payload, command->payload);
        return TRUE;
        
    case GAIA_COMMAND_CANCEL_NOTIFICATION:
        GAIA_DEBUG(("G: GAIA_COMMAND_CANCEL_NOTIFICATION\n"));
        gaia_cancel_notification(command->size_payload, command->payload);
        return TRUE;
        
    case GAIA_EVENT_NOTIFICATION:
        GAIA_DEBUG(("G: GAIA_COMMAND_EVENT_NOTIFICATION\n"));
        gaia_send_invalid_parameter(GAIA_EVENT_NOTIFICATION);
        return TRUE;
        
    default:
        return FALSE;
    }
}


/*************************************************************************
NAME
    gaia_handle_command
    
DESCRIPTION
    Handle a GAIA_UNHANDLED_COMMAND_IND from the Gaia library
*/
static void gaia_handle_command(Task task, GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    bool handled = FALSE;
    
    GAIA_DEBUG(("G: cmd: %04x:%04x %d\n", 
                command->vendor_id, command->command_id, command->size_payload));
        
    if (command->vendor_id == GAIA_VENDOR_CSR)
    {
        switch (command->command_id & GAIA_COMMAND_TYPE_MASK)
        {
        case GAIA_COMMAND_TYPE_CONFIGURATION:
            GAIA_DEBUG(("G: GAIA_COMMAND_TYPE_CONFIGURATION\n"));
            handled = gaia_handle_configuration_command(task, command);
            break;
            
        case GAIA_COMMAND_TYPE_CONTROL:
            GAIA_DEBUG(("G: GAIA_COMMAND_TYPE_CONTROL\n"));
            handled = gaia_handle_control_command(task, command);
            break;
            
        case GAIA_COMMAND_TYPE_STATUS :
            GAIA_DEBUG(("G: GAIA_COMMAND_TYPE_STATUS\n"));
            handled = gaia_handle_status_command(task, command);
            break;
            
        case GAIA_COMMAND_TYPE_FEATURE:
            GAIA_DEBUG(("G: GAIA_COMMAND_TYPE_FEATURE\n"));
            handled = gaia_handle_feature_command(task, command);
            break;
            
        case GAIA_COMMAND_TYPE_NOTIFICATION:
            GAIA_DEBUG(("G: GAIA_COMMAND_TYPE_NOTIFICATION\n"));
            handled = gaia_handle_notification_command(task, command);
            break;
            
        default:
            GAIA_DEBUG(("G: command type %02x unknown\n", command->command_id >> 8));
            break;
        }
    }
        
    if (!handled)
        gaia_send_response(command->vendor_id, command->command_id, GAIA_STATUS_NOT_SUPPORTED, 0, NULL);
}


/*************************************************************************
NAME
    gaia_handle_disconnect_ind
    
DESCRIPTION
    Handle a disconnection indication from the Gaia library
    Cancel all notifications and let the library know we're done
*/
static void gaia_handle_disconnect_ind(GAIA_DISCONNECT_IND_T *ind)
{
    gaia_data.pio_change_mask = 0UL;
    gaia_data.notify_battery_charged = FALSE;
    gaia_data.notify_charger_connection = FALSE;
    gaia_data.notify_ui_event = FALSE;
    gaia_data.notify_speech_rec = FALSE;
            
    GaiaDisconnectResponse(ind->transport);
}


/*************************************************************************
NAME
    handleGaiaMessage
    
DESCRIPTION
    Handle messages passed up from the Gaia library
*/
void handleGaiaMessage(Task task, MessageId id, Message message) 
{
    switch (id)
    {
    case GAIA_INIT_CFM:
        {
            GAIA_INIT_CFM_T *m = (GAIA_INIT_CFM_T *) message;    
                
            GAIA_DEBUG(("G: GAIA_INIT_CFM: %d\n", m->success));

#ifdef CVC_PRODTEST
            if (BootGetMode() != BOOTMODE_CVC_PRODTEST)
            {
                GAIA_DEBUG(("G: GaiaStartService\n"));
                if (m->success)
                {
                    GaiaStartService(gaia_transport_spp);
                }
            }
            else
            {
                if (m->success)
                {
                    GAIA_DEBUG(("G: CVC_PRODTEST_PASS\n"));
                    exit(CVC_PRODTEST_PASS);
                }
                else
                {
                    GAIA_DEBUG(("G: CVC_PRODTEST_FAIL\n"));
                    exit(CVC_PRODTEST_FAIL);
                }
            }
#else
            if (m->success)
            {
                GaiaSetApiMinorVersion(GAIA_API_MINOR_VERSION);
                GaiaStartService(gaia_transport_spp);
            }
#endif /*CVC_PRODTEST */
        }
        break;
        
    case GAIA_CONNECT_IND:
        {
            GAIA_CONNECT_IND_T *m = (GAIA_CONNECT_IND_T *) message;
            GAIA_DEBUG(("G: GAIA_CONNECT_IND: %04x %d\n", (uint16) m->transport, m->success));
            
            if (m->success)
                gaia_data.gaia_transport = m->transport;
        }
        break;
        
    case GAIA_DISCONNECT_IND:
        {
            GAIA_DISCONNECT_IND_T *m = (GAIA_DISCONNECT_IND_T *) message;
            GAIA_DEBUG(("G: GAIA_DISCONNECT_IND: %04x\n", (uint16) m->transport));
            gaia_handle_disconnect_ind(m);
        }
        break;
        
    case GAIA_START_SERVICE_CFM:
        GAIA_DEBUG(("G: GAIA_START_SERVICE_CFM: %d\n", ((GAIA_START_SERVICE_CFM_T *)message)->success));
        break;
        
    case GAIA_UNHANDLED_COMMAND_IND:
        GAIA_DEBUG(("G: GAIA_UNHANDLED_COMMAND_IND\n"));
        gaia_handle_command(task, (GAIA_UNHANDLED_COMMAND_IND_T *) message);
        break;

    case GAIA_SEND_PACKET_CFM:
        {
            GAIA_SEND_PACKET_CFM_T *m = (GAIA_SEND_PACKET_CFM_T *) message;
            GAIA_DEBUG(("G: GAIA_SEND_PACKET_CFM: s=%d\n", VmGetAvailableAllocations()));
            
            if (m->packet)
                freePanic(m->packet);
        }
        break;
        
    default:
        GAIA_DEBUG(("G: unhandled message %04x\n", id));
        break;
    }
}


/*************************************************************************
NAME
    gaiaReportPioChange
    
DESCRIPTION
    Relay any registered PIO Change events to the Gaia client
*/
void gaiaReportPioChange(uint32 pio_state)
{
    uint8 payload[8];

    if ((pio_state ^ gaia_data.pio_old_state) & gaia_data.pio_change_mask) 
    {  
        dwunpack(payload + 4, VmGetClock());
        dwunpack(payload, pio_state);        
        GAIA_DEBUG(("G: GAIA_EVENT_PIO_CHANGED: %08lx\n", pio_state));
        gaia_send_notification(GAIA_EVENT_PIO_CHANGED, sizeof payload, payload);
    }
        
    gaia_data.pio_old_state = pio_state;
}


/*************************************************************************
NAME
    gaiaReportEvent
    
DESCRIPTION
    Relay any significant application events to the Gaia client
*/
void gaiaReportEvent(uint16 id)
{
    uint8 payload[1];
    
    switch (id)
    {
    case EventChargeComplete:
        if (gaia_data.notify_battery_charged)
            gaia_send_notification(GAIA_EVENT_BATTERY_CHARGED, 0, NULL);
        
        break;

    case EventChargerConnected:
        if (gaia_data.notify_charger_connection)
        {
            payload[0] = 1;
            gaia_send_notification(GAIA_EVENT_CHARGER_CONNECTION, 1, payload);
        }
        break;

    case EventChargerDisconnected:
        if (gaia_data.notify_charger_connection)
        {
            payload[0] = 0;
            gaia_send_notification(GAIA_EVENT_CHARGER_CONNECTION, 1, payload);
        }
        break;
    }
}


/*************************************************************************
NAME
    gaiaReportUserEvent
    
DESCRIPTION
    Relay any user-generated events to the Gaia client
*/
void gaiaReportUserEvent(uint16 id)
{
    if (gaia_data.notify_ui_event)
    {
        uint8 payload[2];
        
        switch (id)
        {
    /*  Avoid redundant and irrelevant messages  */
        case EventLongTimer:
        case EventVLongTimer:
        case EventEnableDisableLeds:
            break;
            
        default:
            payload[0] = id >> 8;
            payload[1] = id & 0xFF;
            gaia_send_notification(GAIA_EVENT_USER_ACTION, 2, payload);
            break;
        }
    }
}


#ifdef ENABLE_SPEECH_RECOGNITION
/*************************************************************************
NAME
    gaiaReportSpeechRecResult
    
DESCRIPTION
    Relay a speech recognition result to the Gaia client
*/
void gaiaReportSpeechRecResult(uint16 id)
{
    if (gaia_data.notify_speech_rec)
    {
        uint8 payload[1];
        bool ok = TRUE;
        
        switch (id)
        {
        case CSR_SR_WORD_RESP_YES:
            payload[0] = GAIA_ASR_RESPONSE_YES;
            break;
    
        case CSR_SR_WORD_RESP_NO:  
            payload[0] = GAIA_ASR_RESPONSE_NO;
            break;
            
        case CSR_SR_WORD_RESP_FAILED_YES:
        case CSR_SR_WORD_RESP_FAILED_NO:
        case CSR_SR_WORD_RESP_UNKNOWN:
        case CSR_SR_APP_TIMEOUT:
        /*  Known unknowns  */
            payload[0] = GAIA_ASR_RESPONSE_UNRECOGNISED;
            break;
    
        default:
            GAIA_DEBUG(("gaia: ASR: %04x?\n", id));
            ok = FALSE;
            break;
        }
        
        if (ok)
            gaia_send_notification(GAIA_EVENT_SPEECH_RECOGNITION, 1, payload);
    }
}
#endif  /*  ifdef ENABLE_SPEECH_RECOGNITION  */

#else
static const int dummy;  /* ISO C forbids an empty source file */

#endif  /*  ifdef ENABLE_GAIA  */
