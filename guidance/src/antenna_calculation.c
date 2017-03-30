// 2014 - 2015

//!  \file antenna_calculation.c
//!  \brief Contains the CAN antenna calculations with the Goertzel Algorithm.
//!  \note This antenna is used for positioning, not for driving.


#include "project_canantenna.h"

// Constants
// Constant Gain for the Pilot Tone
static const Uint32 __attribute__((space(auto_psv))) ANT_AMPLITUDE_GAIN = 4000UL;
// 2*PI is used to calculate the Frequency coefficients
static const double __attribute__((space(auto_psv))) math_2pi = (double)MATH_2PI;

// External variables
int16	AntAmpGainLeft[NBR_INPUT_FREQ];
int16	AntAmpGainRight[NBR_INPUT_FREQ];
int16   ANT_Deviation[NBR_INPUT_FREQ];
Uint8	ANT_k;
Uint8	ANT_k_max = (Uint8)HN_WDW_SZ;
#if SECOND_HARMONIC_FIRST_FREQUENCY
int32 AntRelPhaseLeft[2];
int32 AntRelPhaseRight[2];
int8  AntRelPhaseLeftSign;
int8  AntRelPhaseRightSign;
#endif

#if DBG_TIME
clock_t t[5]; // ticks of step, final step, per batch, total, A/D interrupt
#endif

// Internal variables
Uint32  	AntResultLeftFinal[NBR_INPUT_FREQ];
Uint32  	AntResultRightFinal[NBR_INPUT_FREQ];
int16   	AntCoeff[NBR_FREQUENCIES];
#if	SECOND_HARMONIC_FIRST_FREQUENCY
int16 AntSINECoeff[2]; // Sine coefficient of the 1st Input Freq. and its 2nd Harmonic
#endif
int16	   	AntQL[NBR_FREQUENCIES][2];
int16 	AntQR[NBR_FREQUENCIES][2];

static Uint16 Input_Freq_Table[NBR_INPUT_FREQ];

// Prototypes
void    ANT_Initialize(T_wireGuid_t *);
void    ANT_Step(int ADValueLeft, int ADValueRight);
void    ANT_FinalStep(T_wireGuid_t *);
Uint32  ANT_Sqrt(Uint32 r3);

