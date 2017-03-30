// 2014 - 2015

//! \file project_canantenna.h
//! \brief Main header file

#ifndef	__CANANTENNA
#define	__CANANTENNA

// dsPIC30F specific Libraries
#include <p30f4013.h>	// dsPIC30F4013 processor header
#include <libpic30.h>		// Contains useful helper functions

// Standard C Libraries
#include <stdlib.h>	// General purpose functions
#include <string.h>	// String, array manipulation
#include <math.h>		// Math library
#include <time.h>		// Timing Goertzel Algorithm and CAN Module

// All Headers
#include "stypes.h"	// Definition of types
#include "configuration.h"	// Configuration file
#include "bitconfiguration.h"	// Configuration of Pilot Tone
#include "gen_math.h"	// Header with mathematical functions and defines
#include "interrupts.h"	// Interrupt routines
#include "eeprom.h"		// EEPROM routine
#include "io.h"
#include "adcmonitoring.h"		// Monitor of A/D functionality
#include "systemmonitoring.h"	// System monitoring
#include "outputcompare.h"	// Pilot Tone generation
#include "wireguidance.h"	// Definitions and functions related to the Wire Guidance
#include "guidance.h"		// General Guidance definitions
#include "antenna_calculation.h"		// Definitions and functions related to Goertzel algorithm
#include "can.h"	// Definitions and functions related to the CAN module
#include "systemtypes.h"	// Configuration of system, device
#include "system.h"	// Configuration of system, device
#include "clock.h"	// Internal clock configuration
#include "adc.h"	// Configuration of 12-bit A/D 

#endif	// End of __CANANTENNA definition

