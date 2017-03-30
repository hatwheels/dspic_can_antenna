//2014 - 2015

/*! \file guidance.c
    \brief Contains high-level guidance function implementation
*/
#include "project_canantenna.h"

//*****************************************************************************
// Global functions
//*****************************************************************************
void Guid_init(T_guidData_t  *guidanceData)
{
	#if GUIDANCE_WIRE
	WireGuid_init(&(guidanceData->wireGuidData));
	#endif

	return;
}

void Guid_process(T_guidData_t  *guidanceData)
{
	#if GUIDANCE_WIRE
	WireGuid_process(&(guidanceData->wireGuidData));
	#endif

	return;
}