//*****************************************************************************
// Local functions
//*****************************************************************************
//! Initializes the frequency detection on the main board antenna entry.
void ANT_Initialize(T_wireGuid_t *pWireGuidData)
{
    // Local counting variables
    Uint8 i;
	
	// Initialize Timers
	#if DBG_TIME
	t[0] = 0;	// instruction-counter of step function
	t[1] = 0;	// instruction-counter of final step function
	t[2] = 0;	// instruction-counter of each whole batch processed (215 * step + final step)
	t[3] = 0;	// instruction-counter of all batches. never reset, except if program restarts (-> resets in antenna init)
	#endif

    // For Input Frequencies
    for (i = 0U; i < NBR_INPUT_FREQ; ++i)
    {
        //Initialize Filter State variables
		// With Test Freq: Test Freq. + All Input Freq. filter states EXCEPT filter state of last Input Freq. are initialized (w/o 2nd Harmonic)
		// Without Test Freq: All Input Freq. are initialized (w/o 2nd Harmonic)
        AntQL[i][0] = 0;
        AntQL[i][1] = 0;
        AntQR[i][0] = 0;
        AntQR[i][1] = 0;

		// Initialize Resulting Amplitudes
		AntResultLeftFinal[i] = 0UL;
		AntResultRightFinal[i] = 0UL;
		pWireGuidData->amplitudeLeft[i]  = AntResultLeftFinal[i];
		pWireGuidData->amplitudeRight[i]  = AntResultRightFinal[i];

		// Initialize Deviations
		ANT_Deviation[i] = WG_DEVIATION_INVALID;
		pWireGuidData->deviation_m2ecm[i] = ANT_Deviation[i];
		
		// Initialize Calibration Gains
		AntAmpGainLeft[i] = pWireGuidData->calibration_left.calibration_param[i];
		AntAmpGainRight[i] = pWireGuidData->calibration_right.calibration_param[i];
    }

	// Takes into account Pilot Tone, 2nd Harmonic 
	#if BIT_WIREGUID_ACTIVE // Test Freq.
		// Initialize Resulting Amplitudes of PWM
		pWireGuidData->amplitudePWM[0] = 0U;
		pWireGuidData->amplitudePWM[1] = 0U;
		#if SECOND_HARMONIC_FIRST_FREQUENCY // 2nd harmonic of the the first input freq. enabled
		// Initialize Filter State of the 2nd Harmonic.
		AntQL[NBR_FREQUENCIES - 1][0] = 0; // PWM enabled, NBR_FREQUENCIES - 1 = NBR_INPUT_FREQ + SECOND_HARMONIC_FIRST_FREQUENCY
		AntQL[NBR_FREQUENCIES - 1][1] = 0;
		AntQR[NBR_FREQUENCIES - 1][0] = 0;
		AntQR[NBR_FREQUENCIES - 1][1] = 0;
		// Initialize Filter State of the last Input Freq.
		AntQL[NBR_FREQUENCIES - 2][0] = 0; // PWM enabled, NBR_FREQUENCIES - 2 = NBR_INPUT_FREQ
		AntQL[NBR_FREQUENCIES - 2][1] = 0;
		AntQR[NBR_FREQUENCIES - 2][0] = 0;
		AntQR[NBR_FREQUENCIES - 2][1] = 0;
		// Initialize relative Phase between 1st Input Freq and its 2nd Harmonic.
		AntRelPhaseLeft[0] = 0L;
		AntRelPhaseLeft[1] = 0L;
		AntRelPhaseRight[0] = 0L;
		AntRelPhaseRight[1] = 0L;
		pWireGuidData->rel_phaseLeft[0] = (int16)AntRelPhaseLeft[0];
		pWireGuidData->rel_phaseLeft[1] = (int16)AntRelPhaseLeft[1];
		pWireGuidData->rel_phaseRight[0] = (int16)AntRelPhaseRight[0];
		pWireGuidData->rel_phaseRight[1] = (int16)AntRelPhaseRight[1];
		pWireGuidData->rel_phaseHIGH[0] = 0;
		pWireGuidData->rel_phaseHIGH[1] = 0;
		// Initialize phase direction correction parameters
		AntRelPhaseLeftSign = 1;
		AntRelPhaseRightSign = 1;
		pWireGuidData->rel_phaseLeft_sign = AntRelPhaseLeftSign;
		pWireGuidData->rel_phaseRight_sign = AntRelPhaseRightSign;
		pWireGuidData->direction_checked = false;
		//
		pWireGuidData->switch_state_MSN = 0x00U;
		pWireGuidData->switch_state_LSN  = 0x00U;
		pWireGuidData->switch_states_to_be_sent = 0x00U;
		pWireGuidData->tx_new_states = false;
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
		pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_UNKNOWN;
		// Store Default Frequency values to a constant table.
			#if (NBR_FREQUENCIES > 6)
				#error "Too many Input Frequencies! Maximum are 4 Input + 2nd Harmonic + 1 Test Frequencies."
			#elif (NBR_FREQUENCIES == 6)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			Input_Freq_Table[3] = FREQ4_HZ;
			#elif (NBR_FREQUENCIES == 5)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			#elif (NBR_FREQUENCIES == 4)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			#elif (NBR_FREQUENCIES == 3)
			Input_Freq_Table[0] = FREQ1_HZ;
			#else
				#error "No input frequencies! Check configuration (configuration.h)"
			#endif
		#else // No 2nd Harmonic
		// Initialize Filter State of the last Input Freq.
		AntQL[NBR_FREQUENCIES - 1][0] = 0;	// NBR_FREQUENCIES - 1 = NBR_INPUT_FREQ when PWM enabled
		AntQL[NBR_FREQUENCIES - 1][1] = 0;
		AntQR[NBR_FREQUENCIES - 1][0] = 0;
		AntQR[NBR_FREQUENCIES - 1][1] = 0;
		// Store Default Frequency values to a constant table.
			#if (NBR_FREQUENCIES > 5)
				#error "Too many Input Frequencies! Maximum are 4 Input + 1 Test Frequencies."
			#elif (NBR_FREQUENCIES == 5)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			Input_Freq_Table[3] = FREQ4_HZ;
			#elif (NBR_FREQUENCIES == 4)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			#elif (NBR_FREQUENCIES == 3)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			#elif (NBR_FREQUENCIES == 2)
			Input_Freq_Table[0] = FREQ1_HZ;
			#else
				#error "No input frequencies! Check configuration (configuration.h)"
			#endif
		#endif // End 2nd Harmonic enabled/disabled
		// Load Frequency values, calculate Frequency coefficients. Stored in wiredguidance struct.
		ANT_Load_Freqs(pWireGuidData->frequencies, &(pWireGuidData->freq_status));
		
	#else // No Test Freq.
		#if SECOND_HARMONIC_FIRST_FREQUENCY // 2nd harmonic of the the first input freq. enabled
		// Initialize Filter State of the 2nd Harmonic.
		AntQL[NBR_FREQUENCIES - 1][0] = 0; // PWM disabled, NBR_FREQUENCIES - 1 = NBR_INPUT_FREQ + SECOND_HARMONIC_FIRST_FREQUENCY
		AntQL[NBR_FREQUENCIES - 1][1] = 0;
		AntQR[NBR_FREQUENCIES - 1][0] = 0;
		AntQR[NBR_FREQUENCIES - 1][1] = 0;
		// Initialize relative Phase between 1st Input Freq and its 2nd Harmonic.
		AntRelPhaseLeft[0] = 0L;
		AntRelPhaseLeft[1] = 0L;
		AntRelPhaseRight[0] = 0L;
		AntRelPhaseRight[1] = 0L;
		pWireGuidData->rel_phaseLeft[0] = (int16)AntRelPhaseLeft[0];
		pWireGuidData->rel_phaseLeft[1] = (int16)AntRelPhaseLeft[1];
		pWireGuidData->rel_phaseRight[0] = (int16)AntRelPhaseRight[0];
		pWireGuidData->rel_phaseRight[1] = (int16)AntRelPhaseRight[1];
		pWireGuidData->rel_phaseHIGH[0] = 0;
		pWireGuidData->rel_phaseHIGH[1] = 0;
		// Initialize phase direction correction parameters
		AntRelPhaseLeftSign = 1;
		AntRelPhaseRightSign = 1;
		pWireGuidData->rel_phaseLeft_sign = AntRelPhaseLeftSign;
		pWireGuidData->rel_phaseRight_sign = AntRelPhaseRightSign;
		pWireGuidData->direction_checked = false;
		//
		pWireGuidData->switch_state_MSN = 0x00U;
		pWireGuidData->switch_state_LSN  = 0x00U;
		pWireGuidData->switch_states_to_be_sent = 0x00U;
		pWireGuidData->tx_new_states = false;
		pWireGuidData->nibble_status = WG_NIBBLE_STATUS_UNKNOWN;
		pWireGuidData->nibble_substatus = WG_NIBBLE_SUBSTATUS_UNKNOWN;
		// Store Default Frequency values to a constant table.
			#if (NBR_FREQUENCIES > 5)
				#error "Too many Input Frequencies! Maximum are 4 Input Frequencies + 2nd Harmonic."
			#elif (NBR_FREQUENCIES == 5)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			Input_Freq_Table[3] = FREQ4_HZ;
			#elif (NBR_FREQUENCIES == 4)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			#elif (NBR_FREQUENCIES == 3)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			#elif (NBR_FREQUENCIES == 2)
			Input_Freq_Table[0] = FREQ1_HZ;
			#else
				#error "No input frequencies! Check configuration (configuration.h)"
			#endif
		#else // No 2nd Harmonic
		// Store Default Frequency values to a constant table.
			#if (NBR_FREQUENCIES > 4)
				#error "Too many Input Frequencies! Maximum are 4 Input Frequencies."
			#elif (NBR_FREQUENCIES == 4)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			Input_Freq_Table[3] = FREQ4_HZ;
			#elif (NBR_FREQUENCIES == 3)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			Input_Freq_Table[2] = FREQ3_HZ;
			#elif (NBR_FREQUENCIES == 2)
			Input_Freq_Table[0] = FREQ1_HZ;
			Input_Freq_Table[1] = FREQ2_HZ;
			#elif (NBR_FREQUENCIES == 1)
			Input_Freq_Table[0] = FREQ1_HZ;
			#else
				#error "No input frequencies! Check configuration (configuration.h)"
			#endif
		#endif // End 2nd Harmonic enabled/disabled
		// Load Frequency values, calculate Frequency coefficients. Stored in wiredguidance struct. 
		ANT_Load_Freqs(pWireGuidData->frequencies, &(pWireGuidData->freq_status));
	#endif // End test freq enabled/disabled

    /// RESET SAMPLE COUNTER
    ANT_k = 0U;

	return;
}

