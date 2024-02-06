/**
 * @file "board_link.c"
 * @author Frederich Stine 
 * @brief High Level API for I2C Controller Communications Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include <string.h>

#include "board_link.h"

/**
 * @brief Initialize the board link interface
 *
 * @param addr: i2c_addr_t: address of this i2c device
 *
 * @return int: negative if error, zero if successful
 *
 * Initialized the underlying i2c_simple interface
*/
int board_link_init(i2c_addr_t addr) {
    return i2c_simple_peripheral_init(addr);
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
 * @brief Send a packet to the AP and wait for ACK
 * 
 * @param len: uint8_t, length of the packet
 * @param packet: uint8_t*, message to be sent
 * This function utilizes the simple_i2c_peripheral library to
 * send a packet to the AP and wait for the message to be received
*/
void send_packet_and_ack(uint8_t len, uint8_t* packet) {
    I2C_REGS[TRANSMIT_LEN][0] = len;
    memcpy((void*)I2C_REGS[TRANSMIT], (void*)packet, len);
    I2C_REGS[TRANSMIT_DONE][0] = false;

    // Wait for ack from AP
    while(!I2C_REGS[TRANSMIT_DONE][0]);
    I2C_REGS[RECEIVE_DONE][0] = false;
}

/**
 * @brief Wait for a new message from AP and process the message
 * 
 * @param packet: uint8_t*, message received
 * 
 * This function waits for a new message to be available from the AP,
 * once the message is available it is returned in the buffer pointer to by packet 
*/
uint8_t wait_and_receive_packet(uint8_t* packet) {
    while(!I2C_REGS[RECEIVE_DONE][0]);

    uint8_t len = I2C_REGS[RECEIVE_LEN][0];
    memcpy(packet, (void*)I2C_REGS[RECEIVE], len);

    return len;
}

// This different from the function above for adding timer to it.
// Waiting that has passed 0.3 seconds will stop to prevent replay attack
// QUESTIONS: will the I2C_REGS be refreshed everytime we calls this function so we will get the new message? We don't want the old queue message be read and procceed.
int timed_wait_and_receive_packet(uint8_t* packet) {
    // while(!I2C_REGS[RECEIVE_DONE][0])
    // Change the waiting for signal loop
    for(int i = 0; i < 3000000; ++i){
        if(!I2C_REGS[RECEIVE_DONE][0]){
            continue;
        }
        else{
            uint8_t len = I2C_REGS[RECEIVE_LEN][0];
            memcpy(packet, (void*)I2C_REGS[RECEIVE], len);
            return 1;
        }
    }

    return -1;
}


/**
 * @brief Send a encryped packet to the AP and wait for ACK
 * 
 * @param len: uint8_t, length of the packet
 * @param packet: uint8_t*, message to be sent
 * @param GLOBAL_KEY: 16 byte globel key
 * This function utilizes the simple_i2c_peripheral library to
 * send a packet to the AP and wait for the message to be received
*/
void secure_send_packet_and_ack(uint8_t len, uint8_t* packet, uint8_t* GLOBAL_KEY) {
    encrypt_sym(packet, len, GLOBAL_KEY, ciphertext);
    send_packet_and_ack(len, ciphertext);
}


/**
 * @brief Wait for a new message from AP, decrypt and process the message
 * 
 * @param packet: uint8_t*, message received
 * @param GLOBAL_KEY: 16 byte globel key
 * This function waits for a new message to be available from the AP,
 * once the message is available it is returned in the buffer pointer to by packet 
*/
uint8_t secure_wait_and_receive_packet(uint8_t* packet, uint8_t* GLOBAL_KEY) {
    uint8_t plaintext[MAX_I2C_MESSAGE_LEN];
    uint8_t len = wait_and_receive_packet(packet);
    decrypt_sym(packet, MAX_I2C_MESSAGE_LEN, GLOBAL_KEY, plaintext);
    memmove(packet, plaintext, MAX_I2C_MESSAGE_LEN);
    return len;
}