// 2014 - 2015

/*! \file wireguidance.h
    \brief Contains wire guidance (wg) function declarations
*/

#ifndef __GEN_WIRE_GUIDANCE_H
#define __GEN_WIRE_GUIDANCE_H

#include "configuration.h"
#include "stypes.h"

#if GUIDANCE_WIRE

//******************************************************************************************************
// Defines
//******************************************************************************************************
/* WG_DEVIATION_INVALID is the value to which deviation is set when it is not valid */
#define WG_DEVIATION_INVALID          (0x7FFF)

#if SECOND_HARMONIC_FIRST_FREQUENCY
/* WG_REL_PHASE_INVALID is the value to which the relative phase is set when the 
	first Input Frequency is not present */
#define WG_REL_PHASE_INVALID          (0x007F)
#endif

/* When calibration is started, the calibration parameter to start from is set
   to WG_CALIBRATION_DEFAULT_PARAM */
#define WG_CALIBRATION_DEFAULT_PARAM  (2000)

/* WG_DELAY_CALIBRATION_COUNTER is a counter used to delay the calibration with 
  500msec, once the 'start calibration' is triggered (50*100Hz) */
#define WG_DELAY_CALIBRATION_COUNTER  (50)

/* WG_MIN_CALIBRATION_COUNTER is the counter used to set the minimal duration
   of calibration, 300msec, once the calibration has started (30*100Hz). When
   at WG_MIN_CALIBRATION_COUNTER, the calibration parameter is still too small
   (WG_MIN_CALIBRATION_PARAM), calibration for that frequency is stopped, as it 
   is failing. */
#define WG_MIN_CALIBRATION_COUNTER    (30+WG_DELAY_CALIBRATION_COUNTER)

/* WG_MAX_CALIBRATION_COUNTER is the counter used to stop the calibration when
   calibration criteria are not met. */
#define WG_MAX_CALIBRATION_COUNTER	(100+WG_DELAY_CALIBRATION_COUNTER)

/* WG_MIN_CALIBRATION_PARAM is used to detect whether a frequency is available 
   for calibration, together with amplitude strength (parameter) */
#define WG_MIN_CALIBRATION_PARAM	(400)

/* Value written to EEPROM to indicate that calibration parameters have been
   stored in EEPROM */
#define WG_EEPROM_PARAM_STORED	(0xAAAA)

/* Amplitude is scaled to this WG_MAX_AMPLITUDE, to compute deviation */
#define WG_MAX_AMPLITUDE              	(180)

//******************************************************************************************************
// Typedefs
//******************************************************************************************************
typedef enum
{
	WG_ANT_TYPE_COIL_2V = 0,
	//WG_ANT_TYPE_COIL_2V_1H -> possible in future
	WG_ANT_TYPE_LAST
} E_wg_antennaType_t;

typedef enum
{
	WG_STATUS_OPERATIONAL = 0,   /* No problems are detected */
	WG_STATUS_NOSIGNAL,          /* Amplitude linked to all possible signals is (nearly) zero */
	WG_STATUS_ANT_BROKEN,        /* Vref of left/right antenna is not Vcc */
	WG_STATUS_SHORT_CIRCUIT,
	WG_STATUS_BIT_FAILED         /* Built-in test to test signal amplification failed for left or right antenna */
} E_wg_status_t;

typedef enum
{
	WG_CALIB_STATUS_DEFAULT = 0,				/* Loading Calibration parameters failed, using Default parameters */
	WG_CALIB_STATUS_START,            			/* PDO to start calibration has been received */
	WG_CALIB_STATUS_ONGOING,          			/* Calibration has been started, still ongoing */
	WG_CALIB_STORE_PARAM_IN_EEPROM,             /* Write determined values to EEPROM */
	WG_CALIB_STATUS_SUCCEEDED,        		    /* Calibration succeeded, parameter is stored */
	WG_CALIB_STATUS_NOTPRESENT,       		    /* Calibration failed, probably because frequency was not present */
	WG_CALIB_STATUS_FAILED            			/* Calibration failed, probably because freq. was not present */
} E_wg_calib_status_t;

typedef enum
{
	WG_COEFF_STATUS_DEFAULT = 0,	/* Default Input Frequency */
	WG_COEFF_STATUS_USER,			/* User-defined Input Frequency */
	WG_COEFF_STATUS_UPDATED,		/* Input Frequency successfully updated through CAN bus */
	WG_COEFF_STATUS_FAILED			/* Loading/Storing Freq. values failed */
} E_wg_coeff_status_t;

