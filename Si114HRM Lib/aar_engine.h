/**
* @file   aar_engine.h
* @Author Invensense (www.invensense.com)
* @date   September, 2013
* @brief  aar_engine Library
*
* aar_engine is a activity detection engine that is powered
* by an Accel and Gyro combination. Gyro, Accel, and quaternion
* values are fed into the aar_engine and a callback will go off when
* a new activity is detected.
*
* $License:
*   Copyright (C) 2013 InvenSense Corporation, All Rights Reserved.
*
*
* \invn_license_start
*
* \page License
*
*
* InvenSense, Inc., Software License Agreement
* - PLEASE READ THIS SOFTWARE LICENSE AGREEMENT ("LICENSE") CAREFULLY BEFORE USING THE INVENSENSE SOFTWARE. BY USING
*      THE INVENSENSE SOFTWARE, YOU ARE AGREEING TO BE BOUND BY THE TERMS OF THIS LICENSE. IF YOU DO NOT AGREE TO THE
*      TERMS OF THIS LICENSE, DO NOT USE THE SOFTWARE. IF YOU DO NOT AGREE TO THE TERMS OF THE LICENSE, YOU MAY RETURN
*      THE INVENSENSE SOFTWARE TO THE PLACE WHERE YOU OBTAINED IT FOR A REFUND. IF THE INVENSENSE SOFTWARE WAS ACCESSED
*      ELECTRONICALLY, CLICK "DISAGREE/DECLINE". FOR INVENSENSE SOFTWARE INCLUDED WITH YOUR PURCHASE OF HARDWARE, YOU
*      MUST RETURN THE ENTIRE HARDWARE/SOFTWARE PACKAGE IN ORDER TO OBTAIN A REFUND.
* - 1. General. The software and documentation and any fonts accompanying this License whether on disk, in read only
*      memory, on any other media or in any other form (collectively the "InvenSense Software") are licensed, not sold,
*      to you by InvenSense, Inc. ("InvenSense") for use only under the terms of this License, and InvenSense reserves
*      all rights not expressly granted to you. The rights granted herein are limited to InvenSense's and its licensors'
*      intellectual property rights in the InvenSense Software and do not include any other patents or intellectual
*      property rights. You own the media on which the InvenSense Software is recorded but InvenSense and/or
*      InvenSense's licensor(s) retain ownership of the InvenSense Software itself. The terms of this License will
*      govern any software upgrades provided by InvenSense that replace and/or supplement the original InvenSense
*      Software product, unless such upgrade is accompanied by a separate license in which case the terms of that
*      license will govern. Title and intellectual property rights in and to any content displayed by or accessed
*      through the InvenSense Software belongs to the respective content owner. Such content may be protected by
*      copyright or other intellectual property laws and treaties, and may be subject to terms of use of the third party
*      providing such content. This License does not grant you any rights to use such content.
* - 2. Permitted License Uses and Restrictions. This License allows you to install and use one copy of the InvenSense
*      Software on a single computer at a time. The InvenSense Software may be used to reproduce materials so long as
*      such use is limited to reproduction of non-copyrighted materials, materials in which you own the copyright, or
*      materials you are authorized or legally permitted to reproduce. This License does not allow the InvenSense
*      Software to exist on more than one computer at a time. You may copy the InvenSense Software in machine-readable
*      form for backup purposes only; provided that the backup copy must include all copyright or other proprietary
*      notices contained on the original. Except as and only to the extent expressly permitted in this License or by
*      applicable law, you may not copy, decompile, reverse engineer, disassemble, modify, or create derivative works of
*      the InvenSense Software or any part thereof.
* - THE INVENSENSE SOFTWARE IS NOT INTENDED FOR USE IN THE OPERATION OF NUCLEAR FACILITIES, AIRCRAFT NAVIGATION OR
*      COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL SYSTEMS, LIFE SUPPORT MACHINES OR OTHER EQUIPMENT IN WHICH THE FAILURE
*      OF THE INVENSENSE SOFTWARE COULD LEAD TO DEATH, PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE.
* - 3. Transfer. You may not rent, lease, lend or sublicense the InvenSense Software. You may, however, make a one-time
*      permanent transfer of all of your license rights to the InvenSense Software to another party, provided that: (a)
*      the transfer must include all of the InvenSense Software, including all its component parts, original media,
*      printed materials and this License; (b) you do not retain any copies of the InvenSense Software, full or partial,
*      including copies stored on a computer or other storage device; and (c) the party receiving the InvenSense 
*      Software reads and agrees to accept the terms and conditions of this License.
* - All components of the InvenSense Software are provided as part of a bundle and may not be separated from the bundle
*      and distributed as standalone applications. NFR (Not for Resale) Copies: Notwithstanding other sections of this
*      License, InvenSense Software labeled or otherwise provided to you on a promotional basis may only be used for
*      demonstration, testing and evaluation purposes and may not be resold or transferred.
* - 4. Termination. This License is effective until terminated. Your rights under this License will terminate
*      automatically without notice from InvenSense if you fail to comply with any term(s) of this License. Upon the
*      termination of this License, you shall cease all use of the InvenSense Software and destroy all copies, full or
*      partial, of the InvenSense Software.
* - 5. Limited Warranty on Media. InvenSense warrants the media on which the InvenSense Software is recorded and
*      delivered by InvenSense to be free from defects in materials and workmanship under normal use for a period of
*      ninety (90) days from the date of original retail purchase. Your exclusive remedy under this Section shall be, at
*      InvenSense's option, a refund of the purchase price of the product containing the InvenSense Software or
*      replacement of the InvenSense Software that is returned to InvenSense or an InvenSense authorized representative
*      with a copy of the receipt. THIS LIMITED WARRANTY AND ANY IMPLIED WARRANTIES ON THE MEDIA INCLUDING, BUT NOT
*      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, OF SATISFACTORY QUALITY, AND OF FITNESS FOR A PARTICULAR
*      PURPOSE, ARE LIMITED IN DURATION TO NINETY (90) DAYS FROM THE DATE OF ORIGINAL RETAIL PURCHASE. SOME
*      JURISDICTIONS DO NOT ALLOW LIMITATIONS ON HOW LONG AN IMPLIED WARRANTY LASTS, SO THE ABOVE LIMITATION MAY NOT
*      APPLY TO YOU. THE LIMITED WARRANTY SET FORTH HEREIN IS THE ONLY WARRANTY MADE TO YOU AND IS PROVIDED IN LIEU OF
*      ANY OTHER WARRANTIES (IF ANY) CREATED BY ANY DOCUMENTATION OR PACKAGING. THIS LIMITED WARRANTY GIVES YOU SPECIFIC
*      LEGAL RIGHTS, AND YOU MAY ALSO HAVE OTHER RIGHTS, WHICH VARY BY JURISDICTION.
* - 6. Disclaimer of Warranties. YOU EXPRESSLY ACKNOWLEDGE AND AGREE THAT USE OF THE INVENSENSE SOFTWARE IS AT YOUR SOLE
*      RISK AND THAT THE ENTIRE RISK AS TO SATISFACTORY QUALITY, PERFORMANCE, ACCURACY AND EFFORT IS WITH YOU. EXCEPT
*      FOR THE LIMITED WARRANTY ON MEDIA SET FORTH ABOVE AND TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THE
*      INVENSENSE SOFTWARE IS PROVIDED "AS IS", WITH ALL FAULTS AND WITHOUT WARRANTY OF ANY KIND, AND INVENSENSE AND
*      INVENSENSE'S LICENSORS (COLLECTIVELY REFERRED TO AS "INVENSENSE" FOR THE PURPOSES OF SECTIONS 6 AND 7) HEREBY
*      DISCLAIM ALL WARRANTIES AND CONDITIONS WITH RESPECT TO THE INVENSENSE SOFTWARE, EITHER EXPRESS, IMPLIED OR
*      STATUTORY, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES AND/OR CONDITIONS OF MERCHANTABILITY, OF
*      SATISFACTORY QUALITY, OF FITNESS FOR A PARTICULAR PURPOSE, OF ACCURACY, OF QUIET ENJOYMENT, AND NON-INFRINGEMENT
*      OF THIRD PARTY RIGHTS. INVENSENSE DOES NOT WARRANT AGAINST INTERFERENCE WITH YOUR ENJOYMENT OF THE INVENSENSE 
*      SOFTWARE, THAT THE FUNCTIONS CONTAINED IN THE INVENSENSE SOFTWARE WILL MEET YOUR REQUIREMENTS, THAT THE OPERATION
*      OF THE INVENSENSE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT DEFECTS IN THE INVENSENSE SOFTWARE WILL
*      BE CORRECTED. NO ORAL OR WRITTEN INFORMATION OR ADVICE GIVEN BY INVENSENSE OR AN INVENSENSE AUTHORIZED
*      REPRESENTATIVE SHALL CREATE A WARRANTY. SHOULD THE INVENSENSE SOFTWARE PROVE DEFECTIVE, YOU ASSUME THE ENTIRE
*      COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION. SOME JURISDICTIONS DO NOT ALLOW THE EXCLUSION OF IMPLIED
*      WARRANTIES OR LIMITATIONS ON APPLICABLE STATUTORY RIGHTS OF A CONSUMER, SO THE ABOVE EXCLUSION AND LIMITATIONS
*      MAY NOT APPLY TO YOU.
* - 7. Limitation of Liability. TO THE EXTENT NOT PROHIBITED BY LAW, IN NO EVENT SHALL INVENSENSE BE LIABLE FOR PERSONAL
*      INJURY, OR ANY INCIDENTAL, SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES WHATSOEVER, INCLUDING, WITHOUT LIMITATION,
*      DAMAGES FOR LOSS OF PROFITS, LOSS OF DATA, BUSINESS INTERRUPTION OR ANY OTHER COMMERCIAL DAMAGES OR LOSSES,
*      ARISING OUT OF OR RELATED TO YOUR USE OR INABILITY TO USE THE INVENSENSE SOFTWARE, HOWEVER CAUSED, REGARDLESS OF
*      THE THEORY OF LIABILITY (CONTRACT, TORT OR OTHERWISE) AND EVEN IF INVENSENSE HAS BEEN ADVISED OF THE POSSIBILITY
*      OF SUCH DAMAGES. SOME JURISDICTIONS DO NOT ALLOW THE LIMITATION OF LIABILITY FOR PERSONAL INJURY, OR OF
*      INCIDENTAL OR CONSEQUENTIAL DAMAGES, SO THIS LIMITATION MAY NOT APPLY TO YOU. In no event shall InvenSense's
*      total liability to you for all damages (other than as may be required by applicable law in cases involving
*      personal injury) exceed the amount of fifty dollars ($50.00). The foregoing limitations will apply even if
*      the above stated remedy fails of its essential purpose.
* - 8. Export Law Assurances. You may not use or otherwise export or reexport the InvenSense Software except as
*      authorized by United States law and the laws of the jurisdiction in which the InvenSense Software was obtained.
*      In particular, but without limitation, the InvenSense Software may not be exported or re-exported (a) into
*      (or to a national or resident of) any U.S. embargoed countries (currently Cuba, Iran, Iraq, Libya, North Korea,
*      Sudan and Syria), or (b) to anyone on the U.S. Treasury Department's list of Specially Designated Nationals or
*      the U.S. Department of Commerce Denied Person's List or Entity List. By using the InvenSense Software, you
*      represent and warrant that you are not located in, under control of, or a national or resident of any such
*      country or on any such list.
* - 9. Government End Users. The InvenSense Software and related documentation are "Commercial Items", as that term is
*      defined at 48 C.F.R. §2.101, consisting of "Commercial Computer Software" and "Commercial Computer Software
*      Documentation", as such terms are used in 48 C.F.R. §12.212 or 48 C.F.R. §227.7202, as applicable. Consistent
*      with 48 C.F.R. §12.212 or 48 C.F.R. §227.7202-1 through 227.7202-4, as applicable, the Commercial Computer
*      Software and Commercial Computer Software Documentation are being licensed to U.S. Government end users (a)
*      only as Commercial Items and (b) with only those rights as are granted to all other end users pursuant to the
*      terms and conditions herein. Unpublished-rights reserved under the copyright laws of the United States.
* - 10. Controlling Law and Severability. This License will be governed by and construed in accordance with the
*      laws of the State of California, as applied to agreements entered into and to be performed entirely within
*      California between California residents. This License shall not be governed by the United Nations Convention
*      on Contracts for the International Sale of Goods, the application of which is expressly excluded. If for any
*      reason a court of competent jurisdiction finds any provision, or portion thereof, to be unenforceable, the
*      remainder of this License shall continue in full force and effect.
* - 11. Complete Agreement; Governing Language. This License constitutes the entire agreement between the parties
*      with respect to the use of the InvenSense Software licensed hereunder and supersedes all prior or contemporaneous
*      understandings regarding such subject matter. No amendment to or modification of this License will be binding
*      unless in writing and signed by InvenSense. Any translation of this License is done for local requirements and
*      in the event of a dispute between the English and any non-English versions, the English version of this License
*      shall govern.
*
* \invn_license_stop
*/
/** @file */