//*****************************************************************************
//! This function calculates the new deviations, time presents and so on when a new set of data
//! is calculated.
void ANT_FinalStep(T_wireGuid_t *pWireGuidData)
{
	/* Implement internal timer */
	#ifdef FUNCTION_INTERNAL
	t[1] = clock(); // start final step instruction-counter
	#endif

	// Local variable
    Uint8  i;

	//Local Filter States declaration
	int16 Q_left[NBR_FREQUENCIES][2];
	int16 Q_right[NBR_FREQUENCIES][2];
	// Copy Filter States to locals and reset Filter States, Sample counter
    for (i = 0U; i < NBR_FREQUENCIES; ++i)
    {
		// Local Filter States copies
		Q_left[i][0] = AntQL[i][0];
		Q_left[i][1] = AntQL[i][1];
		Q_right[i][0] = AntQR[i][0];
		Q_right[i][1] = AntQR[i][1];

		/// Reset Filter States
    	AntQL[i][0] = 0;
       	AntQL[i][1] = 0;
       	AntQR[i][0] = 0;
       	AntQR[i][1] = 0;
   	}
    // Reset Sample counter
    ANT_k = 0U;

#if BIT_WIREGUID_ACTIVE // Test Frequency enabled
	// Calculate amplitude for left/right channel, for all Input Frequencies +
    // Test Frequency (only square), calculate squared amplitudes and square roots.
    for (i = 0U; i < NBR_INPUT_FREQ; ++i)
    {
		// Calculate square amplitudes for Test Frequency
		if (i == 0U)
		{
			// Left channel Test Frequency
			pWireGuidData->amplitudePWM[0] = ((int32)((int32)Q_left[i][1] * (int32)Q_left[i][1]) >> 13)
				+ ((int32)((int32)Q_left[i][0] * (int32)Q_left[i][0]) >> 13)
				- ((int32)(((int32)((int32)Q_left[i][0] * (int32)Q_left[i][1]) >> 15) * (int32)AntCoeff[i]) >> 10);
			// Limit
			if (pWireGuidData->amplitudePWM[0] > 255UL)
					pWireGuidData->amplitudePWM[0] = 255UL;

			// Right channel Test Frequency
			pWireGuidData->amplitudePWM[1] = ((int32)((int32)Q_right[i][1] * (int32)Q_right[i][1]) >> 13)
				+ ((int32)((int32)Q_right[i][0] * (int32)Q_right[i][0]) >> 13)
				- ((int32)(((int32)((int32)Q_right[i][0] * (int32)Q_right[i][1]) >> 15) * (int32)AntCoeff[i]) >> 10);
			// Limit
			if (pWireGuidData->amplitudePWM[1] > 255UL)
					pWireGuidData->amplitudePWM[1] = 255UL;
		}
	
	    // Local variable
        Uint32  TempResult = 0UL;
	
		// Left channel Input Frequencies
        TempResult = ((int32)((int32)Q_left[i+1][1] * (int32)Q_left[i+1][1]) >> 13)
			+ ((int32)((int32)Q_left[i+1][0] * (int32)Q_left[i+1][0]) >> 13)
            - ((int32)(((int32)((int32)Q_left[i+1][0] * (int32)Q_left[i+1][1]) >> 15) * (int32)AntCoeff[i+1]) >> 10);

        // Take square root and limit
        pWireGuidData->amplitudeLeft[i] = ANT_Sqrt(TempResult);
        if (pWireGuidData->amplitudeLeft[i] > 255UL) 
				pWireGuidData->amplitudeLeft[i] = 255UL; /* 255 = 2^8 - 1 -> 8bits = 1 byte (short type)
                                                            limited to fit into CAN transmit buffer */

		// Right channel Input Frequencies
        TempResult = ((int32)((int32)Q_right[i+1][1] * (int32)Q_right[i+1][1]) >> 13)
            + ((int32)((int32)Q_right[i+1][0] * (int32)Q_right[i+1][0]) >> 13)
            - ((int32)(((int32)((int32)Q_right[i+1][0] * (int32)Q_right[i+1][1]) >> 15) * (int32)AntCoeff[i+1]) >> 10);

        // Take square root and limit
        pWireGuidData->amplitudeRight[i] = ANT_Sqrt(TempResult);
        if (pWireGuidData->amplitudeRight[i] > 255UL) 
            pWireGuidData->amplitudeRight[i] = 255UL; /* 255 = 2^8 - 1 -> 8 bits = 1 byte (short type)
														limited to fit into CAN transmit buffer */

		// Copy amplitude results to variables to avoid ISR overwrite
		AntResultLeftFinal[i] = pWireGuidData->amplitudeLeft[i];
		AntResultRightFinal[i] = pWireGuidData->amplitudeRight[i];
    }
	// Calculate deviations and, if enabled, relative phase
    #if SECOND_HARMONIC_FIRST_FREQUENCY // 2nd Harmonic enabled
    // Calculate relative phase between 1st Input Freq. and its 2nd Harmonic
    {
        // Local variables
        /* Cosine and Sine of 2nd Harmonic phase */
        int32	phi_2ndHarmonic[] = { ((int32)((int32)Q_left[NBR_FREQUENCIES-1][0] * (int32)AntCoeff[NBR_FREQUENCIES-1]) >> 13)
                                                - (int32)Q_left[NBR_FREQUENCIES-1][1], 
                                                (int32)((int32)Q_left[NBR_FREQUENCIES-1][0] * (int32)AntSINECoeff[1]) >> 13 };
        /* Cosine and Sine of 1st Input Freq. */
        int32 phi_1stFreq[] = { ((int32)((int32)Q_left[1][0] * (int32)AntCoeff[1]) >> 13) - (int32)Q_left[1][1],
                                        (int32)((int32)Q_left[1][0] * (int32)AntSINECoeff[0]) >> 13 };
        /* Twice Angle Cosine and Sine of 1st Input Freq. */
        int32	phi_double_1stFreq[] = { ((int32)(phi_1stFreq[0] * phi_1stFreq[0]) - (int32)(phi_1stFreq[1] * phi_1stFreq[1])) >> 13,
                                                    (int32)(phi_1stFreq[0] * phi_1stFreq[1]) >> 12 };
        /* Relative Phase normalizer */
        Uint32 normalize_phi = (Uint32)(AntResultLeftFinal[0] * AntResultLeftFinal[0] * AntResultLeftFinal[0] * ANT_Sqrt(8192UL) / 100UL);
    
        // Relative Phase Calculation, Scaled Linear Factor of 100
        // Normalized Cosine -> In-Phase component
        AntRelPhaseLeft[0] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[0]) 
                                        + (int32)(phi_double_1stFreq[1] * phi_2ndHarmonic[1])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseLeft[0] = (int16)AntRelPhaseLeft[0];
        // Normalized Sine -> Quadrature Component
        AntRelPhaseLeft[1] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[1]) 
                                        - (int32)(phi_double_1stFreq[1] * phi_2ndHarmonic[0])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseLeft[1] = (int16)AntRelPhaseLeft[1];
    
        /* Right Channel */			
        /* Cosine and Sine of 2nd Harmonic phase */		
        phi_2ndHarmonic[0] = ((int32)((int32)Q_right[NBR_FREQUENCIES-1][0] * (int32)AntCoeff[NBR_FREQUENCIES-1]) >> 13)
                                        - (int32)Q_right[NBR_FREQUENCIES-1][1];
        phi_2ndHarmonic[1] = (int32)((int32)Q_right[NBR_FREQUENCIES-1][0] * (int32)AntSINECoeff[1]) >> 13;
    
        /* Cosine and Sine of 1st Input Freq. */
        phi_1stFreq[0] = ((int32)((int32)Q_right[1][0] * (int32)AntCoeff[1]) >> 13) - (int32)Q_right[1][1];
        phi_1stFreq[1] = (int32)((int32)Q_right[1][0] * (int32)AntSINECoeff[0]) >> 13;
    
        /* Twice Angle Cosine and Sine of 1st Input Freq. */
        phi_double_1stFreq[0] = ((int32)(phi_1stFreq[0] * phi_1stFreq[0]) - (int32)(phi_1stFreq[1] * phi_1stFreq[1])) >> 13;
        phi_double_1stFreq[1] = (int32)(phi_1stFreq[0] * phi_1stFreq[1]) >> 12;
    
        /* Relative Phase normalizer */
        normalize_phi = (Uint32)(AntResultRightFinal[0] * AntResultRightFinal[0] * AntResultRightFinal[0] * ANT_Sqrt(8192UL) / 100UL);
    
        // Relative Phase Calculation, Scaled Linear Factor of 100
        // Normalized Cosine -> In-Phase component
        AntRelPhaseRight[0] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[0]) 
                                            + (int32)(phi_double_1stFreq[1] * phi_2ndHarmonic[1])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseRight[0] = (int16)AntRelPhaseRight[0];
        // Normalized Sine -> Quadrature Component
        AntRelPhaseRight[1] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[1]) 
                                            - (int32)(phi_double_1stFreq[1] * phi_2ndHarmonic[0])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseRight[1] = (int16)AntRelPhaseRight[1];
    }
    #endif // End 2nd Harmonic check

