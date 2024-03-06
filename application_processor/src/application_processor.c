/**
 * @file application_processor.c
 * @author Jacob Doll
 * @brief eCTF AP Example Design Implementation
 * @date 2024
 *
 * This source file is part of an example system for MITRE's 2024 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2024 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include "board.h"
#include "i2c.h"
#include "icc.h"
#include "led.h"
#include "mxc_delay.h"
#include "mxc_device.h"
#include "nvic_table.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_link.h"
#include "simple_flash.h"
#include "host_messaging.h"
#include "simple_crypto.h"

#ifdef POST_BOOT
#include <stdint.h>
#include <stdio.h>
#endif

// Includes from containerized build
#include "ectf_params.h"
#include "global_secrets.h"

/********************************* CONSTANTS **********************************/

// Passed in through ectf-params.h
// Example of format of ectf-params.h shown here
/*
#define AP_PIN "123456"
#define AP_TOKEN "0123456789abcdef"
#define COMPONENT_IDS 0x11111124, 0x11111125
#define COMPONENT_CNT 2
#define AP_BOOT_MSG "Test boot message"
*/

// Flash Macros
#define FLASH_ADDR ((MXC_FLASH_MEM_BASE + MXC_FLASH_MEM_SIZE) - (2 * MXC_FLASH_PAGE_SIZE))

// Library call return types
#define SUCCESS_RETURN 0
#define ERROR_RETURN -1

/******************************** TYPE DEFINITIONS ********************************/
// Data structure for sending commands to component
// Params allows for up to MAX_I2C_MESSAGE_LEN - 1 bytes to be send
// along with the opcode through board_link. This is not utilized by the example
// design but can be utilized by your design.
typedef struct {
    uint8_t opcode;
    uint8_t params[MAX_I2C_MESSAGE_LEN-1];
} command_message;

// Data type for receiving a validate message
typedef struct {
    uint32_t component_id;
} validate_message;

// Data type for receiving a scan message
typedef struct {
    uint32_t component_id;
} scan_message;

// Datatype for information stored in flash
typedef struct {
    uint32_t flash_magic;
    uint32_t component_cnt;
    uint32_t component_ids[32];
} flash_entry;

// Datatype for commands sent to components
typedef enum {
    COMPONENT_CMD_NONE,
    COMPONENT_CMD_SCAN,
    COMPONENT_CMD_VALIDATE,
    COMPONENT_CMD_BOOT,
    COMPONENT_CMD_ATTEST
} component_cmd_t;

/********************************* GLOBAL VARIABLES **********************************/
// Variable for information stored in flash memory
flash_entry flash_status;
ecc_key privateKey;
ecc_key publicKeys[COMPONENT_CNT];
size_t secure_msg_size = BLOCK_SIZE * 5;
uint8_t symmetric_key[32];

/******************************* POST BOOT FUNCTIONALITY *********************************/
/**
 * @brief Secure Send 
 * 
 * @param address: i2c_addr_t, I2C address of recipient
 * @param buffer: uint8_t*, pointer to data to be send
 * @param len: uint8_t, size of data to be sent 
 * 
 * Securely send data over I2C. This function is utilized in POST_BOOT functionality.
 * This function must be implemented by your team to align with the security requirements.

*/
int secure_send(uint8_t address, uint8_t* buffer, uint8_t len) {
    // multiplex into structure
    
    uint8_t padded_buffer[secure_msg_size];
    uint8_t ciph[secure_msg_size];

    // Copy the original plaintext to the padded buffer
    memcpy(padded_buffer, buffer, len);

    // Add padding bytes with the chosen character
    for (int i = len; i < secure_msg_size; i++) {
        padded_buffer[i] = PADDING_CHAR;
    }
    
    uint8_t key[KEY_SIZE];
    memcpy(key, symmetric_key, KEY_SIZE * sizeof(uint8_t));
    encrypt_sym((uint8_t*)padded_buffer, secure_msg_size, key, ciph);

    // Send the encrypted data
    return send_packet(address, secure_msg_size, ciph);
}