/** \defgroup AARDefines AAR Definitions
* \brief AAR definitions.
* AAR definitions provide named mask values for the AAR detect activity.
*  @{
*/

/** Reserved/Undefined activity. */
#define NULL_NUM (0) 
/** Standing activity. */
#define STAND_NUM (1) 
/** Running activity. */
#define RUN_NUM (2) 
/** Walking activity. */
#define WALK_NUM (3) 
/** Driving activity. */
#define DRIVE_NUM (4) 
/** Biking activity. */
#define BIKE_NUM (5) 
/** Sitting activity. */
#define SIT_NUM (6) 
/** Swimming activity. */
#define SWIM_NUM (7) 
/** Sleeping activity. */
#define SLEEP_NUM (128) 
/** Transition. */
#define TRANSITION_NUM (9) 
/** Total activities count. */
#define ACTIVITY_COUNT TRANSITION_NUM 

/** @}*/

#define BOOL int32_t
#define BTRUE  1
#define BFALSE 0
#define SUCCESS 0
#define FAILURE 1

/*#define AAR_USB_DEBUGGING*/
#undef AAR_USB_DEBUGGING

typedef int32_t (*_MLPrintLogCB_t) (int32_t priority, const int8_t*  tag, const int8_t*  fmt, ...);
#ifdef AAR_USB_DEBUGGING
extern _MLPrintLogCB_t _MLPrintLogCB;

