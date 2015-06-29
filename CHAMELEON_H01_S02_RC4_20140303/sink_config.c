/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2013

FILE NAME
    sink_config.c
    
DESCRIPTION
    
*/

#include "sink_config.h"

#include <ps.h>
#include <string.h>
#include <panic.h>
#include <stdlib.h>

/****************************************************************************/
/* Reference the default configuartion definition files */

 	extern const config_type csr_stereo_default_config;
 
 	extern const config_type csr_mono_default_config;

    extern const config_type csr_car_default_config;


/****************************************************************************/
/* Identify each default configuration, this defines an offset into the main config table */
typedef enum
{
	/* CSR Sink Stereo */
 	csr_stereo = (0),
	/* CSR Sink Mono */ 
 	csr_mono = (1),
    /* CSR Car Kit*/
    csr_car = (2),
 	entry_3 = (0),
 	entry_4 = (0),   
 	entry_5 = (0),
    entry_6 = (0),
    entry_7 = (0),
 	entry_8 = (0),
 	entry_9 = (0),
 	entry_10 = (0),
 	entry_11 = (0),
	last_config_id = (12)
}config_id_type;
 

/****************************************************************************/
/* Table of default configurations */
const config_type* const default_configs[] = 
{
 
	/* CSR Sink Stereo */
	 &csr_stereo_default_config,
    /* CSR Sink Mono */ 
 	&csr_mono_default_config,
    /* CSR Car Kit */ 
 	&csr_car_default_config,
 	0,
  	0,
 	0,
 	0,
 	0,
 	0,
	0,
	0,
	0
};


/****************************************************************************/
const uint8 * const default_service_records[] = 
{
    /* CSR Sink Stereo */
 	0,
    /* CSR Sink Mono */ 
 	0,
    /* CSR Car Kit */  
 	0,
 	0,
 	0,
 	0,
    0,
    0,
	0,	
	0,
	0,
	0,
} ;

/****************************************************************************/
const uint16 * const default_service_record_lengths[] = 
{
    /* CSR Sink Stereo */
 	0,
    /* CSR Sink Mono */             
 	0,
    /* CSR Car Kit*/ 
 	0,
 	0,
 	0,
 	0,
    0,
    0,
	0,	
	0,
	0,
	0,
} ;

/****************************************************************************/
/****************************************************************************/


/****************************************************************************
NAME 
 	set_config_id

DESCRIPTION
 	This function is called to read and store the configuration ID
 
RETURNS
 	void
*/
void set_config_id(uint16 key)
{
 	/* Default to CSR standard configuration */
 	theSink.config_id = 0;
 
 	/* Read the configuration ID.  This identifies the configuration held in
       constant space */
 	if(PsRetrieve(key, &theSink.config_id, sizeof(uint16)))
 	{
  		if(theSink.config_id >= last_config_id)
  		{
   			theSink.config_id = 0;
  		}
 	}

    DEBUG(("CONF:ID [0x%x]\n",theSink.config_id)) ;	
}


/****************************************************************************
NAME 
 	ConfigRetrieve

DESCRIPTION
 	This function is called to read a configuration key.  If the key exists
 	in persistent store it is read from there.  If it does not exist then
 	the default is read from constant space
 
RETURNS
 	0 if no data was read otherwise the length of data
*/
uint16 ConfigRetrieve(uint16 config_id, uint16 key_id, void* data, uint16 len)
{
 	uint16 ret_len;
 
 	/* Read requested key from PS if it exists */
 	ret_len = PsRetrieve(key_id, data, len);
     
 	/* If no key exists then read the parameters from the default configuration
       held in constant space */
 	if(!ret_len)
 	{
  		/* Access the required configuration */
  		if( default_configs[ config_id ] )
  		{  
            key_type * key;
            
            /* as the default configuration structures are aligned in key_id order
               it is safe to set the pointer such that it treats the default config
               config_type structure as an array and index that via the key_id value.

               The following line casts the default_config pointer to the first entry
               in the configuration structure which will be (key_type *)&battery_config, 
               the key_id (0 to 31) is then added to the start of the config to give the
               correct offset with the structure. Key then retrieves the .length and .data
               pointer from the configuration */
            key = *((key_type**)default_configs[config_id] + key_id);
            
            /* Providing the retrieved length is not zero. */
   			if(key->length == 0)
			{
				/* This will indicate an error. */
				ret_len = 0;
			}
			else
			{
	   			if(key->length == len)
	   			{
	    			/* Copy from constant space */
					memmove(data, &key->data, len);
	    			ret_len = len;
	   			}
	   			else
	   			{
					if(key->length > len)
					{
						DEBUG(("CONF:BADLEN! PSKEY=[%x] ActualLen[%x] ExpectedLen[%x]\n",key_id, key->length, len)) ;	
						Panic() ;
					}
					else
					{
		   				/* (key.length < len) && (key.length != 0) here since we're comparing unsigned numbers. */

		   				/* We have more space than the size of the key in constant space.
		   				   Just copy the data for the key.
		   				   The length returned will let the caller know about the length mismatch. */
		    			/* Copy from constant space */
						memmove(data, &key->data, key->length);
		    			ret_len = key->length;
	    			}
	   			}
   			}
  		}
 	}
    else
    {
    	switch(key_id)
    	{
			/* PS keys where it's ok for (ret_len != len) */
            case(PSKEY_CONFIG_TONES):
                break;
    		default:
				if (ret_len != len)
				{
					DEBUG(("CONF:BADLEN![%x][%x][%x]\n",key_id, ret_len, len)) ;	
					Panic() ;
				}		 
    			break;
    	}
    }   
 
 	return ret_len;
}

/****************************************************************************
NAME 
 	get_service_record

DESCRIPTION
 	This function is called to get a special service record associated with a 
 	given configuration
 
RETURNS
 	a pointer to the service record
*/
uint8 * get_service_record ( void ) 
{   
	uint8 * lServiceRecord = (uint8*) default_service_records[theSink.config_id] ;
	
	DEBUG(("CONF: Service_Record[%d][%d]\n",theSink.config_id , (int)lServiceRecord)) ;
    	
	return lServiceRecord ;
}

/****************************************************************************
NAME 
 	get_service_record_length

DESCRIPTION
 	This function is called to get the length of a special service record 
 	associated with a given configuration
 
RETURNS
 	the length of the service record
*/
uint16 get_service_record_length ( void ) 
{
    uint16 lLength = 0 ;

    if (default_service_record_lengths[theSink.config_id])
    {
	   lLength = *default_service_record_lengths[theSink.config_id] ;
    }

	DEBUG(("CONF: Service Record Len = [%d]\n", lLength)) ;
	
	return lLength ;
}
