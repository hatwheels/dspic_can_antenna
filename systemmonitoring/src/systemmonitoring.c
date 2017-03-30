// 2014 - 2015

/*! \file systemmonitoring.c
    \brief Contains high-level function implementation, related to system 
           monitoring, for ADC, battery, etc.
*/
#include "project_canantenna.h"

//*****************************************************************************
// Global functions
//*****************************************************************************
void SysMon_init(T_sysMonData_t  *monitoringData)
{
#if SYSTEMMONITORING_ADC
  SysMon_init(&(monitoringData->adcMonData));
#endif

  return;
}
