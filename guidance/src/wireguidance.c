// 2014 - 2015

/*! \file wireguidance.c
    \brief Contains wire guidance function implementations
*/

#include "project_canantenna.h"

#if GUIDANCE_WIRE

//*****************************************************************************************************************************************
// Constants
//*****************************************************************************************************************************************
#if SECOND_HARMONIC_FIRST_FREQUENCY
/* Constants for the QAM Decoding Process */
// Normal Size of the Hanning Window
static const Uint8 __attribute__((space(auto_psv))) WG_SYNC_WDW_SZ = (Uint8)HN_WDW_SZ;
// Shifting Length of the Hanning Window for synchronization procedure
static const Uint8 __attribute__((space(auto_psv))) WG_SYNC_WDW_VAR = (Uint8)HN_WDW_VAR;
// Component Noise
static const int16 __attribute__((space(auto_psv))) WG_QAM_COMP_NOISE = 5;
// 
static const int16 __attribute__((space(auto_psv))) WG_QAM_COMP_MAX = 75;
static const int16 __attribute__((space(auto_psv))) WG_QAM_COMP_MIN	= 25;
static const int16 __attribute__((space(auto_psv))) WG_QAM_COMP_MAX_NEG = -75;
static const int16 __attribute__((space(auto_psv))) WG_QAM_COMP_MIN_NEG = -25;
// Square Amplitude Noise
static const Uint16 __attribute__((space(auto_psv))) WG_QAM_NOISE	= 50U;
// Max Square Amplitude: (+/-75,+/-75) constellations. 2*(75^2)
static const Uint16 __attribute__((space(auto_psv))) WG_QAM_MAX_VAL = 11250U;
// Max Square Amplitude with Noise: (+/-80,+/-80) constellations. WG_QAM_MAX_VAL + 75*20 + WG_QAM_NOISE
static const Uint16 __attribute__((space(auto_psv))) WG_QAM_MAX_VAL_NOISE = 12800U;
// Middle Constellation (+/-50,+/-50) -> 2*50^2
static const Uint16 __attribute__((space(auto_psv))) WG_QAM_MID_VAL = 5000U;
#endif
//
static const Uint32 __attribute__((space(auto_psv))) DEVIATION_SCALE = 20000UL;
static const int16 __attribute__((space(auto_psv))) DEVIATION_RNG_MAX = 5000;
static const int16 __attribute__((space(auto_psv))) DEVIATION_RNG_MIN = -5000;
static const Uint32 __attribute__((space(auto_psv))) AMPLITUDE_MIN = 26UL;
//
static const int16 __attribute__((space(auto_psv))) 
BIT_MAX_REFVOLT_SHORT_CIRCUIT = (int16)BIT_ANT_MAX_REFVOLT_SHORT_CIRCUIT;
static const int16 __attribute__((space(auto_psv))) BIT_MAX_REFVOLT = (int16)BIT_ANT_MAX_REFVOLT;
static const int16 __attribute__((space(auto_psv))) BIT_MIN_REFVOLT = (int16)BIT_ANT_MIN_REFVOLT;

//*****************************************************************************************************************************************
// Static functions
//*****************************************************************************************************************************************
static void wireGuid_antennaGood(T_wireGuid_t  *pWireGuidData)
{
	/* Declaration */
	int16   refVoltLeft;
	int16   refVoltRight;
  
	/* Copy data to avoid that it is overwritten under interrupt */
	refVoltLeft  = ADC_refVoltLeft_1;
	refVoltRight = ADC_refVoltRight_1;

	/* Set all test values to OK */
	pWireGuidData->status_left_antenna.antenna_cable_ok  = 1;
	pWireGuidData->status_left_antenna.no_short_circuit  = 1;
	pWireGuidData->status_right_antenna.antenna_cable_ok = 1;
	pWireGuidData->status_right_antenna.no_short_circuit = 1;

	if (refVoltLeft < BIT_MAX_REFVOLT_SHORT_CIRCUIT)
	{
		pWireGuidData->status_left_antenna.no_short_circuit = 0;
	}
	else if ( (refVoltLeft <  BIT_MIN_REFVOLT) ||
				(refVoltLeft >  BIT_MAX_REFVOLT) )
	{
		pWireGuidData->status_left_antenna.antenna_cable_ok = 0;
	}

	if (refVoltRight <  BIT_MAX_REFVOLT_SHORT_CIRCUIT)
	{
		pWireGuidData->status_right_antenna.no_short_circuit = 0;
	}
	else if ( (refVoltRight <  BIT_MIN_REFVOLT) ||
				(refVoltRight >  BIT_MAX_REFVOLT) )
	{
		pWireGuidData->status_right_antenna.antenna_cable_ok = 0;
	}

	if (!(pWireGuidData->status_left_antenna.antenna_cable_ok)||
		!(pWireGuidData->status_right_antenna.antenna_cable_ok))
	{
		pWireGuidData->status = WG_STATUS_ANT_BROKEN;
	}
	else if (!(pWireGuidData->status_left_antenna.no_short_circuit)||
			!(pWireGuidData->status_right_antenna.no_short_circuit))
	{
		pWireGuidData->status = WG_STATUS_SHORT_CIRCUIT;
	}
	else
	{
		pWireGuidData->status = WG_STATUS_OPERATIONAL;
	}

	return;
}

//*****************************************************************************************************************************************
static void wireGuid_set_deviation_invalid(T_wireGuid_t  *pWireGuidData)
{
	Uint8     i;

	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		pWireGuidData->deviation_m2ecm[i] = WG_DEVIATION_INVALID;
	}
	#if SECOND_HARMONIC_FIRST_FREQUENCY
	pWireGuidData->rel_phaseLeft[0] = WG_REL_PHASE_INVALID;
	pWireGuidData->rel_phaseLeft[1] = WG_REL_PHASE_INVALID;
	pWireGuidData->rel_phaseRight[0] = WG_REL_PHASE_INVALID;
	pWireGuidData->rel_phaseRight[1] = WG_REL_PHASE_INVALID;
	AntRelPhaseLeftSign = 1;
	AntRelPhaseRightSign = 1;
	pWireGuidData->rel_phaseLeft_sign = AntRelPhaseLeftSign;
	pWireGuidData->rel_phaseRight_sign = AntRelPhaseRightSign;
	pWireGuidData->direction_checked = false;
	#endif

	return;
}

