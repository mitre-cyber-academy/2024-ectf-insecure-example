/**
 * @file component.c
 * @author Jacob Doll
 * @brief eCTF Component Example Design Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded
 * System CTF (eCTF). This code is being provided only for educational purposes
 * for the 2024 MITRE eCTF competition, and may not meet MITRE standards for
 * quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include "board.h"
#include "i2c.h"
#include "led.h"
#include "mxc_delay.h"
#include "mxc_errors.h"
#include "nvic_table.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "Code_warehouse/c/Rand_lib.h"

#include "board_link.h"
#include "simple_i2c_peripheral.h"

// Includes from containerized build
#include "ectf_params.h"
#include "global_secrets.h"

// Include cache disable
#include "disable_cache.h"

#ifdef POST_BOOT
#include "led.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#endif

/********************************* CONSTANTS **********************************/

// Passed in through ectf-params.h
// Example of format of ectf-params.h shown here
/*
#define COMPONENT_ID 0x11111124
#define COMPONENT_BOOT_MSG "Component boot"
#define ATTESTATION_LOC "McLean"
#define ATTESTATION_DATE "08/08/08"
#define ATTESTATION_CUSTOMER "Fritz"
*/
//AES
#define AES_SIZE 16// 16 bytes
unit GLOBAL_KEY[AES_SIZE];
uint8_t synthesized=0; 

/******************************** TYPE DEFINITIONS
 * ********************************/
// Commands received by Component using 32 bit integer
typedef enum {
    uint8_t COMPONENT_CMD_NONE,
    uint8_t COMPONENT_CMD_SCAN,
    uint8_t COMPONENT_CMD_VALIDATE,
    uint8_t COMPONENT_CMD_BOOT,
    uint8_t COMPONENT_CMD_ATTEST,
} component_cmd_t;

/******************************** TYPE DEFINITIONS
 * ********************************/
// Data structure for receiving messages from the AP
typedef struct {
    uint8_t params[AES_SIZE];
} message;

typedef struct {
    uint32_t component_id;
} validate_message;


/********************************* FUNCTION DECLARATIONS
 * **********************************/
// Core function definitions
void component_process_cmd(void);
void process_boot(void);
void process_scan(void);
void process_validate(void);
void process_attest(void);

/********************************* GLOBAL VARIABLES
 * **********************************/
// Global varaibles
uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];
uint8_t string_buffer[MAX_I2C_MESSAGE_LEN];
uint8_t cipher_text_buffer[MAX_I2C_MESSAGE_LEN];
uint8_t plain_text_buffer[MAX_I2C_MESSAGE_LEN];
uint8_t plaintext[AES_SIZE];
uint8_t ciphertext[AES_SIZE];

/******************************* POST BOOT FUNCTIONALITY
 * *********************************/
/**
 * @brief Secure Send
 *
 * @param buffer: uint8_t*, pointer to data to be send
 * @param len: uint8_t, size of data to be sent
 *
 * Securely send data over I2C. This function is utilized in POST_BOOT
 * functionality. This function must be implemented by your team to align with
 * the security requirements.
 */
void secure_send(uint8_t *buffer, uint8_t len) {
    send_packet_and_ack(len, buffer);
}

/**
 * @brief Secure Receive
 *
 * @param buffer: uint8_t*, pointer to buffer to receive data to
 *
 * @return int: number of bytes received, negative if error
 *
 * Securely receive data over I2C. This function is utilized in POST_BOOT
 * functionality. This function must be implemented by your team to align with
 * the security requirements.
 */
int secure_receive(uint8_t *buffer) { return wait_and_receive_packet(buffer); }

/******************************* FUNCTION DEFINITIONS
 * *********************************/

// Example boot sequence
// Your design does not need to change this
void boot() {

// POST BOOT FUNCTIONALITY
// DO NOT REMOVE IN YOUR DESIGN
#ifdef POST_BOOT
    POST_BOOT
#else
    // Anything after this macro can be changed by your design
    // but will not be run on provisioned systems
    LED_Off(LED1);
    LED_Off(LED2);
    LED_Off(LED3);
    // LED loop to show that boot occurred
    while (1) {
        LED_On(LED1);
        MXC_Delay(500000);
        LED_On(LED2);
        MXC_Delay(500000);
        LED_On(LED3);
        MXC_Delay(500000);
        LED_Off(LED1);
        MXC_Delay(500000);
        LED_Off(LED2);
        MXC_Delay(500000);
        LED_Off(LED3);
        MXC_Delay(500000);
    }
#endif
}

