// 2014 - 2015

//! \file gen_math.c
//! \brief Contains generic mathematical functions and defines

#include "project_canantenna.h"

//**********************************************************************************************
// LOCAL FUNCTIONS
//**********************************************************************************************
Uint16  Math_sqrtUint32(Uint32  a)
{
	/* Variable declaration */
	Uint32 t2 = 0;
	Uint32 b2 = 0;
	Uint32 sqrt_a = 0;

	for (b2=0x10000000; b2!=0; b2>>=2)
	{
		t2 = sqrt_a + b2;
		sqrt_a >>= 1;
		if (t2 <= a)
		{
			a      -= t2;
			sqrt_a += b2;
		}
	}

	return ((Uint16)sqrt_a);
}

//**********************************************************************************************
int16   Math_sin(double x_rad)
{
	int16     sin_xrad = 0;

	/* Make sure that there is no overflow when sin(x) becomes 1, due to the 2^15-
		scaling. No problem when reaching -1. */
	//if (abs(x_rad - MATH_PI_DIV2) < 1e-2)
	//{
	//  sin_xrad = INT16_MAX;
	//}
	//else
	//{
	sin_xrad = (int16)(sin(x_rad)*MATH_SIN_COS_SCALEFACTOR);
	//}
	return (sin_xrad);
}

//**********************************************************************************************
int16   Math_cos(double x_rad)
{
	int16     cos_xrad = 0;

	/* Make sure that there is no overflow when sin(x) becomes 1, due to the 2^15-
		scaling. No problem when reaching -1 */
	//if ((abs(x_rad)            < 1e-2) ||
	//    (abs(x_rad - MATH_2PI) < 1e-2) )
	//{
	//  cos_xrad = INT16_MAX;
	//}
	//else
	//{
    cos_xrad = (int16)(cos(x_rad)*MATH_SIN_COS_SCALEFACTOR);
	//}
	return (cos_xrad);
}
