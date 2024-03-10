extern flash_status;
#include "ectf_params.h" //to get to all the macros
#include "board_link.h"
#include "simple_i2c_peripheral.h"
#include "xor_secure.h"

#include "key_exchange.h"
#include <stdio.h>
uint8_t transmit_buffer1[MAX_I2C_MESSAGE_LEN];
uint8_t receive_buffer1[MAX_I2C_MESSAGE_LEN];



uint8_t sync2(char* dest, char* k2_r){
    char cash_k2_r[18];
    XOR_secure(k2_r,KEY_SHARE,16,cash_k2_r); //k2_r_k1

    XOR_secure(KEY_SHARE, MASK, 16, k2_r); //k2_r is buffer name
    send_packet_and_ack(16, k2_r); // send k m1

    uint8_t len = wait_and_receive_packet(k2_r);
    if(len==16){
        XOR_secure(cash_k2_r, k2_r, 16, dest);
        XOR_secure(dest, FINAL_MASK, 16, dest);
        return len;
    }else{
        return -1;
    }

}


void sync1(char* dest, char* k2_m1){
    // generate the final key
    XOR_secure(k2_m1, KEY_SHARE, 16, dest);
    XOR_secure(dest, MASK, 16, dest);

    // give the AP the keyshare
    XOR_secure(KEY_SHARE, FINAL_MASK, 16, k2_m1);
    send_packet_and_ack(16, k2_m1);

    return;
}

uint8_t key_sync(char* dest){
    uint8_t len=wait_and_receive_packet(receive_buffer1);
    if(len!=18){
        //we have an error so just return
        return -1;
    }
    char rv = receive_buffer1[17];
    switch (rv)
    {
    case '1':
        sync1(dest,receive_buffer1);
        return len;
    
    case '2':
        return sync2(dest,receive_buffer1);

    default:
        //default should never be invoked
        return -1;
    }

}