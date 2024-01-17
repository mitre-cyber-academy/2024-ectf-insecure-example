/**
 * @file "simple_i2c_controller.c"
 * @author Frederich Stine 
 * @brief Simple Synchronous I2C Controller Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */


#include "simple_i2c_controller.h"

/******************************** FUNCTION PROTOTYPES ********************************/
/**
 * @brief Built-In I2C Interrupt Handler
 *
 * Utilize the built-in I2C interrupt handler to allow for the use
 * of MXC_I2C_Master_Transaction() function calls
 */
static void I2C_Handler(void) { MXC_I2C_AsyncHandler(I2C_INTERFACE); }

/******************************** FUNCTION DEFINITIONS ********************************/
/**
 * @brief Initialize the I2C Connection
 * 
 * Initialize the I2C by enabling the module, setting the correct
 * frequency, and enabling the interrupt to our I2C_Handler
*/
int i2c_simple_controller_init(void) {
    int error;

    // Initialize the I2C Interface
    error = MXC_I2C_Init(I2C_INTERFACE, true, 0);
    if (error != E_NO_ERROR) {
        printf("Failed to initialize I2C.\n");
        return error;
    }
    // Set frequency to frequency macro
    MXC_I2C_SetFrequency(I2C_INTERFACE, I2C_FREQ);
    
    // Set up interrupt
    MXC_NVIC_SetVector(MXC_I2C_GET_IRQ(MXC_I2C_GET_IDX(I2C_INTERFACE)), I2C_Handler);
    NVIC_EnableIRQ(MXC_I2C_GET_IRQ(MXC_I2C_GET_IDX(I2C_INTERFACE)));

    return E_NO_ERROR;
}

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
int i2c_simple_read_receive_done(i2c_addr_t addr) {
    return i2c_simple_read_status_generic(addr, RECEIVE_DONE);
}

/**
 * @brief Read RECEIVE_LEN reg
 * 
 * @param addr: i2c_addr_t, address of I2C device
 * 
 * @return uint8_t: RECEIVE_LEN value, negative if error
 *
 * Read the RECEIVE_LEN for an I2C peripheral
 * and return the value 
*/
int i2c_simple_read_receive_len(i2c_addr_t addr) {
    return i2c_simple_read_status_generic(addr, RECEIVE_LEN);
}

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
int i2c_simple_read_transmit_done(i2c_addr_t addr) {
    return i2c_simple_read_status_generic(addr, TRANSMIT_DONE);
}

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
int i2c_simple_read_transmit_len(i2c_addr_t addr) {
    return i2c_simple_read_status_generic(addr, TRANSMIT_LEN);
}

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
int i2c_simple_write_receive_done(i2c_addr_t addr, bool done) {
    return i2c_simple_write_status_generic(addr, RECEIVE_DONE, done); 
}

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
int i2c_simple_write_receive_len(i2c_addr_t addr, uint8_t len) {
    return i2c_simple_write_status_generic(addr, RECEIVE_LEN, len); 
}

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
int i2c_simple_write_transmit_done(i2c_addr_t addr, bool done) {
    return i2c_simple_write_status_generic(addr, TRANSMIT_DONE, done);
}

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
int i2c_simple_write_transmit_len(i2c_addr_t addr, uint8_t len) {
    return i2c_simple_write_status_generic(addr, TRANSMIT_LEN, len); 
}

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
int i2c_simple_read_data_generic(i2c_addr_t addr, ECTF_I2C_REGS reg, uint8_t len, uint8_t* buf)
{
    mxc_i2c_req_t request;
    request.i2c = I2C_INTERFACE;
    request.addr = addr;
    request.tx_len = 1;
    request.tx_buf = (uint8_t*)&reg;
    request.rx_len = (unsigned int) len;
    request.rx_buf = buf;
    request.restart = 0;
    request.callback = NULL;

    return MXC_I2C_MasterTransaction(&request);
}

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
int i2c_simple_write_data_generic(i2c_addr_t addr, ECTF_I2C_REGS reg, uint8_t len, uint8_t* buf) {
    uint8_t packet[257];
    packet[0] = reg;
    memcpy(&packet[1], buf, len);
    
    mxc_i2c_req_t request;
    request.i2c = I2C_INTERFACE;
    request.addr = addr;
    request.tx_len = len+1;
    request.tx_buf = packet;
    request.rx_len = 0;
    request.rx_buf = 0;
    request.restart = 0;
    request.callback = NULL;

    return MXC_I2C_MasterTransaction(&request);
}

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
int i2c_simple_read_status_generic(i2c_addr_t addr, ECTF_I2C_REGS reg) {
    uint8_t value = 0;

    mxc_i2c_req_t request;
    request.i2c = I2C_INTERFACE;
    request.addr = addr;
    request.tx_len = 1;
    request.tx_buf = (uint8_t*)&reg;
    request.rx_len = 1;
    request.rx_buf = (uint8_t*)&value;
    request.restart = 0;
    request.callback = NULL;

    int result = MXC_I2C_MasterTransaction(&request);
    if (result < 0) {
        return result;
    }
    return value;
}

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
int i2c_simple_write_status_generic(i2c_addr_t addr, ECTF_I2C_REGS reg, uint8_t value) {
    uint8_t packet[2];
    packet[0] = (uint8_t) reg;
    packet[1] = value;
    
    mxc_i2c_req_t request;
    request.i2c = I2C_INTERFACE;
    request.addr = addr;
    request.tx_len = 2;
    request.tx_buf = packet;
    request.rx_len = 0;
    request.rx_buf = 0;
    request.restart = 0;
    request.callback = NULL;

    return MXC_I2C_MasterTransaction(&request);
}
