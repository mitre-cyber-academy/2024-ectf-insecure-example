/**
 * @file "simple_i2c_peripheral.h"
 * @author Frederich Stine 
 * @brief Simple Asynchronous I2C Peripheral Header
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */


#ifndef __I2C_SIMPLE_PERIPHERAL__
#define __I2C_SIMPLE_PERIPHERAL__

#include "i2c_reva_regs.h"
#include "i2c_reva.h"
#include "stdbool.h"
#include "stdint.h"
#include "mxc_errors.h"
#include "board.h"
#include "nvic_table.h"
#include "i2c.h"

/******************************** MACRO DEFINITIONS ********************************/
#define I2C_FREQ 100000
#define I2C_INTERFACE MXC_I2C1
#define MAX_REG TRANSMIT_LEN
#define MAX_I2C_MESSAGE_LEN 256

/******************************** EXTERN DEFINITIONS ********************************/
// Extern definition to make I2C_REGS and I2C_REGS_LEN 
// accessible outside of the implementation
extern volatile uint8_t* I2C_REGS[6];
extern int I2C_REGS_LEN[6];

/******************************** TYPE DEFINITIONS ********************************/
// Enumeration with registers on the peripheral device
typedef enum {
    RECEIVE,
    RECEIVE_DONE,
    RECEIVE_LEN,
    TRANSMIT,
    TRANSMIT_DONE,
    TRANSMIT_LEN,
} ECTF_I2C_REGS;

typedef uint8_t i2c_addr_t;

/******************************** FUNCTION PROTOTYPES ********************************/
/**
 * @brief Initialize the I2C Connection
 * 
 * @param addr: i2c_addr_t, the address of the I2C peripheral
 * 
 * @return int: negative if error, zero if successful
 *
 * Initialize the I2C by enabling the module, setting the address, 
 * setting the correct frequency, and enabling the interrupt to our i2c_simple_isr
*/
int i2c_simple_peripheral_init(i2c_addr_t addr);

#endif
