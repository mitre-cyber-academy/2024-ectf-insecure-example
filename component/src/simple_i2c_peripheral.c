/**
 * @file "simple_i2c_peripheral.c"
 * @author Frederich Stine 
 * @brief Simple Asynchronous I2C Peripheral Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include "simple_i2c_peripheral.h"

/******************************** GLOBAL DEFINITIONS ********************************/
// Data for all of the I2C registers
volatile uint8_t RECEIVE_REG[MAX_I2C_MESSAGE_LEN];
volatile uint8_t RECEIVE_DONE_REG[1];
volatile uint8_t RECEIVE_LEN_REG[1];
volatile uint8_t TRANSMIT_REG[MAX_I2C_MESSAGE_LEN];
volatile uint8_t TRANSMIT_DONE_REG[1];
volatile uint8_t TRANSMIT_LEN_REG[1];

// Data structure to allow easy reference of I2C registers
volatile uint8_t* I2C_REGS[6] = {
    [RECEIVE] = RECEIVE_REG,
    [RECEIVE_DONE] = RECEIVE_DONE_REG,
    [RECEIVE_LEN] = RECEIVE_LEN_REG,
    [TRANSMIT] = TRANSMIT_REG,
    [TRANSMIT_DONE] = TRANSMIT_DONE_REG,
    [TRANSMIT_LEN] = TRANSMIT_LEN_REG,
};

// Data structure to allow easy reference to I2C register length
int I2C_REGS_LEN[6] = {
    [RECEIVE] = MAX_I2C_MESSAGE_LEN,
    [RECEIVE_DONE] = 1,
    [RECEIVE_LEN] = 1,
    [TRANSMIT] = MAX_I2C_MESSAGE_LEN,
    [TRANSMIT_DONE] = 1,
    [TRANSMIT_LEN] = 1,
};

/******************************** FUNCTION PROTOTYPES ********************************/
static void i2c_simple_isr(void);

/******************************** FUNCTION DEFINITIONS ********************************/
/**
 * @brief Initialize the I2C Connection
 * 
 * @param addr: uint8_t, the address of the I2C peripheral
 * 
 * @return int: negative if error, zero if successful
 *
 * Initialize the I2C by enabling the module, setting the address, 
 * setting the correct frequency, and enabling the interrupt to our i2c_simple_isr
*/
int i2c_simple_peripheral_init(uint8_t addr) {
    int error;
    // Initialize the I2C Interface
    error = MXC_I2C_Init(I2C_INTERFACE, false, addr);
    if (error != E_NO_ERROR) {
        printf("Failed to initialize I2C.\n");
        return error;
    }
    
    // Set frequency and clear FIFO
    MXC_I2C_SetFrequency(I2C_INTERFACE, I2C_FREQ);
    MXC_I2C_ClearRXFIFO(I2C_INTERFACE);

    // Enable interrupts and link ISR
    MXC_I2C_EnableInt(I2C_INTERFACE, MXC_F_I2C_INTFL0_RD_ADDR_MATCH, 0);
    MXC_I2C_EnableInt(I2C_INTERFACE, MXC_F_I2C_INTFL0_WR_ADDR_MATCH, 0);
    MXC_I2C_EnableInt(I2C_INTERFACE, MXC_F_I2C_INTFL0_STOP, 0);
    MXC_NVIC_SetVector(MXC_I2C_GET_IRQ(MXC_I2C_GET_IDX(I2C_INTERFACE)), i2c_simple_isr);
    NVIC_EnableIRQ(MXC_I2C_GET_IRQ(MXC_I2C_GET_IDX(I2C_INTERFACE)));
    MXC_I2C_ClearFlags(I2C_INTERFACE, 0xFFFFFFFF, 0xFFFFFFFF);

    // Prefix READY values for registers
    I2C_REGS[RECEIVE_DONE][0] = false;
    I2C_REGS[TRANSMIT_DONE][0] = true;

    return E_NO_ERROR;
}

