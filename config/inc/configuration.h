// 2014 - 2015

//! \file configuration.h
//! \brief Configure program (application dependent, processor independent)

#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

//*************************************************************************************************************
// Define development is ongoing and asserts are allowed to be triggered.
// Shall be 0 when AGV is operational (if 0, all debug output is disabled)
//*************************************************************************************************************
#define DEVELOPMENT  1

//*************************************************************************************************************
// Define compiler
//*************************************************************************************************************
// #define COMPILER_MIKROC_PRO  0
// #define COMPILER_MPLAB       1

//*************************************************************************************************************
//* Guidance defines
//*************************************************************************************************************
#define GUIDANCE_WIRE           1   /* - WHEN USED, CHECK ANTENNA AND FREQUENCY PARAMETERS BELOW */
#define GUIDANCE_CAMERA         0
#define GUIDANCE_LASER          0   /* - WHEN GOETTING LASER IS USED, RS232 CAN BE
														  USED FOR ANOTHER DEVICE.
														  WHEN PSI NAVIGATOR (GUIDANCE NAV) IS USED,
														  RS232 SHALL BE USED FOR  THIS LASER */
#define GUIDANCE_STARGAZER  0

//*** Define sensor and other defines for laser navigation
#if GUIDANCE_LASER
	#define LASERSENSOR_GUIDANCENAV   1
	#define LASERSENSOR_GOETTING      0

	#define POSLAS_MAX_NBR_REFLECTORS 100
#else
	#define LASERSENSOR_GUIDANCENAV   0
	#define LASERSENSOR_GOETTING      0
#endif

//*** Define parameters for wire guidance, if applicable.
#if GUIDANCE_WIRE
	#define BIT_WIREGUID_ACTIVE     (1)
#if BIT_WIREGUID_ACTIVE
	#define NBR_TEST_FREQ           (1)  // 1 if PWM signal is used for BIT
#else
	#define NBR_TEST_FREQ           (0)
#endif
	#define NBR_ANTENNAS            (1)
	#define NBR_INPUT_FREQ          (4)  // number of frequencies for navigation. Always 4 with sample-by-sample
	#define SECOND_HARMONIC_FIRST_FREQUENCY     1
    // currently: Test Freq. + 4 Inputs + 2nd Harmonic = 6
	#define NBR_FREQUENCIES         (NBR_INPUT_FREQ+SECOND_HARMONIC_FIRST_FREQUENCY+NBR_TEST_FREQ)
	#define TEST_FREQUENCY_HZ       (5000)
	#define	FREQ1_HZ						(2790)
	#define	FREQ2_HZ						(3209)
	#define	FREQ3_HZ						(3627)
	#define	FREQ4_HZ						(4046)
#endif

//*************************************************************************************************************
//* Defines for interface
//*************************************************************************************************************
#define COM_MAX_NBR_CONNECTIONS_RS232     0

//*** Define device that is connected to RS232 (only 1 connection available). If
//    other RS232 connection is needed, see if possible to use RS232_to_CAN
#if (COM_MAX_NBR_CONNECTIONS_RS232 > 0)
  #define COM_MAX_BUFFERSIZE                (30)
  #define RS232_PILZPNOZMULTI               0
  #define RS232_SICK_BARCODESCANNER         0
  #define RS232_SICK_RK512                  0
#if LASERSENSOR_GUIDANCENAV
  #define RS232_LASER_GUIDANCENAV           0
#else
  #define RS232_LASER_GUIDANCENAV           0
#endif
  #define RS232_STARGAZER                   0
#endif

//*************************************************************************************************************
//* System monitoring defines
//*************************************************************************************************************
// Active if temperature, current, analog ground and input voltage are monitored
// from ADC readings
#define SYSTEMMONITORING_ADC                (0)

// Active if battery monitoring system is enabled
#define SYSTEMMONITORING_BATTERY            (0)

//*************************************************************************************************************
//* Hardware configuration defines
//*************************************************************************************************************
#define ADC_SAMPLING_FREQ_Hz    (15000)      /* Hz, shall become define, depending on register configuration */
#define ADC_SAMPLING_INT_sec    (0.0000667)  /* actual sampling interval, 15 Tad's 
																		 of 1111,111... nsec with ADCS =43 and 
																		 4 AN to scan -> Tad*15*4 
																		 (previous TAD = 425nsec w/ ADCS = 16 */

//*************************************************************************************************************
//* Debug defines, shall all be 0 when AGV is operational
//*************************************************************************************************************
#if DEVELOPMENT // Debugging parameters.
	#define	DBG_TIME            0 // sets timers for exec. time meas. of Goertzel Algo + A/D ISR
	#define	DBG_TIME_CAN        0 // sets timers for exec. time meas. of CAN module
	#if BIT_WIREGUID_ACTIVE
		#define  DBG_PWM        0 // Transmit Test Frequency results through CAN
	#endif
	#if DBG_TIME
		#define DISABLE_ADC_ISR_GOERTZEL    1 // Disable A/D interrupt when the resulting amplitudes, deviations and rel. phases are computed in Final Step
		#define SET_DBG_TIME_TYPE           1 // Define time debug type. 1 --> function itself + function call || 0 --> function only (internal)
		#if SET_DBG_TIME_TYPE
			#define FUNCTION_CALL             // function itself + function call
		#else
			#define FUNCTION_INTERNAL         // function only (internal)
		#endif
	#elif DBG_TIME_CAN
        /* Disable A/D interrupt when deviations, amplitudes and antenna status are transmitted 
          * to calculate actual clock ticks of Result, Status, Raw TX
          */
		#define DISABLE_ADC_ISR_CAN         1
        // Define time debug type for CAN. 1 --> function itself + function call || 0 --> function only (internal)
		#define SET_DBG_TIME_CAN_TYPE       1
		#if SET_DBG_TIME_CAN_TYPE
			#define FUNCTION_CALL_CAN           // function itself + function call
		#else
			#define FUNCTION_INTERNAL_CAN       // function only (internal)
		#endif
	#endif
#endif

#endif // End of __CONFIGURATION_H definition