/**
 * @brief Secure Symmetric Key Send 
 * 
 * @param address: i2c_addr_t, I2C address of recipient
 * @param buffer: uint8_t*, pointer to data to be send
 * @param len: uint8_t, size of data to be sent 
 * 
 * Securely send symmetric key data over I2C.
*/
int secure_symkey_send(uint8_t address, uint8_t* buffer, uint8_t len) {
    // multiplex into structure
    
    uint8_t padded_buffer[secure_msg_size];
    uint8_t ciph[secure_msg_size];

    // Copy the original plaintext to the padded buffer
    memcpy(padded_buffer, buffer, len);

    // Add padding bytes with the chosen character
    for (int i = len; i < secure_msg_size; i++) {
        padded_buffer[i] = PADDING_CHAR;
    }
    
    uint8_t key[KEY_SIZE];
    memcpy(key, VALIDATION_KEY, KEY_SIZE * sizeof(uint8_t));
    encrypt_sym((uint8_t*)padded_buffer, secure_msg_size, key, ciph);

    // Send the encrypted data
    return send_packet(address, secure_msg_size, ciph);
}


/**
 * @brief Secure Receive
 * 
 * @param address: i2c_addr_t, I2C address of sender
 * @param buffer: uint8_t*, pointer to buffer to receive data to
 * 
 * @return int: number of bytes received, negative if error
 * 
 * Securely receive data over I2C. This function is utilized in POST_BOOT functionality.
 * This function must be implemented by your team to align with the security requirements.
*/
int secure_receive(i2c_addr_t address, uint8_t* buffer) {
    poll_and_receive_packet(address, buffer);
    
    // //Decrypt receive
    uint8_t key[KEY_SIZE];
    memcpy(key, symmetric_key, KEY_SIZE * sizeof(uint8_t));
    size_t plaintext_length;
    decrypt_sym(buffer, secure_msg_size, key, buffer, &plaintext_length);
    
    return plaintext_length;
}

/**
 * @brief Secure Key Receive
 * 
 * @param address: i2c_addr_t, I2C address of sender
 * @param buffer: uint8_t*, pointer to buffer to receive data to
 * 
 * @return int: number of bytes received, negative if error
 * 
 * Securely receive key data over I2C. 
*/
int secure_pubkey_receive(i2c_addr_t address, uint8_t* buffer, int id) {
    poll_and_receive_packet(address, buffer);
    
    // //Decrypt receive
    uint8_t key[KEY_SIZE];
    memcpy(key, VALIDATION_KEY, KEY_SIZE * sizeof(uint8_t));
    size_t plaintext_length;
    decrypt_sym(buffer, secure_msg_size, key, buffer, &plaintext_length);
    
    // import public key from byte format
    int test = wc_ecc_init(&publicKeys[id]);
    int result = import_pub_key(buffer, 65, &publicKeys[id]);
	if (result != 0) {
		print_error("Import error failed: %d\n", result);
	}

    return plaintext_length;
}

/**
 * @brief Get Provisioned IDs
 * 
 * @param uint32_t* buffer
 * 
 * @return int: number of ids
 * 
 * Return the currently provisioned IDs and the number of provisioned IDs
 * for the current AP. This functionality is utilized in POST_BOOT functionality.
 * This function must be implemented by your team.
*/
int get_provisioned_ids(uint32_t* buffer) {
    memcpy(buffer, flash_status.component_ids, flash_status.component_cnt * sizeof(uint32_t));
    return flash_status.component_cnt;
}

/********************************* UTILITIES **********************************/

// Initialize the device
// This must be called on startup to initialize the flash and i2c interfaces
void init() {

    // Enable global interrupts    
    __enable_irq();

    // Setup Flash
    flash_simple_init();

    // Test application has been booted before
    flash_simple_read(FLASH_ADDR, (uint32_t*)&flash_status, sizeof(flash_entry));

    // Write Component IDs from flash if first boot e.g. flash unwritten
    if (flash_status.flash_magic != FLASH_MAGIC) {
        print_debug("First boot, setting flash!\n");

        flash_status.flash_magic = FLASH_MAGIC;
        flash_status.component_cnt = COMPONENT_CNT;
        uint32_t component_ids[COMPONENT_CNT] = {COMPONENT_IDS};
        memcpy(flash_status.component_ids, component_ids, 
            COMPONENT_CNT*sizeof(uint32_t));

        flash_simple_write(FLASH_ADDR, (uint32_t*)&flash_status, sizeof(flash_entry));
    }
    
    // Initialize board link interface
    board_link_init();
}

