// 2014 - 2015

/*! \file can.c
    \brief Contains function implementations related to the CAN interface
*/

#include "project_canantenna.h"

//*****************************************************************************
// Constants
//*****************************************************************************
static const Uint16 __attribute__((space(psv)))
CAN_PDO_SID_RX_START_CALIBRATION = 0x0200U;
static const Uint16 __attribute__((space(auto_psv)))
CAN_PDO_SID_RX_CONFIG_FREQS = 0x0300U;
static const Uint16 __attribute__((space(auto_psv)))
CAN_PDO_SID_TX_DEVIATION = 0x0180U;
static const Uint16 __attribute__((space(auto_psv)))
CAN_PDO_SID_TX_STATUS_QUALITY = 0x0280U;
static const Uint16 __attribute__((space(auto_psv)))
CAN_PDO_SID_TX_RAW = 0x0380U;
static const Uint16 __attribute__((space(auto_psv)))
CAN_PDO_SID_TX_SWITCH_STATES = 0x0480U;

// Global variables
#if DBG_TIME_CAN
clock_t t_can[5]; // C1 ISR, TX Results, TX Status, TX Raw, actual Transmit (TX) function
#endif

//*****************************************************************************
// Static functions
//*****************************************************************************
/* Put can into configuration mode */
static void can_set_in_config_mode(void)
{
	/* Request configuration mode */
	C1CTRLbits.REQOP  = 4;
  
	/* CAN master clock is FCY */
	C1CTRLbits.CANCKS = 1;
  
	/* Wait until configuration mode entered */
	while(C1CTRLbits.OPMODE != 4)
	{
	}
  
	return;
}

/*************************************************************************/
/* This function sets the values for the acceptance filter masks for standard 
   length identifiers.
   Parameters: 	Uint8:  mask_no:  	(Mask number 0-1)
						Uint16: mask:     	(Mask value)
                   
   C1RxMnSID = 0x0001 -> Only accept one message type as specified in EXIDE
*/
static void can_set_mask_rx(Uint8   mask_no, Uint16  mask)
{
	switch(mask_no)
	{
		case 0: 
			C1RXM0SID = 0x0001;
			C1RXM0SIDbits.SID = mask & 0x7FF;
			break;
            
		case 1: 
			C1RXM1SID = 0x0001;
			C1RXM1SIDbits.SID = mask & 0x7F;
			break;

		default:
			C1RXM0SID = 0x0001;
			C1RXM0SIDbits.SID = mask & 0x7F;
		break;
	}
	return;
}

/***********************************************************************/
/* This function sets the acceptance filter, SID for standard length identifiers.
   Parameters: 	Uint8 : filter_no:  (Filter number 0-5)
						Uint16: sid:        (SID value)
                       
   C1RXF0SID = 0x0000 -> Enable filter for standard identifier
*/
static void can_set_filter_rx(Uint8   filter_no, Uint16  sid)
{
	switch(filter_no)
	{
		case 0: 
			C1RXF0SID = 0x0000;
			C1RXF0SIDbits.SID = (sid & 0x07FF);
			break;
      
		case 1: 
			C1RXF1SID = 0x0000;
			C1RXF1SIDbits.SID = (sid & 0x07FF);
			break;
      
		case 2: 
			C1RXF2SID = 0x0000;
			C1RXF2SIDbits.SID = (sid & 0x07FF);
			break;

		case 3: 
			C1RXF3SID = 0x0000;
			C1RXF3SIDbits.SID = (sid & 0x07FF);
			break;
      
		case 4: 
			C1RXF4SID = 0x0000;
			C1RXF4SIDbits.SID = (sid & 0x07FF);
			break;
      
		case 5: 
			C1RXF5SID = 0x0000;
			C1RXF5SIDbits.SID = (sid & 0x07FF);
			break;

		default: 
			C1RXF0SID = 0x0000;
			C1RXF0SIDbits.SID = (sid & 0x07FF);
			break;
	}

	return;
}

