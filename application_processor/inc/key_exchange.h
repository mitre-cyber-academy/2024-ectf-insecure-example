#ifndef KEY_EXCHANGE
#define KEY_EXCHANGE

// include random generator from Zack's API

// premise the simple write and receive is sufficient to send a 16 byte stream 
// this assumes the key is 16 bytes 
char* key_exchange1(char* dest, uint32_t component_id);
// we may need a add a tag to it...
char* key_exchange2(char* dest, char* random; uint32_t component_id1, uint32_t component_id2);
char* key_sync(char* dest, uint32_t component_cnt, uint32_t component_id1, uint32_t component_id2);

#endif