/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013

FILE NAME
    sink_speech_recognition.c

DESCRIPTION
    Speech Recognition application interface.
        
NOTES

    This enables and disables the csr speech recognition plugin and
    converts the response into sink device user events. 
    
    Speech Recognition can be used for answering / rejecting a call and is 
    enabled when the sink device event EventSpeechRecognitionStart is received at 
    the application event handler. This calls speechRecognitionStart()
    which starts the speech recognition engine and registers a const task for the 
    callback message. 
    
    Once the callback is received, this is converted into an applcication message
    (user event) which actions the sink device application.
    
    No globals used 
    1 bit required in sink_private.h for storing whther the speech rec is active 
    or not - this is in turn used (via speechRecognitionIsActive() ) for the 
    sink_audio.c code to decide whether or not to process any audio stream 
    connections 
    
    

*/

#ifdef ENABLE_SPEECH_RECOGNITION

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_speech_recognition.h"
#include "sink_configmanager.h"
#include "sink_audio.h"
#include "sink_statemanager.h"
#include "sink_audio_routing.h"

#ifdef ENABLE_GAIA
#include "sink_gaia.h"
#endif

#include <csr_speech_recognition_plugin.h>
#include <audio.h>

#ifdef DEBUG_SPEECH_REC
#define SR_DEBUG(x) DEBUG(x)
#else
#define SR_DEBUG(x) 
#endif


static void speech_rec_handler(Task task, MessageId id, Message message);       

/* task for messages to be delivered */
static const TaskData speechRecTask = {speech_rec_handler};   


/****************************************************************************
DESCRIPTION
  	This function is called to enable speech recognition mode
*/
void speechRecognitionStart(void)
{
    AudioPluginFeatures features;

    /* determine additional features applicable for this audio plugin */
    features.stereo = (AUDIO_PLUGIN_FORCE_STEREO || theSink.features.stereo);
    features.use_i2s_output = theSink.features.UseI2SOutputCapability;

    SR_DEBUG(("Speech Rec START - AudioConnect\n")) ;

    theSink.csr_speech_recognition_is_active = TRUE ;

    /*reconnect any audio streams that may need connecting*/
    if(theSink.routed_audio)
    {
        /* get status info in malloc'd memory slot */
        audio_source_status * lAudioStatus = audioGetStatus(theSink.routed_audio);        
        
        /* attempt to disconnect and/or suspend current source */
        if(!audioSuspendDisconnectSource(lAudioStatus))
            SR_DEBUG(("Speech Rec START - Audio Already Connected NOT disconnected\n")) ;

        /* release memory */        
        freePanic(lAudioStatus);
    }

    /*connect the speech recognition plugin - this will be disconnected 
    automatically when a word has been recognised (successfully or unsuccessfully  */
    AudioConnect((TaskData *)&csr_speech_recognition_plugin,
                  0,0,0,0,0,features,0,0,0,
                  &theSink.conf1->PIOIO.digital, 
				  (TaskData*)&speechRecTask);
           
    /*post a timeout message to restart the SR if no recognition occurs*/
    if(!theSink.csr_speech_recognition_tuning_active)
        MessageSendLater((TaskData*)&speechRecTask , CSR_SR_APP_TIMEOUT , 0, theSink.conf1->timeouts.SpeechRecRepeatTime_ms ) ;
}

/****************************************************************************
DESCRIPTION
  	This function is called to reenable speech recognition mode
*/
void speechRecognitionReStart(void)
{
    AudioPluginFeatures features;

    /* determine additional features applicable for this audio plugin */
    features.stereo = (AUDIO_PLUGIN_FORCE_STEREO || theSink.features.stereo);
    features.use_i2s_output = theSink.features.UseI2SOutputCapability;

    SR_DEBUG(("Speech Rec RESTART - AudioConnect\n")) ;

    theSink.csr_speech_recognition_is_active = TRUE ;

    /*connect the speech recognition plugin - this will be disconnected 
    automatically when a word has been recognised (successfully or unsuccessfully  */
    AudioConnect((TaskData *)&csr_speech_recognition_plugin,
                  0,0,0,0,0,features,0,0,0,
                  &theSink.conf1->PIOIO.digital, 
				  (TaskData*)&speechRecTask);
           
}
/****************************************************************************
DESCRIPTION
  	This function is called to disable speech recognition mode
*/
void speechRecognitionStop(void)
{
    SR_DEBUG(("Speech Rec STOP\n")) ;

    /*if the SR plugin is attached / disconnect it*/
    if ( theSink.csr_speech_recognition_is_active )
    {
        SR_DEBUG(("Disconnect SR Plugin\n")) ; 
        AudioDisconnect() ;
    }

    theSink.csr_speech_recognition_is_active = FALSE ;
    
    /*cancel any potential APP timeout message */
    MessageCancelAll( (TaskData*)&speechRecTask , CSR_SR_APP_TIMEOUT ) ;

    /* cancel any queued start messages */    
    MessageCancelAll (&theSink.task , EventSpeechRecognitionStart) ;

    /*reconnect any audio streams that may need connecting*/
    audioHandleRouting(audio_source_none) ;

}

