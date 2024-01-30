#include "ectf_params.h" //to get to all the macros
#include "board_link.h"
#include "simple_i2c_peripheral.h"
#include "xor.h"
extern flash_status;

// include random generator from Zack's API

// premise the simple write and receive is sufficient to send a 16 byte stream 
// this assumes the key is 16 bytes 
char* key_exchange1(char* dest, uint32_t component_id){
    char* cache=char[18];
    i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[0]);
    XOR(M1,KEY_SHARE,16,cache);
    int result = send_packet(addr, 16, cache);
    int len = poll_and_receive_packet(addr, cache);
    if(len==16){
        XOR(KEY_SHARE,cache,16,cache);
        XOR(cache,F1,16,cache);
        return;
    }else{
        print_info("receiving length ERROR :( ");
        return;
    }
}

// we may need a add a tag to it...
char* key_exchange2(char* dest, char* random; uint32_t component_id1, uint32_t component_id2){
    //Allocation Temp Caches
    char* cache1=char[18]; // this stores K1
    char* cache2=char[18]; // this stores k3

    //deal with the first component to obtain k1
    i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[0]);
    XOR(random,KEY_SHARE,16,cache1);
    int result = send_packet(addr, 16, cache1);
    int len = poll_and_receive_packet(addr, cache1);
    if(len==16){
        XOR(M1,cache1,16,cache1);
    }else{
        print_info("receiving length ERROR 1 :( \n");
        return;
    }

    //deal with the second component to obtain k2
    i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[1]);
    XOR(random,KEY_SHARE,16,cache2);
    int result = send_packet(addr, 16, cache2);
    int len = poll_and_receive_packet(addr, cache2);
    if(len==16){
        XOR(M2,cache1,16,cache1);

        // we compute the final key here
        XOR(cache1,cache2,16,dest);
    }else{
        print_info("receiving length ERROR 2 :( \n");
        return;
    }

    //send out the last few pieces of keys to both sides
    //sends k_Comp2 f1 R
    XOR(F1,cache2,16,cache2);
    XOR(cache2,random,16,cache2);
    int result = send_packet(addr, 16, cache2);

    //sends k_Comp1 f2 R 
    XOR(F1,cache2,16,cache2);
    XOR(cache2,random,16,cache2);
    int result = send_packet(addr, 16, cache2);

}


char* key_sync(char* dest, uint32_t component_cnt, uint32_t component_id1, uint32_t component_id2){
    char* random_number=char[18];
    random(random_number);
    if (component_cnt==2){
        key_exchange2(dest, random_number, component_id1, component_id2);
    }
    else{
        key_exchange1(dest, component_id1);
    }

}