/**
 * @brief ISR for the I2C Peripheral
 * 
 * This ISR allows for a fully asynchronous interface between controller and peripheral
 * Transactions are able to begin immediately after a transaction ends
*/
void i2c_simple_isr (void) {
    // Variables for state of ISR
    static bool WRITE_START = false;
    static int READ_INDEX = 0;
    static int WRITE_INDEX = 0;
    static ECTF_I2C_REGS ACTIVE_REG = RECEIVE;

    // Read interrupt flags
    uint32_t Flags = I2C_INTERFACE->intfl0;
    
    // Transaction over interrupt
    if (Flags & MXC_F_I2C_INTFL0_STOP) {
        
        // Ready any remaining data
        if (WRITE_START == true) {
            MXC_I2C_ReadRXFIFO(I2C_INTERFACE, (volatile unsigned char*) &ACTIVE_REG, 1);
            WRITE_START = false;
        }
        if (ACTIVE_REG <= MAX_REG) {
            int available = MXC_I2C_GetRXFIFOAvailable(I2C_INTERFACE);
            if (available < (I2C_REGS_LEN[ACTIVE_REG]-WRITE_INDEX)) {
                WRITE_INDEX += MXC_I2C_ReadRXFIFO(I2C_INTERFACE,
                    &I2C_REGS[ACTIVE_REG][WRITE_INDEX],
                    MXC_I2C_GetRXFIFOAvailable(I2C_INTERFACE));
            }
            else {
                WRITE_INDEX += MXC_I2C_ReadRXFIFO(I2C_INTERFACE,
                    &I2C_REGS[ACTIVE_REG][WRITE_INDEX],
                    I2C_REGS_LEN[ACTIVE_REG]-WRITE_INDEX);
            }
        } else {
            MXC_I2C_ClearRXFIFO(I2C_INTERFACE);
        }

        // Disable bulk send/receive interrupts
        MXC_I2C_DisableInt(I2C_INTERFACE, MXC_F_I2C_INTEN0_RX_THD, 0);
        MXC_I2C_DisableInt(I2C_INTERFACE, MXC_F_I2C_INTEN0_TX_THD, 0);

        // Clear FIFOs if full
        if (MXC_I2C_GetRXFIFOAvailable(I2C_INTERFACE) != 0) {
            MXC_I2C_ClearRXFIFO(I2C_INTERFACE);
        }
        if (MXC_I2C_GetTXFIFOAvailable(I2C_INTERFACE) != 8) {
            MXC_I2C_ClearTXFIFO(I2C_INTERFACE);
        }

        // Reset state
        READ_INDEX = 0;
        WRITE_INDEX = 0;
        WRITE_START = false;

        // Clear ISR flag
        MXC_I2C_ClearFlags(I2C_INTERFACE, MXC_F_I2C_INTFL0_STOP, 0);
    }

    // TX Fifo Threshold Met on Read
    if (Flags & MXC_F_I2C_INTEN0_TX_THD && (I2C_INTERFACE->inten0 & MXC_F_I2C_INTEN0_TX_THD)) {

        if (Flags & MXC_F_I2C_INTFL0_TX_LOCKOUT) {
            MXC_I2C_ClearFlags(I2C_INTERFACE, MXC_F_I2C_INTFL0_TX_LOCKOUT, 0);
        }
        // 2 bytes in TX fifo triggers threshold by default
        // 8 byte FIFO length by default
        // More data is needed within the FIFO
        if (ACTIVE_REG <= MAX_REG) {
            READ_INDEX += MXC_I2C_WriteTXFIFO(I2C_INTERFACE,
                (volatile unsigned char*)&I2C_REGS[ACTIVE_REG][READ_INDEX],
                I2C_REGS_LEN[ACTIVE_REG]-READ_INDEX);
            if (I2C_REGS_LEN[ACTIVE_REG]-1 == READ_INDEX) {
                MXC_I2C_DisableInt(I2C_INTERFACE, MXC_F_I2C_INTEN0_TX_THD, 0);
            }
        }

        // Clear ISR flag
        MXC_I2C_ClearFlags(I2C_INTERFACE, MXC_F_I2C_INTFL0_TX_THD, 0);
    }

    // Read from Peripheral from Controller Match
    if (Flags & MXC_F_I2C_INTFL0_WR_ADDR_MATCH) {
        // Clear ISR flag
        MXC_I2C_ClearFlags(I2C_INTERFACE, MXC_F_I2C_INTFL0_WR_ADDR_MATCH, 0);
        
        // TX_LOCKOUT Triggers at the start of a just-in-time read
        if (Flags & MXC_F_I2C_INTFL0_TX_LOCKOUT) {
            MXC_I2C_ClearFlags(I2C_INTERFACE, MXC_F_I2C_INTFL0_TX_LOCKOUT, 0);

            // Select active register
            MXC_I2C_ReadRXFIFO(I2C_INTERFACE, (volatile unsigned char*) &ACTIVE_REG, 1);
            
            // Write data to TX Buf
            if (ACTIVE_REG <= MAX_REG) {
                READ_INDEX += MXC_I2C_WriteTXFIFO(I2C_INTERFACE, (volatile unsigned char*)I2C_REGS[ACTIVE_REG], I2C_REGS_LEN[ACTIVE_REG]);
                if (READ_INDEX < I2C_REGS_LEN[ACTIVE_REG]) {
                    MXC_I2C_EnableInt(I2C_INTERFACE, MXC_F_I2C_INTEN0_TX_THD, 0);
                }
            }
        }
    }

    // Write to Peripheral from Controller Match
    if (Flags & MXC_F_I2C_INTFL0_RD_ADDR_MATCH) {
        // Set write start variable
        WRITE_START = true;

        // Enable bulk receive interrupt
        MXC_I2C_EnableInt(I2C_INTERFACE, MXC_F_I2C_INTEN0_RX_THD, 0);

        // Clear flag
        MXC_I2C_ClearFlags(I2C_INTERFACE, MXC_F_I2C_INTFL0_RD_ADDR_MATCH, 0);
    }

    // RX Fifo Threshold Met on Write
    if (Flags & MXC_F_I2C_INTEN0_RX_THD) {
        // We always write a register before writing data so select register
        if (WRITE_START == true) {
            MXC_I2C_ReadRXFIFO(I2C_INTERFACE, (volatile unsigned char*) &ACTIVE_REG, 1);
            WRITE_START = false;
        }
        // Read remaining data
        if (ACTIVE_REG <= MAX_REG) {
            int available = MXC_I2C_GetRXFIFOAvailable(I2C_INTERFACE);
            if (available < (I2C_REGS_LEN[ACTIVE_REG]-WRITE_INDEX)) {
                WRITE_INDEX += MXC_I2C_ReadRXFIFO(I2C_INTERFACE,
                    &I2C_REGS[ACTIVE_REG][WRITE_INDEX],
                    MXC_I2C_GetRXFIFOAvailable(I2C_INTERFACE));
            }
            else {
                WRITE_INDEX += MXC_I2C_ReadRXFIFO(I2C_INTERFACE,
                    &I2C_REGS[ACTIVE_REG][WRITE_INDEX],
                    I2C_REGS_LEN[ACTIVE_REG]-WRITE_INDEX);
            }
        // Clear out FIFO if invalid register specified
        } else {
            MXC_I2C_ClearRXFIFO(I2C_INTERFACE);
        }

        // Clear ISR flag
        MXC_I2C_ClearFlags(I2C_INTERFACE, MXC_F_I2C_INTFL0_RX_THD, 0);
    }
}
