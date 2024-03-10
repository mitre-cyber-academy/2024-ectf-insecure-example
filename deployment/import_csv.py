import csv
import os
import sys
import secrets
from pathlib import Path

def change_byte_to_const(byte_stream, name)->str:
    hex_representation = ', '.join([f'0x{byte:02X}' for byte in byte_stream])
    macro_string = f"const uint8_t {name}[16] = {{ {hex_representation} }};"
    return macro_string

def generate_csv(filename, rows):
    # Generate a CSV file with random data
    with open(filename, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        for i in range(0,rows,2):
            writer.writerow([change_byte_to_const(secrets.token_bytes(16), "MASK")])
            writer.writerow([change_byte_to_const(secrets.token_bytes(16), "FINAL_MASK")])
    csvfile.close()

def get_secret_key_from_csv(filename, row):
    # Read the secret key from the CSV file
    with open(filename, 'r') as csvfile:
        reader = csv.reader(csvfile)
        for i, line in enumerate(reader):
            if i == row:
                print("Secret key: {}".format(line[0]))
                return line[0]

def get_nums():
    file_path = Path("../comp_count.txt")
    if not os.path.exists(file_path):
        # If the file doesn't exist, create it and write "1"
        with open(file_path, "w") as f:
            # write nothing
            #just create the file
            pass
        
        return 1


if __name__ == '__main__':
   
    filename  = Path("cc.csv")
    rows = 600
    generate_csv(filename, rows)
    # print("Generated CSV file: {}".format(filename))
    # Read the secret key from the CSV file 10 rows down
    get_nums()
    # for i in range(10):
    #     get_secret_key_from_csv(filename, i)
    sys.exit(0)