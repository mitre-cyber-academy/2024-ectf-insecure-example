import os
import json
import ctypes
import sys
from pathlib import Path
import secrets
import 
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





# some main helper functions

def create_shares(stream_length=16):
    """
    The function returns 3 random byte streams and k1 k2 k3
    """
    byte_stream1 = secrets.token_bytes(stream_length)
    byte_stream2 = secrets.token_bytes(stream_length)
    byte_stream3 = secrets.token_bytes(stream_length)
    return [byte_stream1,byte_stream2,byte_stream3]

def gen_AES_key(shares:list):
    # get all the byte shares and xor them together, return the final byte
    # this returns the K 

    result=shares[0] ^ shares[1] ^ shares[2]
    return result # this is the K = k1 ^ k2 ^ k3

   # This is for Ap
    """
    1. Get pin, token, component ids, component count boot message
    2. Get the ids, create file in deployment folder 
    3. Encrypt using AES for Token 
    4. Encrypt using SHA256 for Pins 
    5. Write all the encrypted messages (Pin sha 256, Token AES)
    """
   # this is for deployment
    """
    1. Read the deployment file, find its respective ID
    2. Read K_share, K from the file, if exist. Otherwise, empty
    3. Read the parmeter.h get all clear-text attestation data 
    4. Encrypt All attestation data and write back to the .h file
    """



def get_file_paths()->list:
    fh = open(Path("./inc/ectf_params.h"), "r")
    
    
    AP=sys.argv[0]
    component_ids=#get them from somewhere

def write_to_files(shares:list, files:list)->None:
    with open(files[0], 'ab') as file:
        file.write(shares[0])
    with open(files[1], 'ab') as file:
        file.write(shares[1])
    with open(files[2], 'ab') as file:
        file.write(shares[2])
    
    return 


















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