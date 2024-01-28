#include "ectf_params.h" //to get to all the macros
#include "board_link.h"
#include "simple_i2c_peripheral.h"
#include "xor.h"
extern flash_status;

// include random generator from Zack's API

// premise the simple write and receive is sufficient to send a 16 byte stream 
// this assumes the key is 16 bytes 
char* key_exchange1(char* dest, uint32_t component_id){
    cache=char[18];
    i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[0]);
    XOR(M1,KEY_SHARE,cache)
    int result = send_packet(addr, 16, cache);
    int len = poll_and_receive_packet(addr, cache);
    if(len==16){
        //generates the key
    }else{
        print_info("receiving length ERROR :( ");
    }
}

char* key_exchange2(char* dest, uint32_t component_id1, uint32_t component_id2){

}





char* key_sync(char* dest, uint32_t component_cnt, uint32_t component_id1, uint32_t component_id2){
    random_number=char[18];
    random(random_number);

    if (component_cnt==2){
        key_exchange2(dest, component_id1, component_id2);
    }
    else{
        key_exchange1(dest, component_id1);
    }

}