/*********************************************************************/
/* Sets baud rate to 250Kbps with 10Tq for a clock of 20MHz
   TODO LVDK: read DIP switches (PORTF) to set baud rate accordingly
*/
static void can_configure(void)
{
	/*  Nominal Bit Time NBT = (SJW + PRSEG + SEG1PH + SEG2PH) * Tq = 10Tq
		Nominal Bit Rate NBR = 1 / 10Tq = 1 / 4 usec = 250Kbps 
	*/
	C1CFG1bits.BRP      	= 3;    /* 20 MHz clock */
	C1CFG1bits.SJW      	= 0;    /* Synchronized jump width time = 1Tq */
	C1CFG2bits.PRSEG    	= 4;    /* Propagation time segment = 5Tq */
	C1CFG2bits.SEG1PH   	= 1;    /* Phase buffer Segment 1 = 2Tq */
	C1CFG2bits.SAM      	= 0;    /* Bus is sampled once */
	C1CFG2bits.SEG2PHTS= 1;    /* PhaseSeg2 is freely programmable */
	C1CFG2bits.SEG2PH   	= 1;    /* Phase buffer Segment 2 = 2Tq */
	C1CTRLbits.REQOP    	= 0;    /* Can clock is Fcy = 20MHz. Request normal operation mode. */
	/*  NBT = (1 + 5 + 2 + 2) * Tq = 10Tq
		point where Sampling of bit takes place: (1 + 5 + 2) / 10 = 8 / 10 = 80%
	*/
	while(C1CTRLbits.OPMODE != 0)
	{
	}
  
	return;
}

/*************************************************************************/
/* This function sets the message transmission priority bits.
   Parameters: 	Uint16: buffer:   (Transmit buffer number 0-3)
						Uint16: priority: (Transmit priority 0-3)
*/
static void can_set_priority(Uint8   buffer, Uint8   priority)
{
	if (priority > 3U)
	{
		priority = 3U;
	}

	switch(buffer)
	{
		case 0: C1TX0CONbits.TXPRI = priority;
					break;
		case 1: C1TX1CONbits.TXPRI = priority;
					break;
		case 2: C1TX2CONbits.TXPRI = priority;
					break;
		default:
					break;
	}

	return;
}

/*************************************************************************/
/* This function returns an available empty transmit buffer.
   Returns: Transmit buffer number 0-2.
*/
static Uint8 can_get_txbuffer(void)
{
	Uint8 buffer = 0U;

	if(C1TX0CONbits.TXREQ == 0)
	{
		return buffer; // buffer = 0
	}
	else if(C1TX1CONbits.TXREQ == 0)
	{
		buffer = 1U;
	}
	else if(C1TX2CONbits.TXREQ == 0)
	{
		buffer = 2U;
	}

	return buffer;
}

/*********************************************************************/
/* This function writes the message identifier(SID), writes the data to be
   transmitted into the Transmit buffer and sets the corresponding Transmit
   request bit.

   Parameters: Pointer to structure T_can_msg_t defined in can.h
*/
static void Can_transmit_message(
  const T_can_msg_t *message)
{
	/* Implement internal timer for Transmission function */
	// start step instruction-counter
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[4] = clock();
	#endif

	Uint8   buffer;
	Uint16  ix;

	/* Divide 11 bits of sid over 2 SID's in CiTXnSID */
	ix = ((message->sid & 0x07C0) << 5) | ((message->sid & 0x003F) << 2); // result: ix = xxxx x000 xxxx xx00 (x == 0 OR 1)
	buffer = can_get_txbuffer();

	switch(buffer)
	{
		case 0:	C1TX0SID = ix;
					// Reset ix variable and use it as index of the message (range 0 ... 7)
					for(ix = 0U; ix < message->length; ++ix)
						*((Uint8 *)&C1TX0B1+ix)= message->content[ix];
					C1TX0DLC = 0x0180;
					C1TX0DLCbits.DLC = message->length;
					C1TX0CONbits.TXREQ = 1;
					break;
					
		case 1:	C1TX1SID = ix;
					// Reset ix variable and use it as index of the message (range 0 ... 7)
					for(ix = 0U; ix < message->length; ++ix)
						*((Uint8 *)&C1TX1B1+ix)= message->content[ix];
					C1TX1DLC = 0x0180;
					C1TX1DLCbits.DLC = message->length;
					C1TX1CONbits.TXREQ = 1;
					break;
					
		case 2:	 C1TX2SID = ix;
					// Reset ix variable and use it as index of the message (range 0 ... 7)
					for(ix = 0U; ix < message->length; ++ix)
						*((Uint8 *)&C1TX2B1+ix)= message->content[ix];
					C1TX2DLC = 0x0180;	
					C1TX2DLCbits.DLC = message->length;
					C1TX2CONbits.TXREQ = 1;
					break;
					
		default:	C1TX0SID = ix;
					// Reset ix variable and use it as index of the message (range 0 ... 7)
					for(ix = 0U; ix < message->length; ++ix)
						*((Uint8 *)&C1TX0B1+ix)= message->content[ix];
					break;
	}

	/*  End timer of Transmission function */
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[4] = clock() - t_can[4];
	#endif
  
	return;
}

