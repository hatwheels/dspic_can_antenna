// 2014 - 2015

/*! \file clock.h
    \brief Contains functions that are needed to configure the clock (100Hz
           loop, for example).
*/

#ifndef __HAL_CLOCK_H
#define __HAL_CLOCK_H

// Defines
#define   TCY_NANOSEC 50 // [nsec], for 20MIPS and FOSC=80MHz
#define   TMR1_PERIOD (1000000/TCY_NANOSEC)
/* Timer1 period for 1ms with TCY
 * in [nsec], causing interrupt
 * every 1msec
 */
#define   TMR3_FORPWMPERIOD (((1000*TMR1_PERIOD)/TEST_FREQUENCY_HZ)-1)
/* Timer3 period for output compare.   5 kHz: 3999
 *  10 kHz: 1999
 */
#define   TIMECOUNTER_1000MSEC 1000
#define   TIMECOUNTER_10MSEC   10

void Clock_init(void);

#endif // End of __HAL_CLOCK_H definition