#else // No test Frequency
	// Calculate amplitude for left/right channel, for all Input Frequencies
    // Calculated squared amplitudes and square roots
    for (i = 0U; i < NBR_INPUT_FREQ; ++i)
    {
	    // Local variable
        Uint32  TempResult = 0UL;

        // Left channel Goertzel Formula
        TempResult = ((int32)((int32)Q_left[i][1] * (int32)Q_left[i][1]) >> 13)
                     + ((int32)((int32)Q_left[i][0] * (int32)Q_left[i][0]) >> 13)
                     - ((int32)(((int32)((int32)Q_left[i][0] * (int32)Q_left[i][1]) >> 15) * (int32)AntCoeff[i]) >> 10);

        // Take square root and limit
        pWireGuidData->amplitudeLeft[i] = ANT_Sqrt(TempResult);
        if(pWireGuidData->amplitudeLeft[i] > 255UL)
            pWireGuidData->amplitudeLeft[i] = 255UL; /* 255 = 2^8 - 1 -> 8bits = 1 byte (short type)
                                                        limited to fit into CAN transmit buffer */

        // Right channel Goertzel formula
        TempResult = ((int32)((int32)Q_right[i][1] * (int32)Q_right[i][1]) >> 13)
                     + ((int32)((int32)Q_right[i][0] * (int32)Q_right[i][0]) >> 13)
                     - ((int32)(((int32)((int32)Q_right[i][0] * (int32)Q_right[i][1]) >> 15) * (int32)AntCoeff[i]) >> 10);

        // Take square root and limit
        pWireGuidData->amplitudeRight[i] = ANT_Sqrt(TempResult);
        if(pWireGuidData->amplitudeRight[i] > 255UL) 
            pWireGuidData->amplitudeRight[i] = 255UL; /* 255 = 2^8 - 1 -> 8 bits = 1 byte (short type)
                                                         limited to fit into CAN transmit buffer */

		// Copy amplitude results to variables to avoid ISR overwrite
		AntResultLeftFinal[i] = pWireGuidData->amplitudeLeft[i];
		AntResultRightFinal[i] = pWireGuidData->amplitudeRight[i];
    }
    #if SECOND_HARMONIC_FIRST_FREQUENCY // 2nd Harmonic enabled
    // Calculate relative phase between 1st Input Freq. and its 2nd Harmonic
    {
        /* Left Channel */
    
        // Local variables
        /* Cosine and Sine of 2nd Harmonic phase */
        int32	phi_2ndHarmonic[] = {
            ((int32)((int32)Q_left[NBR_FREQUENCIES-1][0] * (int32)AntCoeff[NBR_FREQUENCIES-1]) >> 13) -
                (int32)Q_left[NBR_FREQUENCIES-1][1], 
            (int32)((int32)Q_left[NBR_FREQUENCIES-1][0] * (int32)AntSINECoeff[1]) >> 13
        };
        /* Cosine and Sine of 1st Input Freq. */
        int32 phi_1stFreq[] = {
            ((int32)((int32)Q_left[0][0] * (int32)AntCoeff[0]) >> 13) - (int32)Q_left[0][1],
            (int32)((int32)Q_left[0][0] * (int32)AntSINECoeff[0]) >> 13
        };
        /* Twice Angle Cosine and Sine of 1st Input Freq. */
        int32	phi_double_1stFreq[] = {
            ((int32)(phi_1stFreq[0] * phi_1stFreq[0]) - (int32)(phi_1stFreq[1] * phi_1stFreq[1])) >> 13,
            (int32)(phi_1stFreq[0] * phi_1stFreq[1]) >> 12
        };
        /* Relative Phase normalizer */ 
        Uint32 normalize_phi = (Uint32)(AntResultLeftFinal[0] * AntResultLeftFinal[0] * AntResultLeftFinal[0] *
            ANT_Sqrt(8192UL) / 100UL);
    
        // Relative phase calculation, Scaled Linear Factor of 100
        // Normalized Cosine
        AntRelPhaseLeft[0] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[0]) 
            + (int32)(phi_double_1stFreq[1] * phi_2ndHarmonic[1])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseLeft[0] = (int16)AntRelPhaseLeft[0];
        // Normalize Sine
        AntRelPhaseLeft[1] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[1]) 
            - (int32)(phi_double_1stFreq[1] * phi_2ndHarmonic[0])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseLeft[1] = (int16)AntRelPhaseLeft[1];

        /* Right Channel */
    
        phi_2ndHarmonic[0] = ((int32)((int32)Q_right[NBR_FREQUENCIES-1][0] *
            (int32)AntCoeff[NBR_FREQUENCIES-1]) >> 13) - (int32)Q_right[NBR_FREQUENCIES-1][1];
        phi_2ndHarmonic[1] = (int32)((int32)Q_right[NBR_FREQUENCIES-1][0] * (int32)AntSINECoeff[1]) >> 13;
    
        phi_1stFreq[0] = ((int32)((int32)Q_right[0][0] * (int32)AntCoeff[0]) >> 13) - (int32)Q_right[0][1];
        phi_1stFreq[1] = (int32)((int32)Q_right[0][0] * (int32)AntSINECoeff[0]) >> 13;
    
        phi_double_1stFreq[0] = ((int32)(phi_1stFreq[0] * phi_1stFreq[0]) -
            (int32)(phi_1stFreq[1] * phi_1stFreq[1])) >> 13;
        phi_double_1stFreq[1] = (int32)(phi_1stFreq[0] * phi_1stFreq[1]) >> 12;
    
        normalize_phi = (Uint32)(AntResultRightFinal[0] * AntResultRightFinal[0] * AntResultRightFinal[0] *
            ANT_Sqrt(8192UL) / 100UL);

        // Relative Phase Calculation, Scaled Linear Factor of 100
        // Normalized Cosine
        AntRelPhaseRight[0] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[0]) + (int32)(phi_double_1stFreq[1] *
            phi_2ndHarmonic[1])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseRight[0] = (int16)AntRelPhaseRight[0];
        // Normalized Sine
        AntRelPhaseRight[1] = ((int32)(phi_double_1stFreq[0] * phi_2ndHarmonic[1]) -
            (int32)(phi_double_1stFreq[1] * phi_2ndHarmonic[0])) / ((int32)normalize_phi);
        pWireGuidData->rel_phaseRight[1] = (int16)AntRelPhaseRight[1];
    }
    #endif // End 2nd Harmonic check	