/*************************************************************************/
/* If a message has been received, read the data from the receive buffer into 
   the structure T_can_msg_t and clear the RXFUL bit. */
static void Can_receive_message(
  E_can_rx_buffer_t   buffer_id,
  Uint8              *msg_content)
{
	Uint8         ix;
	Uint16		check;
	T_can_msg_t  *message;

	message = &(gSystemData.can_data.can_rx_msg_buffer[buffer_id]);

	switch(buffer_id)
	{
		case 0:	message->sid    = (C1RX0SID >> 2) & 0x07ff;
					message->length = C1RX0DLCbits.DLC;
					for(ix = 0U; ix < message->length; ++ix)
						msg_content[ix] = *((Uint8 *)&C1RX0B1 + ix);
					break;
			
		case 1: message->sid    = (C1RX1SID >> 2) & 0x07ff;
					message->length = C1RX1DLCbits.DLC;
					for(ix = 0U; ix < message->length; ++ix)
						msg_content[ix] = *((Uint8 *)&C1RX1B1 + ix);
					break;
					
		default: break;
	}

	check = message->sid - (Uint16)gSystemData.can_data.nodeID_DIP;
	
	/* Process data */
	if(check == CAN_PDO_SID_RX_START_CALIBRATION)
		gGuidanceData.wireGuidData.calibration_status = WG_CALIB_STATUS_START;
	else if(check ==	CAN_PDO_SID_RX_CONFIG_FREQS)
	{
		ANT_Set_Freqs(msg_content, gGuidanceData.wireGuidData.frequencies,
								&(gGuidanceData.wireGuidData.freq_status));
		ANT_Store_Freqs(gGuidanceData.wireGuidData.frequencies, 
									&(gGuidanceData.wireGuidData.freq_status));
	}

	return;
}

//*****************************************************************************
// Local functions
//*****************************************************************************
/*! Can_init() is used to configure CAN module.
*/
void Can_init(void)
{
	// Initialize instruction-counters for the CAN module timing
	#if DBG_TIME_CAN
	t_can[0] = 0;
	t_can[1] = 0;
	t_can[2] = 0;
	t_can[3] = 0;
	t_can[4] = 0; 
	#endif

	/* Obtain node ID: restart system if nodeID changed as it requires new filters
		for CAN message reception (CAN system needs to be in configuration mode) */
	gSystemData.can_data.nodeID_DIP = ((~PORTB) & 0x000F);

	/* Go into configuration mode */
	can_set_in_config_mode();

	/* Receive buffer 0 status and control register
		- No receive buffer overflow from RB0 to RB1 (for now) */
	C1RX0CONbits.DBEN = 0;
  
	/* Transmit buffer n Standard identifier
		- Enable filter for standard identifier */
	C1TX0SIDbits.TXIDE = 0;
	C1TX1SIDbits.TXIDE = 0;
	C1TX2SIDbits.TXIDE = 0;

	/* Reset receive buffer */
	C1RX0B1 = 0;
	C1RX0B2 = 0;
	C1RX0B3 = 0;
	C1RX0B4 = 0;
  
	/* Set masks and filters */
	can_set_mask_rx(0U,0x000FU);
	can_set_mask_rx(1U,0x000FU);

	can_set_filter_rx(0,0x0200 + (Uint16)(gSystemData.can_data.nodeID_DIP));
	can_set_filter_rx(1,0x0300 + (Uint16)(gSystemData.can_data.nodeID_DIP));

	can_set_priority(0U,2U);
	can_set_priority(1U,2U);
	can_set_priority(2U,2U);
  
	can_configure();

	return;
}