// Handle a command from the AP
void component_process_cmd() {
    message* command = (message*) receive_buffer;
    memset(plaintext, 0, sizeof(plaintext));
    decrypt_sym(command->params, AES_SIZE, GLOBAL_KEY, plaintext);

    // Output to application processor dependent on command received
    switch (plaintext[0]) {
    case COMPONENT_CMD_VALIDATE:
        process_boot();
        break;
    case COMPONENT_CMD_SCAN:
        process_scan();
        break;
    case COMPONENT_CMD_ATTEST:
        process_attest();
        break;
    default:
        printf("Error: Unrecognized command received %d\n", command->opcode);
        break;
    }
}

// This if for the functionality of Boot
void process_boot() {
    // The AP requested a boot. 
    //Validate the Component ID
    for(int i = 0; i < 4; ++i){
        if(plaintext[1+i] != (uint8_t)((COMPONENT_ID >> 8*(3-i)) & 0xFF)){
            print_error("The Component ID checks failed at the component sided");
            return;
        }
    }
    //Validation passed
    //Starts Boot
    boot();
    //Send Boot comfirmation message back to AP
    plaintext[0] = COMPONENT_CMD_BOOT;
    memset(transmit_buffer, 0, sizeof(transmit_buffer));//DO WE NEED THIS?
    message * send_packet = (message*) transmit_buffer;
    encrypt_sym(plaintext, AES_SIZE, GLOBAL_KEY, send_packet->params);
    // memcpy((void*)transmit_buffer, ciphertext, AES_SIZE);
    send_packet_and_ack(sizeof(message), transmit_buffer);
}

void process_scan() {
    // The AP requested a scan. Respond with the Component ID
    memset(transmit_buffer, 0, sizeof(transmit_buffer));//DO WE NEED THIS?
    message* packet = (message*) transmit_buffer;
    for(int i = 0; i < 4; ++i){
        packet->params[i] = (uint8_t)((COMPONENT_ID >> 8*(3-i)) & 0xFF);
    }
    send_packet_and_ack(sizeof(message), transmit_buffer);
}

void process_attest() {
    // The AP requested attestation. Respond with the attestation data

    //Validate the Component ID; plaintext[1:4]
    for(int i = 0; i < 4; ++i){
        if(plaintext[1+i] != (uint8_t)((COMPONENT_ID >> 8*(3-i)) & 0xFF)){
            print_error("The Component ID checks failed at the component sided");
            return;
        }
    }

    // Start to move atttestation data into the transmit_buffer
    memset(string_buffer, 0, sizeof(string_buffer));
    uint8_t len = sprintf((char*)string_buffer, "LOC>%s\nDATE>%s\nCUST>%s\n",
                ATTESTATION_LOC, ATTESTATION_DATE, ATTESTATION_CUSTOMER) + 1;
    
    uint8_t encrypted_len = 8 + len + 1;

    //Move the size of the string into the start of the message
    plain_text_buffer[0] = encrypted_len;
    //Combine the sending plain text
    //Move the Z into the plain_text_buffer
    memset(plain_text_buffer, 0, sizeof(plain_text_buffer));
    for(int i = 0; i < 8: ++i){
        plain_text_buffer[i+1] = plaintext[i+5]; // plaintext[5:12]
    }
    //Move the string buffer into the plain_text_buffer
    for(int i = 0; i < len; ++i){
        plain_text_buffer[i+8] = string_buffer[i];
    }

    //This will encrypt the plain text by sgementing the text into different segement of 16 and encrypt them one by one
    memset(cipher_text_buffer, 0, sizeof(cipher_string_buffer));

    //Encrypt the message out
    encrypt_sym(plain_text_buffer, sizeof(plain_text_buffer), GLOBAL_KEY, cipher_text_buffer);
    //Move the cipher text into the transmit_buffer and reday for transfer
    memset(transmit_buffer, 0, sizeof(transmit_buffer));//DO WE NEED THIS?
    full_message* send_packet = (full_message*)transmit_buffer;
    memcpy(send_packet->param, cipher_text_buffer, sizeof(cipher_text_buffer));
    send_packet_and_ack(sizeof(full_message), transmit_buffer);
}


/*********************************** MAIN *************************************/

int main(void) {
    printf("Component Started\n");
    // Disable the cache
    disable_cache();
    // Enable Global Interrupts
    __enable_irq();

    // Initialize Component
    i2c_addr_t addr = component_id_to_i2c_addr(COMPONENT_ID);
    board_link_init(addr);

    LED_On(LED2);

    //We assume the first message from the AP will be merging the key.

    while (1) {
        wait_and_receive_packet(receive_buffer);
        if(synthesized == 0){
            //The key_sync takes the first argument as a char* array, don't know if it will cause problem
            //Since the GLOBAL_KEY is unit8_t
            key_sync(GLOBAL_KEY);
            synthesized = 1;
        }
        else{
            component_process_cmd();
        }
    }
}
