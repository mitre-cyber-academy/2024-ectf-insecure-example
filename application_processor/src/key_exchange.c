// extern flash_status;

#include "key_exchange.h"



// premise the simple write and receive is sufficient to send a 16 byte stream
// this assumes the key is 16 bytes
int key_exchange1(unsigned char *dest, uint32_t component_id) {
    char cache[18];
    i2c_addr_t addr = component_id_to_i2c_addr(component_id);
    XOR_secure(M1, KEY_SHARE, 16, cache);
    cache[17] = '1';

    int return_status = send_packet(addr, 18, cache);
    
    memset(cache, 0, 18);
    int len = poll_and_receive_packet(addr, cache);
    
    XOR_secure(KEY_SHARE, cache, 16, cache);
    XOR_secure(cache, F1, 16, dest); // Should be the dest for the fourth
                                // argument?
    return 0;
}

int key_exchange2(unsigned char *dest, char *random,
                   uint32_t component_id1, uint32_t component_id2) {
    // Allocation Temp Caches
    char cache1[18]; // this stores K1
    char cache2[18]; // this stores k3

    // deal with the first component to obtain k1
    i2c_addr_t addr = component_id_to_i2c_addr(component_id1);
    XOR_secure(random, KEY_SHARE, 16, cache1);
    cache1[17] = '2';
    int result = send_packet(addr, 18, cache1);
    int len = poll_and_receive_packet(addr, cache1); 
    if (len == 16) {
        XOR_secure(M1, cache1, 16, cache1); // k1 == cach1
    } else {
        return -1;
    }

    // deal with the second component to obtain k2
    i2c_addr_t addr2 = component_id_to_i2c_addr(component_id2);
    XOR_secure(random, KEY_SHARE, 16, cache2);
    cache2[17] = '2';

    int return_status = send_packet(addr2, 18, cache2);
    int len2 = poll_and_receive_packet(addr2, cache2);
    if (len2 == 16) {
        XOR_secure(M2, cache2, 16, cache2); // k3 == cach2
        XOR_secure(cache1, cache2, 16, dest); // k1 * k3 == dest
        XOR_secure(dest, KEY_SHARE, 16, dest); // k1 * k3 * k2 == dest
    } else {
        return -1;
    }

    // send out the last few pieces of keys to both sides
    // sends k_Comp2 f1 R
    XOR_secure(F1, cache2, 16, cache2);
    XOR_secure(cache2, random, 16, cache2); // cach2 == k3 * f1 * r
    int result2 = send_packet(addr, 16, cache2);

    // sends k_Comp1 f2 R
    XOR_secure(F2, cache1, 16, cache1);
    XOR_secure(cache1, random, 16, cache1); // cach1 == k1 * f2 * r
    int result3 = send_packet(addr2, 16, cache1);

    return 0;
}

int key_sync(unsigned char *dest, uint32_t component_cnt,
               uint32_t component_id1, uint32_t component_id2) {
    char random_number[18];
    Rand_NASYC(random_number, 16);
    if (component_cnt == 2) {
        return key_exchange2(dest, random_number, component_id1, component_id2);
    } else {
        return key_exchange1(dest, component_id1);
    }
}
