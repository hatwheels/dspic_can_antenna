// 2014 - 2015

//! \file stypes.h
//! \brief Contains type definition of types

#ifndef __STYPES_H
#define __STYPES_H

/* typecast standard types */
typedef char            int8;
typedef int             int16;
typedef long            int32;
typedef unsigned char   Uint8;
typedef unsigned int    Uint16;
typedef unsigned long   Uint32;

/* Make a boolean type and add TRUE/FALSE (if not defined by processor) */
#define true            (1)
#define false           (0)
typedef char            sbool;

/* special defines, related to type */
#define  INT16_MAX       (32767)
#define  INT16_MIN       (-32768)

#define  UINT16_INVALID  (65535)

#endif // End of __STYPES_H definition
