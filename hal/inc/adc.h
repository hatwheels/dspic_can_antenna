// 2014 - 2015

/*! \file adc.h
    \brief Contains function declarations related to the 12-bit ADC
*/

#ifndef __HAL_ADC_H
#define __HAL_ADC_H

#include "systemtypes.h" // for E_LEDColor_t

/* Global variables */
extern int16 ADC_refVoltLeft_1;     /* Needed to check that antenna still ok */
extern int16 ADC_refVoltRight_1;    /* Needed to check that antenna still ok */

/* Function declarations */
void  Adc_init(void);
void  Adc_BlinkLED(E_LEDColor_t ledColor, sbool ledON);

#endif // End of __HAL_ADC_H definition