// Check when testing whether published values are the same as the values of the deviation variables
void Can_transmit_wireguid_result(
  const T_wireGuid_t	*wire_guid_data,
  T_can_data_t			*can_data,
  Uint8                 		*msg_content) 
                                         
{
	/* Implement internal timer for TX Results */
	// start step instruction-counter
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[1] = clock();
	#endif
  
	T_can_msg_t    *can_msg;
	Uint8           nodeID;

	can_msg  = &(can_data->can_tx_msg_buffer[CAN_TX_MSG_BUFFER_0]);
	nodeID   = can_data->nodeID_DIP;

	can_msg->sid    = CAN_PDO_SID_TX_DEVIATION + (Uint16)nodeID;
	can_msg->length = 8U;
  
	/* Fill content with deviations:
		- msg: [0x18n  8  deviation f4, deviation f3, deviation f2, deviation f1]*/
	/* Frequency 4 */
	msg_content[0] = ((wire_guid_data->deviation_m2ecm[3] & 0xFF00) >> 8); // upper 8 bits
	msg_content[1] = ((wire_guid_data->deviation_m2ecm[3] & 0x00FF) >> 0); // lower 8 bits 
	/* Frequency 3 */
	msg_content[2] = ((wire_guid_data->deviation_m2ecm[2] & 0xFF00) >> 8);
	msg_content[3] = ((wire_guid_data->deviation_m2ecm[2] & 0x00FF) >> 0);
	/* Frequency 2 */
	msg_content[4] = ((wire_guid_data->deviation_m2ecm[1] & 0xFF00) >> 8);
	msg_content[5] = ((wire_guid_data->deviation_m2ecm[1] & 0x00FF) >> 0);
	/* Frequency 1 */
	msg_content[6] = ((wire_guid_data->deviation_m2ecm[0] & 0xFF00) >> 8);
	msg_content[7] = ((wire_guid_data->deviation_m2ecm[0] & 0x00FF) >> 0);
  
	/* Implement internal timer for Transmission function (call) */
	// start step instruction-counter
	#ifdef FUNCTION_CALL_CAN
	t_can[4] = clock();
	#endif
  
	Can_transmit_message(can_msg);
  
	/*  End timer of Transmission function (call) */
	#ifdef FUNCTION_CALL_CAN
	t_can[4] = clock() - t_can[4];
	#endif
  
	/*  End timer of TX Results */
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[1] = clock() - t_can[1];
	#endif
  
	return;
}

void Can_transmit_wireguid_raw(
  const T_wireGuid_t	*wire_guid_data,
  T_can_data_t			*can_data,
  Uint8                 		*msg_content) 
                                         
{
	/* Implement internal timer for TX Raws */
	// start step instruction-counter
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[3] = clock();
	#endif

	T_can_msg_t *can_msg;
	Uint8             nodeID;

	can_msg  = &(can_data->can_tx_msg_buffer[CAN_TX_MSG_BUFFER_2]);
	nodeID   = can_data->nodeID_DIP;

	can_msg->sid    = CAN_PDO_SID_TX_RAW + (Uint16)nodeID;
	can_msg->length = 8U;

	/* Fill content with amplitudes:
		- msg: [0x38n  8  amplitude f4, amplitude f3, amplitude f2, amplitude f1]*/
	#if DBG_PWM
	/* Debug PWM mode - Check Test Freq. Resulting Amplitude values*/
	/**/msg_content[0] = (Uint8)(wire_guid_data->amplitudePWM[0]);/****/
	/**/msg_content[1] = (Uint8)(wire_guid_data->amplitudePWM[1]);/****/
	/*******************************************************************************/
	#else
	/* Frequency 4 */
	msg_content[0] = (Uint8)(wire_guid_data->amplitudeLeft[3]);	// amplitudes 'Uint32' type -> Uint8
	msg_content[1] = (Uint8)(wire_guid_data->amplitudeRight[3]);
	#endif
	/* Frequency 3 */
	msg_content[2] = (Uint8)(wire_guid_data->amplitudeLeft[2]);
	msg_content[3] = (Uint8)(wire_guid_data->amplitudeRight[2]);
	/* Frequency 2 */
	msg_content[4] = (Uint8)(wire_guid_data->amplitudeLeft[1]);
	msg_content[5] = (Uint8)(wire_guid_data->amplitudeRight[1]);
	/* Frequency 1 */
	msg_content[6] = (Uint8)(wire_guid_data->amplitudeLeft[0]);
	msg_content[7] = (Uint8)(wire_guid_data->amplitudeRight[0]);
  
	/* Implement internal timer for Transmission function (call) */
	// start step instruction-counter
	#ifdef FUNCTION_CALL_CAN
	t_can[4] = clock();
	#endif
  
	Can_transmit_message(can_msg);
  
	/*  End timer of Transmission function (call) */
	#ifdef FUNCTION_CALL_CAN
	t_can[4] = clock() - t_can[4];
	#endif

	/*  End timer of TX Raws */
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[3] = clock() - t_can[3];
	#endif
  
	return;
}

