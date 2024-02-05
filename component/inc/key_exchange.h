#include "ectf_params_comp.h" //to get to all the macros
#include "board_link.h"
#include "simple_i2c_peripheral.h"
#include "xor.h"
extern flash_status;

// include random generator from Zack's API

// premise the simple write and receive is sufficient to send a 16 byte stream 
// this assumes the key is 16 bytes 

char* key_sync(char* dest);