//*****************************************************************************************************************************************
static void wireGuid_computeDefaultFreqDeviation(T_wireGuid_t  *pWireGuidData)
{
	Uint8 i;
	
	// Case Default Parameters (No Calibration)
	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		// Calculate the Deviations
		if ((pWireGuidData->amplitudeLeft[i] > AMPLITUDE_MIN) &&
			(pWireGuidData->amplitudeRight[i] > AMPLITUDE_MIN))
		{				
			// Range: ]-20000 ; 20000[, scaled in 200cm
			ANT_Deviation[i] = (int16)(DEVIATION_SCALE / pWireGuidData->amplitudeLeft[i]) - 
										(int16)(DEVIATION_SCALE / pWireGuidData->amplitudeRight[i]);
			// CHECK RANGE -5000 ... 5000
			if (ANT_Deviation[i] >  DEVIATION_RNG_MAX)
						ANT_Deviation[i] =  DEVIATION_RNG_MAX;
			else if (ANT_Deviation[i] < DEVIATION_RNG_MIN)
						ANT_Deviation[i] = DEVIATION_RNG_MIN;
			pWireGuidData->deviation_m2ecm[i] = ANT_Deviation[i];
		}
		// Invalidate the Deviations
		else
		{
			pWireGuidData->deviation_m2ecm[i] = WG_DEVIATION_INVALID;
			/* Invalidate relative phase in case Resulting Amplitudes
				of the first Input Frequency is very low */
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			if (i == 0U)
			{
				pWireGuidData->rel_phaseLeft[0] = WG_REL_PHASE_INVALID;
				pWireGuidData->rel_phaseLeft[1] = WG_REL_PHASE_INVALID;
				pWireGuidData->rel_phaseRight[0] = WG_REL_PHASE_INVALID;
				pWireGuidData->rel_phaseRight[1] = WG_REL_PHASE_INVALID;					
			}
			#endif
		}
	}
	
	return;
}

//*****************************************************************************************************************************************
static void wireGuid_computeCalibFreqDeviation(T_wireGuid_t  *pWireGuidData)
{
	Uint8 i;

	// Case successful calibration
	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		// Calculate the Deviations
		if ((pWireGuidData->amplitudeLeft[i] > AMPLITUDE_MIN) &&
			(pWireGuidData->amplitudeRight[i] > AMPLITUDE_MIN) &&
			(pWireGuidData->calibration_left.calibration_status_freq[i] == WG_CALIB_STATUS_SUCCEEDED) &&
			(pWireGuidData->calibration_right.calibration_status_freq[i] == WG_CALIB_STATUS_SUCCEEDED))
		{
			// Range: ]-20000 ; 20000[, scaled in 200cm
			ANT_Deviation[i] = (int16)(DEVIATION_SCALE / pWireGuidData->amplitudeLeft[i]) - 
										(int16)(DEVIATION_SCALE / pWireGuidData->amplitudeRight[i]);
			// CHECK RANGE -5000 ... 5000
			if (ANT_Deviation[i] >  DEVIATION_RNG_MAX)
						ANT_Deviation[i] =  DEVIATION_RNG_MAX;
			else if (ANT_Deviation[i] < DEVIATION_RNG_MIN)
						ANT_Deviation[i] = DEVIATION_RNG_MIN;
			pWireGuidData->deviation_m2ecm[i] = ANT_Deviation[i];
		}
		// Invalidate the Deviations
		else
		{
			pWireGuidData->deviation_m2ecm[i] = WG_DEVIATION_INVALID;
			/* Invalidate relative phase in case Resulting Amplitudes
				of the first Input Frequency is very low */
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			if (i == 0U)
			{
				pWireGuidData->rel_phaseLeft[0] = WG_REL_PHASE_INVALID;
				pWireGuidData->rel_phaseLeft[1] = WG_REL_PHASE_INVALID;
				pWireGuidData->rel_phaseRight[0] = WG_REL_PHASE_INVALID;
				pWireGuidData->rel_phaseRight[1] = WG_REL_PHASE_INVALID;
			}
			#endif
		}
	}
	
	return;
}

//*****************************************************************************************************************************************
#if SECOND_HARMONIC_FIRST_FREQUENCY
static sbool wireGuid_eval_antenna_direction(T_wireGuid_t *pWireGuidData, const int16 I_value, const int16 Q_value)
{
	// I_value = -Q_value

	// check direction of the left coil
	if ((pWireGuidData->rel_phaseLeft[0] > (I_value - WG_QAM_COMP_NOISE)) &&
		(pWireGuidData->rel_phaseLeft[0] < (I_value + WG_QAM_COMP_NOISE)) &&
		(pWireGuidData->rel_phaseLeft[1] > (Q_value - WG_QAM_COMP_NOISE)) &&
		(pWireGuidData->rel_phaseLeft[1] < (Q_value + WG_QAM_COMP_NOISE)))
	{
		AntRelPhaseLeftSign = 1;
		pWireGuidData->rel_phaseLeft_sign = AntRelPhaseLeftSign;
	}
	else if (	(pWireGuidData->rel_phaseLeft[0] > (Q_value - WG_QAM_COMP_NOISE)) &&
				(pWireGuidData->rel_phaseLeft[0] < (Q_value + WG_QAM_COMP_NOISE)) &&
				(pWireGuidData->rel_phaseLeft[1] > (I_value - WG_QAM_COMP_NOISE)) &&
				(pWireGuidData->rel_phaseLeft[1] < (I_value + WG_QAM_COMP_NOISE)))
	{
		AntRelPhaseLeftSign = -1;
		pWireGuidData->rel_phaseLeft_sign = AntRelPhaseLeftSign;
	}
	else
		// wrong constellation, so do not evaluate direction
		return false;
	
	// check direction of the right coil
	if ((pWireGuidData->rel_phaseRight[0] > (I_value - WG_QAM_COMP_NOISE)) &&
		(pWireGuidData->rel_phaseRight[0] < (I_value + WG_QAM_COMP_NOISE)) &&
		(pWireGuidData->rel_phaseRight[1] > (Q_value - WG_QAM_COMP_NOISE)) &&
		(pWireGuidData->rel_phaseRight[1] < (Q_value + WG_QAM_COMP_NOISE)))
	{
		AntRelPhaseRightSign = 1;
		pWireGuidData->rel_phaseRight_sign = AntRelPhaseRightSign;
	}
	else if (	(pWireGuidData->rel_phaseRight[0] > (Q_value - WG_QAM_COMP_NOISE)) &&
					(pWireGuidData->rel_phaseRight[0] < (Q_value + WG_QAM_COMP_NOISE)) &&
					(pWireGuidData->rel_phaseRight[1] > (I_value - WG_QAM_COMP_NOISE)) &&
					(pWireGuidData->rel_phaseRight[1] < (I_value + WG_QAM_COMP_NOISE)))
	{
		AntRelPhaseRightSign = -1;
		pWireGuidData->rel_phaseRight_sign = AntRelPhaseRightSign;
	}
	else
		// wrong constellation, so do not evaluate direction
		return false;
	
	// Sign of left and right relative phase calibrated
	pWireGuidData->direction_checked = true;

	return true;
}

