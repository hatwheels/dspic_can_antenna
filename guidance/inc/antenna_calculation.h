// 2014 - 2015

//!  \file antenna_calculation.h
//!  \brief  Contains function declarations for the amplitude calculation of each sample pair 
//!  \brief  of antenna signals (left, right) with the Goertzel algorithm (4 frequencies) ,
//!  \brief  as well as calculation of the relative phase between the 1st Frequency and its
//!  \brief  2nd Harmonic.
//!  \note  This antenna is used for positioning, not for driving.


#ifndef __DSPIC4013F_ANT_H
#define __DSPIC4013F_ANT_H

#include <time.h> // timers for execution time measuring
#include "wireguidance.h"

#define	WG_EEPROM_COEFFS_STORED	(0xBBBB)
// Normal Size of the Hanning Window
#define HN_WDW_SZ	(215)
#if SECOND_HARMONIC_FIRST_FREQUENCY
// Shifting length of the Hanning Window
#define HN_WDW_VAR	(20)
#endif

// Antenna Initialization
void ANT_Initialize(T_wireGuid_t *);

// Needs to be processed once each sample period (called in timer interrupt)
void ANT_Step(int ADValueLeft, int ADValueRight);

// Final step for antenna calculations
void ANT_FinalStep(T_wireGuid_t *);

// Changes Frequency Coefficients when appropriate ID is received through the CAN bus
void ANT_Set_Freqs(Uint8 *, T_wg_coefficient_t *, E_wg_coeff_status_t *);

// Stores updated Frequency values
void ANT_Store_Freqs(T_wg_coefficient_t *, E_wg_coeff_status_t *);

// Loads Frequency values
void ANT_Load_Freqs(T_wg_coefficient_t *, E_wg_coeff_status_t *);

// Global Variables
extern Uint8    ANT_k;
extern Uint8    ANT_k_max;
extern int16    ANT_Deviation[NBR_INPUT_FREQ];
extern int16    AntAmpGainLeft[NBR_INPUT_FREQ];
extern int16    AntAmpGainRight[NBR_INPUT_FREQ];
#if SECOND_HARMONIC_FIRST_FREQUENCY
extern int32    AntRelPhaseLeft[2];
extern int32    AntRelPhaseRight[2];
extern int8     AntRelPhaseLeftSign;
extern int8     AntRelPhaseRightSign;
#endif

#if	DBG_TIME
extern clock_t t[5]; // ticks of step, final step, per batch, total, A/D interrupt
#endif

#endif  // End of __DSPIC4013F_ANT_H definition


