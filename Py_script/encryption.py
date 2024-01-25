import os
import json
import ctypes
import sys
from pathlib import Path
import secrets
import hashlib
import re
# cc -fPIC -shared -o encryptlib.so encrypt.c check if linux has this

path_to_lib = str(Path.cwd() / "encryptlib.so")
encryptlib = ctypes.CDLL(path_to_lib)

# setting up functions
encryption = encryptlib.encryption
encryption.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
encryption.restype = ctypes.c_char_p

decryption = encryptlib.decryption
decryption.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
decryption.restype = ctypes.c_char_p

class Encrypt:
    def __init__(self, key, path=None):
        if path==None:
            with open(path) as f:
                content=f.read()
            content=content[:16].encode()
            self.key=content
        else:
            self.key=key

    def encrypt(self, message):
        message_bytes = str.encode(message)
        return encryption(self.key, message_bytes)
    
    def encrypt_byte(self,message_byte):
        return encryption(self.key, message_byte)

    def decrypt(self, cipher_text):
        return decryption(self.key, cipher_text)



# this is for deployment
"""
1. Read the deployment file, find its respective ID
2. Read K_share, K from the file, if exist. Otherwise, empty
3. Read the parmeter.h get all clear-text attestation data 
4. Encrypt All attestation data and write back to the .h file
"""








# This is for Ap
"""
1. Get pin, token, component ids, component count boot message
2. Get the ids, create file in deployment folder 
3. Encrypt using AES for Token 
4. Encrypt using SHA256 for Pins 
5. Write all the encrypted messages (Pin sha 256, Token AES)
"""


# ------------------------------ This is for key generation ------------------------------

def create_shares(stream_length=16, num_shares=3):
    """
    The function returns 3 random byte streams and k1 k2 k3
    """
    if num_shares==2:
        byte_stream1 = secrets.token_bytes(stream_length)
        byte_stream2 = secrets.token_bytes(stream_length)
        return [byte_stream1,byte_stream2]
    else:
        byte_stream1 = secrets.token_bytes(stream_length)
        byte_stream2 = secrets.token_bytes(stream_length)
        byte_stream3 = secrets.token_bytes(stream_length)
        return [byte_stream1,byte_stream2,byte_stream3]

def gen_AES_key(shares:list):
    # get all the byte shares and xor them together, return the final byte
    # this returns the K 
    if len(shares)==2:
        result=shares[0] ^ shares[1]
    else:
        result=shares[0] ^ shares[1] ^ shares[2]
    return result # this is the K = k1 ^ k2 ^ k3


# ------------------------------ this is for file Writing and modification --------------------


# here we define some Global Data: 
shares=[]
macro_information={}
# End of Global Data Definition: 

def get_ids(ap_macro):
    pattern = r"#define COMPONENT_IDS\s*([\d,]+)"
    match = re.search(pattern, ap_macro)
    
    if match:
        component_ids = match.group(1)
        return component_ids.split(', ')
    else:
        print("No match found.")

def get_cnt(ap_macro):
    pattern = r"#define COMPONENT_CNT\s*([\d,]+)"
    match = re.search(pattern, ap_macro)
    
    if match:
        component_cnt = match.group(1)
        return component_cnt
    else:
        print("No match found.")
        
def get_boot_message(ap_macro):
    pattern = r'#define AP_BOOT_MSG\s*"([^"]+)"'
    match = re.search(pattern, ap_macro)
    
    # Check if a match is found
    if match:
        boot_message = match.group(1)
        return boot_message
    else:
        print("No match found.")

def get_token(ap_macro):
    pattern = r'#define AP_TOKEN\s*"([^"]+)"'
    match = re.search(pattern, ap_macro)
    
    # Check if a match is found
    if match:
        boot_message = match.group(1)
        return boot_message
    else:
        print("No match found.")

def get_pin(ap_macro):
    pattern = r'#define AP_PIN\s*"([^"]+)"'
    match = re.search(pattern, ap_macro)
    
    # Check if a match is found
    if match:
        boot_message = match.group(1)
        return boot_message
    else:
        print("No match found.")
        
def extract_info():
    """
    Pure helper function that put everything into the global data of macro
    """
    fh = open(Path("./inc/ectf_params.h"), "r")
    lines = fh.readlines()
    fh.close()
    macro_information["pin"]=get_pin(lines[2])
    macro_information["token"]=get_token(lines[3])
    macro_information["ids"]=get_ids(lines[4])
    macro_information["cnt"]=get_cnt(lines[5])
    macro_information["message"]=get_boot_message(lines[6])
    return 
    
def get_file_paths()->list:
    """
    This gets the file paths for AP, Components
    
    Returns:
        list: [Path For Component1, Path for Component 2, Count of Components]
    """
    ids=macro_information["ids"]
    count=macro_information["cnt"]
    return [ids[0]+".txt", ids[1]+".txt", count]

def write_key_to_files(file_paths:list)->None:
    """
    Given some paths for component, writes the key shares repsectively to the file
    Also write everything back to the AP file, encrypted, of course
    """
    # hashlib.sha256(key.encode()).digest()
    
    shares=None
    key=None
    if len(file_paths)==1:
        shares=create_shares(16,2)
        key=gen_AES_key(shares)
        f = open(Path(f"../deployment/{macro_information['ids'][0]}.txt"), "w")
        f.write()
        
        
        
        