// Send a command to a component and receive the result
int issue_cmd(i2c_addr_t addr, uint8_t* transmit, uint8_t* receive) {
    // Encrypt and send message
    int result = secure_send(addr, transmit, secure_msg_size);
    
    if (result == ERROR_RETURN) {
        return ERROR_RETURN;
    }
    
    int len = secure_receive(addr, receive);

    if (len == ERROR_RETURN) {
        return ERROR_RETURN;
    }

    return len;
}

/******************************** COMPONENT COMMS ********************************/

int scan_components() {
    // Print out provisioned component IDs
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        print_info("P>0x%08x\n", flash_status.component_ids[i]);
    }

    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Scan scan command to each component 
    for (i2c_addr_t addr = 0x8; addr < 0x78; addr++) {
        // I2C Blacklist:
        // 0x18, 0x28, and 0x36 conflict with separate devices on MAX78000FTHR
        if (addr == 0x18 || addr == 0x28 || addr == 0x36) {
            continue;
        }

        // Create command message 
        command_message* command = (command_message*) transmit_buffer;
        command->opcode = COMPONENT_CMD_SCAN;

        // Send out command and receive result
        int len = issue_cmd(addr, transmit_buffer, receive_buffer);

        // Success, device is present
        if (len > 0) {
            scan_message* scan = (scan_message*) receive_buffer;
            print_info("F>0x%08x\n", scan->component_id);
        }
    }
    print_success("List\n");
    return SUCCESS_RETURN;
}

int validate_components() {
    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Send validate command to each component
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        // Set the I2C address of the component
        i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[i]);

        // Create command message
        command_message* command = (command_message*) transmit_buffer;
        command->opcode = COMPONENT_CMD_VALIDATE;
        
        // Send out command and receive result
        int len = issue_cmd(addr, transmit_buffer, receive_buffer);
        if (len == ERROR_RETURN) {
            print_error("Could not validate component\n");
            return ERROR_RETURN;
        }

        validate_message* validate = (validate_message*) receive_buffer;
        // Check that the result is correct
        if (validate->component_id != flash_status.component_ids[i]) {
            print_error("Component ID: 0x%08x invalid\n", flash_status.component_ids[i]);
            return ERROR_RETURN;
        }
    }
    return SUCCESS_RETURN;
}

int boot_components() {
    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Send boot command to each component
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        // Set the I2C address of the component
        i2c_addr_t addr = component_id_to_i2c_addr(flash_status.component_ids[i]);
        
        // Create command message
        command_message* command = (command_message*) transmit_buffer;
        command->opcode = COMPONENT_CMD_BOOT;


        // Send out command and receive result
        int len = issue_cmd(addr, transmit_buffer, receive_buffer);
        if (len == ERROR_RETURN) {
            print_error("Could not boot component\n");
            return ERROR_RETURN;
        }

        // Print boot message from component
        print_info("0x%08x>%s\n", flash_status.component_ids[i], receive_buffer);
    }
    return SUCCESS_RETURN;
}

int attest_component(uint32_t component_id) {
    // Buffers for board link communication
    uint8_t receive_buffer[MAX_I2C_MESSAGE_LEN];
    uint8_t transmit_buffer[MAX_I2C_MESSAGE_LEN];

    // Set the I2C address of the component
    i2c_addr_t addr = component_id_to_i2c_addr(component_id);

    // Create command message
    command_message* command = (command_message*) transmit_buffer;
    command->opcode = COMPONENT_CMD_ATTEST;

    // Send out command and receive result
    int len = issue_cmd(addr, transmit_buffer, receive_buffer);
    if (len == ERROR_RETURN) {
        print_error("Could not attest component\n");
        return ERROR_RETURN;
    }

    // Print out attestation data 
    print_info("C>0x%08x\n", component_id);
    print_info("%s", receive_buffer);
    return SUCCESS_RETURN;
}

/********************************* AP LOGIC ***********************************/

