// 2014 - 2015

/*! \file systemmonitoring.h
    \brief Contains declaration of high-level functions, related to system
			monitoring, for ADC, battery, etc.
*/
#ifndef __GEN_SYSTEM_MONITORING_H
#define __GEN_SYSTEM_MONITORING_H

#include "adcmonitoring.h"

/* Type definitions */
typedef struct{
#if SYSTEMMONITORING_ADC
  T_adcMonData_t    adcMonData;
#endif
}T_sysMonData_t;

/* Function declarations */
void SysMon_init(T_sysMonData_t  *monitoringData);

#endif // End of __GEN_SYSTEM_MONITORING_H definition