//*****************************************************************************************************************************************
/* Helper Function with the Quadrature Component for the Nibble Codeword Evaluation */
static Uint8 wireGuid_QAM_Nibble_Q(int16 *Quad)
{
	if ( (*Quad > (WG_QAM_COMP_MAX_NEG - WG_QAM_COMP_NOISE)) &&
		 (*Quad < (WG_QAM_COMP_MAX_NEG + WG_QAM_COMP_NOISE))   )
	// (I, -75) -> ...+ 0x00
		return (0x00U);
		
	else if ( (*Quad > (WG_QAM_COMP_MIN_NEG - WG_QAM_COMP_NOISE)) && 
				(*Quad < (WG_QAM_COMP_MIN_NEG + WG_QAM_COMP_NOISE))   )
	// (I, -25) -> ...+ 0x01
		return (0x01U);
		
	else if ( (*Quad > (WG_QAM_COMP_MAX - WG_QAM_COMP_NOISE)) &&
				(*Quad < (WG_QAM_COMP_MAX + WG_QAM_COMP_NOISE))    )
	// (I, 75) -> ...+ 0x02
		return (0x02U);
		
	else if ( (*Quad > (WG_QAM_COMP_MIN - WG_QAM_COMP_NOISE)) &&
				(*Quad < (WG_QAM_COMP_MIN + WG_QAM_COMP_NOISE))   )
	// (I, 25) -> ...+ 0x03
		return (0x03U);
		
	else
	// Invalid Quadrature Component
		return (0x10U);
}

//*****************************************************************************************************************************************
/* Evaluate the appropriate codeword of the QAM Position */
static Uint8 wireGuid_QAM_Nibble(int16 InPhase, int16 Quad)
{
	// (I, Q) -> bxxyy -> 0x0X

	if ( (InPhase > (WG_QAM_COMP_MAX_NEG - WG_QAM_COMP_NOISE)) &&
		 (InPhase < (WG_QAM_COMP_MAX_NEG + WG_QAM_COMP_NOISE))    )
	// (-75, Q) constellation -> 0x00 + 0x00...0x03
	// If invalid -> 0x10
		return (wireGuid_QAM_Nibble_Q(&Quad));
		
	else if (  (InPhase > (WG_QAM_COMP_MIN_NEG - WG_QAM_COMP_NOISE)) && 
				(InPhase < (WG_QAM_COMP_MIN_NEG + WG_QAM_COMP_NOISE))    )
	// (-25, Q) constellation -> 0x04 + 0x00...0x03
	// If invalid 0x14
		return (0x04U + wireGuid_QAM_Nibble_Q(&Quad));
		
	else if (  (InPhase > (WG_QAM_COMP_MAX - WG_QAM_COMP_NOISE)) &&
				(InPhase < (WG_QAM_COMP_MIN + WG_QAM_COMP_NOISE))     )
	// (75, Q) constellation -> 0x08 + 0x00...0x03
	// If invalid 0x18
		return (0x08U + wireGuid_QAM_Nibble_Q(&Quad));

	else if (  (InPhase > (WG_QAM_COMP_MIN - WG_QAM_COMP_NOISE)) &&
				(InPhase < (WG_QAM_COMP_MIN + WG_QAM_COMP_NOISE))    )
	// (25, Q) constellation -> 0x0C + 0x00...0x03
	// If invalid 0x1C
		return (0x0CU + wireGuid_QAM_Nibble_Q(&Quad));

	else
	// Other invalid constellations
	// or (0,0) -> no 2nd harmonic
		return (0x10U);
}

//*****************************************************************************************************************************************
/* Evaluate Parity Nibble of a Data Nibble (works vice versa as well) */
static Uint8 wireGuid_QAM_Parity(Uint8 nibble)
{
	// Normal QAM-16 Size = 16
	// With element for invalid (or no 2nd Harmonic constellations) = 17
	static const Uint8 __attribute__((space(auto_psv)))
	LkUpTbl_Data2Parity[17U] = {
        /* Input: Data Nibble
         * Output: Appropriate Parity Nibble
         * Symmetrical, so works the other way too
         */
        0x00U, 0x0EU, 0x0DU, 0x03U,
        0x0BU, 0x05U, 0x06U, 0x08U,
        0x07U, 0x09U, 0x0AU, 0x04U,
        0x0CU, 0x02U, 0x01U, 0x0FU,
        0x10U
    };

	return (LkUpTbl_Data2Parity[nibble]);
}

//*****************************************************************************************************************************************
/* See wireGuid_QAM_decode function for further details */
static void wireGuid_QAM_status_synchronize(T_wireGuid_t *pWireGuidData)
{
	// Compute square amplitude
	Uint16 buffer = (Uint16)(pWireGuidData->rel_phaseHIGH[0] * pWireGuidData->rel_phaseHIGH[0] +
        pWireGuidData->rel_phaseHIGH[1] * pWireGuidData->rel_phaseHIGH[1]);

	if (buffer >= WG_QAM_MAX_VAL_NOISE)
	// Square Amplitude too high
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
	else if (buffer >= WG_QAM_MAX_VAL)
	// Synchronized, no need to change Hanning Window size
	{
        pWireGuidData->nibble_status = WG_NIBBLE_STATUS_START;
        pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_MSN;	
	}
	else if (buffer > WG_QAM_MID_VAL)
	// Most samples of the batch processed by Goertzel Algorithm were of the Start Data QAM Position
	// Remaining samples were of the lone baseband signal. 
	// Thus, CURRENT QAM Position is Start Data.
	// Right shift in the time axis needed for the next batch to include 
	// more samples of the NEXT QAM Position: Start Parity 
	// -> Speed Down Decoding Process
	{
		// Current QAM position is Start Data position, next is Start Parity Position
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_START;
		pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_MSN;
		// Increase Hanning Window Size to include more samples of the Start Parity QAM Position
		ANT_k_max += WG_SYNC_WDW_VAR;	
	}
	else if (buffer >= WG_QAM_NOISE)
	// Most samples of the batch processed by Goertzel Algorithm were of the lone baseband signal
	// Remaining samples were of the NEXT QAM Position: Start Data
	// Left shift in the time axis needed for the next batch to exclude 
	// samples of the Start Parity QAM Position that comes after the Start Data
	// -> Speed Up decoding process  	
	{
		// Next QAM Position is Start Data
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_START;
		pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_DATA_MSN;
		// Decrease Hanning Window Size to avoid including in the batch samples of the Start Parity QAM Position
		ANT_k_max -= WG_SYNC_WDW_VAR;	
	}
	//else if (buffer < WG_QAM_NOISE)
		// No 2nd Harmonic, synchronization procedure

	return;
}