// Boot sequence
// YOUR DESIGN MUST NOT CHANGE THIS FUNCTION
// Boot message is customized through the AP_BOOT_MSG macro
void boot() {
    // POST BOOT FUNCTIONALITY
    // DO NOT REMOVE IN YOUR DESIGN
    #ifdef POST_BOOT
        POST_BOOT
    #else
    // Everything after this point is modifiable in your design
    // LED loop to show that boot occurred
    while (1) {
        LED_On(LED1);
        MXC_Delay(500000);
        LED_On(LED2);
        MXC_Delay(500000);
        LED_On(LED3);
        MXC_Delay(500000);
        LED_Off(LED1);
        MXC_Delay(500000);
        LED_Off(LED2);
        MXC_Delay(500000);
        LED_Off(LED3);
        MXC_Delay(500000);
    }
    #endif
}

// Compare the entered PIN to the correct PIN
int validate_pin() {
    size_t size = 50;
    char buf[size];
    recv_input("Enter pin: ", buf, size);
    if (!strcmp(buf, AP_PIN)) {
        print_debug("Pin Accepted!\n");
        return SUCCESS_RETURN;
    }
    print_error("Invalid PIN!\n");
    return ERROR_RETURN;
}

// Function to validate the replacement token
int validate_token() {
    size_t size = 50;
    char buf[size];
    recv_input("Enter token: ", buf, size);
    if (!strcmp(buf, AP_TOKEN)) {
        print_debug("Token Accepted!\n");
        return SUCCESS_RETURN;
    }
    print_error("Invalid Token!\n");
    return ERROR_RETURN;
}

// Boot the components and board if the components validate
void attempt_boot() {
    if (validate_components()) {
        print_error("Components could not be validated\n");
        return;
    }
    print_debug("All Components validated\n");
    if (boot_components()) {
        print_error("Failed to boot all components\n");
        return;
    }

    // Print boot message
    // This always needs to be printed when booting
    print_info("AP>%s\n", AP_BOOT_MSG);
    print_success("Boot\n");
    // Boot
    boot();
}

// Replace a component if the PIN is correct
void attempt_replace() {
    size_t size = 50;
    char buf[size];

    if (validate_token()) {
        return;
    }

    uint32_t component_id_in = 0;
    uint32_t component_id_out = 0;

    recv_input("Component ID In: ", buf, size);
    sscanf(buf, "%x", &component_id_in);
    recv_input("Component ID Out: ", buf, size);
    sscanf(buf, "%x", &component_id_out);

    // Find the component to swap out
    for (unsigned i = 0; i < flash_status.component_cnt; i++) {
        if (flash_status.component_ids[i] == component_id_out) {
            flash_status.component_ids[i] = component_id_in;

            // write updated component_ids to flash
            flash_simple_erase_page(FLASH_ADDR);
            flash_simple_write(FLASH_ADDR, (uint32_t*)&flash_status, sizeof(flash_entry));

            print_debug("Replaced 0x%08x with 0x%08x\n", component_id_out,
                    component_id_in);
            print_success("Replace\n");
            return;
        }
    }

    // Component Out was not found
    print_error("Component 0x%08x is not provisioned for the system\r\n",
            component_id_out);
}

// Attest a component if the PIN is correct
void attempt_attest() {
    size_t size = 50;
    char buf[size];

    if (validate_pin()) {
        return;
    }
    uint32_t component_id;
    recv_input("Component ID: ", buf, size);
    sscanf(buf, "%x", &component_id);
    if (attest_component(component_id) == SUCCESS_RETURN) {
        print_success("Attest\n");
    }
}

void generate_keys(byte* publicKey, uint8_t* symKey) {
    WC_RNG rng;

	int keygen = ecc_keygen(&privateKey, &rng, publicKey);
	if (keygen != 0) {
		print_error("Error generating key: %d\n", keygen);
	}

    for (int i = 0; i < 32; i++) {
        symKey[i] = rand() % 10; // Generates random numbers between 0 and 99
    }
}

// void hooven() {
//     char* data = "Crypto Example!";
//     uint8_t ciphertext[BLOCK_SIZE];
//     uint8_t key[KEY_SIZE];
//     byte publicKey[ECC_BUFSIZE]; /* Public key size for secp256r1 */
//     byte privateKey[ECC_BUFSIZE]; /* Public key size for secp256r1 */
//     ecc_key curve_key;
// 	WC_RNG rng;
// 	byte genKeyTest[ECC_BUFSIZE];
// 	uint8_t genSymKey[32];

// 	generate_keys(&curve_key, genKeyTest, genSymKey); 

