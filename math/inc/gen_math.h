// 2014 -2015

//! \file math.h
//! \brief Contains generic mathematical functions and defines

#ifndef __MATH_MATH_H
#define __MATH_MATH_H

#include "stypes.h"

// Defines
#define   MATH_PI                         			(3.141592653589793)
#define   MATH_2PI                        			(2*MATH_PI)
#define   MATH_PI_DIV2                    			(MATH_PI*0.5)
#define   MATH_SIN_COS_SCALEFACTOR                  (32768)         /* 2^15 */

/* For cos/sin, x is provided in [rad] and result is scaled with factor 2^15 */
#define   MATH_SIN(x)    				            (sin(x)*32768)
#define   MATH_COS(x)                 				(cos(x)*32768)

// Function declarations
Uint16  Math_sqrtUint32(Uint32  a);

/* Range x_rad shall be between [0 2pi]. When sin reaches 1 (@pi/2), it is 
   clipped to the value 32677 causing a small inaccuracy. */
int16   Math_sin(double x_rad);

/* Range x_rad shall be between [0 2pi]. When cos reaches 1 (@0 and 2*pi), it is 
   clipped to the value 32677 causing a small inaccuracy. */
int16   Math_cos(double x_rad);

#endif // End of __MATH_MATH_H definition