typedef enum
{
	WG_NIBBLE_STATUS_SYNC = 0,
	WG_NIBBLE_STATUS_START,
	WG_NIBBLE_STATUS_SWITCH,
	WG_NIBBLE_STATUS_RESERVED,
	WG_NIBBLE_STATUS_END,
	WG_NIBBLE_STATUS_UNKNOWN
} E_wg_nibble_status_t;

typedef enum
{
	WG_NIBBLE_SUBSTATUS_DATA_MSN = 0,
	WG_NIBBLE_SUBSTATUS_PARITY_MSN,
	WG_NIBBLE_SUBSTATUS_DATA_LSN,
	WG_NIBBLE_SUBSTATUS_PARITY_LSN,
	WG_NIBBLE_SUBSTATUS_UNKNOWN
} E_wg_nibble_substatus_t;

typedef struct
{
	int16   antenna_cable_ok;     /* 1 if antenna cable is not broken */
	int16   pilot_tone_ok;        /* 1 if pilot tone is present, so no problem with signal amplification */
	int16   no_short_circuit;     /* 1 if no short circuit is detected */
} T_wg_ant_status_t;

typedef struct
{
	/* This variable keeps track of calibration status of individual frequencies */
	E_wg_calib_status_t   calibration_status_freq[NBR_INPUT_FREQ];

	/*	Determined parameters during antenna calibration. Invalid value is frequency
		not present. */
	Uint16                calibration_param[NBR_INPUT_FREQ];
} T_wg_calibration_t;

typedef struct
{
	/* Input Frequency value */
	Uint16	freq_value;
	
	/* Input Frequency scaled Cosine Coefficient value */
	int16		cos_coefficient;

	/* Input Frequency scaled Sine Coefficient value */
	int16		sin_coefficient;
	
	/* Status of individual frequency */
	E_wg_coeff_status_t coeff_status;
} T_wg_coefficient_t;

typedef struct
{
	/* Already foreseen, in case of omni-directional antenna in future */
	E_wg_antennaType_t  antennaType;
  
	/* Calibration_status indicates the overall calibration status */
	E_wg_calib_status_t 	calibration_status;
  
	/* Calibration counter is used to delay calibration and to check the minimal calibration time */
	Uint16              			calibration_counter;

	/* calibration_left and _right contain all variables related to calibration procedure for the left and right antenna coil */
	T_wg_calibration_t  	calibration_left;
	T_wg_calibration_t  	calibration_right;

	/* All parameters that are linked to antenna and signal status */
	E_wg_status_t			status;
	T_wg_ant_status_t   	status_left_antenna;
	T_wg_ant_status_t   	status_right_antenna;

	/* Values and Coefficients of the Input Frequencies */
	T_wg_coefficient_t		frequencies[NBR_INPUT_FREQ+SECOND_HARMONIC_FIRST_FREQUENCY];
	
	/* Overall status of the Frequency values, coefficients */
	E_wg_coeff_status_t	freq_status;
	
	/* Result of the wire guidance: a left and right amplitude. Based on this
		amplitude, the deviation is determined. */
	Uint32						amplitudeLeft[NBR_INPUT_FREQ];
	Uint32						amplitudeRight[NBR_INPUT_FREQ];
	#if BIT_WIREGUID_ACTIVE
	Uint32						amplitudePWM[2]; // Resulting Amplitudes of the Test Frequency
	#endif

	/* Deviation has not to be computed for test frequency and the 2nd Harmonic */
	int16						deviation_m2ecm[NBR_INPUT_FREQ];

	#if SECOND_HARMONIC_FIRST_FREQUENCY
	/* Relative Phase between 1st Input Freq. - 2nd Harmonic (normalized cosine and sine) */
	int16 						rel_phaseLeft[2];
	int16						rel_phaseRight[2];
	//
	int16						rel_phaseHIGH[2];

	/* Phase sign correction for synchronization purpose (part of the calibration) */
	int8 						rel_phaseLeft_sign;
	int8						rel_phaseRight_sign;

	/* Boolean that checks whether phase direction correction was executed */
	sbool 					direction_checked;
	
	/* Parameters for the QAM decoding process of the switch states */
	Uint8						switch_state_MSN;
	Uint8						switch_state_LSN;
	Uint8						switch_states_to_be_sent;
	sbool					    tx_new_states;
	E_wg_nibble_status_t		nibble_status;
	E_wg_nibble_substatus_t 	nibble_substatus;
	#endif
}T_wireGuid_t;

/* Function declarations */
void WireGuid_init(T_wireGuid_t  *wireGuidData);
void WireGuid_process(T_wireGuid_t  *pWireGuidData);

#endif

#endif // End of __GEN_WIRE_GUIDANCE_H definition

