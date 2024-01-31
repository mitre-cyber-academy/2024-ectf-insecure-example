from pathlib import Path
import re


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

# ------------------------------ this is for file Writing and modification --------------------------------------
macro_information={}

# End of Global Data Definition: 

def get_ids(macro):
    pattern = r'0x[\da-fA-F]+'    
    ids = re.findall(pattern, macro)
    return ids
        
def get_boot_message(macro):
    pattern = r'#define COMPONENT_BOOT_MSG\s*"([^"]+)"'
    match = re.search(pattern, macro)
    
    # Check if a match is found
    if match:
        boot_message = match.group(1)
        return boot_message
    else:
        print("No match found.")

def get_atts_loc(macro):
    pattern = r'#define ATTESTATION_LOC\s*"([^"]+)"'
    match = re.search(pattern, macro)
    
    # Check if a match is found
    if match:
        boot_message = match.group(1)
        return boot_message
    else:
        print("No match found.")

def get_atts_date(macro):
    pattern = r'#define ATTESTATION_DATE\s*"([^"]+)"'
    match = re.search(pattern, macro)
    
    # Check if a match is found
    if match:
        boot_message = match.group(1)
        return boot_message
    else:
        print("No match found.")

def get_atts_customer(macro):
    pattern = r'#define ATTESTATION_CUSTOMER\s*"([^"]+)"'
    match = re.search(pattern, macro)
    
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
    macro_information["ids"]=get_ids(lines[2])
    macro_information["message"]=get_boot_message(lines[3])
    macro_information["location"]=get_atts_loc(lines[4])
    macro_information["date"]=get_atts_date(lines[5])
    macro_information["customer"]=get_atts_customer(lines[6])
    return 

def file_exist(file_path)->bool:
    if file_path.exists():
        return True
    else:
        return False

def change_byte_to_macro(byte_stream, name)->str:
    hex_representation = ', '.join([f'0x{byte:02X}' for byte in byte_stream])
    macro_string = f"#define {name} {{ {hex_representation} }}"
    return macro_string
    
def Read_files()->None:
    if file_exist(Path(f"../deployment/{macro_information['ids']}.txt")):
        fh = open(f"../deployment/{macro_information['ids']}.txt", "r")
        lines = fh.readlines()
        fh.close()
        macro_information["share"]=get_atts_customer(lines[1])
        macro_information["mask"]=get_atts_customer(lines[2])
        macro_information["final"]=get_atts_customer(lines[3])
    else:
        macro_information["share"]=change_byte_to_macro("0000000000000000".encode(),'KEY_SHARE')
        macro_information["mask"]=change_byte_to_macro("0000000000000000".encode(),'MASK')
        macro_information["final"]=change_byte_to_macro("0000000000000000".encode(),'FINAL_MASK')


def write_key_to_files()->None:
    """
    Given some paths for component, writes the key shares repsectively to the file
    Also write everything back to the AP file, encrypted, of course
    """

    # Finally write the keys into the AP's parameter header
    fh = open("inc/ectf_params.h", "w")
    fh.write("#ifndef __ECTF_PARAMS__\n")
    fh.write("#define __ECTF_PARAMS__\n")
    fh.write(f"#define COMPONENT_ID \"{macro_information['ids']}\"\n") 
    fh.write(f"#define COMPONENT_BOOT_MSG \"{macro_information['message']}\"\n") 
    fh.write(f"#define ATTESTATION_LOC \"{macro_information['location']}\"\n") 
    fh.write(f"#define ATTESTATION_DATE \"{macro_information['date']}\"\n") 
    fh.write(f"#define ATTESTATION_CUSTOMER \"{macro_information['customer']}\"\n") 
    fh.write(macro_information["share"]+'\n')
    fh.write(macro_information["mask"]+'\n')
    fh.write(macro_information["final"]+'\n')    
    fh.write("#endif\n")
    fh.close()



# ------------------------------ End of Previous Deinition, this is the main file -----------------------------------

if __name__ == "__main__":
    extract_info()
    Read_files()
    write_key_to_files()