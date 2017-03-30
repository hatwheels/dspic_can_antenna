// 2014 - 2015

/*! \file interrupts.h
    \brief Contains function declarations and variable declarations linked to
           interrupts. Interrupt routines themselves are declared and implemented
           in the module they are related to.
*/

#ifndef __HAL_INTERRUPTS_H
#define __HAL_INTERRUPTS_H

/*! Initializes the system, incl. ADC, timers, etc. */
void Interrupt_init(void);

#endif // End of __HAL_INTERRUPTS_H definition