#define AAR_LOG(fmt, ...) _MLPrintLogCB(1, "AAR", fmt, ##__VA_ARGS__)

#else
/*#define AAR_LOG(fmt, ...) do {} while (0)*/
#endif




/** @}*/

/** \defgroup aarFunctions AAR Functions
* \brief AAR interface functions.
* The following sections detail the AAR interface functions utilized
* for initialization of AAR, providing aiding data, and for reporting
* of AAR results and confidence level.
*  @{
*/


/** Defines the invalid motion state for AAR library.*/
#define AAR_INVALID_MOTION_STATE		(0)
/** Define the no motion state for AAR library.*/
#define AAR_NO_MOTION_STATE				(1)
/** Defines the motion state for AAR library.*/
#define AAR_MOTION_STATE				(2)


#define AAR_PRESS_DATA_RATE_LOW			(0)
#define AAR_PRESS_DATA_RATE_HIGH			(1)


/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to trigger when motion or no motion state is detected.
*
* @param[in] state Motion-No-Motion State of the sensor
*
*/
typedef void (*AAR_motion_state_cb) (uint8_t state);


/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to trigger when ever a activity is detected.
*
*  @param[in] activity   A variable with the activity detected from the
*  InvenSense library definition.
*  @param[in] confidence   A variable with the activity's confidence level 0-100.
*
*/
typedef void (*AAR_activity_cb) (int activity,
                                 int confidence);