#endif // End Test Freq. / No Test Freq. branching

	/* End timer */
	#ifdef FUNCTION_INTERNAL
	t[1] = clock() - t[1]; // update final step function instruction-counter 
	t[2] += t[1];	// update batch instruction-counter
	t[3] += t[2];	// update total instruction-counter
	t[2] = 0; // reset batch instruction-counter
	#endif

	return;
}

//*****************************************************************************
//! Goertzel calculations for the k-th sample, for all (1...4) Frequencies
void ANT_Step(int16 ADValueLeft, int16 ADValueRight)
{
	/* Implement internal timer */
	// start step instruction-counter
	#ifdef FUNCTION_INTERNAL
	t[0] = clock(); // start step instruction-counter
	#endif

    // Hanning Filter coefficients (N=215) (Keep in RAM: No const)
    static const Uint16 __attribute__((space(auto_psv)))
	Hanning[(Uint8)(HN_WDW_SZ+HN_WDW_VAR)] = {
        2U,      16U,     44U,     86U,     141U,    211U,    295U,    392U,    503U,
        627U,    765U,    917U,    1081U,   1259U,   1449U,   1652U,   1868U,   2096U,   2337U,
        2589U,   2853U,   3129U,   3416U,   3714U,   4023U,   4343U,   4672U,   5012U,   5362U,
        5721U,   6089U,   6466U,   6851U,   7244U,   7645U,   8054U,   8470U,   8893U,   9322U,
        9756U,   10197U,  10643U,  11094U,  11549U,  12009U,  12472U,  12939U,  13408U,  13880U,
        14354U,  14830U,  15307U,  15786U,  16264U,  16743U,  17222U,  17699U,  18176U,  18651U,
        19124U,  19595U,  20063U,  20528U,  20989U,  21447U,  21900U,  22349U,  22792U,  23230U,
        23662U,  24088U,  24507U,  24919U,  25324U,  25721U,  26111U,  26492U,  26864U,  27228U,
        27582U,  27927U,  28262U,  28586U,  28901U,  29204U,  29497U,  29778U,  30048U,  30307U,
        30553U,  30787U,  31009U,  31219U,  31416U,  31600U,  31771U,  31929U,  32073U,  32205U,
        32322U,  32426U,  32517U,  32593U,  32656U,  32705U,  32740U,  32761U,  32767U,  32761U,
        32740U,  32705U,  32656U,  32593U,  32517U,  32426U,  32322U,  32205U,  32073U,  31929U,
        31771U,  31600U,  31416U,  31219U,  31009U,  30787U,  30553U,  30307U,  30048U,  29778U,
        29497U,  29204U,  28901U,  28586U,  28262U,  27927U,  27582U,  27228U,  26864U,  26492U,
        26111U,  25721U,  25324U,  24919U,  24507U,  24088U,  23662U,  23230U,  22792U,  22349U,
        21900U,  21447U,  20989U,  20528U,  20063U,  19595U,  19124U,  18651U,  18176U,  17699U,
        17222U,  16743U,  16264U,  15786U,  15307U,  14830U,  14354U,  13880U,  13408U,  12939U,
        12472U,  12009U,  11549U,  11094U,  10643U,  10197U,  9756U,   9322U,   8893U,   8470U,
        8054U,   7645U,   7244U,   6851U,   6466U,   6089U,   5721U,   5362U,   5012U,   4672U,
        4343U,   4023U,   3714U,   3416U,   3129U,   2853U,   2589U,   2337U,   2096U,   1868U,
        1652U,   1449U,   1259U,   1081U,   917U,    765U,    627U,    503U,    392U,    295U,
        211U,    141U,    86U,     44U,     16U,     2U,
		//1        2	  3       4			5		 6		  7		   8		9
		2U,      16U,     44U,     86U,     141U,    211U,    295U,    392U,    503U,
		//10	 11		  12	   13		14		 15		  16	   17		18		 19
        627U,    765U,    917U,    1081U,   1259U,   1449U,   1652U,   1868U,   2096U,   2337U,
		//20 = HN_WDW_VAR
        2589U
		}; // Normal Hanning Window 215-long. Added in the end the first HN_WDW_VAR elements for 
		// synchronization. Using modulo is slower

    // Local variables
    int16  ValueL;
    int16  ValueR;
    int32  SampleL;
    int32  SampleR;
    int32  TempCalc;

	// counter
	Uint8 i;

    // Take sample for left and right, and apply Hanning window
    ValueL = (int16)(((int32)ADValueLeft * (int32)Hanning[ANT_k]) >> 15) * (int16)AntRelPhaseLeftSign; // 15 bits (32768) hanning window scaled to 2^15
    ValueR = (int16)(((int32)ADValueRight * (int32)Hanning[ANT_k]) >> 15) * (int16)AntRelPhaseRightSign; // 15 bits 
	
	/// For all Frequencies (Test Freq, Input Freqs, 2nd Harmonic)
	for (i = 0U; i < NBR_FREQUENCIES; ++i)
	{
		#if BIT_WIREGUID_ACTIVE
		if (i == 0U)
		{
			// Apply Gain from Parameters
			// shift by 13 bits left (2^13 = 8192) -> normalization of the frequency gain
			SampleL = ((int32)ValueL * ANT_AMPLITUDE_GAIN)  >> 13;
			SampleR = ((int32)ValueR * ANT_AMPLITUDE_GAIN) >> 13;
		}
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			else if (i == (NBR_FREQUENCIES-1))
			{
				// Apply Gain from Parameters
				// shift by 13 bits left (2^13 = 8192) -> normalization of the frequency gain
				SampleL = ((int32)ValueL * (int32)AntAmpGainLeft[0])  >> 13;
				SampleR = ((int32)ValueR * (int32)AntAmpGainRight[0]) >> 13;
			}
			#endif
		else
		{
			// Apply Gain from Parameters
			// shift by 13 bits left (2^13 = 8192) -> normalization of the frequency gain
			SampleL = ((int32)ValueL * (int32)AntAmpGainLeft[i-1])  >> 13;
			SampleR = ((int32)ValueR * (int32)AntAmpGainRight[i-1]) >> 13;
		}
		#else // Case No Pilot Tone
		if (i < (NBR_FREQUENCIES-1))
		{
			// Apply Gain from Parameters
			// shift by 13 bits left (2^13 = 8192) -> normalization of the frequency gain
			SampleL = ((int32)ValueL * (int32)AntAmpGainLeft[i])  >> 13;
			SampleR = ((int32)ValueR * (int32)AntAmpGainRight[i]) >> 13;
		}
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			else
			{
				// Apply Gain from Parameters
				// shift by 13 bits left (2^13 = 8192) -> normalization of the frequency gain
				SampleL = ((int32)ValueL * (int32)AntAmpGainLeft[0])  >> 13;
				SampleR = ((int32)ValueR * (int32)AntAmpGainRight[0]) >> 13;
			}
			#endif
		#endif // End Pilot Tone 
		
		// Goertzel for left channel
		TempCalc	= (((int32)AntCoeff[i] * (int32)AntQL[i][1]) >> 12) - (int32)AntQL[i][0] + SampleL; // 12 bits (4096)
		AntQL[i][0] = AntQL[i][1];
		AntQL[i][1] = (int16) TempCalc;
		// Goertzel for right channel
		TempCalc = (((int32)AntCoeff[i] * (int32)AntQR[i][1]) >> 12) - (int32)AntQR[i][0] + SampleR; // 12 bits
		AntQR[i][0] = AntQR[i][1];
		AntQR[i][1] = (int16) TempCalc;
	}
	
    // Increase Sample counter
    ++ANT_k;

	/* End timer */
	#ifdef FUNCTION_INTERNAL
	t[0] = clock() - t[0]; // stores nbr of instructions required by step function
	if (ANT_k != 1U) // due to incrementation @l.357 (++ANT_k), range is 1 -> 215
		t[2] += t[0]; // update per-batch instruction-counter
	else
		t[2] = t[0]; // new batch instruction-counter
	#endif
	
	return;
}

