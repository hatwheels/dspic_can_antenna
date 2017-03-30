// 2014 - 2015

//! \file bitconfiguration.h
//! \brief Built-in test configuration contains test values used in the BIT.
//!         These values are (can be) platform dependent

#ifndef __BIT_CONFIGURATION_H
#define __BIT_CONFIGURATION_H

#include "configuration.h"

/* BIT comparison values for wire guidance */
#if GUIDANCE_WIRE

/*! BIT_ANT_MIN_REFVOLT defines the min measured ADC value for antenna reference
    voltage, to decide that antenna is still good (327 corresponds with 0.4V )*/
#define BIT_ANT_MIN_REFVOLT  (327)

/*! BIT_ANT_MAX_REFVOLT defines the max measured ADC value for antenna reference
    voltage, to decide that antenna is still good (490 corresponds with 0.6V )*/
#define BIT_ANT_MAX_REFVOLT  (490)

/*! BIT_ANT_MAX_REFVOLT_SHORT_CIRCUIT defines the max measured ADC value for
    antenna reference voltage, to decide that antenna is still good (10 corresponds
    with 0.012V )*/
#define BIT_ANT_MAX_REFVOLT_SHORT_CIRCUIT  (10)

/*! BIT_MIN_AMPLITUDE_PILOT defines the minimal amplitude of the pilot tone that
    needs to be present to be valid */
#define BIT_MIN_AMPLITUDE_PILOT  (5)

/*! BIT_MIN_SQUARE_AMPLITUDE_PILOT defines the minimal square amplitude of the pilot tone that
    needs to be present to be valid */
#define BIT_MIN_SQUARE_AMPLITUDE_PILOT  (BIT_MIN_AMPLITUDE_PILOT*BIT_MIN_AMPLITUDE_PILOT)

#endif
#endif // End of __BIT_CONFIGURATION_H definition