/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to trigger when ever a tap is detected.
*
*  @param[in] none
*
*/
typedef void (*AAR_tap_cb) (void);


typedef char (*enter_hp_cb)(void);

/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to trigger the device into low power mode.
*
*  @param[in] none
*
*/
typedef char (*enter_lp_cb)(void);

/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to trigger an i2c read.
*
*  @param[in] slave_addr I2C device address
*  @param[in] reg_addr I2C read address
*  @param[in] length I2C read length
*  @param[out] data I2C read data
*
*/
typedef int (*AAR_i2c_read)(uint8_t slave_addr, uint8_t reg_addr,
                            uint8_t length, uint8_t *data);

typedef void (*AAR_press_data_rate_cb) (uint8_t rate);

#ifdef AAR_USB_DEBUGGING
/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to log AAR engine messages and for debugging.
* AAR_USB_DEBUGGING should be defined in order to log messages.
*
* @param[in] _MLPrintLogCB_t   A call back function pointer within the
*					debug logger 
* @return     inv_status: 0 success or -1 failure
*/
void AAR_set_log_cb( _MLPrintLogCB_t cb );
#endif

/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to set the callback for high power mode.
*
*  @param[in] enter_hp_cb   A call back function pointer within the
*					InvenSense Library for high power mode.
* @return     inv_status: 0 success or -1 failure
*
*/
void AAR_set_hp_cb( enter_hp_cb on_cb );
/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to set the callback for low power mode.
*
*  @param[in] enter_lp_cb   A call back function pointer within the
*					InvenSense Library for low power mode.
* @return     inv_status: 0 success or -1 failure
*
*/
void AAR_set_lp_cb(enter_lp_cb off_cb);

void AAR_set_press_data_rate_cb(AAR_press_data_rate_cb cb);

/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to set the callback for activity detection.
* The callback will go off whenever an activity is detected.
*
* @param[in]  det_cb A callback function pointer within the InvenSense
*					Library that will go off when an activity is detected
* @return     inv_status: 0 success or -1 failure
*
*/
void AAR_set_act_det_cb( AAR_activity_cb det_cb );



