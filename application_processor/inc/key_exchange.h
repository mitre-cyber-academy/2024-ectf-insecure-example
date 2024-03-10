#ifndef KEY_EXCHANGE
#define KEY_EXCHANGE
#include "Rand_lib.h"
#include "board_link.h"
#include "ectf_params.h" //to get to all the macros
#include "simple_i2c_controller.h"
#include "xor_secure.h"
#include "key.h"
// include random generator from Zack's API

// premise the simple write and receive is sufficient to send a 16 byte stream
// this assumes the key is 16 bytes
int key_exchange1(unsigned char *dest, uint32_t component_id);
// we may need a add a tag to it...
int key_exchange2(unsigned char *dest, char *random, uint32_t component_id1,
                    uint32_t component_id2);
int key_sync(unsigned char *dest, uint32_t component_cnt,
               uint32_t component_id1, uint32_t component_id2);

#endif
