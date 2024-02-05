#include "aes.h"
#include "Code_warehouse/c/Rand_lib.h"
#include "application_processor/inc/simple_crypto.h"

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
    uint8_t COMPONENT_CMD_NONE,
    uint8_t COMPONENT_CMD_SCAN,
    uint8_t COMPONENT_CMD_VALIDATE,
    uint8_t COMPONENT_CMD_BOOT,
    uint8_t COMPONENT_CMD_ATTEST,
} component_cmd_t;

/******************************** Testing Variables ********************************/

uint32_t COMP_ID1 = 123456789
uint32_t COMP_ID2 = 987654321

uint8_t GLOBAL_KEY[AES_SIZE]; // Need to define this better

/******************************** TYPE DEFINITIONS ********************************/

// structure for limited protocol: message[0] = op_code; message[1:4] = Comp_ID; message[5:12] RAND_Z
typedef struct {
    uint8_t message[AES_SIZE];
} limited_protocol;

// structure
typedef struct {
    uint8_t message[MAX_I2C_MESSAGE_LEN]
} maximum_protocol;

int test_validate_and_boot_protocol():
    return SUCCESS_RETURN

int test_attest_protocol():
    return SUCCESS_RETURN

int test_post_boot_protocol():
    return SUCCESS_RETURN