/****************************************************************************
DESCRIPTION
  	This function is called to determine if speech rec is currently running
RETURNS
    True if Speech Rec is active
*/
bool speechRecognitionIsActive(void)
{
    SR_DEBUG(("Speech Rec ACTIVE[%x]\n" , (int)theSink.csr_speech_recognition_is_active  )) ;

    return theSink.csr_speech_recognition_is_active ;
}

/****************************************************************************
DESCRIPTION
  	This function is called to determine if speech rec is enabled
RETURNS
    True if Speech Rec is enabled
*/
bool speechRecognitionIsEnabled(void) 
{
    /* Check if SR is enabled globally and return accordingly*/
    return (theSink.features.speech_rec_enabled && theSink.ssr_enabled);
}


/****************************************************************************
DESCRIPTION
  	This function is the message handler which receives the messages from the 
    SR library and converts them into suitable application messages 
*/
static void speech_rec_handler(Task task, MessageId id, Message message)
{
    SR_DEBUG(("ASR message received \n")) ;
    
    switch (id) 
    {
        case CSR_SR_WORD_RESP_YES:
        {
            SR_DEBUG(("\nSR: YES\n"));

            /* when in tuning mode, restart after a successful match */
            if(theSink.csr_speech_recognition_tuning_active)
            {
                /* recognition suceeded, restart */
                AudioDisconnect() ;
                theSink.routed_audio = NULL ;
                MessageSendLater((TaskData*)&speechRecTask , CSR_SR_APP_RESTART , 0, 100 ) ;
            }
            /* normal operation mode for incoming call answer */
            else if (stateManagerGetState() == deviceIncomingCallEstablish)
            {
                MessageSend (&theSink.task , EventAnswer , 0) ;
            }

            MessageSend (&theSink.task , EventSpeechRecognitionTuningYes , 0) ;
        }
        break;

        case CSR_SR_WORD_RESP_NO:  
        {
            SR_DEBUG(("\nSR: NO\n"));

            /* when in tuning mode, restart after a successful match */
            if(theSink.csr_speech_recognition_tuning_active)
            {
                /* recognition suceeded, restart */
                AudioDisconnect() ;
                theSink.routed_audio = NULL ;
                MessageSendLater((TaskData*)&speechRecTask , CSR_SR_APP_RESTART , 0, 100 ) ;
            }
            /* normal operation mode for incoming call answer */
            else if (stateManagerGetState() == deviceIncomingCallEstablish)
            {
                MessageSend (&theSink.task , EventReject , 0) ;
            }

            MessageSend (&theSink.task , EventSpeechRecognitionTuningNo , 0) ;
        }    
        break;

        case CSR_SR_WORD_RESP_FAILED_YES:
        case CSR_SR_WORD_RESP_FAILED_NO:
        case CSR_SR_WORD_RESP_UNKNOWN:
        {
            SR_DEBUG(("\nSR: Unrecognized word, reason %x\n",id));

            AudioDisconnect() ;
            theSink.routed_audio = NULL ;

            MessageSendLater((TaskData*)&speechRecTask , CSR_SR_APP_RESTART , 0, 100 ) ;
        }   
        break;
        
        case CSR_SR_APP_RESTART:
            SR_DEBUG(("SR: Restart\n"));
            /* recognition failed, try again */
            speechRecognitionReStart();
        break;
        
        case CSR_SR_APP_TIMEOUT:
        {
            SR_DEBUG(("\nSR: TimeOut - Restart\n"));

            speechRecognitionStop();
            
            /* disable the timeout when in tuning mode */
            if ((!theSink.csr_speech_recognition_tuning_active)&&(stateManagerGetState() == deviceIncomingCallEstablish))
            {
                MessageCancelAll (&theSink.task , EventSpeechRecognitionStart) ;
                MessageSend (&theSink.task , EventSpeechRecognitionStart , 0) ; 
            }                
        }
        break ;

        default:
            SR_DEBUG(("SR: Unhandled message 0x%x\n", id));
        /*    panic();*/
        break;
    }
    
#ifdef ENABLE_GAIA
    if (stateManagerGetState() != deviceIncomingCallEstablish)
    {
        gaiaReportSpeechRecResult(id);
    }
#endif
}


#else
static const int dummy ;

#endif


