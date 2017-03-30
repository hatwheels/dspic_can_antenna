// 2014 - 2015

/*! \file adc.c
    \brief Contains function implementations related to the 12 bit ADC and LEDs
           (as analog port pins are configured here)
*/

#include "project_canantenna.h"

// Global variables
int16 ADC_refVoltLeft_1;
int16 ADC_refVoltRight_1;

// Local variables
int16 Adc_antennaMeasLeft_1;
int16 Adc_antennaMeasRight_1;

//*****************************************************************************
// Local functions
//*****************************************************************************
/* Adc_init() is used to configure A/D. The A/D is set up for a sampling rate of
   40KHz or 15kHz, automatically generate an interrupt every sample and to scan 
   all analog inputs. */
void Adc_init(void)
{
	// Initialize A/D ISR Timer
	#if DBG_TIME
	t[4] = 0;	// instruction-counter of A/D interrupt.
	#endif
  
	/* AD control register 1:
		- Turn off ADC
		- Continue operation when idle
		- integer (int16) output format
		- internal counter ends sampling and starts conversion (auto convert)
		- start sampling when conversion done  */
	ADCON1bits.ADON   = 0;
	ADCON1bits.ADSIDL = 0;
	ADCON1bits.FORM   = 0;
	ADCON1bits.SSRC   = 7; // when = 3, use T3
	ADCON1bits.ASAM   = 1;

	/* AD Control register 2:
		- AVDD, AVSS used for VREFH, VREFL
		- Input scan enabled (otherwise not possible to scan >2 AN)
		- Generate interrupt @4th sample, as scanning over all 4 inputs (-> 3)
		- Buffer is a one 16-bit word buffer
		- Always used MUX A for scan */
	ADCON2bits.VCFG   = 0;
	ADCON2bits.CSCNA  = 1; // if 0, do not scan inputs
	ADCON2bits.SMPI   = 3; // if 0, every sample interrupt
	ADCON2bits.BUFM   = 0;
	ADCON2bits.ALTS   = 0;

	/* AD Control register 3 
		Configured to have a 15kHz sampling rate, for a 20MIPS device
		- 1 TAD between sampling and conversion
		- clock derived from system clock, as idle/sleep mode not entered and same
		  clock is used for different devices
		- conversion clock (add comment how to compute) */
	ADCON3bits.ADCS   = 0x2B; // = 43 so that TAD = 1111,11... ns >= 667 ns. @ 15kHz interrupt generation 
	ADCON3bits.SAMC   = 1;
	ADCON3bits.ADRC   = 0;    // System Clock

	/* Port configuration register:
		- PORTB = all analog, except:
		- RB0, RB1, RB2, RB3 (DIP switches for ID) */
	ADPCFG            = 0x000F;
  
	/* Input Select register:
		Ignored as input scan is enabled (see ADCON2)
		- Select Vref- for CH0- input */
	ADCHSbits.CH0NA   = 0;

	/* Input scan select register
		- Select ANx for input scan 
		- 4/5:   for antenna input voltage (AM0/1) -> stored in ADCBUF0/1
		- 11/12: for antenna input signal (A0/1)   -> stored in ADCBUF2/3 */
	ADCSSL            = 0x0000;
	ADCSSLbits.CSSL4  = 1;
	ADCSSLbits.CSSL5  = 1;
  
	ADCSSLbits.CSSL11 = 1;
	ADCSSLbits.CSSL12 = 1;
  
	/* Configuring the analog port pins
		- TRIS: determine whether each pin associated with the I/O port is an
          input (1) or an output (0). All port pins are inputs by default.
          The ADPCFG register and TRISB register control the operation of 
          the AD port pins together
		- PORT: not needed for AD
		- LAT: eliminates the problems that could occur with read-modify-write 
				  instructions*/
	TRISBbits.TRISB9  = 0;        // Initialize AN9  as output (LED 0, green)
	TRISBbits.TRISB10 = 0;        // Initialize AN10 as output (LED 1, red)
  
	return;
}

//*****************************************************************************
// Interrupt functions
//*****************************************************************************
/*! _ADCInterrupt() is the A/D interrupt service routine used to obtain antenna
    measurements at high update rates, and to estimate the measurements such
    that the amplitude and phase of the different frequencies can be derived.

  \Note: 
  - an ISR must be defined with the '__attribute__' keyword 
    and the 'interrupt' attribute in MPLAB C30.
    Standard Interrupt Vector Table is used (INTCON2bits.ALTIVT is initialized to 0.)
*/
void __attribute__ ((interrupt, auto_psv)) _ADCInterrupt(void)
{
	/* Copy data from ADC buffer to variables, depending on the number of
		antennas. Inputs that are scanned are set using ADCSSL-register.
	*/

	// Start A/D interrupt timer
	#if DBG_TIME
	t[4] = clock();
	#endif

   LATBbits.LATB9 = 1;
   #if (NBR_ANTENNAS==1)
   ADC_refVoltLeft_1       		= (int16)ADCBUF0;
   ADC_refVoltRight_1      		= (int16)ADCBUF1;
   /*	Remove constant offset 0x0800 = 2048d.
		10-bit ADC measurement, computations are
		8-bit scaled, so downscaling is required.
	*/
   Adc_antennaMeasLeft_1   	= ((int16)(ADCBUF2 - 0x800));// >> 2;
   Adc_antennaMeasRight_1  	= ((int16)(ADCBUF3 - 0x800));// >> 2;
   #else
		#error "Unexpected number of antennas"
   #endif

   #if GUIDANCE_WIRE
		#ifdef FUNCTION_CALL // Goertzel debug time (function + call) mode
		// Put sample in calculation
		if (ANT_k < ANT_k_max)
		{
			/* ################################################################# */
			t[0] = clock(); // start step instruction-counter
			/* ################################################################# */
			ANT_Step(Adc_antennaMeasLeft_1, Adc_antennaMeasRight_1);
			/* ################################################################# */
			t[0] = clock() - t[0]; // stores nbr of instructions required by step function
			if (ANT_k != 1U) 	// due to incrementation @l.357 (++ANT_k), range is 1 -> 215
					t[2] += t[0]; 	// update per-batch instruction-counter
			else
					t[2] = t[0]; 	// new batch instruction-counter
			/* ################################################################# */
		}
		#else // Goertzel normal mode
		// Put sample in calculation
		if (ANT_k < ANT_k_max)
			ANT_Step(Adc_antennaMeasLeft_1, Adc_antennaMeasRight_1);
		#endif // end deviation calculation method loop
   #endif

   LATBbits.LATB9 = 0;
   /* It is necessary to clear manually the interrupt flag for ADC */
   IFS0bits.ADIF = 0;

	// End A/D interrupt instruction-counter
	#if DBG_TIME
	t[4] = clock() - t[4]; // Store nbr of instructions required by A/D interrupt
	#endif
}
