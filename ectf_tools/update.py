# @file update.py
# @author Jacob Doll
# @brief Tool for installing a new binary onto the eCTF bootloader
# @date 2024
#
# This source file is part of an example system for MITRE's 2024 Embedded CTF (eCTF).
# This code is being provided only for educational purposes for the 2024 MITRE eCTF
# competition, and may not meet MITRE standards for quality. Use this code at your
# own risk!
#
# @copyright Copyright (c) 2024 The MITRE Corporation

import argparse
import serial
from pathlib import Path
from tqdm import tqdm

PAGE_SIZE = 8192
APP_PAGES = 28
TOTAL_SIZE = APP_PAGES * PAGE_SIZE

success_codes = [1, 2, 3, 4, 5, 7, 8, 10, 11, 13, 16, 18, 19, 20]
error_codes = [6, 9, 12, 14, 15, 17]

UPDATE_COMMAND = b"\x00"


# Wait for expected bootloader repsonse byte
# Exit if response does not match
def verify_resp(ser, print_out=True):
    resp = ser.read(1)
    while (resp == b"") or (not ord(resp) in (success_codes + error_codes)):
        resp = ser.read(1)
    if ord(resp) not in success_codes:
        print(f"Error. Bootloader responded with: {ord(resp)}")
        exit()
    if print_out:
        print(f"Success. Bootloader responded with code {ord(resp)}")

    return ord(resp)


def image_update(in_file, port):
    # Open serial port
    ser = serial.Serial(
        port=port,
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=2,
    )
    ser.reset_input_buffer()

    print(f"Connected to bootloader on {port}")

    # Open protected image
    img_file = Path(in_file)
    if not img_file.exists():
        print(f"Image file {img_file} not found. Exiting")
        exit()

    with open(img_file, "rb") as image_fp:
        # Send update command
        print("Requesting update")
        ser.write(b"\x00")

        verify_resp(ser)
        verify_resp(ser)

        # Send image and verify each block
        print("Update started")
        print("Sending image data")

        t = tqdm(total=(TOTAL_SIZE))

        block_bytes = image_fp.read(16)
        while block_bytes != b"":
            t.update(16)

            ser.write(block_bytes)
            verify_resp(ser, print_out=False)
            block_bytes = image_fp.read(16)

        t.close()

        print("Listening for installation status...\n")

        # Wait for update finish
        resp = -1
        while resp != success_codes[-1]:
            resp = verify_resp(ser)

        print("\nUpdate Complete!\n")

    ser.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--infile", required=True, type=Path, help="Path to the input binary"
    )
    parser.add_argument("--port", required=True, help="Serial port")

    args = parser.parse_args()
    image_update(args.infile, args.port)


if __name__ == "__main__":
    main()
