// 2014 - 2015

/*! \file system.h
    \brief Contains definitions that are needed to configure the whole
           system/device.
*/

#ifndef __HAL_SYSTEM_TYPES_H
#define __HAL_SYSTEM_TYPES_H

#include "eeprom.h"
#include "can.h"

#define ADC_INTERRUPT_CYCLICBUFFERSIZE  (50)

/* Real-time clock */
typedef struct {
  Uint16    puls_100Hz;     /* Every 100Hz, set to 1 */
  Uint16    ticks_10msec;   /* relative time    [msec] */
  Uint16    ticks_1msec;    /* relative time    [msec] */
  Uint16    ticks_1sec;     /* absolute time    [sec] */
}T_clockTimer_t;

/* ADC LEDs */
typedef enum {
  LED_COLOR_GREEN = 0,
  LED_COLOR_RED,
  LED_COLOR_LAST
}E_LEDColor_t;

typedef enum {
  LED_BLINKING_OFF = 0,    /* Continuously off */
  LED_BLINKING_1XSEC,
  LED_BLINKING_2XSEC,
  LED_BLINKING_3XSEC,
  LED_BLINKING_4XSEC,
  LED_BLINKING_5XSEC,
  LED_BLINKING_ON          /* Continuously on */
}E_LEDBlinking_t;

/*! Define general system interface */
typedef struct{
  T_clockTimer_t    clockT1SysData;
  int16              		ledBlinkingTime[LED_COLOR_LAST];
  T_can_data_t		can_data;
  T_eeprom_data_t	eeprom_data;
}T_systemData_t;

extern T_systemData_t    gSystemData;

#endif // End of __HAL_SYSTEM_TYPES_H definition