//*****************************************************************************************************************************************
/* See wireGuid_QAM_decode function for further details */
static void wireGuid_QAM_status_start(T_wireGuid_t *pWireGuidData)
{
	sbool check;

	// Reset Hanning Window Size if synchronization was done  during the synchronization QAM-stream
	if (ANT_k_max != WG_SYNC_WDW_SZ)
		ANT_k_max = WG_SYNC_WDW_SZ;

	switch (pWireGuidData->nibble_substatus)
	{
		case WG_NIBBLE_SUBSTATUS_DATA_MSN:
		// Start Data
			if (!pWireGuidData->direction_checked)
			// Do antenna direction correction
			{
				check = wireGuid_eval_antenna_direction(pWireGuidData, WG_QAM_COMP_MAX, WG_QAM_COMP_MAX_NEG);
			}
			else
			{
				check = (pWireGuidData->rel_phaseHIGH[0] > (WG_QAM_COMP_MAX - WG_QAM_COMP_NOISE)) &&
							(pWireGuidData->rel_phaseHIGH[0] < (WG_QAM_COMP_MAX + WG_QAM_COMP_NOISE)) &&
							(pWireGuidData->rel_phaseHIGH[1] > (WG_QAM_COMP_MAX_NEG - WG_QAM_COMP_NOISE)) &&
							(pWireGuidData->rel_phaseHIGH[1] < (WG_QAM_COMP_MAX_NEG + WG_QAM_COMP_NOISE));
			}
			if (check)
			// (75,-75) constellation -> 0x08 Codeword
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_MSN;
			else
			// Other constellation due to Noise or Bug	
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
			break;

		case WG_NIBBLE_SUBSTATUS_PARITY_MSN:
		// Start Parity
			if (!pWireGuidData->direction_checked)
			// Do antenna direction correction
			{
				check = wireGuid_eval_antenna_direction(pWireGuidData, WG_QAM_COMP_MIN_NEG, WG_QAM_COMP_MIN);
			}
			else
			{
				check = (pWireGuidData->rel_phaseHIGH[0] > (WG_QAM_COMP_MIN_NEG - WG_QAM_COMP_NOISE)) &&
							(pWireGuidData->rel_phaseHIGH[0] < (WG_QAM_COMP_MIN_NEG + WG_QAM_COMP_NOISE)) &&
							(pWireGuidData->rel_phaseHIGH[1] > (WG_QAM_COMP_MIN - WG_QAM_COMP_NOISE)) &&
							(pWireGuidData->rel_phaseHIGH[1] < (WG_QAM_COMP_MIN + WG_QAM_COMP_NOISE));
			}
			if (check)
			// (-25, 25) constellation -> 0x07 Codeword
			{
				// Next are expected the Switch QAM Positions
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_SWITCH;
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_DATA_MSN;
			}
			else
			// Other constellation due to Noise or Hardware/Software Failure
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
			break;

		default:
			break;
	}

	return;
}

//*****************************************************************************************************************************************
/* See wireGuid_QAM_decode function for further details */
static void wireGuid_QAM_status_switches(T_wireGuid_t *pWireGuidData)
{
	Uint8 nibble;
	switch (pWireGuidData->nibble_substatus)
	{
		case WG_NIBBLE_SUBSTATUS_DATA_MSN:
		// Switch Data MSN (4 first switches)
			// Compute the nibble of the constellation
			pWireGuidData->switch_state_MSN = wireGuid_QAM_Nibble(pWireGuidData->rel_phaseLeft[0], 
																									  pWireGuidData->rel_phaseLeft[1]);
			// Check Validity of the Nibble
			if (pWireGuidData->switch_state_MSN < 0x10U) 
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_MSN;
			else
			// Invalid Constellation and Nibble (due to Noise or Hardware/Software failure)
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
			break;
				
		case WG_NIBBLE_SUBSTATUS_PARITY_MSN:
		// Switch Parity MSN
			nibble = wireGuid_QAM_Nibble(pWireGuidData->rel_phaseHIGH[0], pWireGuidData->rel_phaseHIGH[1]);
			// Check if the Most Significant Nibble of the Switches has the correct Parity Codeword.
			// switch_state_MSN < 0x10 since it's checked in the previous case
			// and branches to Unknown Status if switch_state_MSN > 0x0F
			if (nibble == wireGuid_QAM_Parity(pWireGuidData->switch_state_MSN))
			{
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_DATA_LSN;
			}
			else
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
			break;
				
		case WG_NIBBLE_SUBSTATUS_DATA_LSN:
		// Switch Data LSN (4 last switches)
			pWireGuidData->switch_state_LSN = wireGuid_QAM_Nibble(pWireGuidData->rel_phaseLeft[0], 
																									 pWireGuidData->rel_phaseLeft[1]);
			// Check Validity of the Nibble
			if (pWireGuidData->switch_state_LSN < 0x10U)
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_LSN;
			else
			// Invalid Constellation and Nibble (due to Noise or Hardware/Software failure)
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
			break;
				
		case WG_NIBBLE_SUBSTATUS_PARITY_LSN:
		// Switch Parity LSN
			nibble = wireGuid_QAM_Nibble(pWireGuidData->rel_phaseHIGH[0], pWireGuidData->rel_phaseHIGH[1]); 
			// Check if the Least Significant Nibble of the Switches has the correct Parity Codeword.
			// switch_state_LSN < 0x10 since it's checked in the previous case
			// and branches to Unknown Status if switch_state_LSN > 0x0F
			if (nibble == wireGuid_QAM_Parity(pWireGuidData->switch_state_LSN))
			{
				// Next are expected the reserved QAM positions
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_RESERVED;
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_DATA_MSN;
				// Received Switch QAM-stream is valid, allow sending the extracted switch state values through CAN message
				pWireGuidData->switch_states_to_be_sent = (Uint8)((pWireGuidData->switch_state_MSN << 4) + pWireGuidData->switch_state_LSN);
				pWireGuidData->tx_new_states = true;
			}
			else
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;				
			break;

		default:
			break;
	}

	return;
}

