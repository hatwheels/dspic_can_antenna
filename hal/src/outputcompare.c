// 2014 - 2015

/*! \file outputcompare.c
    \brief Contains function to generate BIT PWM signal
*/

#include "project_canantenna.h"

// Defines
#define OCXRS_DUTYCYCLE     (((500*TMR1_PERIOD)/TEST_FREQUENCY_HZ)-1)
												/* This value is linked to
													TMR3_FORPWMPERIOD, as it shall be half of
													it (50% duty cycle) */
										 
//*****************************************************************************
// Local functions
//*****************************************************************************
void OC_init(void)
{
	/* Output compare control register 1
		- Continue output compare when CPU in idle mode
		- Output compare timer is Timer 3
		- PWM on OC1 with Fault pin disabled  */
	OC1CONbits.OCSIDL = 0;
	OC1CONbits.OCTSEL = 1;
	OC1CONbits.OCM    = 6; // = 110 (binary)

	/* Initialize OcR register*/
	OC1R  = 0;
	OC1RS = OCXRS_DUTYCYCLE;

	/* Output compare control register 2
		- Continue output compare when CPU in idle mode
		- Output compare timer is Timer 3
		- PWM on OC2 with Fault pin disabled  */
	OC2CONbits.OCSIDL = 0;
	OC2CONbits.OCTSEL = 1;
	OC2CONbits.OCM    = 6; // = 110 (binary)

	/* Initialize OcR register*/
	OC2R  = 0;
	OC2RS = OCXRS_DUTYCYCLE;

	return;
}