void Can_transmit_wireguid_status(
  const T_wireGuid_t	*wire_guid_data,
  T_can_data_t			*can_data,
  Uint8                 		*msg_content) 
                                         
{
	/* Implement internal timer for TX Statuses */
	// start step instruction-counter
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[2] = clock();
	#endif

	T_can_msg_t    *can_msg;
	Uint8           nodeID;
	int16           left_ok  = 0;
	int16           right_ok = 0;

	can_msg  	= &(can_data->can_tx_msg_buffer[CAN_TX_MSG_BUFFER_1]);
	nodeID   	= can_data->nodeID_DIP;

	can_msg->sid   	= CAN_PDO_SID_TX_STATUS_QUALITY + (Uint16)nodeID;
	can_msg->length 	= 8U;

	/* Fill content with status:
		- msg: [0x28n  8  Quality f1, f2, f3, f4, Left antenna status, right antenna status]*/
	#if SECOND_HARMONIC_FIRST_FREQUENCY
	/* Transmit relative Phase */
	msg_content[0] = (wire_guid_data->rel_phaseLeft[0] & 0x00FF);
	msg_content[1] = (wire_guid_data->rel_phaseRight[0] & 0x00FF);
	msg_content[2] = (wire_guid_data->rel_phaseLeft[1] & 0x00FF);	
	msg_content[3] = (wire_guid_data->rel_phaseRight[1] & 0x00FF);
	#else
	/* Void */
	msg_content[0] = 0x00U;
	msg_content[1] = 0x00U;
	msg_content[2] = 0x00U;
	msg_content[3] = 0x00U;
	#endif

	/* Status (to be added)*/
	/* Left and right antenna */
	if ((wire_guid_data->status_left_antenna.antenna_cable_ok +
        wire_guid_data->status_left_antenna.no_short_circuit +
        wire_guid_data->status_left_antenna.pilot_tone_ok) == 3)
	{
		left_ok = 1;
	}
	if ((wire_guid_data->status_right_antenna.antenna_cable_ok +
		wire_guid_data->status_right_antenna.no_short_circuit +
		wire_guid_data->status_right_antenna.pilot_tone_ok) == 3)
	{
		right_ok = 1;
	}

	msg_content[4] = ((((wire_guid_data->status_left_antenna.antenna_cable_ok & 0x0001) << 3) |
								((wire_guid_data->status_left_antenna.no_short_circuit  & 0x0001) << 2) |
								((wire_guid_data->status_left_antenna.pilot_tone_ok     & 0x0001) << 1) |
								(left_ok & 0x0001)) << 4) |
								((((wire_guid_data->status_right_antenna.antenna_cable_ok & 0x0001) << 3) |
								((wire_guid_data->status_right_antenna.no_short_circuit  & 0x0001) << 2) |
								((wire_guid_data->status_right_antenna.pilot_tone_ok     & 0x0001) << 1) |
								(right_ok & 0x0001))    );
	
	#if SECOND_HARMONIC_FIRST_FREQUENCY
	/* Transmit Phase Direction Correction */
	msg_content[5] = (wire_guid_data->direction_checked & 0x01);
	if (wire_guid_data->direction_checked)
		msg_content[6] = ((wire_guid_data->rel_phaseLeft_sign & 0x0F) << 4) | 
									(wire_guid_data->rel_phaseRight_sign & 0x0F);
	else
		msg_content[6] = 0x00U;
	#else
	/* Void */
	msg_content[5] = 0x00U;
	msg_content[6] = 0x00U;
	#endif
	/* Calibration indication */
	msg_content[7] = (Uint8)wire_guid_data->calibration_status;
	
	/* Implement internal timer for Transmission function (call) */
	// start step instruction-counter
	#ifdef FUNCTION_CALL_CAN
	t_can[4] = clock();
	#endif
  
	Can_transmit_message(can_msg);
  
	/*  End timer of Transmission function (call) */
	#ifdef FUNCTION_CALL_CAN
	t_can[4] = clock() - t_can[4];
	#endif

	/*  End timer of TX Statuses */
	#ifdef FUNCTION_INTERNAL_CAN
	t_can[2] = clock() - t_can[2];
	#endif
  
	return;
}