//*****************************************************************************************************************************************
/* See wireGuid_QAM_decode function for further details */
static void wireGuid_QAM_status_reserved(T_wireGuid_t *pWireGuidData)
{
	// (-75,-75) constellation -> 0x00 Codeword
	sbool check = (pWireGuidData->rel_phaseHIGH[0] > (WG_QAM_COMP_MAX_NEG - WG_QAM_COMP_NOISE)) &&
						(pWireGuidData->rel_phaseHIGH[0] < (WG_QAM_COMP_MAX_NEG + WG_QAM_COMP_NOISE)) &&
						(pWireGuidData->rel_phaseHIGH[1] > (WG_QAM_COMP_MAX_NEG - WG_QAM_COMP_NOISE)) &&
						(pWireGuidData->rel_phaseHIGH[1] < (WG_QAM_COMP_MAX_NEG + WG_QAM_COMP_NOISE));

	if (!check)
	// Other constellation due to Noise or Hardware/Software Failure
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
	else
	{
		switch (pWireGuidData->nibble_substatus)
		{
			case WG_NIBBLE_SUBSTATUS_DATA_MSN:
			// Reserved Data MSN
				// Valid Switch States were sent, disable the transmission until the next switch states are received. 
				pWireGuidData->tx_new_states = false;
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_MSN;
				break;
					
			case WG_NIBBLE_SUBSTATUS_PARITY_MSN:
			// Reserved Parity MSN
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_DATA_LSN;
				break;
					
			case WG_NIBBLE_SUBSTATUS_DATA_LSN:
			// Reserved Data LSN
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_LSN;
				break;
					
			case WG_NIBBLE_SUBSTATUS_PARITY_LSN:
			// Reserved Parity LSN
				// The End QAM positions are expected next
				pWireGuidData->nibble_status = WG_NIBBLE_STATUS_END;
				pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_DATA_MSN;
				break;
					
			default:
				break;
		}
	}

	return;
}

//*****************************************************************************************************************************************
/* See wireGuid_QAM_decode function for further details */
static void wireGuid_QAM_status_end(T_wireGuid_t *pWireGuidData)
{
	// (75,75) constellation -> 0x0A Codeword
	sbool check = (pWireGuidData->rel_phaseHIGH[0] > (WG_QAM_COMP_MAX - WG_QAM_COMP_NOISE)) &&
						(pWireGuidData->rel_phaseHIGH[0] < (WG_QAM_COMP_MAX + WG_QAM_COMP_NOISE)) &&
						(pWireGuidData->rel_phaseHIGH[1] > (WG_QAM_COMP_MAX - WG_QAM_COMP_NOISE)) &&
						(pWireGuidData->rel_phaseHIGH[1] < (WG_QAM_COMP_MAX + WG_QAM_COMP_NOISE));
			
	if (!check)
	// Other constellation due to Noise or Hardware/Software Failure
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
	else
		{
			switch (pWireGuidData->nibble_substatus)
			{
				case WG_NIBBLE_SUBSTATUS_DATA_MSN:
				// End Data
					pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_PARITY_MSN;
					break;
					
				case WG_NIBBLE_SUBSTATUS_PARITY_MSN:
				// End Parity
					// The synchronization procedure is expected next (no 2nd Harmonic)
					pWireGuidData->nibble_status = WG_NIBBLE_STATUS_SYNC;
					break;
					
				default:
					break;
			}
		}

	return;
}

//*****************************************************************************************************************************************
/* See wireGuid_QAM_decode function for further details */
static void wireGuid_QAM_status_unknown(T_wireGuid_t *pWireGuidData)
{
	// Left/Right square amplitudes
	Uint16 buffer = (Uint16)(pWireGuidData->rel_phaseHIGH[0] * pWireGuidData->rel_phaseHIGH[0] +
                    pWireGuidData->rel_phaseHIGH[1] * pWireGuidData->rel_phaseHIGH[1]);

	// Detect baseband signal
	if (buffer < WG_QAM_NOISE)
		// No 2nd Harmonic present -> go to synchronization procedure
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_SYNC;

	return;
}

//*****************************************************************************************************************************************
static void wireGuid_QAM_decode(T_wireGuid_t *pWireGuidData)
{
	// Decode QAM-stream to Hamming Data Codeword
	// QAM: Quadrature Amplitude Modulation
	/*
		QAM-stream sent by Frequency Generator:
		
		Synchronization:	(0,0)		No 2nd Harmonic (0x10 Codeword, invalid in the constellation)
									(0,0)
									(0,0)
									(0,0)
		Start:					(75,-75)		Data	->	0x08 Codeword
									(-25,25)		Parity -> 0x07 Codeword
		Switch States:		(0xXX)		Data, Most Significant Nibble -> States of the 4 first switches
									(0xX'X')		Parity MSN
									(0xYY)		Data LSN -> States of the 4 last switches
									(0xY'Y')		Parity LSN
		Reserved:				(-75,-75)	Data MSN -> 0x00 Codeword
		for future use			(-75,-75)	Parity MSN -> 0x00 Codeword
									(-75,-75)	Data LSN -> 0x00 Codeword
									(-75,-75)	Parity LSN -> 0x00 Codeword
		End:						(75,75)		Data -> 0x0A Codeword
									(75,75)		Data -> 0x0A Codeword
	*/
	
	if (pWireGuidData->amplitudeRight[0] > pWireGuidData->amplitudeLeft[0])
	{
		pWireGuidData->rel_phaseHIGH[0] = pWireGuidData->rel_phaseRight[0];
		pWireGuidData->rel_phaseHIGH[1] = pWireGuidData->rel_phaseRight[1];
	}
	else
	{
		pWireGuidData->rel_phaseHIGH[0] = pWireGuidData->rel_phaseLeft[0];
		pWireGuidData->rel_phaseHIGH[1] = pWireGuidData->rel_phaseLeft[1];
	}
	
	switch(pWireGuidData->nibble_status)
	{
		// Synchronization procedure of the received QAM-stream
		case WG_NIBBLE_STATUS_SYNC:
			wireGuid_QAM_status_synchronize(pWireGuidData);
			break;

		// Check received QAM starting positions
		case WG_NIBBLE_STATUS_START:
			wireGuid_QAM_status_start(pWireGuidData);
			break;
		
		// Check and extract information of the switch states 
		case WG_NIBBLE_STATUS_SWITCH:
			wireGuid_QAM_status_switches(pWireGuidData);
			break;
		
		// Check received QAM reserved positions
		case WG_NIBBLE_STATUS_RESERVED:
			wireGuid_QAM_status_reserved(pWireGuidData);
			break;
		
		// Check received QAM end positions
		case WG_NIBBLE_STATUS_END:
			wireGuid_QAM_status_end(pWireGuidData);
			break;
			
		// Status of the QAM-stream unknown. 
		// Detect Baseband signal w/o 2nd Harmonic to start synchronization
		case WG_NIBBLE_STATUS_UNKNOWN:
			wireGuid_QAM_status_unknown(pWireGuidData);
			break;
	
		default:
			break;
	}

	return;
}
#endif

