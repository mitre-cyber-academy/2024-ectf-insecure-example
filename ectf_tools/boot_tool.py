# @file boot_tool.py
# @author Frederich Stine
# @brief Host tool for booting system
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
import re
from loguru import logger
import sys

# Logger formatting
fmt = (
    "<green>{time:YYYY-MM-DD HH:mm:ss.SSS}</green> | "
    "{extra[extra]: <6} | "
    "<level>{level: <8}</level> | "
    "<level>{message}</level> "
)

logger.remove(0)
logger.add(sys.stdout, format=fmt)


# Boot function
def boot(args):
    ser = serial.Serial(
        port=args.application_processor,
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
    )

    # Send command
    ser.write(b"boot\r")
    logger.bind(extra="INPUT").debug("boot\r")
    output = ""
    # Receive messages until done
    while True:
        byte = ser.read()
        char = byte.decode("utf-8")
        output += char
        output = process_output(output)


def process_output(output):
    # Find INFO level messages
    match = re.search("%info: ((.|\n|\r)*?)%", output)
    if match != None:
        # Output all of the data and remove from buffer
        output = output[:match.start()] + output[match.end():]
        for line in match.group(1).strip().split('\n'):
            logger.bind(extra="OUTPUT").info(line.strip())
    # Find DEBUG level messages
    match = re.search("%debug: ((.|\n|\r)*?)%", output)
    if match != None:
        # Output all of the data and remove from buffer
        output = output[:match.start()] + output[match.end():]
        for line in match.group(1).strip().split('\n'):
            logger.bind(extra="OUTPUT").debug(line.strip())
    # Find ACK level messages
    match = re.search("%ack%", output)
    if match != None:
        # Ignore
        output = output[:match.start()] + output[match.end():]
    # Find SUCCESS level messages
    match = re.search("%success: ((.|\n|\r)*?)%", output)
    if match != None:
        # Output all of the data and remove from buffer
        output = output[:match.start()] + output[match.end():]
        for line in match.group(1).strip().split('\n'):
            logger.bind(extra="OUTPUT").success(line.strip())
        exit(0)
    # Find ERROR level messages
    match = re.search("%error: ((.|\n|\r)*?)%", output)
    if match != None:
        # Output all of the data and remove from buffer
        output = output[:match.start()] + output[match.end():]
        for line in match.group(1).strip().split('\n'):
            logger.bind(extra="OUTPUT").error(line.strip())
        exit(1)
    # Return the spliced output
    return output


# Main function
def main():
    parser = argparse.ArgumentParser(
        prog="eCTF Boot Host Tool", description="Boot the medical device"
    )

    parser.add_argument(
        "-a", "--application-processor", required=True, help="Serial device of the AP"
    )

    args = parser.parse_args()

    boot(args)


if __name__ == "__main__":
    main()