//*****************************************************************************
//! This function implements a square root for this antenna.
Uint32 ANT_Sqrt(Uint32 r3)
{
    // Local variables
    Uint32 t3,b3,c3;

    // Initialize the result
    c3 = 0UL;

    // Iteration
    for (b3 = 0x10000000UL; b3 != 0UL; b3 >>= 2)
    {
        t3 = c3 + b3;
        c3 >>= 1;
        if (t3 <= r3)
        {
            r3 -= t3; 
            c3 += b3;
        }
    }

    // Return the result
    return(c3);
}

//*****************************************************************************
//! This function calculates the Coefficients of the Frequency values 
//!	received through the CAN Bus.
void ANT_Set_Freqs(Uint8 *content, T_wg_coefficient_t *frequencies, E_wg_coeff_status_t *status)
{
	// Counter of Input Frequencies.
	Uint8 i;
	// Index of the Frequency Coefficients, takes into account presence 
	// of Test Frequency/Pilot Tone (PWM)
	Uint8 freq_idx;
	
	// Reset Freq. values and Coeffs.
	if	((content[0] & content[1]) == 0xFF)
	// Bitwise comparison: All 1s then 0xFF
	// If at least one bit is 0, then result != 0xFF
	{
		Uint32 read_address;
		Uint16 read_data;
	
		for (i = 0U; i < NBR_INPUT_FREQ; ++i)
		{
			#if BIT_WIREGUID_ACTIVE
			freq_idx = i + 1U;
			#else
			freq_idx = i;
			#endif
			
			// Reset i-th Freq. value to default value
			frequencies[i].freq_value = Input_Freq_Table[i];
			
			double freq_ratio = (double)frequencies[i].freq_value / (double)ADC_SAMPLING_FREQ_Hz ;
			
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			if (i == 0U)
			{
				// Calculate cos, sin coeffs. of 2nd Harmonic and sin coeff. of 1st Freq.
				AntCoeff[NBR_FREQUENCIES-1] = (int16)(cos(2.0 * math_2pi * freq_ratio) * 8192.0);
				AntSINECoeff[0] = (int16)(sin(math_2pi * freq_ratio) * 8192.0);
				AntSINECoeff[1] = (int16)(sin(2.0 * math_2pi * freq_ratio) * 8192.0);
				// Copy coeffs. to data structure and update status of 2nd Harm to default.
				frequencies[i].sin_coefficient = AntSINECoeff[0];
				frequencies[NBR_INPUT_FREQ].freq_value = 2 * frequencies[i].freq_value;
				frequencies[NBR_INPUT_FREQ].cos_coefficient = AntCoeff[NBR_FREQUENCIES-1];
				frequencies[NBR_INPUT_FREQ].sin_coefficient = AntSINECoeff[1];
				frequencies[NBR_INPUT_FREQ].coeff_status = WG_COEFF_STATUS_DEFAULT;
			}
			#endif

			// Calculate default coefficient for the i-th Freq.
			AntCoeff[freq_idx] = (int16)(cos(math_2pi * freq_ratio) * 8192.0);
			// Copy coeff. to data structure and update status of i-th Freq to default.
			frequencies[i].cos_coefficient =  AntCoeff[freq_idx];
			frequencies[i].coeff_status = WG_COEFF_STATUS_DEFAULT;
		}
		// Set overall coefficient status to default.
		*status = WG_COEFF_STATUS_DEFAULT;
		
		// Check whether any user-defined Freq. vales are stored in EEPROM.
		read_address = eeprom_get_read_write_address(EEPROM_ANT_COEFF_DATA_WRITTEN);
		read_data = eeprom_read_word(read_address);
		if (read_data == WG_EEPROM_COEFFS_STORED)
		{
			// Delete stored flag, so that default Freq. values are loaded at system initialization.
			eeprom_write_word(read_address, 0xFFFF);
			for (i = 0U; i < NBR_INPUT_FREQ; ++i)
			{
				read_address = eeprom_get_read_write_address(i+EEPROM_ANT_COEFF_FREQ1);
				read_data = eeprom_read_word(read_address);
				// Delete any user-defined Frequency value.
				if (read_data != 0xFFFFU)
					eeprom_write_word(read_address, 0xFFFF);
			}
		}
		
		return;
	}
	
	// Set new Freq. values and compute new coefficients from CAN message. 
	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		#if BIT_WIREGUID_ACTIVE
		freq_idx = i + 1;
		#else
		freq_idx = i;
		#endif
		
		// If i-th Frequency shall remain intact, its Coefficient is not calculated.
		// Previous value is preserved.
		if ((content[2*i] == 0U) && (content[(2*i)+1] == 0U))
			continue;
		
		// Set overall status to updated, when at least one Freq. is to be updated 
		*status = WG_COEFF_STATUS_UPDATED;
		
		// Copy new value of i-th Freq. to data structure
		frequencies[i].freq_value = (Uint16)content[(2*i)+1] + (((Uint16)content[2*i]) << 8);
		
		double freq_ratio = (double)frequencies[i].freq_value / (double)ADC_SAMPLING_FREQ_Hz;
		
		#if SECOND_HARMONIC_FIRST_FREQUENCY
		if (i == 0U)
		{
			// Calculate cos, sin coeffs. of 2nd Harmonic and sin coeff. of 1st Freq. 
			AntCoeff[NBR_FREQUENCIES-1] = (int16)(cos(2.0 * math_2pi * freq_ratio) * 8192.0);
			AntSINECoeff[0] = (int16)(sin(math_2pi * freq_ratio) * 8192.0);
			AntSINECoeff[1] = (int16)(sin(2.0 * math_2pi * freq_ratio) * 8192.0);
			// Copy coeffs. to data structure, calculate new Freq. and update status of 2nd Harm.
			frequencies[i].sin_coefficient = AntSINECoeff[0];
			frequencies[NBR_INPUT_FREQ].freq_value = 2 * frequencies[i].freq_value;
			frequencies[NBR_INPUT_FREQ].cos_coefficient = AntCoeff[NBR_FREQUENCIES-1];
			frequencies[NBR_INPUT_FREQ].sin_coefficient = AntSINECoeff[1];
			frequencies[NBR_INPUT_FREQ].coeff_status = WG_COEFF_STATUS_UPDATED;
		}
		#endif
		
		// Calculate new coefficient for the i-th Freq.
		AntCoeff[freq_idx] = (int16)(cos(math_2pi * freq_ratio) * 8192.0);
		// Copy coeff. to data structure and update status of i-th Freq.
		frequencies[i].cos_coefficient = AntCoeff[freq_idx];
		frequencies[i].coeff_status = WG_COEFF_STATUS_UPDATED;
	}		
		
	return;
}

