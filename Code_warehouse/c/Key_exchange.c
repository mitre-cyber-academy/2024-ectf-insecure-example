#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define KEY_SIZE 16


#define XOR ^
#define SEND(x , y) 1
#define RECV(x , y) 1

// variables For  AP and CP
uint8_t K1[KEY_SIZE] = { 0 };
uint8_t K2[KEY_SIZE] = { 0 };
uint8_t K3[KEY_SIZE] = { 0 };
uint8_t R[KEY_SIZE] = { 0 };
uint8_t M1[KEY_SIZE] = { 0 };
uint8_t M2[KEY_SIZE] = { 0 };
uint8_t F1[KEY_SIZE] = { 0 };
uint8_t F2[KEY_SIZE] = { 0 };

void Calculate_XOR(uint8_t *a, uint8_t *b,size_t size, uint8_t *result){
    for (int i = 0; i < size; i++) {
        result[i] = a[i] XOR b[i];
    }
}

void AP_jobs(){
    uint8_t cache[KEY_SIZE] = { 0 };
    // CP1 part
    Calculate_XOR(K1, R, KEY_SIZE, cache);
    SEND(cache, KEY_SIZE);
    while (!RECV(cache, KEY_SIZE)) {
        continue;
    }
    // cache = M1 XOR K1, K1 = cache XOR M1
    Calculate_XOR(cache, M1, KEY_SIZE, K1);
    // Need K3 XOR F1 XOR R
    Calculate_XOR(K3, F1, KEY_SIZE, cache);
    Calculate_XOR(cache, R, KEY_SIZE, cache);
    SEND(cache, KEY_SIZE);
    // CP2 part
    Calculate_XOR(K3, R, KEY_SIZE, cache);
    SEND(cache, KEY_SIZE);
    while (!RECV(cache, KEY_SIZE)) {
        continue;
    }
    // cache = M2 XOR K3, K3 = cache XOR M2
    Calculate_XOR(cache, M2, KEY_SIZE, K3);
    // Need K1 XOR F2 XOR R
    Calculate_XOR(K1, F2, KEY_SIZE, cache);
    Calculate_XOR(cache, R, KEY_SIZE, cache);
    SEND(cache, KEY_SIZE);

    // right now, K1, K2, K3 are all set then we can get the key
    Calculate_XOR(K1, K2, KEY_SIZE, cache);
    Calculate_XOR(cache, K3, KEY_SIZE, cache);
    printf("key: %s\n", cache);
}

void CP1_jobs(){
    // Only have K1 M1 F1
    uint8_t cache[KEY_SIZE] = { 0 };
    // AP part
    while (!RECV(cache, KEY_SIZE)) {
        continue;
    }
    // cache = K2 XOR R
    uint8_t temp[KEY_SIZE] = { 0 };
    // temp = K2 XOR R XOR K1
    Calculate_XOR(cache, K1, KEY_SIZE, temp);
    // Send K1 XOR M1
    Calculate_XOR(K1, M1, KEY_SIZE, cache);
    SEND(cache, KEY_SIZE);
    while (!RECV(cache, KEY_SIZE)) {
        continue;
    }
    // cache = K3 XOR F1 XOR R
    // now we can get key
    uint8_t key[KEY_SIZE] = { 0 };
    // Key = K3 XOR F1 XOR R XOR K2 XOR R XOR K1 XOR F1
    Calculate_XOR(cache, temp, KEY_SIZE, key);
    Calculate_XOR(key, F1, KEY_SIZE, key);
    printf("key: %s\n", key);
}

void CP2_jobs(){
    // Only have K3 M2 F2
    uint8_t cache[KEY_SIZE] = { 0 };
    // AP part
    while (!RECV(cache, KEY_SIZE)) {
        continue;
    }
    // cache = K2 XOR R
    uint8_t temp[KEY_SIZE] = { 0 };
    // temp = K2 XOR R XOR K3
    Calculate_XOR(cache, K3, KEY_SIZE, temp);
    // Send K3 XOR M2
    Calculate_XOR(K3, M2, KEY_SIZE, cache);
    SEND(cache, KEY_SIZE);
    while (!RECV(cache, KEY_SIZE)) {
        continue;
    }
    // cache = K1 XOR F2 XOR R
    // now we can get key
    uint8_t key[KEY_SIZE] = { 0 };
    // Key = K1 XOR F2 XOR R XOR K2 XOR R XOR K3 XOR F2
    Calculate_XOR(cache, temp, KEY_SIZE, key);
    Calculate_XOR(key, F2, KEY_SIZE, key);
    printf("key: %s\n", key);
}


int main (int argc, char *argv[])
{
    char p[16] = "hello world";
    for (int i = 0; i < 16; i++) {
        p[i] = 'a'+i;
        
    }
    return 0;
}