// 	print_debug("Public Key: ");
//     for (int i = 0; i < 65; ++i) {
//         printf("%02X", publicKey[i]);
//     }
//     print_debug("\n");
    
// 	memcpy(key, VALIDATION_KEY, KEY_SIZE * sizeof(uint8_t));

//     // Encrypt example data and print out
//     encrypt_sym((uint8_t*)data, BLOCK_SIZE, key, ciphertext); 
//     print_debug("Encrypted data: ");
//     print_hex_debug(ciphertext, BLOCK_SIZE);



//     //******** signature start ********//
// 	word32 sig_len = ECC_MAX_SIG_SIZE;
// 	byte signature[sig_len];
// 	uint8_t digest[HASH_SIZE];
	

//     // sign message
// 	int sigCheck = asym_sign(ciphertext, signature, &curve_key, &sig_len, &rng, digest);
// 	if (sigCheck != 0) {
// 		print_error("Error with signing: %d\n");
// 	}
// 	else {
// 		print_debug("SIGN SUCCESS: %d\n", sigCheck);
// 		print_hex_debug(signature, sig_len);
// 		print_hex_debug(ciphertext, HASH_SIZE);
// 	}


//     // validation start
// 	int status = 0;
// 	ecc_key importTestKey;
// 	word32 eccPubSize = 65;
// 	int test = wc_ecc_init(&importTestKey);
// 	if (test != 0) {
// 		print_error("ECC Key Init Failure: %d\n", test);
// 	}

//     // import public key
// 	int importVal = import_pub_key(genKeyTest, eccPubSize, &importTestKey);
// 	if (importVal != 0) {
// 		print_error("Import error failed: %d | eccPubSize: %d\n", importVal, eccPubSize);
// 	}
	
//     // validate signature
// 	int validCheck = asym_validate(signature, sig_len, digest, HASH_SIZE, &status, &importTestKey);
// 	if (validCheck != 0) {
// 		print_error("Validation failed: %d\n", validCheck);
//        exit(EXIT_FAILURE);
// 	} else if (status == 0) {
// 		print_error("Invalid Signature: %d\n", status);
//        exit(EXIT_FAILURE);
// 	} else {
// 		print_debug("Verification succeeded. Signature is Valid!\n");
		
// 		// Decrypt the encrypted message and print out
// 		uint8_t decrypted[BLOCK_SIZE];
        
//         size_t plaintext_length;
//     	decrypt_sym(ciphertext, BLOCK_SIZE, key, decrypted, &plaintext_length);
//     	print_debug("Decrypted message: %s\r\n", decrypted);
// 	}
// }


/*********************************** MAIN *************************************/

int main() {
    // Initialize board
    init();

    // generate symmetric and asymmetric keys
    byte publicKey[ECC_BUFSIZE];
	generate_keys(publicKey, symmetric_key);

    // Print the component IDs to be helpful
    // Your design does not need to do this
    print_info("Application Processor Started\n");
    
    bool keysExchanged = false;

    // Handle commands forever
    size_t size = 50;
    char buf[size];
    while (1) {
        recv_input("Enter Command: ", buf, size);

        if (!keysExchanged) {
            // Print out provisioned component IDs
            for (unsigned i = 0; i < flash_status.component_cnt; i++) {
                // Send symmetric key
                secure_symkey_send(flash_status.component_ids[i], symmetric_key, 32);

                // // Receive component public keys
                // uint8_t receive_buffer[secure_msg_size];
                // int received_length = secure_pubkey_receive(flash_status.component_ids[i], receive_buffer, i);
                
                // // Print received data
                // printf("Received pubKey %d: ", received_length);
                // for (int i = 0; i < received_length; i++) {
                //     printf("%d", receive_buffer[i]);
                // }
                // printf("\n");

                // Send public key
				// secure_send(flash_status.component_ids[i], publicKey, sizeof(publicKey));
            }
            keysExchanged = true;
        }

        // Execute requested command
        if (!strcmp(buf, "list")) {
            scan_components();
        } else if (!strcmp(buf, "boot")) {
            attempt_boot();
        } else if (!strcmp(buf, "replace")) {
            attempt_replace();
        } else if (!strcmp(buf, "attest")) {
            attempt_attest();
        } else {
            print_error("Unrecognized command '%s'\n", buf);
        }
    }

    // Code never reaches here
    return 0;
}
