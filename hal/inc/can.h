// 2014 - 2015

/*! \file can.h
    \brief Contains definitions and functions related to the CAN module
*/

#ifndef __HAL_CAN_H
#define __HAL_CAN_H

#include <time.h> // timers for execution time measuring
#include "guidance.h"

/* Typedefs */
typedef struct
{
	Uint16  sid;
	Uint8   length;
	Uint8   content[8];
}T_can_msg_t;

typedef enum
{
	CAN_RX_MSG_BUFFER_0 = 0,
	CAN_RX_MSG_BUFFER_1,
	CAN_RX_MSG_BUFFER_LAST
}E_can_rx_buffer_t;

typedef enum
{
	CAN_TX_MSG_BUFFER_0 = 0,
	CAN_TX_MSG_BUFFER_1,
	CAN_TX_MSG_BUFFER_2,
	CAN_TX_MSG_BUFFER_3,
	CAN_TX_MSG_BUFFER_LAST
}E_can_tx_buffer_t;

typedef struct
{
	T_can_msg_t		can_tx_msg_buffer[CAN_TX_MSG_BUFFER_LAST];
	T_can_msg_t		can_rx_msg_buffer[CAN_RX_MSG_BUFFER_LAST];
	Uint8			nodeID_DIP;    /* Node ID as set by DIP switches 2,3,4,5 */
}T_can_data_t;

/* Global variables */
#if DBG_TIME_CAN
extern clock_t t_can[5]; // C1 ISR, TX Results, TX Status, TX Raw, actual Transmit (TX) function
#endif

/* Function declarations */
void Can_init(void);
void Can_transmit_wireguid_result(const T_wireGuid_t *wire_guid_data, T_can_data_t *can_data, Uint8 *msg_content);
void Can_transmit_wireguid_status(const T_wireGuid_t *wire_guid_data, T_can_data_t *can_data, Uint8 *msg_content);
void Can_transmit_wireguid_raw(const T_wireGuid_t *wire_guid_data, T_can_data_t *can_data, Uint8 *msg_content);
void Can_transmit_wireguid_switches(const T_wireGuid_t *wire_guid_data, T_can_data_t *can_data, Uint8 *msg_content);

#endif  // End of __HAL_CAN_H definition
