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
#include "board_link.h"
#include "simple_i2c_peripheral.h"

// Includes from containerized build
#include "ectf_params.h"

// Include cache disable
#include "disable_cache.h"
#include "Rand_lib.h"
#include "key_exchange.h"
#include <inttypes.h>


#ifdef POST_BOOT
#include "led.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#endif

/********************************* Global Variables **********************************/

// Random Number
#define RAND_Z_SIZE 8
uint8_t RAND_Z[RAND_Z_SIZE];

// Maximum length of an I2C register
#define MAX_I2C_MESSAGE_LEN 256

// 16 bytes
#define AES_SIZE 16

// Success & Error flags
#define SUCCESS_RETURN 0
#define ERROR_RETURN -1

typedef enum {
    COMPONENT_CMD_NONE,
    COMPONENT_CMD_SCAN,
    COMPONENT_CMD_VALIDATE,
    COMPONENT_CMD_BOOT,
    COMPONENT_CMD_ATTEST,
    COMPONENT_CMD_SECURE_SEND_VALIDATE,
    COMPONENT_CMD_SECURE_SEND_CONFIMRED,
} component_cmd_t;

/******************************** Testing Variables ********************************/

uint32_t COMP_ID1 = 123456789;
uint32_t COMP_ID2 = 987654321;

uint8_t GLOBAL_KEY[AES_SIZE]; // Need to define this better

/******************************** TYPE DEFINITIONS ********************************/

// Datatype for all messages
typedef struct {
    uint8_t opcode;
    uint8_t comp_ID[4];
    uint8_t rand_z[RAND_Z_SIZE];
    uint8_t rand_y[RAND_Z_SIZE];    
    uint8_t remain[MAX_I2C_MESSAGE_LEN   - 21];
} message;

void uint32_to_uint8(uint8_t str_uint8[4], uint32_t str_uint32) {
    for (int i = 0; i < 4; i++) str_uint8[i] = (uint8_t)(str_uint32 >> 8 * (3-i));
}

void uint8Arr_to_uint8Arr(uint8_t target[RAND_Z_SIZE], uint8_t control[RAND_Z_SIZE]) {
    for (int i = 0; i < RAND_Z_SIZE; i++) target[i] = (control[i]);
}

bool random_checker(uint8_t target[RAND_Z_SIZE], uint8_t control[RAND_Z_SIZE]) {
    for (int i = 0; i < RAND_Z_SIZE; i++){
        if(target[i] != control[i]){
            return false;
        }
    }
    return true;
}

int uint8_uint32_cmp(uint8_t str_uint8[4], uint32_t str_uint32){
    int counter = 0;
    for(int i = 0; i < 4; i++)
        if(str_uint8[i] == (uint8_t)(str_uint32 >> (8 * (3-i)))) ++counter;
    return counter == 4;
}
int test_validate_and_boot_protocol(){
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    message* command = (message*)transmit_buffer;

    command->opcode = COMPONENT_CMD_VALIDATE;
    uint32_to_uint8(command->comp_ID, COMP_ID1);

    Rand_NASYC(RAND_Z, RAND_Z_SIZE);
    uint8Arr_to_uint8Arr(command->rand_z, RAND_Z);

    printf("Data before encryption:\n\n \
        opcode = %02x\n \
        Comp_ID1 = %"PRIu32"\n \
        Rand_Z = ", COMPONENT_CMD_VALIDATE & 0xff, COMP_ID1);
    for(int x = 0; x < RAND_Z_SIZE; x++){
        printf("%02x", RAND_Z[x] & 0xff);
    }

    uint8_t ciphertext[MAX_I2C_MESSAGE_LEN];

    encrypt_sym(transmit_buffer, MAX_I2C_MESSAGE_LEN, GLOBAL_KEY, ciphertext);
    

    printf("\n\nCiphertext: \n");
    for(int x = 0; x < MAX_I2C_MESSAGE_LEN; x++){
        printf("%02x", ciphertext[x] & 0xff);
    }
    
    decrypt_sym(ciphertext, MAX_I2C_MESSAGE_LEN, GLOBAL_KEY, receive_buffer);

    message* response = (message* )receive_buffer;

    int result = uint8_uint32_cmp(response->comp_ID, COMP_ID1);
    int wrong = uint8_uint32_cmp(response->comp_ID, COMP_ID2);
    int rand_z_check = random_checker(response->rand_z, RAND_Z);

    printf("\n\nData after decryption:\n\n \
    opcode = %02x\n \
    Is the recieved id == COMP_ID1?  %d\n \
    Is the received id == COMP_ID2?  %d\n \
    Are both Z's equal to each other?  %d\n \
    Rand_Z = ", response->opcode & 0xff, result, wrong, rand_z_check);
    for(int x = 0; x < RAND_Z_SIZE; x++){
        printf("%02x", response->rand_z[x] & 0xff);
    }
    printf("\n DONE. \n\n");

    return SUCCESS_RETURN;
}

int main() {
    test_validate_and_boot_protocol();
    return 0;
}
