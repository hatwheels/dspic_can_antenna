// 2014 - 2015

/*! \file interrupts.c
    \brief Contains functions that are needed to configure the interrupts. The
           interrupt functions themselves, are declared and implemented in the
           related module.
*/

#include "project_canantenna.h"

//*****************************************************************************
// Local functions
//*****************************************************************************
void Interrupt_init(void)
{
	/* Interrupt control register 1
		Do not allow nested interrupts (bit 15)*/
	INTCON1bits.NSTDIS = 1;

	/* Interrupt control register 2
		Use standard vector table */
	INTCON2bits.ALTIVT = 0;

	/* Status register (In CPU)
		CPU interrupt level is 3 (bit 7-5), enable CPU priority levels 4-7. If this 
		value is 0 it is disabled, linked with CORCONbits.IPL3 */
	SRbits.IPL = 1;

	/* Core control register
		CPU interrupt priority level < 7 (bit 3), linked with SRbits.IPL */
	CORCONbits.IPL3	= 0;

	/* Interrupt priority control register 0 
		Set interrupt priority for Timer1 (bits 14-12)
		Set interrupt priority for Timer3 */
	IPC0bits.T1IP = 6;
	//IPC1bits.T3IP = 1;

	/* Interrupt priority control register 2
		Set interrupt priority for AD Conversion complete (bits 14-12) */
	IPC2bits.ADIP = 5;

	/* Interrupt priority control register 3 
		Set interrupt priority for Interrupt Change Notification Flag Status */
	IPC3bits.CNIP = 2;
  
	/* Interrupt priority control register 6 
		Set interrupt priority for CAN (bit 14-12) */
	IPC6bits.C1IP = 3;

	/* Interrupt flag status register 0
		Clear interrupt flag status bit associated with AD Conversion complete
		(bit 11), Timer1 (bit 3), Timer3, Interrupt Change Notification Flag Status 
		(bit 15) */
	IFS0bits.T1IF		= 0;
	IFS0bits.T3IF		= 0;
	IFS0bits.ADIF    = 0;
	IFS0bits.CNIF    = 0;
  
	/* Interrupt flag status register 1 
		Clear interrupt flag status bit associated with CAN (bit 11) */
	IFS1bits.C1IF = 0;

	/* Input Change Notification Interrupt Enable Register 1:
		Enable interrupt on change of pin 0, 1, 2 and 3 (used for nodeID) */
	CNEN1bits.CN0IE = 1;
	CNEN1bits.CN1IE = 1;
	CNEN1bits.CN2IE = 1;
	CNEN1bits.CN3IE = 1;

	/* Interrupt enable control register 0
		Enable AD conversion interrupt (bit 11), Timer1 (bit 3), Timer3,
		Interrupt Change, Notification Flag Status (bit 15) */
	IEC0bits.T1IE      = 1;
	//IEC0bits.T3IE      = 1;
	IEC0bits.ADIE     = 1;
	IEC0bits.CNIE     = 1;

	/* CAN interrupt enable register: enable all CAN interrupt sources
		IVRIE (invalid message received), WAKIE (Bus Wake Up), ERRIE (Error),
		TX2IE (Transmit Buffer 2), TX1IE (Transmit Buffer 1), TX0IE (Transmit Buffer 0),
		RX1IE (Receive Buffer 1), RX0IE (Receive Buffer 0)
	*/
	C1INTE = 0x00FF;
  
	/* Interrupt enable control register 1 
		Enable CAN interrupt (bit 11) */
	IEC1bits.C1IE  = 1;
  
	return;
}