//*****************************************************************************************************************************************
static void wireGuid_reset_calibration_data(T_wireGuid_t  *pWireGuidData)
{
	Uint8     i;

	/* Reset calibration procedure when start calibration is requested */
	pWireGuidData->calibration_counter = 0U;
	pWireGuidData->calibration_status = WG_CALIB_STATUS_ONGOING;
	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		/* Set calibration ongoing */
		pWireGuidData->calibration_left.calibration_status_freq[i] = WG_CALIB_STATUS_ONGOING;
		pWireGuidData->calibration_right.calibration_status_freq[i]= WG_CALIB_STATUS_ONGOING;

		/* Reset calibration parameter to default value WG_CALIBRATION_DEFAULT_PARAM */
		pWireGuidData->calibration_left.calibration_param[i]  = WG_CALIBRATION_DEFAULT_PARAM;
		pWireGuidData->calibration_right.calibration_param[i] = WG_CALIBRATION_DEFAULT_PARAM;
		AntAmpGainLeft[i] = pWireGuidData->calibration_left.calibration_param[i];
		AntAmpGainRight[i] = pWireGuidData->calibration_right.calibration_param[i];
	}

	return;
}

//*****************************************************************************************************************************************
static Uint16 wireGuid_calibrate_coil(
  const Uint16        	calibration_counter,
  Uint32             		*meas_amplitude,
  T_wg_calibration_t 	*calib_data)
{
	Uint8	i;
	Uint16   ret_calibration_ongoing = 0U;

	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		if (calib_data->calibration_status_freq[i] == WG_CALIB_STATUS_ONGOING)
		{
			calib_data->calibration_param[i] += (Uint16)((int16)WG_MAX_AMPLITUDE - (int16)meas_amplitude[i]);

			/* Calibration is ok, and minimum calibration time has expired */
			if ((meas_amplitude[i] == (Uint32)WG_MAX_AMPLITUDE) &&
				(calibration_counter > WG_MIN_CALIBRATION_COUNTER))
			{
				calib_data->calibration_status_freq[i] = WG_CALIB_STATUS_SUCCEEDED;
			}
			/* Calibration parameter remains below limits after minimum calibration time.
			   Frequency is probably not present. */
			else if ((calibration_counter > WG_MIN_CALIBRATION_COUNTER) &&
					  (calib_data->calibration_param[i] < WG_MIN_CALIBRATION_PARAM))
			{
				calib_data->calibration_status_freq[i] = WG_CALIB_STATUS_NOTPRESENT;
				/* Reset calibration parameter */
				calib_data->calibration_param[i] = WG_CALIBRATION_DEFAULT_PARAM;
			}
			/* Calibration criteria are not met after maximum time, so calibration has
				failed */
			else if (calibration_counter > WG_MAX_CALIBRATION_COUNTER)
			{
				calib_data->calibration_status_freq[i] = WG_CALIB_STATUS_FAILED;
				/* Reset calibration parameter and scale factor */
				calib_data->calibration_param[i] = WG_CALIBRATION_DEFAULT_PARAM;
			}
			/* Continue calibration for this frequency*/
			else
			{
				++ret_calibration_ongoing;
			}
		}
  }

  return (ret_calibration_ongoing);
}

//*****************************************************************************************************************************************
static void wireGuid_calibrate_antenna(T_wireGuid_t  *pWireGuidData)
{
	Uint8    i;
	Uint16   calib_ongoing_left  = 0;
	Uint16   calib_ongoing_right = 0;

	++pWireGuidData->calibration_counter;

	if (pWireGuidData->calibration_counter > WG_DELAY_CALIBRATION_COUNTER)
	{
		calib_ongoing_left = wireGuid_calibrate_coil(
								pWireGuidData->calibration_counter,
								&(pWireGuidData->amplitudeLeft[0]),
								&(pWireGuidData->calibration_left));
		calib_ongoing_right = wireGuid_calibrate_coil(
								pWireGuidData->calibration_counter,
								&(pWireGuidData->amplitudeRight[0]),
								&(pWireGuidData->calibration_right));
        /* Update Calibration Gains for the Goertzel Step */
		for (i = 0U; i < NBR_INPUT_FREQ; ++i)
		{
			AntAmpGainLeft[i] = pWireGuidData->calibration_left.calibration_param[i];
			AntAmpGainRight[i] = pWireGuidData->calibration_right.calibration_param[i];
		}
		/* If all frequencies left and right are calibrated (or not present), set
			calib. status to succeeded/failed/etc. */
		if (!calib_ongoing_left && !calib_ongoing_right)
		{
			Uint8  nbr_success		= 0U;
			Uint8  nbr_not_present	= 0U;
			Uint8  nbr_failed				= 0U;

			for (i = 0U; i < NBR_INPUT_FREQ; i++)
			{
				if ((pWireGuidData->calibration_right.calibration_status_freq[i] == WG_CALIB_STATUS_SUCCEEDED) &&
					(pWireGuidData->calibration_left.calibration_status_freq[i]  == WG_CALIB_STATUS_SUCCEEDED))
				{
					++nbr_success; // Successful calibration 
				}
				else if ((pWireGuidData->calibration_right.calibration_status_freq[i] == WG_CALIB_STATUS_NOTPRESENT) &&
						  (pWireGuidData->calibration_left.calibration_status_freq[i]  == WG_CALIB_STATUS_NOTPRESENT))
				{
					++nbr_not_present; // Frequency not present
				}
				else
				{
					++nbr_failed; // Calibration failed
				}
			}
      
			// Calibration succeeded at least for one Freq.
			if (nbr_success > 0U)
			{
				pWireGuidData->calibration_status = WG_CALIB_STORE_PARAM_IN_EEPROM;
			}
			// Calibration failed at least for one Freq. No Freq. was successfully calibrated
			else if (nbr_failed > 0U) // nbr_success == 0
			{
				pWireGuidData->calibration_status = WG_CALIB_STATUS_FAILED;
			}
			// No Freq. is present
			else // nbr_success == 0, nbr_failed == 0, nbr_not_present > 0
			{
				pWireGuidData->calibration_status = WG_CALIB_STATUS_NOTPRESENT;
			}
		}
		// Calibration is not finished
		else
		{
			pWireGuidData->calibration_status = WG_CALIB_STATUS_ONGOING;
		}
	}
  
  return;
}

