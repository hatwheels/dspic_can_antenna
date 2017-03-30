// 2014 - 2015

/*! \file eeprom.c
    \brief Contains function implementations related to the EEPROM. This file
           only contains wrapping functions for the compiler specific library 
           functions
*/

#include "project_canantenna.h"

/* Defines */
#define	EEPROM_ADDRESS_START			            (0x7FFE00)
#define  EEPROM_ADDRESS_PARAM_LEFT_FREQ1			(0x7FFE02)
#define  EEPROM_ADDRESS_PARAM_LEFT_FREQ2 			(0x7FFE04)
#define  EEPROM_ADDRESS_PARAM_LEFT_FREQ3 			(0x7FFE06)
#define  EEPROM_ADDRESS_PARAM_LEFT_FREQ4 			(0x7FFE08)
#define  EEPROM_ADDRESS_PARAM_RIGHT_FREQ1			(0x7FFE0A)
#define  EEPROM_ADDRESS_PARAM_RIGHT_FREQ2			(0x7FFE0C)
#define  EEPROM_ADDRESS_PARAM_RIGHT_FREQ3			(0x7FFE0E)
#define  EEPROM_ADDRESS_PARAM_RIGHT_FREQ4			(0x7FFE10)
#define  EEPROM_ADDRESS_CALIB_DATA_WRITTEN  		(0x7FFE12)
#define	EEPROM_ADDRESS_COEFF_FREQ1					(0x7FFE14)
#define	EEPROM_ADDRESS_COEFF_FREQ2					(0x7FFE16)
#define	EEPROM_ADDRESS_COEFF_FREQ3					(0x7FFE18)
#define	EEPROM_ADDRESS_COEFF_FREQ4					(0x7FFE1A)
#define	EEPROM_ADDRESS_COEFF_DATA_WRITTEN			(0x7FFE1C)
#define  EEPROM_ADDRESS_END							(0x7FFE1E)

#define  VERIFY_WRITE_COUNT                         (3)

/* Local variables */
Uint16      Eeprom_read_data;
Uint16      Eeprom_test_data_read;
Uint16      Eeprom_test_data_write;

/*********************************************************************************/
/* Local functions */
/*********************************************************************************/
void eeprom_init(T_eeprom_data_t  *eeprom_data)
{
	eeprom_data->wait_read_counter  = 0;
	eeprom_data->write_enable       = 1;
	eeprom_data->read_enable		= 0;
	eeprom_data->write_address 		= EEPROM_ADDRESS_START;
	eeprom_data->read_address       = EEPROM_ADDRESS_START;

	Eeprom_test_data_read  = 0;
	Eeprom_test_data_write = 0x5555;

  return;
}

/*********************************************************************************/
/* Read data stored in address and return it */
Uint16 eeprom_read_word(
  Uint32    read_address)
{  
	_memcpy_p2d16(&Eeprom_read_data, read_address, _EE_WORD);

  return (Eeprom_read_data);
}

/*********************************************************************************/
/* Write value to address and check if writing succeeded */
sbool eeprom_write_word(
  Uint32    write_address,
  Uint16    word_to_write)
{
	static const Uint16 __attribute__((space(auto_psv))) 
	verify_write_count = (Uint16)VERIFY_WRITE_COUNT;

	// Counter for the erase-write loop
	Uint16 cnt = 0U;

	/*
		The loop checks if the word to be written was successfully stored
		by reading back the content of the destination address.
	*/ 
	while (eeprom_read_word(write_address) != word_to_write)
	/*
		- 1st iteration: Check FIRST (to avoid writing the same word) if the stored 
		word is identical to the word to be stored. If not, THEN initiate
		erase-write procedure.
		- subsequent iterations: if the stored word is not identical to the word to 
		be stored, then rewrite the word to be stored to the destination address.
	*/
	{     
        /* If the counter surpasses a threshold defined by VERIFY_WRITE_COUNT,
           then the writing procedure has failed too many times and the right
           word was not read. Return that the word to write could not be written 
           to the destination address.   
        */
        if (++cnt > verify_write_count)
            return false;

        /* First, the content of the EEDATA address must be erased */
       _erase_eedata(write_address, _EE_WORD);

       /* Wait until erase operation is completed */
       _wait_eedata();

       /* Write Word to blank address */
       _write_eedata_word(write_address, word_to_write);

       /* Wait until write operation is completed */
       _wait_eedata();  
	}
  
	// Word to write was successfully read. Return that writing procedure succeeded
	return true;
}