//*****************************************************************************
//! This function stores the updated Frequency values to the EEPROM.
//! 
void ANT_Store_Freqs(T_wg_coefficient_t *frequencies, E_wg_coeff_status_t *status)
{
	Uint8 		i;
	Uint32   	write_address;
	sbool	check_write;
	// Counter of the number of updated Freq. values
	Uint8 count_updated = 0U;
	// Counter of the number of failed writings
	Uint8 count_failed = 0U;

	if (*status != WG_COEFF_STATUS_UPDATED)
		return;
	
	for(i = 0U; i < NBR_INPUT_FREQ; ++i)	
	{
		// Check if i-th Freq. was updated w/ new value
		if (frequencies[i].coeff_status != WG_COEFF_STATUS_UPDATED)
			continue;
		
		++count_updated;
		
		write_address = eeprom_get_read_write_address(i+EEPROM_ANT_COEFF_FREQ1);
		check_write = eeprom_write_word(write_address, frequencies[i].freq_value);
		/* Check success/failure of storing process and update 
			i-th freq. status accordingly */
		if (check_write)
		{
			frequencies[i].coeff_status = WG_COEFF_STATUS_USER;
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			if (i == 0U)
				frequencies[NBR_INPUT_FREQ].coeff_status = WG_COEFF_STATUS_USER;
			#endif
		}
		else
		{
			++count_failed;
			frequencies[i].coeff_status = WG_COEFF_STATUS_FAILED;
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			if (i == 0U)
				frequencies[NBR_INPUT_FREQ].coeff_status = WG_COEFF_STATUS_USER;
			#endif
		}
	}
	write_address = eeprom_get_read_write_address(EEPROM_ANT_COEFF_DATA_WRITTEN);
	// Check if storing updated Freqs. to EEPROM failed
	if ((count_failed != 0U) && (count_failed == count_updated))
	{
		eeprom_write_word(write_address, 0xFFFF);
		*status = WG_COEFF_STATUS_FAILED;
		return;
	}
	check_write = eeprom_write_word(write_address, WG_EEPROM_COEFFS_STORED);
	// Check if storing Successful Update value to EEPROM failed
	if (!check_write)
	{
		*status = WG_COEFF_STATUS_FAILED;
		return;
	}
	*status = WG_COEFF_STATUS_USER;
	
	return;
}

