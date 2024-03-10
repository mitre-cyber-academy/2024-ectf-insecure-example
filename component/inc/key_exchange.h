#ifndef KEY_EXCHANGE
#define KEY_EXCHANGE
#include "key.h"
uint8_t sync2(char* dest, char* k2_m1);

void sync1(char* dest, char* k2_m1);

uint8_t key_sync(char* dest);

#endif