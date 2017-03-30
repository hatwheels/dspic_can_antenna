// 2014 - 2015

/*! \file system.c
    \brief Contains functions that are needed to configure the whole system/
			device.
			
	\references
   [1] dsPIC Family Reference Manual, Section 24. Device Configuration -
       dsPIC30F FRM, doc no.DS70071D, Microchip (r),
       http://ww1.microchip.com/downloads/en/DeviceDoc/70071E.pdf
   [2] dsPIC Family Reference Manual, Section 07. Oscillator - dsPIC30F FRM,
       doc no.DS70054E, Microchip (r),
       http://ww1.microchip.com/downloads/en/DeviceDoc/70054e.pdf
*/

#include "project_canantenna.h"

// Defines
#define  LED_BLINKING_PERIOD_MSEC     40
#define  LED_BLINKING_2XSEC_MSEC      150
#define  LED_BLINKING_3XSEC1_MSEC     150
#define  LED_BLINKING_3XSEC2_MSEC     300
#define  LED_BLINKING_4XSEC1_MSEC     150
#define  LED_BLINKING_4XSEC2_MSEC     300
#define  LED_BLINKING_4XSEC3_MSEC     450
#define  LED_BLINKING_5XSEC1_MSEC     150
#define  LED_BLINKING_5XSEC2_MSEC     300
#define  LED_BLINKING_5XSEC3_MSEC     450
#define  LED_BLINKING_5XSEC4_MSEC     600

//*********************************************************************************************
// Global functions
//*********************************************************************************************
/*! System_init() initializes the system: ADC, interrupts, etc.

  The device configuration registers are set as project parameters (for MPLAB
  C30 for dsPIC, see Configure > Configuration Bits) (ref. [1] and [2]):
  - FOSC (0xF80000): Oscillator Configuration Register
    * FCKSM = 10(b):  Clock switching is disabled, Fail-Safe Clock Monitor is
                      disabled
    * FOS   = 111(b): Oscillator Source Selection: PLL Oscillator, PLL source
                      selected by FPR bits
    * FPR   = 0110(b): Oscillator Selection within Primary Group bits 
							   (see ref. [2], table 7.4)
  - FWDT (0xF80002):  Watchdog Timer Configuration Register -> set to 0x0000:
    * FWDTEN = 0: Watchdog is disabled
    * Prescaler values are set to 1:1 (not used as watchdog is disabled)
  - FBORPOR (0xF80004): BOR (Brown-Out Reset) and POR (Power-On-Reset)
                        Configuration Register  -> set to 0x87B3  (default)
    * MCLREN = 1:     Pin function is MCLR
    * PWMPIN = 1:     PWM module pins controlled by PORT register at device Reset
    * HPOL   = 1:     PWM module high-side output pins have active-high output
                      polarity
    * LPOL   = 1:     PWM module low-side output pins have active-high output
                      polarity
    * BOREN  = 1:     PBOR Enabled
    * BORV   = 11(b): Brown-out Voltage is 2.0V
    * FPWRT  = 11(b): Power-on Reset Timer Value is 64 ms
  - FGS (0xF8000A): General Code Segment Configuration Register -> set to 0x0006
    * ESS    = 0:     Secure Data EEPROM Segment is 2048 bytes
    * SSS    = 011(b):No secure segment program flash code protection
    * SWRP   = 0:     Secure Segment Program Flash is write-protected
  - FGS: General Segment Configuration Register for Devices with Advanced
         Security (ref. [1]), not set
  - ICD ($F8000C): In-Circuit Debugger Configuration Register -> set to 0xC003
    * BKBUG  = 1: Device will reset in User mode (Background Debug)
    * COE    = 0: Device will reset in Clip-On Emulation mode
    * ICS    = 11(b): ICD Communication Channel Select Enable bits; Communicate
                      on PGC/EMUC and PGD/EMUD
    !!! NOTES about ICD setting:
    !!! 1. no other setting for BKBUG and COE possible
    !!! 2. BKBUG (ref. [1]) and COE are bit 6 and 7, while here bit 14 and 15
    !!!    are set.

  The project setting (for MPLAB C30 for dsPIC, see Configure > Configuration Bits) for
  "Master Clear Enable" shall be set to DISABLED as long as the condensator C30 
  is not mounted on the board. This condensator shall not be mounted as long as 
  the flash programmer is used to program/debug.
*/
void System_init(void)
{
	/* Stop systems for configuration */
	/* Clock (Timer1) */
	T1CONbits.TON   = 0;
	T3CONbits.TON   = 0;
	/* ADC */
	ADCON1bits.ADON = 0;
	/* Output Compare */
	OC1CONbits.OCM  = 0; // Output Compare Channel disabled
	/* CAN (reset all CAN interrupts) */
	C1INTF = 0; 
  
	/* Configure Core Control Register. This bit does not affect MUL.SS, but should
		affect MPY (multiply function that uses 40 bit accumulator
		- ACCSAT = 0: 1:31 saturation (normal) */
	CORCON = 0x0000;
	CORCONbits.IF = 1;
	CORCONbits.ACCSAT = 1;
	SR = 0x0000;
  
	/* Configure systems */
	/* Clock (Timer1 and 3) */
	Clock_init();
	/* ADC */
	Adc_init();

	/* Output compare (uses Timer 3)*/
	OC_init();

	/* CAN init */
	Can_init();

	/* EEPROM init */
	eeprom_init(&(gSystemData.eeprom_data));
	// Test EEPROM read/write procedures
	eeprom_test_read_write(&(gSystemData.eeprom_data));

	/* Configure the interrupts and enable required interrupts */
	Interrupt_init();

	return;
}

/*!  System_start() starts all systems after configuration and initialization,
     if needed */
void System_start(void)
{
	/* - Clock (Timer1) */
	T1CONbits.TON = 1;

	/* Timer 3 enabled for PWM */
	T3CONbits.TON = 1;

	/* - ADC */
	ADCON1bits.ADON = 1;

	return;
}
