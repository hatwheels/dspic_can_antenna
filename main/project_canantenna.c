// 2014 - 2015

/*! \file project_canantenna.c
    \brief Contains the main function for the CAN antenna
   
   The goal of the CAN antenna is to provide position deviation and direction
   information in case of wired guidance. Each CAN antenna contains only 1 antenna.
   
   \note Development:
   - Variable initialization @variable declaration is only possible when 
     initialized to a constant value. Else, compiler error will be generated
   - When starting testing on platform: check timings
     a) a single instruction cycle corresponds with 4 clock cycles, so for 80MHz
        the execution time is: nbr instructions * 4 * 12.5 [nsec]
*/

#include "project_canantenna.h"

/****************FOR MPLAB X IDE (successor to MPLAB 8) - created auto on MPLAB X******************************/
// DSPIC30F4013 Configuration Bit Settings
// 'C' source line config statements

// FOSC
//#pragma config FOSFPR = XT_PLL8           // Oscillator (XT w/PLL 8x)
//#pragma config FCKSMEN = CSW_FSCM_OFF     // Clock Switching and Monitor (Sw Disabled, Mon Disabled)

// FWDT
//#pragma config FWPSB = WDTPSB_1           // WDT Prescaler B (1:1)
//#pragma config FWPSA = WDTPSA_1           // WDT Prescaler A (1:1)
//#pragma config WDT = WDT_OFF              // Watchdog Timer (Disabled)

// FBORPOR
//#pragma config FPWRT = PWRT_64            // POR Timer Value (64ms)
//#pragma config BODENV = BORV20            // Brown Out Voltage (Reserved)
//#pragma config BOREN = PBOR_ON            // PBOR Enable (Enabled)
//#pragma config MCLRE = MCLR_DIS           // Master Clear Enable (Disabled)

// FGS
//#pragma config GWRP = GWRP_OFF            // General Code Segment Write Protect (Disabled)
//#pragma config GCP = CODE_PROT_OFF        // General Segment Code Protection (Disabled)

// FICD
//#pragma config ICS = ICS_PGD              // Comm Channel Select (Use PGC/EMUC and PGD/EMUD)

/************************************************************************************************************************************/

//*****************************************************************************
// Global variable declarations
//*****************************************************************************
T_guidData_t      gGuidanceData;
T_systemData_t    gSystemData;

//*****************************************************************************
// MAIN function
//*****************************************************************************
int main()
{
	/* Set up system configuration */
	System_init();

	/* Initialize general modules that are applicable */
	/* Initialize guidance module */
	Guid_init(&gGuidanceData);
			
	/* Re-start all systems after configuration and initialization */
	System_start();


	/* Start endless loop */
	do{
			/* Run 100Hz computations */
			if (gSystemData.clockT1SysData.puls_100Hz)
			{
				/* Process measurements used for guidance */
				Guid_process(&gGuidanceData);

				#if DISABLE_ADC_ISR_CAN
				// Disable A/D interrupt so that it does not interfere
				IEC0bits.ADIE = 0; // Control Bit for individual enabling/disabling of A/D interrupt 
				#endif

				/* Output messages */
				// Re-enable CAN (Phase 3)
				/* Measure clock ticks for TX Results, Statuses, Raws functions (calls) */
				#ifdef FUNCTION_CALL_CAN
				t_can[1] = clock();
				#endif
				Can_transmit_wireguid_result(&(gGuidanceData.wireGuidData), &(gSystemData.can_data),
                                                gSystemData.can_data.can_tx_msg_buffer[CAN_TX_MSG_BUFFER_0].content);
				#ifdef FUNCTION_CALL_CAN
				t_can[1] = clock() - t_can[1];
				t_can[2] = clock();
				#endif 
				Can_transmit_wireguid_status(&(gGuidanceData.wireGuidData), &(gSystemData.can_data),
                                                gSystemData.can_data.can_tx_msg_buffer[CAN_TX_MSG_BUFFER_1].content);
				#ifdef FUNCTION_CALL_CAN
				t_can[2] = clock() - t_can[2];
				t_can[3] = clock();
				#endif 
				Can_transmit_wireguid_raw(&(gGuidanceData.wireGuidData), &(gSystemData.can_data),
                                            gSystemData.can_data.can_tx_msg_buffer[CAN_TX_MSG_BUFFER_2].content);
				#ifdef FUNCTION_CALL_CAN
				t_can[3] = clock() - t_can[3];
				#endif 
		
				#if DISABLE_ADC_ISR_CAN
				// Re-enable A/D interrupt
				IEC0bits.ADIE = 1;
				#endif

				Can_transmit_wireguid_switches(&(gGuidanceData.wireGuidData), &(gSystemData.can_data),
                                                gSystemData.can_data.can_tx_msg_buffer[CAN_TX_MSG_BUFFER_3].content);
				
				/* Reset 100Hz pulse */
				gSystemData.clockT1SysData.puls_100Hz = 0;
			}
		} while(1);
}
