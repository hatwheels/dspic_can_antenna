// 2014 - 2015

/*! \file eeprom.h
    \brief Contains function declarations related to the EEPROM
*/

#ifndef __HAL_EEPROM_H
#define __HAL_EEPROM_H

#include "stypes.h"

/* Global variables */
typedef struct{
	Uint16    wait_read_counter;
	Uint16    write_enable;
	Uint16    read_enable;
	Uint32    read_address;
	Uint32    write_address;
} T_eeprom_data_t;

typedef enum{
	EEPROM_ANT_LEFT_FREQ1 = 0,
	EEPROM_ANT_LEFT_FREQ2,
	EEPROM_ANT_LEFT_FREQ3,
	EEPROM_ANT_LEFT_FREQ4,
	EEPROM_ANT_RIGHT_FREQ1,
	EEPROM_ANT_RIGHT_FREQ2,
	EEPROM_ANT_RIGHT_FREQ3,
	EEPROM_ANT_RIGHT_FREQ4,
	EEPROM_ANT_CALIB_DATA_WRITTEN,
	EEPROM_ANT_COEFF_FREQ1,
	EEPROM_ANT_COEFF_FREQ2,
	EEPROM_ANT_COEFF_FREQ3,
	EEPROM_ANT_COEFF_FREQ4,
	EEPROM_ANT_COEFF_DATA_WRITTEN
} E_eeprom_ID_t;

/* Function declarations */
void  eeprom_init(T_eeprom_data_t *eeprom_data);
Uint16 eeprom_read_word(Uint32 read_address);
sbool eeprom_write_word(Uint32 write_address, Uint16 word_to_write);
void eeprom_test_read_write(T_eeprom_data_t *eeprom_data);
Uint32 eeprom_get_read_write_address(E_eeprom_ID_t eeprom_ID);

#endif // End of __HAL_EEPROM_H definition

