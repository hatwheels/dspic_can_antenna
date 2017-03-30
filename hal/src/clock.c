// 2014 - 2015

/*! \file clock.c
    \brief Contains functions that are needed to configure the clock (100Hz
			loop, for example).
*/

#include "project_canantenna.h"

//*****************************************************************************
// Local functions
//*****************************************************************************
// Initializes (resets) clock
void Clock_init(void)
{
	/* Initialize clock variables */
	gSystemData.clockT1SysData.ticks_1msec    = 0;
	gSystemData.clockT1SysData.ticks_1sec 		= 0;
	gSystemData.clockT1SysData.ticks_10msec	= 0;
	gSystemData.clockT1SysData.puls_100Hz		= 1;
  
	/* - Clear timer1 register, to start counting from zero such that comparison
                              with PR1 is correct from the beginning.
		- Set period 1 register
		- Set internal clock source
		- Start of timer is set in system_start()
		- Use 1:1 prescaler */
	TMR1            	= 0;
	PR1             		= TMR1_PERIOD;
	T1CONbits.TCS	= 0;
  
	/* 	- Clear timer3 register, to start counting from zero such that comparison
		  with PR1 is correct from the beginning.
		- Set period 3 register
		- Set internal clock source
		- Start of timer is set in system_start()
		- Use 1:1 prescaler */
	TMR3						= 0;
	PR3							= TMR3_FORPWMPERIOD;
	T3CONbits.TCS			= 0;
	T3CONbits.TCKPS	= 0;

  return;
}

//*****************************************************************************
// Interrupt functions
//*****************************************************************************
/*! _T1Interrupt is the Timer1 interrupt service routine.

  \Note:
  - an ISR must be defined with the '__attribute__' keyword 
    and the 'interrupt' attribute in MPLAB C30.
  - Standard Interrupt Vector Table is used (INTCON2bits.ALTIVT is initialized to 0).
*/
void __attribute__((interrupt, auto_psv)) _T1Interrupt(void)
{
	LATBbits.LATB10 = 1;
  
	/* Increment ticks counter */
	++gSystemData.clockT1SysData.ticks_1msec;
	++gSystemData.clockT1SysData.ticks_10msec;

	/* Set 100Hz puls and clear 10msec tick */
	if (gSystemData.clockT1SysData.ticks_10msec > TIMECOUNTER_10MSEC)
	{
		gSystemData.clockT1SysData.ticks_10msec 	= 0;
		gSystemData.clockT1SysData.puls_100Hz   	= 1;
	}

	/* Increase 1sec tick, if time rollover and clear 1msec tick */
	if ( gSystemData.clockT1SysData.ticks_1msec > TIMECOUNTER_1000MSEC)
	{
		gSystemData.clockT1SysData.ticks_1msec = 0;
		++gSystemData.clockT1SysData.ticks_1sec;
	}
  
	LATBbits.LATB10 = 0;
  
	/* Clear interrupt flag manually */
	IFS0bits.T1IF = 0;
}