//*****************************************************************************
//! This function retrieves the Frequency values from EEPROM.
//! 
/* FREQUENCY COSINE COEFFICIENTS FOR GOERTZEL:
	HEX(8192d x cos(2.pi.Ft/Fs)),
	Fs=15 kHz, Ft=5 kHz[0](Test Freq. enabled), 2790 Hz[1], 
	3209 Hz[2], 3627 Hz[3], 4046 Hz[4], 2*2790=5580 Hz[5] (2nd Harmonic)
	COEFFS: 	DEC: -4096 HEX: 0xF000	[0] (Test Freq.), 
					DEC:  3206 HEX: 0x0C86	[1], 
					DEC:  1842 HEX: 0x0732	[2], 
					DEC:    421 HEX: 0x01A5	[3],
					DEC: -1012 HEX: 0xFC0C	[4], 
					DEC: -5682 HEX: 0xE9CE	[5] (2nd Harmonic).
*/ 
/*	FREQUENCY SINE COEFFS FOR 1ST FREQ AND ITS 2ND HARMONIC:
	HEX(8192d x sin(2.pi.Ft/Fs)),
	Fs=15 kHz, Ft=2790 Hz[0] (1st Freq.), Ft = 2*2790=5580 Hz[1] (2nd Harmonic)
	COEFFS: 	DEC: 7539 HEX: 0x1D73	[0] (1st Freq.),
					DEC: 5901 HEX: 0x170D	[1] (2nd Freq.)
*/
void ANT_Load_Freqs(T_wg_coefficient_t *frequencies, E_wg_coeff_status_t *status		)
{
	Uint8 i;
	Uint32 read_address;
	Uint16 read_data;
	// Index of local Freq. Coeffs., takes into account presence 
	// of Test Frequency/Pilot Tone (PWM)
	Uint8 freq_idx;
	
	read_address = eeprom_get_read_write_address(EEPROM_ANT_COEFF_DATA_WRITTEN);
	read_data = eeprom_read_word(read_address);
	
	// Freq. values are present in EEPROM, 
	if (read_data == WG_EEPROM_COEFFS_STORED)
	{
		// Counter of the number of failed loadings/invalid Freq. values. 
		Uint8 load_fail = 0U;
		*status = WG_COEFF_STATUS_USER;
		for (i = 0U; i < NBR_INPUT_FREQ; ++i)
		{
			read_address = eeprom_get_read_write_address(i+EEPROM_ANT_COEFF_FREQ1);
			frequencies[i].freq_value = eeprom_read_word(read_address);
			
			if (frequencies[i].freq_value == 0xFFFFU)
			{
				/* Use default Freq. value if no stored value present in EEPROM address or loading failed */
				frequencies[i].freq_value = Input_Freq_Table[i];
				double freq_ratio = (double)frequencies[i].freq_value / (double)ADC_SAMPLING_FREQ_Hz;
				frequencies[i].cos_coefficient = (int16)(cos(math_2pi * freq_ratio) * 8192.0);
				frequencies[i].sin_coefficient = 0;
				frequencies[i].coeff_status = WG_COEFF_STATUS_DEFAULT;
				#if SECOND_HARMONIC_FIRST_FREQUENCY
				if (i == 0U)
				{
					frequencies[i].sin_coefficient = (int16)(sin(math_2pi * freq_ratio) * 8192.0);
					frequencies[NBR_INPUT_FREQ].freq_value = 2 * frequencies[i].freq_value;
					frequencies[NBR_INPUT_FREQ].cos_coefficient = (int16)(cos(2.0 * math_2pi * freq_ratio) * 8192.0);
					frequencies[NBR_INPUT_FREQ].sin_coefficient = (int16)(sin(2.0 * math_2pi * freq_ratio) * 8192.0);
					frequencies[NBR_INPUT_FREQ].coeff_status = WG_COEFF_STATUS_DEFAULT;
				}
				#endif
				++load_fail;
			}
			else
			{
				double freq_ratio = (double)frequencies[i].freq_value / (double)ADC_SAMPLING_FREQ_Hz;
				frequencies[i].cos_coefficient = (int16)(cos(math_2pi * freq_ratio) * 8192.0);
				frequencies[i].sin_coefficient = 0;
				frequencies[i].coeff_status = WG_COEFF_STATUS_USER;
				#if SECOND_HARMONIC_FIRST_FREQUENCY
				if (i == 0U)
				{
					frequencies[i].sin_coefficient = (int16)(sin(math_2pi * freq_ratio) * 8192.0);
					frequencies[NBR_INPUT_FREQ].freq_value = 2 * frequencies[i].freq_value;
					frequencies[NBR_INPUT_FREQ].cos_coefficient = (int16)(cos(2.0 * math_2pi * freq_ratio) * 8192.0);
					frequencies[NBR_INPUT_FREQ].sin_coefficient = (int16)(sin(2.0 * math_2pi * freq_ratio) * 8192.0);
					frequencies[NBR_INPUT_FREQ].coeff_status = WG_COEFF_STATUS_USER;
				}
				#endif
			}
		}
		// Set Coeff. status to Default, if all loadings failed/no Freq. values were present in EEPROM		
		if (load_fail == NBR_INPUT_FREQ)
			*status = WG_COEFF_STATUS_DEFAULT;
	}
	// No Freq. values are stored in EEPROM, use default Freq. values.
	else
	{
		*status = WG_COEFF_STATUS_DEFAULT;
		for (i = 0U; i < NBR_INPUT_FREQ; ++i)
		{
			frequencies[i].freq_value = Input_Freq_Table[i];
			double freq_ratio = (double)frequencies[i].freq_value / (double)ADC_SAMPLING_FREQ_Hz;
			frequencies[i].cos_coefficient = (int16)(cos(math_2pi * freq_ratio) * 8192.0);
			frequencies[i].sin_coefficient = 0;
			frequencies[i].coeff_status = WG_COEFF_STATUS_DEFAULT;
			#if SECOND_HARMONIC_FIRST_FREQUENCY
			if (i == 0U)
			{
				frequencies[i].sin_coefficient = (int16)(sin(math_2pi * freq_ratio) * 8192.0);
				frequencies[NBR_INPUT_FREQ].freq_value = 2 * frequencies[i].freq_value;
				frequencies[NBR_INPUT_FREQ].cos_coefficient = (int16)(cos(2.0 * math_2pi * freq_ratio) * 8192.0);
				frequencies[NBR_INPUT_FREQ].sin_coefficient = (int16)(sin(2.0 * math_2pi * freq_ratio) * 8192.0);
				frequencies[NBR_INPUT_FREQ].coeff_status = WG_COEFF_STATUS_DEFAULT;
			}
			#endif
		}
	}
	//	Copy loaded Frequency Coefficients to local variables
	for (i = 0U; i < NBR_INPUT_FREQ; ++i)
	{
		#if BIT_WIREGUID_ACTIVE
		freq_idx = i + 1;
		#else
		freq_idx = i;
		#endif

		AntCoeff[freq_idx] = frequencies[i].cos_coefficient;
	}
	#if BIT_WIREGUID_ACTIVE
	AntCoeff[0] = (int16)(cos(math_2pi * ((double)TEST_FREQUENCY_HZ /
										(double)ADC_SAMPLING_FREQ_Hz) ) * 8192.0);
	#endif
	#if SECOND_HARMONIC_FIRST_FREQUENCY
	AntSINECoeff[0] = frequencies[0].sin_coefficient;
	AntCoeff[NBR_FREQUENCIES-1] = frequencies[NBR_INPUT_FREQ].cos_coefficient;
	AntSINECoeff[1] = frequencies[NBR_INPUT_FREQ].sin_coefficient;
	#endif
	
	return;
}
