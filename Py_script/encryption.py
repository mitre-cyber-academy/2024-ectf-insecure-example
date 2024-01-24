# encrypt messages (AES)
# append nonce to pair pin

# To use this function, cryptography need to be installed
# use command pip install cryptography in your virtual environment
# documentation can be found here : https://pypi.org/project/pycrypto/

import os
import json
import ctypes
import sys
from pathlib import Path
import secrets
# cc -fPIC -shared -o encryptlib.so encrypt.c

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
    The function returns 3 random byte streams and 
    """
    byte_stream1 = secrets.token_bytes(stream_length)
    byte_stream2 = secrets.token_bytes(stream_length)
    byte_stream3 = secrets.token_bytes(stream_length)
    return [byte_stream1,byte_stream2,byte_stream3]

def gen_AES_key(shares:list):
    # get all the byte shares and xor them together, return the final byte

    result=shares[0] ^ shares[1] ^ shares[2]
    return result 

def get_file_paths():
    AP=sys.argv[0]
    component_ids=#get them from somewhere

def write_to_files(files:list):


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