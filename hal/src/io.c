// 2014 - 2015

/*! \file io.c
    \brief Contains function implementations related to the I/O interfaces 
			(digital). For usage of analog inputs, see adc.c
*/

#include "project_canantenna.h"

//*****************************************************************************************
// Interrupt functions
//*****************************************************************************************
/*! Change Notification (CN) is a feature that, if enabled, provides us with an
   interrupt when the logic level on the pin changes. This is needed in case the
   node ID or baud rate has been updated 
   
   _CNInterrupt() is the Change Notification interrupt.
*/
void __attribute__((interrupt, auto_psv)) _CNInterrupt(void)
{
	// It is necessary to clear manually the interrupt flag for CN 
	IFS0bits.CNIF = 0;
}