/******************************************************************************/
/* Return EEPROM address defined by identifier */
Uint32 eeprom_get_read_write_address(
  E_eeprom_ID_t   eeprom_ID)
{
	Uint32  eeprom_address;
  
	switch (eeprom_ID)
	{
		case EEPROM_ANT_LEFT_FREQ1:
			eeprom_address = EEPROM_ADDRESS_PARAM_LEFT_FREQ1;
			break;
      
		case EEPROM_ANT_LEFT_FREQ2:
			eeprom_address = EEPROM_ADDRESS_PARAM_LEFT_FREQ2;
			break;
      
		case EEPROM_ANT_LEFT_FREQ3:
			eeprom_address = EEPROM_ADDRESS_PARAM_LEFT_FREQ3;
			break;
      
		case EEPROM_ANT_LEFT_FREQ4:
			eeprom_address = EEPROM_ADDRESS_PARAM_LEFT_FREQ4;
			break;

		case EEPROM_ANT_RIGHT_FREQ1:
			eeprom_address = EEPROM_ADDRESS_PARAM_RIGHT_FREQ1;
			break;

		case EEPROM_ANT_RIGHT_FREQ2:
			eeprom_address = EEPROM_ADDRESS_PARAM_RIGHT_FREQ2;
			break;

		case EEPROM_ANT_RIGHT_FREQ3:
			eeprom_address = EEPROM_ADDRESS_PARAM_RIGHT_FREQ3;
			break;

		case EEPROM_ANT_RIGHT_FREQ4:
			eeprom_address = EEPROM_ADDRESS_PARAM_RIGHT_FREQ4;
			break;

		case EEPROM_ANT_CALIB_DATA_WRITTEN:
			eeprom_address = EEPROM_ADDRESS_CALIB_DATA_WRITTEN;
			break;
      
		case EEPROM_ANT_COEFF_FREQ1:
			eeprom_address = EEPROM_ADDRESS_COEFF_FREQ1;
			break;
	
		case EEPROM_ANT_COEFF_FREQ2:
			eeprom_address = EEPROM_ADDRESS_COEFF_FREQ2;
			break;
	
		case EEPROM_ANT_COEFF_FREQ3:
			eeprom_address = EEPROM_ADDRESS_COEFF_FREQ3;
			break;
		
		case EEPROM_ANT_COEFF_FREQ4:
			eeprom_address = EEPROM_ADDRESS_COEFF_FREQ4;
			break;
		
		case EEPROM_ANT_COEFF_DATA_WRITTEN:
			eeprom_address = EEPROM_ADDRESS_COEFF_DATA_WRITTEN;
			break;
		  
		default:
			eeprom_address = EEPROM_ADDRESS_START;
			break;
  }

  return(eeprom_address);
}


/******************************************************************************/
/* Test read/write procedure from/to EEPROM */
void eeprom_test_read_write(T_eeprom_data_t  *eeprom_data)
{
	while(1)
	{		
		if (eeprom_data->write_enable)
		{
			eeprom_write_word(eeprom_data->write_address, Eeprom_test_data_write);
			eeprom_data->write_enable  = 0;
			eeprom_data->wait_read_counter = 0;
			eeprom_data->read_enable   = 1;
			eeprom_data->read_address = eeprom_data->write_address;
		}
		if (eeprom_data->read_enable)
		{
			if (eeprom_data->wait_read_counter > 5)
			{
				Eeprom_test_data_read = eeprom_read_word(eeprom_data->read_address);
				eeprom_data->read_enable = 0;
				eeprom_data->write_enable = 1;
				Eeprom_test_data_write = 0x6666;
				eeprom_data->write_address =   EEPROM_ADDRESS_END;
			}
			else
				++eeprom_data->wait_read_counter;
		}
		if (Eeprom_test_data_read == 0x6666)
		{
			LATBbits.LATB10 = 1;
			break;
		}
	}
	
	return;
}
