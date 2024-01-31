#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "nvic_table.h"
#include "trng.h"

/**
 * @brief   Get a random number
 *
 * @return  A random 32-bit number
 */
int RandomInt(void);

/**
 * @brief   Get a random number of length len
 *
 * @param   data    Pointer to a location to store the number
 * @param   len     Length of random number in bytes
 *
 * @return  Success/Fail, see \ref MXC_Error_Codes for a list of return codes.
 */
void Rand_NASYC(uint8_t *buf, uint32_t len);

/**
 * @brief   Get a random number of length len, do not block while generating data
 * @note    The user must call MXC_TRNG_Handler() in the ISR
 *
 * @param   data      Pointer to a location to store the number
 * @param   len       Length of random number in bytes
 *
 */
void Rand_ASYC(uint8_t *data, uint32_t len);

// Helper functions
void TRNG_IRQHandler(void);
void Test_Callback(void *req, int result);