from pathlib import Path
import secrets
import re
import encryption as ec


# this is for deployment
"""
1. Read the deployment file, find its respective ID
2. Read K_share, K from the file, if exist. Otherwise, empty
3. Read the parmeter.h get all clear-text attestation data 
4. Encrypt All attestation data and write back to the .h file

-----------------------------------------------------------------------------------------------------------------------------

A simple Sanity Check for How to Read the textfile and Encrypt with the first key

    encryption_tool=ec.Encrypt(key) 
    print(encryption_tool.encrypt("12345"))

    with open(Path(f"../deployment/{macro_information['ids'][0]}.txt"), 'rb') as file:
        # Read the first line
        first_line = file.readline()
        encryption_tool=ec.Encrypt(first_line)
        print(encryption_tool.encrypt("12345")) 
"""








# This is for Ap, Maintain and Tested by Jinyao on Jan 25
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
        result=bytes(a ^ b for a, b in zip(shares[0], shares[1]))
    else:
        result = bytes(a ^ b ^ c for a, b, c in zip(shares[0], shares[1], shares[2]))
    return result # this is the K = k1 ^ k2 ^ k3


def change_byte_to_macro(byte_stream, name)->str:
    hex_representation = ', '.join([f'0x{byte:02X}' for byte in byte_stream])
    macro_string = f"#define {name} {{ {hex_representation} }}"
    return macro_string


# ------------------------------ this is for file Writing and modification --------------------------------------


# here we define some Global Data: 
shares=[]
macro_information={}

# End of Global Data Definition: 

def get_ids(ap_macro):
    pattern = r'0x[\da-fA-F]+'
    match = re.search(pattern, ap_macro)
    
    ids = re.findall(pattern, ap_macro)
    return ids

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

def gen_masks(comp_cnt,len=16):
    if comp_cnt==1:
        F1=secrets.token_bytes(len)
        M1=secrets.token_bytes(len)
        return F1,F2
    else:
        F1=secrets.token_bytes(len)
        F2=secrets.token_bytes(len)
        M1=secrets.token_bytes(len)
        M2=secrets.token_bytes(len)
        return F1,F2,M1,M2


def write_key_to_files(file_paths:list)->None:
    """
    Given some paths for component, writes the key shares repsectively to the file
    Also write everything back to the AP file, encrypted, of course
    """
    
    shares=None
    key=None
    comp_val=0
    masks=None
    if len(file_paths)==1:
        masks=gen_masks(1)
        comp_val=1
        shares=create_shares(16,2)
        key=gen_AES_key(shares)

        # Write the keys into the Component file
        f = open(Path(f"../deployment/{macro_information['ids'][0]}.txt"), "wb")
        f.write(key)
        f.write(b'\n')
        f.write(change_byte_to_macro(shares[1],"KEY_SHARE").encode())
        f.write(change_byte_to_macro(masks[0],"MASK").encode())
        f.write(change_byte_to_macro(masks[1],"FINAL_MASK").encode())
        f.close()
    
    else:
        comp_val=2
        shares=create_shares(16,3)
        key=gen_AES_key(shares)
        masks=gen_masks(2)

        # Write the keys into the Component file
        f = open(Path(f"../deployment/{macro_information['ids'][0]}.txt"), "wb")
        f.write(key)
        f.write(b'\n')
        f.write(change_byte_to_macro(shares[1],"KEY_SHARE").encode())
        f.write(b'\n')
        f.write(change_byte_to_macro(masks[0],"MASK").encode())
        f.write(b'\n')
        f.write(change_byte_to_macro(masks[1],"FINAL_MASK").encode())
        f.close()

        f = open(Path(f"../deployment/{macro_information['ids'][1]}.txt"), "wb")
        f.write(key)
        f.write(b'\n')
        f.write(change_byte_to_macro(shares[2],"KEY_SHARE").encode())
        f.write(b'\n')
        f.write(change_byte_to_macro(masks[2],"MASK").encode())
        f.write(b'\n')
        f.write(change_byte_to_macro(masks[3],"FINAL_MASK").encode())
        f.close()
        

    encryption_tool=ec.Encrypt(key) 
    encrypted_pin=encryption_tool.encrypt(macro_information['pin'])
    encrypted_token=encryption_tool.encrypt(macro_information['token'])

    # Finally write the keys into the AP's parameter header
    fh = open("inc/ectf_params.h", "w")
    fh.write("#ifndef __ECTF_PARAMS__\n")
    fh.write("#define __ECTF_PARAMS__\n")
    fh.write(change_byte_to_macro(encrypted_pin,"AP_PIN")+"\n")
    fh.write(change_byte_to_macro(encrypted_token,"AP_TOKEN")+"\n")
    if comp_val==2:
        fh.write(f"#define COMPONENT_IDS {macro_information['ids'][0]+' '+macro_information['ids'][1]}\n") 
    else:
        fh.write(f"#define COMPONENT_IDS {macro_information['ids'][0]}\n") 
        
    fh.write(f"#define COMPONENT_CNT {macro_information['cnt']}\n")
    fh.write(f"#define AP_BOOT_MSG \"{macro_information['message']}\"\n")
    fh.write(change_byte_to_macro(shares[0],"KEY_SHARE")+"\n")
    
    if comp_val==1:
        f.write(change_byte_to_macro(masks[0],f"MASK_{macro_information['ids'][0]}").encode())
        f.write(change_byte_to_macro(masks[1],f"FINAL_MASK_{macro_information['ids'][0]}").encode())
    else:
        f.write(change_byte_to_macro(masks[0],f"MASK_{macro_information['ids'][0]}").encode())
        f.write(change_byte_to_macro(masks[1],f"FINAL_MASK_{macro_information['ids'][0]}").encode())
        f.write(change_byte_to_macro(masks[2],f"MASK_{macro_information['ids'][1]}").encode())
        f.write(change_byte_to_macro(masks[3],f"FINAL_MASK_{macro_information['ids'][1]}").encode())
    
    fh.write("#endif\n")
    fh.close()



# ------------------------------ End of Previous Deinition, this is the main file -----------------------------------

if __name__ == "__main__":
    # this is for test running
    extract_info()
    write_key_to_files(get_file_paths())