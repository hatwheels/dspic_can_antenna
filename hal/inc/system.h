// 2014 - 2015

/*! \file system.h
    \brief Contains function declarations that are needed to configure the whole 
           system/device.
*/

#ifndef __HAL_SYSTEM_H
#define __HAL_SYSTEM_H

#include "systemtypes.h"

/*! Initializes the system, incl. ADC, timers, etc. */
void System_init(void);

/*! Starts the system after configuration and initialization */
void System_start(void);

/*! This function (de)activates the LEDs, based on the requested blink times and
    LED colors available.

  \param ledColor        		[in]	Color of LED to be activated
  \param ledBlinksPerSec 	[in]	Number of times/sec the LED needs to blink: 
										- Off: LED is continuously off
										- On:	LED is continuously on

  \note
   - For debugging: When all LEDs remain continuously on, a non-existing LED 
     color is provided as function argument.
*/
/*
void System_blinkLED(E_LEDColor_t     ledColor,
                     E_LEDBlinking_t  ledBlinksPerSec);
*/
					 
#endif // End of __HAL_SYSTEM_H definition

