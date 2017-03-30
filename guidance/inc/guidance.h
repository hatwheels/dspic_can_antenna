// 2014 - 2015

/*! \file guidance.h
    \brief Contains high-level guidance function declarations
*/

#ifndef __GEN_GUIDANCE_H
#define __GEN_GUIDANCE_H

#include "wireguidance.h"

/* Define general guidance interface */
typedef struct{
#if GUIDANCE_WIRE
  T_wireGuid_t	wireGuidData;
#endif
}T_guidData_t;

/* Function declarations */
void Guid_init(T_guidData_t  *guidanceData);
void Guid_process(T_guidData_t  *guidanceData);

extern T_guidData_t	gGuidanceData;

#endif // End of __GEN_GUIDANCE_H definition
