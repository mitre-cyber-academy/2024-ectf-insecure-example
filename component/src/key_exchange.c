extern flash_status;
#include "key_exchange.h"
// include random generator from Zack's API



uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];


char* key_sync(char* dest){
    memset(receive_buffer, 0, sizeof(receive_buffer));
    memset(transmit_buffer, 0, sizeof(transmit_buffer));
    char * FINAL_KEY = char[18];
    char * cash_k2_r = char[18];
    wait_and_receive_packet(receive_buffer);
    memcpy(cash_k2_r, receive_buffer, 16);
    char * cash_k1_m1 = char[18];
    XOR(MASK, KEY_SHARE, cash_k1_m1);
    memcpy(cash_k1_m1, transmit_buffer, 16);
    send_packet_and_ack(16, transmit_buffer);
    memset(receive_buffer, 0, sizeof(receive_buffer));
    wait_and_receive_packet(receive_buffer);
    char * cash_k3_f1_r = char[18];
    memcpy(cash_k3_f1_r, receive_buffer, 16);
    XOR(cash_k2_r, cash_k3_f1_r, FINAL_KEY);
    XOR(FINAL_MASK, FINAL_KEY, FINAL_KEY);
    memcpy(dest, FINAL_KEY, 16);
}