//*****************************************************************************************************************************************
/* Executing this function will hold other processes and interrupts as interrupts
   have to be disabled when writing to EEPROM */
static void wireGuid_store_parameters(T_wireGuid_t  *pWireGuidData)
{
	Uint8		i;
	Uint8		fail_count = 0U;
	Uint32    	write_address;
	Uint16    	write_data;
	sbool	check_write;
  
	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		/* Store parameters of Left Antenna */
		write_address = eeprom_get_read_write_address(i);
		// Left Antenna calibrated for Input Frequency i
		if (pWireGuidData->calibration_left.calibration_status_freq[i] == WG_CALIB_STATUS_SUCCEEDED)
			write_data = pWireGuidData->calibration_left.calibration_param[i];
		// Input i-th Frequency not present or calibration failed
		else
			write_data = 0xFFFFU;	
		// Save Calibration in EEPROM
		check_write = eeprom_write_word(write_address, write_data);
		// If writing procedure fails, calibration of Freq. fails
		if (!check_write)
			pWireGuidData->calibration_left.calibration_status_freq[i] = WG_CALIB_STATUS_FAILED;

		/* Store parameters of Right Antenna */	
		write_address = eeprom_get_read_write_address(i + EEPROM_ANT_RIGHT_FREQ1);
		// Right Antenna calibrated for Input Frequency i
		if (pWireGuidData->calibration_right.calibration_status_freq[i] == WG_CALIB_STATUS_SUCCEEDED)
			write_data =  pWireGuidData->calibration_right.calibration_param[i];
		// Input Frequency i not present or calibration failed
		else
			write_data = 0xFFFFU;
		// Save Calibration in EEPROM
		check_write = eeprom_write_word(write_address, write_data);
		// If writing procedure fails, calibration of Freq. fails
		if (!check_write)
			pWireGuidData->calibration_right.calibration_status_freq[i] = WG_CALIB_STATUS_FAILED;
	}
	
	/* Check success/failure of storing the calibration parameters. */
    for(i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		if ((pWireGuidData->calibration_left.calibration_status_freq[i] == WG_CALIB_STATUS_SUCCEEDED) &&
			(pWireGuidData->calibration_right.calibration_status_freq[i]) == WG_CALIB_STATUS_SUCCEEDED)
		{
			// At least one Frequency Pair successfully calibrated
			break;
		}
		else
			++fail_count;
	}
	// If storing was a failure, set calibration status as failed and quit writing process
	if (fail_count == NBR_INPUT_FREQ)
	{
		pWireGuidData->calibration_status = WG_CALIB_STATUS_FAILED;
		/* Store invalid data to EEPROM. We don't care if it fails, since in this case something
			is wrong w/ the EEPROM */
		write_address = eeprom_get_read_write_address(EEPROM_ANT_CALIB_DATA_WRITTEN);
		eeprom_write_word(write_address, 0xFFFFU);
		return;
	}
	
	/* Write flag to tell that calibration data has been sent */
	write_address = eeprom_get_read_write_address(EEPROM_ANT_CALIB_DATA_WRITTEN);
	write_data = WG_EEPROM_PARAM_STORED;
	check_write = eeprom_write_word(write_address, write_data);
	
	/* Change calibration status */
	// If writing procedure fails, calibration fails and quit writing process
	if (!check_write)
	{
		pWireGuidData->calibration_status = WG_CALIB_STATUS_FAILED;
		return;
	}
	// Writing procedure succeeded, calibration is successful
	pWireGuidData->calibration_status = WG_CALIB_STATUS_SUCCEEDED; 
	LATBbits.LATB10 = 1;

	return;
}

//*****************************************************************************************************************************************
static void wireGuid_retrieve_parameters(T_wireGuid_t  *pWireGuidData)
{
	Uint8     i;
	Uint32   read_address;
	Uint16   read_data;

	/* Check whether valid parameters have been written to EEPROM before */
	read_address = eeprom_get_read_write_address(EEPROM_ANT_CALIB_DATA_WRITTEN);
	read_data = eeprom_read_word(read_address);

	if (read_data == WG_EEPROM_PARAM_STORED)
	{
		pWireGuidData->calibration_status = WG_CALIB_STATUS_SUCCEEDED;
		
		for (i = 0U; i < NBR_INPUT_FREQ; ++i)
		{
			/* Retrieve parameters left antenna */
			read_address = eeprom_get_read_write_address(i);
			pWireGuidData->calibration_left.calibration_param[i] = eeprom_read_word(read_address);

			if (pWireGuidData->calibration_left.calibration_param[i] == 0xFFFFU)
			{
				pWireGuidData->calibration_left.calibration_param[i]       = WG_CALIBRATION_DEFAULT_PARAM;
				pWireGuidData->calibration_left.calibration_status_freq[i] = WG_CALIB_STATUS_DEFAULT;
			}
			else
			{
				pWireGuidData->calibration_left.calibration_status_freq[i] = WG_CALIB_STATUS_SUCCEEDED;
			}
			
			/* Retrieve parameters right antenna */
			read_address = eeprom_get_read_write_address(i+EEPROM_ANT_RIGHT_FREQ1);
			pWireGuidData->calibration_right.calibration_param[i] = eeprom_read_word(read_address);

			if (pWireGuidData->calibration_right.calibration_param[i] == 0xFFFFU)
			{	
				pWireGuidData->calibration_right.calibration_param[i]       = WG_CALIBRATION_DEFAULT_PARAM;
				pWireGuidData->calibration_right.calibration_status_freq[i] = WG_CALIB_STATUS_DEFAULT;
			}
			else
			{
				pWireGuidData->calibration_right.calibration_status_freq[i] = WG_CALIB_STATUS_SUCCEEDED;
			}
		}
	}
	else
	{
		pWireGuidData->calibration_status = WG_CALIB_STATUS_DEFAULT;

		for (i = 0U; i < NBR_INPUT_FREQ; ++i)
		{
			pWireGuidData->calibration_left.calibration_status_freq[i] = WG_CALIB_STATUS_DEFAULT;
			pWireGuidData->calibration_right.calibration_status_freq[i]= WG_CALIB_STATUS_DEFAULT;

			/* Initialize parameter settings as known. They will be updated after every
				calibration. Array loops from freq 1 to 4 and does not include Test Freq. */
			pWireGuidData->calibration_left.calibration_param[i]  = WG_CALIBRATION_DEFAULT_PARAM;
			pWireGuidData->calibration_right.calibration_param[i] = WG_CALIBRATION_DEFAULT_PARAM;
		}
	}
  
  return;
}

