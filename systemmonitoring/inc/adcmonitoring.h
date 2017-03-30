// 2014 - 2015

/*! \file adcmonitoring.h
    \brief Contains functions related to monitoring of ADC functionality
*/

#ifndef __GEN_ADC_MONITORING_H
#define __GEN_ADC_MONITORING_H

#include "stypes.h"
#include "configuration.h"

#if SYSTEMMONITORING_ADC
/* type definition */
typedef struct{
  Uint16    adcFilteredCurrent;
  Uint16    adcFilteredTemp;
  Uint16    adcFilteredAnalogGnd;
  Uint16    adcFilteredRefVolt;
  
  Uint16    adcRawCurrent;
  Uint16    adcRawTemp;
  Uint16    adcRawAnalogGnd;
  Uint16    adcRawRefVolt;
}T_adcMonData_t;


/* Function declaration */
void adcMon_init(T_adcMonData_t  *adcMonData);

#endif
#endif // End of __GEN_ADC_MONITORING_H definition