/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to set the callback for motion state.
* The callback will go off to indicate motion or no_motion state.
*
* @param[in]  det_cb A callback function pointer within the InvenSense Library that 
*                  will go off when an activity is detected
* @param[out] state   A variable with motion state, .
* @return     inv_status: 0 success or -1 failure
*
*/
void AAR_set_motion_state_cb( AAR_motion_state_cb state_cb );

/** Defines the InvenSense AAR Library function.
* Reset the motion state. Call this function when the chip has awoken from a
* motion. 
*
*
* @return     inv_status: 0 success or -1 failure
*
*/
void AAR_reset_motion_state( void);



/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to initialize the AAR library.
*
* @param[in] AAR_i2c_read_cb   A call back function pointer within the
*					InvenSense Library for i2c read
* @param[in] use_pressure_sensor Set to 1 to use pressure sensor otherwise AAR will assume there is no pressure sensor..
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_init( AAR_i2c_read read, int8_t use_press_sensor);

/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to start the AAR library. This must be called or else the AAREngine
* will not process any data. 
*
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_start(void);

/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to stop the AAR library. This will stop the library from processing
*data. 
*
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_stop(void);

/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to reset the AAR library. This will reset the state of the library 
*
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_reset(void);


/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to process sensor data to determine the activity
*
* @param[in]  gyro A pointer variable for gyro data in hardware units
* @param[in]  accel A pointer variable for accel data in hardware units
* @param[in]  quat A pointer variable for quaternion data in hardware units
* @param[in]  press Pressure sensor value in Pa
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_act_det_process( int16_t*  gyro,
                            int16_t*  accel,
                            int32_t*  quat,
                            int32_t   press);

/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to process accel data to calculate accel only quaternions
*
* @param[in]  accel A pointer variable for input accel data in hardware units
* @param[in]  accel A pointer variable to output quaternion data in q30 format.
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_calculate_accel_only_quat( int16_t*  accel, int32_t*  quat );

/** Defines the InvenSense AAR Library function.
* Defines the InvenSense AAR function prototype used
* to set the callback for activity detection in the arbitration layer.
* The callback will go off when ever an activity is detected.
*
* @param[in]  arb_cb A callback function pointer within the InvenSense
*					Library that will go off when an activity is detected.
* @return     inv_status: 0 success or -1 failure
*
*/
void AAR_set_arb_cb( AAR_activity_cb arb_cb );

/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to trigger when ever a tap is detected.
*
* @param[in]  tap_cb A callback function pointer within the InvenSense
*				Library that will setup the AAR for detecting a tap.
* @return     inv_status: 0 success or -1 failure
*
*/
void AAR_set_tap_cb( AAR_tap_cb tap_cb );


/** Defines the InvenSense AAR Library callback.
* Defines the InvenSense callback function prototype used
* to read all the activities analytics.
*
* @param[in]  activity_Type  takes vales WALK_NUM, RUN_NUM, WALKNUM+RUN_NUM, SLEEP_NUM
* @param[out] analytics Return analytics associated with the corresponding activities
*                                 ex. walk steps(WALK_NUM), run steps(RUN_NUM), total
*                                 steps(WALKNUM+RUN_NUM), Motion count(SLEEP)
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_get_analytics( uint8_t activity_Type, int16_t* analytics );

/** Defines the InvenSense AAR Library Version.
* Defines the InvenSense version function prototype used
* to reset all the step counts.
*
* @param[in]  none
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_reset_analytics( void );

/** Defines the InvenSense AAR Library Version.
* Defines the InvenSense version function prototype used
* to reset all the step counts.
*
* * @param[in]  activity_Type  WALK_NUM, RUN_NUM
* * @param[in]  analytics  set analytics value to the given one.
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_set_analytics( uint8_t activity_Type, int16_t analytics );

/** Defines the InvenSense AAR Library Version.
* Defines the InvenSense version function prototype used
* to set the activity mode to sleep.
*
* @param[in]  none
* @return     inv_status 0 if sucessful; -1 failure *
*
*/
int32_t AAR_enable_sleep_activity( void );

/** Defines the InvenSense AAR Library Version.
* Defines the InvenSense version function prototype used
* to retrieve library version data.
*
* @param[out] aar_verstion  pointer to a string with revision number of the
*                            InvenSense AAR Library
* @return     inv_status: 0 success or -1 failure
*
*/
int32_t AAR_get_version( uint8_t*  aar_verstion );
/** @}*/

int32_t AAR_get_climbed_floors(int16_t *floors);

int32_t AAR_reset_climbed_floors(void);

/** Defines the mode the present device is in, normal mode or
* the activity sleep mode. 
* @return devie_mode== SLEEP_NUM or STAND_NUM
*/
uint8_t aar_get_device_mode(void);

int32_t AAR_update_pressure(int32_t pressure);

