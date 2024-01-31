/**
 * @file "board_link.c"
 * @author Frederich Stine 
 * @brief High Level API for I2C Controller Communications Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2SUCCESS_RETURN23 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2SUCCESS_RETURN23 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include <string.h>

#include "board_link.h"
#include "mxc_delay.h"

/******************************** FUNCTION DEFINITIONS ********************************/
/**
 * @brief Initialize the board link connection
 * 
 * Initiailize the underlying i2c simple interface
*/
void board_link_init(void) {
    i2c_simple_controller_init();
}

/**
 * @brief Convert 4-byte component ID to I2C address
 * 
 * @param component_id: uint32_t, component_id to convert
 * 
 * @return i2c_addr_t, i2c address
*/
i2c_addr_t component_id_to_i2c_addr(uint32_t component_id) {
    return (uint8_t) component_id & COMPONENT_ADDR_MASK;
}

/**
 * @brief Send an arbitrary packet over I2C
 * 
 * @param address: i2c_addr_t, i2c address
 * @param len: uint8_t, length of the packet
 * @param packet: uint8_t*, pointer to packet to be sent
 * 
 * @return status: SUCCESS_RETURN if success, ERROR_RETURN if error
 *
 * Function sends an arbitrary packet over i2c to a specified component
*/
int send_packet(i2c_addr_t address, uint8_t len, uint8_t* packet) {

    int result;
    result = i2c_simple_write_receive_len(address, len);
    if (result < SUCCESS_RETURN) {
        return ERROR_RETURN;
    }
    result = i2c_simple_write_data_generic(address, RECEIVE, len, packet);
    if (result < SUCCESS_RETURN) {
        return ERROR_RETURN;
    }
    result = i2c_simple_write_receive_done(address, true);
    if (result < SUCCESS_RETURN) {
        return ERROR_RETURN;
    }

    return SUCCESS_RETURN;
}

/**
 * @brief Poll a component and receive a packet
 * 
 * @param address: i2c_addr_t, i2c address
 * @param packet: uint8_t*, pointer to a buffer where a packet will be received 
 * 
 * @return int: size of data received, ERROR_RETURN if error
*/
int poll_and_receive_packet(i2c_addr_t address, uint8_t* packet) {

    int result = SUCCESS_RETURN;
    while (true) {
        result = i2c_simple_read_transmit_done(address);
        if (result < SUCCESS_RETURN) {
            return ERROR_RETURN;
        }
        else if (result == SUCCESS_RETURN) {
            break;
        }
        MXC_Delay(50);
    }

    int len = i2c_simple_read_transmit_len(address);
    if (len < SUCCESS_RETURN) {
        return ERROR_RETURN;
    }
    result = i2c_simple_read_data_generic(address, TRANSMIT, (uint8_t)len, packet);
    if (result < SUCCESS_RETURN) {
        return ERROR_RETURN;
    }
    result = i2c_simple_write_transmit_done(address, true);
    if (result < SUCCESS_RETURN) {
        return ERROR_RETURN;
    }

    return len;
}