void Can_transmit_wireguid_switches(
  const T_wireGuid_t	*wire_guid_data,
  T_can_data_t			*can_data,
  Uint8                 		*msg_content) 
                                         
{
	T_can_msg_t    *can_msg;
	Uint8           nodeID;

	can_msg  	= &(can_data->can_tx_msg_buffer[CAN_TX_MSG_BUFFER_3]);
	nodeID   	= can_data->nodeID_DIP;

	can_msg->sid   	= CAN_PDO_SID_TX_SWITCH_STATES + (Uint16)nodeID;
	can_msg->length 	= 8U;

	/* Transmit status of QAM decoding */
	msg_content[0] = (Uint8)wire_guid_data->nibble_status;
	msg_content[1] = (Uint8)wire_guid_data->nibble_substatus;

	/* Transmit extracted switch states from left or right coils */
	msg_content[2] = (Uint8)wire_guid_data->tx_new_states;
	msg_content[3] = wire_guid_data->switch_states_to_be_sent;
	msg_content[4] = 0x00U;
	msg_content[5] = 0x00U;
	msg_content[6] = 0x00U;
	msg_content[7] = 0x00U;

	Can_transmit_message(can_msg);

	return;
}

/*! _C1Interrupt() is the CAN receive interrupt.*/
void __attribute__((interrupt, auto_psv)) _C1Interrupt(void)
{
	/* Implement internal timer for CAN Interrupt */
	// start step instruction-counter
	#if DBG_TIME_CAN
	t_can[0] = clock();
	#endif

	/* For now, only messages are transmitted and only receive interrupt is
		cleared. They are not processed */
	if (C1INTFbits.TX0IF) C1INTFbits.TX0IF = 0;
	if (C1INTFbits.TX1IF) C1INTFbits.TX1IF = 0;
	if (C1INTFbits.TX2IF) C1INTFbits.TX2IF = 0;
	if (C1INTFbits.RX0IF)
	{
		Can_receive_message(CAN_RX_MSG_BUFFER_0,
			gSystemData.can_data.can_rx_msg_buffer[CAN_RX_MSG_BUFFER_0].content);
		C1RX0CONbits.RXFUL 	= 0;
		C1INTFbits.RX0IF   		= 0;
	}
	if (C1INTFbits.RX1IF)
	{
		Can_receive_message(CAN_RX_MSG_BUFFER_1,
			gSystemData.can_data.can_rx_msg_buffer[CAN_RX_MSG_BUFFER_1].content);
		C1RX1CONbits.RXFUL 	= 0;
		C1INTFbits.RX1IF 			= 0;
	}
	if (C1INTFbits.WAKIF) C1INTFbits.WAKIF = 0; // Add wake-up handler code
	if (C1INTFbits.ERRIF) C1INTFbits.ERRIF = 0; // Add error handler code
	if (C1INTFbits.IVRIF) C1INTFbits.IVRIF = 0; // Add invalid message received handler code
	if ( (C1INTF & C1INTE) == 0 ) IFS1bits.C1IF = 0;

	/* End timer  for CAN Interrupt */
	#if DBG_TIME_CAN
	t_can[0] = clock() - t_can[0];
	#endif
}
