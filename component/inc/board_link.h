/**
 * @file "board_link.h"
 * @author Frederich Stine 
 * @brief High Level API for I2C Controller Communications Header
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#ifndef __BOARD_LINK__
#define __BOARD_LINK__

#include "simple_i2c_peripheral.h"

/******************************** MACRO DEFINITIONS ********************************/
// Last byte of the component ID is the I2C address
#define COMPONENT_ADDR_MASK 0x000000FF             
#define SUCCESS_RETURN 0
#define ERROR_RETURN -1

/******************************** FUNCTION PROTOTYPES ********************************/

/**
 * @brief Initialize the board link interface
 *
 * @param addr: i2c_addr_t: address of this i2c device
 *
 * @return int: negative if error, zero if successful
 *
 * Initialized the underlying i2c_simple interface
*/
int board_link_init(i2c_addr_t addr);

/**
 * @brief Convert 4-byte component ID to I2C address
 * 
 * @param component_id: uint32_t, component_id to convert
 * 
 * @return i2c_addr_t, i2c address
*/
i2c_addr_t component_id_to_i2c_addr(uint32_t component_id);

/**
 * @brief Send a packet to the AP and wait for ACK
 * 
 * @param message: uint8_t*, message to be sent
 * 
 * This function utilizes the simple_i2c_peripheral library to
 * send a packet to the AP and wait for the message to be received
*/
void send_packet_and_ack(uint8_t len, uint8_t* packet);

/**
 * @brief Wait for a new message from AP and process the message
 * 
 * @param packet: uint8_t*, message received
 * 
 * @return uint8_t: length of message received
 *
 * This function waits for a new message to be available from the AP,
 * once the message is available it is returned in the buffer pointer to by packet 
*/
uint8_t wait_and_receive_packet(uint8_t* packet);

#endif
