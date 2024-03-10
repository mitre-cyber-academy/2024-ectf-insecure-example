#include "xor_secure.h"


void XOR_secure(unsigned char* arr1, unsigned char* arr2, int size, unsigned char* dest){
    /*
    Xor two bytes array correspondingly and write the final result to the dest array
    Helper function 

    Inputs: 
        array1 in bytes eg: { 0xDF, 0x68, 0x06, 0x80, 0x0C, 0xAA } 
        array2 in bytes, same size as array 1 
        Size: len of any array
        Dest: result array
    
    Outputs: None
    */

    for (int i = 0; i < size; i++) {
        dest[i] = arr1[i] ^ arr2[i];
    }
    return;
}