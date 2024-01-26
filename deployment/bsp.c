

//define the function that XOR's two string values
void XOR(char str1[], char str2[], char out[]){
    //put 16 so we are strict for AES Key
    for (int i = 0; i < 16; i++) {
        out[i] = str1[i] ^ str2[i];
    }
    return
}