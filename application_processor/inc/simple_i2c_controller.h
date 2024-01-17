/**
 * @file "simple_i2c_controller.h"
 * @author Frederich Stine 
 * @brief Simple Synchronous I2C Controller Header
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#ifndef __I2C_SIMPLE_CONTROLLER__
#define __I2C_SIMPLE_CONTROLLER__

#include "stdbool.h"
#include "stdint.h"
#include "mxc_errors.h"
#include "board.h"
#include "nvic_table.h"
#include "i2c.h"
#include "string.h"

/******************************** MACRO DEFINITIONS ********************************/
// I2C frequency in HZ
#define I2C_FREQ 100000
// Physical I2C interface
#define I2C_INTERFACE MXC_I2C1
// Last register for out-of-bounds checking
#define MAX_REG TRANSMIT_LEN 
// Maximum length of an I2C register
#define MAX_I2C_MESSAGE_LEN 256

/******************************** TYPE DEFINITIONS ********************************/
/* ECTF_I2C_REGS
 * Emulated hardware registers for sending and receiving I2C messages
*/ 
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
 * Initialize the I2C by enabling the module, setting the correct
 * frequency, and enabling the interrupt to our I2C_Handler
*/
int i2c_simple_controller_init(void);

/**
 * @brief Read RECEIVE_DONE reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * 
 * @return int: RECEIVE_DONE value, negative if error
 *
 * Read the RECEIVE_DONE for an I2C peripheral
 * and return the value 
*/
int i2c_simple_read_receive_done(i2c_addr_t addr);
/**
 * @brief Read RECEIVE_LEN reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * 
 * @return int: RECEIVE_LEN value, negative if error
 *
 * Read the RECEIVE_LEN for an I2C peripheral
 * and return the value 
*/
int i2c_simple_read_receive_len(i2c_addr_t addr);
/**
 * @brief Read TRANSMIT_DONE reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * 
 * @return int: TRANSMIT_DONE value, negative if error
 *
 * Read the TRANSMIT_DONE for an I2C peripheral
 * and return the value 
*/
int i2c_simple_read_transmit_done(i2c_addr_t addr);
/**
 * @brief Read TRANSMIT_LEN reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * 
 * @return int: TRANSMIT_LEN value, negative if error
 * 
 * Read the TRANSMIT_LEN for an I2C peripheral
 * and return the value 
*/
int i2c_simple_read_transmit_len(i2c_addr_t addr);

/**
 * @brief Write RECEIVE_DONE reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param done: uint8_t, RECEIVE_DONE value
 * 
 * @return int: negative if error, 0 if success
 * 
 * Write the RECEIVE_DONE reg for an I2C peripheral to the 
 * specified value 
*/
int i2c_simple_write_receive_done(i2c_addr_t addr, bool done);
/**
 * @brief Write RECEIVE_LEN reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param len: uint8_t, RECEIVE_LEN value
 * 
 * @return int: negative if error, 0 if success
 * 
 * Write the RECEIVE_LEN reg for an I2C peripheral to the 
 * specified value
*/
int     i2c_simple_write_receive_len(i2c_addr_t addr, uint8_t len);
/**
 * @brief Write TRANSMIT_DONE reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param done: bool, TRANSMIT_DONE value
 * 
 * @return int: negative if error, 0 if success
 *
 * Write the TRANSMIT_DONE reg for an I2C peripheral to the 
 * specified value
*/
int i2c_simple_write_transmit_done(i2c_addr_t addr, bool done);
/**
 * @brief Write TRANSMIT_LEN reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param len: uint8_t, TRANSMIT_LEN value
 * 
 * @return int: negative if error, 0 if success
 *
 * Write the TRANSMIT_LEN reg for an I2C peripheral to the 
 * specified value
*/
int i2c_simple_write_transmit_len(i2c_addr_t addr, uint8_t len);

/**
 * @brief Read generic data reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param reg: ECTF_I2C_REGS, register to read from
 * @param len: uint8_t, length of data to read
 * @param buf: uint8_t*, buffer to read data into
 * 
 * @return int: negative if error, 0 if success
 * 
 * Read any register larger than 1B in size
 * Can be used to read the PARAMS or RESULT register
*/
int i2c_simple_read_data_generic(i2c_addr_t addr, ECTF_I2C_REGS reg, uint8_t len, uint8_t* buf);
/**
 * @brief Write generic data reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param reg: ECTF_I2C_REGS, register to write to
 * @param len: uint8_t, length of data to write
 * @param buf: uint8_t*, buffer to write data from
 * 
 * @return int: negative if error, 0 if success
 * 
 * Write any register larger than 1B in size
 * Can be used to write the PARAMS or RESULT register
*/
int i2c_simple_write_data_generic(i2c_addr_t addr, ECTF_I2C_REGS reg, uint8_t len, uint8_t* buf);
/**
 * @brief Read generic status reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param reg: ECTF_I2C_REGS, register to read to
 *
 * @return int: value returned from device, negative if error
 * 
 * Read any register that is 1B in size
*/
int i2c_simple_read_status_generic(i2c_addr_t addr, ECTF_I2C_REGS reg);
/**
 * @brief Write generic status reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * @param reg: ECTF_I2C_REGS, register to write to
 * @param value: uint8_t, value to write to register
 * 
 * @return int: negative if error, 0 if success
 *
 * Write any register that is 1B in size
*/
int i2c_simple_write_status_generic(i2c_addr_t addr, ECTF_I2C_REGS reg, uint8_t value);

#endif
