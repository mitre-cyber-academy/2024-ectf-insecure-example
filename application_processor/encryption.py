import ctypes
from pathlib import Path
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
    def __init__(self, key):
        self.key=key

    def encrypt(self, message):
        message_bytes = str.encode(message)
        return encryption(self.key, message_bytes)
    
    def encrypt_byte(self,message_byte):
        return encryption(self.key, message_byte)

    def decrypt(self, cipher_text):
        return decryption(self.key, cipher_text)