//*****************************************************************************************************************************************
/* For the BIT, a PWM signal is continuously provided to the antenna input,
   in addition to the input signals from the signal generator (and used for 
   navigation) */
#if BIT_WIREGUID_ACTIVE
static void wireGuid_checkBIT(T_wireGuid_t  *pWireGuidData)
{
	static const Uint32 __attribute__((space(auto_psv)))
	BIT_MIN_SQR_AMPLITUDE_PILOT = (Uint32)BIT_MIN_SQUARE_AMPLITUDE_PILOT;

	if (pWireGuidData->amplitudePWM[0] < BIT_MIN_SQR_AMPLITUDE_PILOT)
	{
		pWireGuidData->status_left_antenna.pilot_tone_ok = 0;
	}
	else
	{
		pWireGuidData->status_left_antenna.pilot_tone_ok = 1;
	}

	if (pWireGuidData->amplitudePWM[1] < BIT_MIN_SQR_AMPLITUDE_PILOT)
	{
		pWireGuidData->status_right_antenna.pilot_tone_ok = 0;
	}
	else
	{
		pWireGuidData->status_right_antenna.pilot_tone_ok = 1;
	}

	return;
}
#endif

//*****************************************************************************************************************************************
// Local functions
//*****************************************************************************************************************************************
void WireGuid_init(T_wireGuid_t  *pWireGuidData)
{
	/* Memset assumed ok at initialization time */
	memset((void*)pWireGuidData,0,sizeof(T_wireGuid_t));
  
	/* Initialize antenna type to default antenna type */
	pWireGuidData->antennaType =  WG_ANT_TYPE_COIL_2V;
  
	/* Set calibration status, read data from EEPROM */
	wireGuid_retrieve_parameters(pWireGuidData);
  
    /* Antenna initialization for amplitude computation*/
	ANT_Initialize(pWireGuidData);
  
	/* Initialize status */
	pWireGuidData->status_left_antenna.antenna_cable_ok  = 1;
	pWireGuidData->status_left_antenna.no_short_circuit  = 1;
	pWireGuidData->status_left_antenna.pilot_tone_ok     = 1;

	pWireGuidData->status_right_antenna.antenna_cable_ok = 1;
	pWireGuidData->status_right_antenna.no_short_circuit = 1;
	pWireGuidData->status_right_antenna.pilot_tone_ok    = 1;

	return;
}

//*****************************************************************************************************************************************
void WireGuid_process(T_wireGuid_t  *pWireGuidData)
{
	/* Check whether antenna data is ok (refV within spec) */
	wireGuid_antennaGood(pWireGuidData);
  
	// COMPUTE DEVIATION FUNCTION CALL from antenna_calculation.c
	/* CHECK IF ANTENNA HAS FINISHED COLLECTING SAMPLES */
	if (ANT_k >= ANT_k_max)
	{
		#if DISABLE_ADC_ISR_GOERTZEL
		// Disable A/D Interrupt to compute real Final Step exec. time
		IEC0bits.ADIE = 0; // Control Bit for individual enabling/disabling of A/D ISR
		#endif
		#if defined(FUNCTION_CALL)
		/* ########################################### */
		t[1] = clock(); // start final step instruction-counter
		/* ########################################### */
		#endif
		ANT_FinalStep(pWireGuidData);
		/* Perform calibration or set deviation to invalid if needed */
		switch (pWireGuidData->calibration_status)
		{
			case WG_CALIB_STATUS_START:
				wireGuid_set_deviation_invalid(pWireGuidData);
				wireGuid_reset_calibration_data(pWireGuidData);
				break;

			case WG_CALIB_STATUS_ONGOING:
				wireGuid_set_deviation_invalid(pWireGuidData);
				wireGuid_calibrate_antenna(pWireGuidData);
				break;

			case WG_CALIB_STORE_PARAM_IN_EEPROM:
				wireGuid_set_deviation_invalid(pWireGuidData);
				wireGuid_store_parameters(pWireGuidData);
				break;

			case WG_CALIB_STATUS_SUCCEEDED:
				#if SECOND_HARMONIC_FIRST_FREQUENCY
				wireGuid_QAM_decode(pWireGuidData);
				#endif
				wireGuid_computeCalibFreqDeviation(pWireGuidData);
				break;

			case WG_CALIB_STATUS_DEFAULT:
				wireGuid_computeDefaultFreqDeviation(pWireGuidData);
				break;

			case WG_CALIB_STATUS_NOTPRESENT:
			case WG_CALIB_STATUS_FAILED:
			default:
				wireGuid_set_deviation_invalid(pWireGuidData);
				break;
		}
		#if defined(FUNCTION_CALL)
		/* ##################################################### */
		t[1] = clock() - t[1]; // update final step function instruction-counter 
		t[2] += t[1];	// update batch instruction-counter
		t[3] += t[2];	// update total instruction-counter
		t[2] = 0; // reset batch instruction-counter
		/* ##################################################### */
		#endif
		#if DISABLE_ADC_ISR_GOERTZEL
		// Re-enable A/D interrupt
		IEC0bits.ADIE = 1;
		#endif
	}
  
	/* Check that BIT succeeded, when active */
	#if BIT_WIREGUID_ACTIVE
	wireGuid_checkBIT(pWireGuidData);
	#endif 
  
	return;
}

#endif
