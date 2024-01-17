# @file replace_tool.py
# @author Frederich Stine
# @brief host tool for replacing a component on the system
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
import time
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


# Replace function
def replace(args):
    ser = serial.Serial(
        port=args.application_processor,
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
    )

    # Arguments passed to the AP
    input_list = [
        "replace\r",
        f"{args.token}\r",
        f"{args.component_in}\r",
        f"{args.component_out}\r"
    ]

    # Send and receive messages until done
    message = input_list.pop(0)
    ser.write(message.encode())
    logger.bind(extra="INPUT").debug(message)
    output = ""
    while True:
        byte = ser.read()
        char = byte.decode("utf-8")
        output += char
        output = process_output(output, input_list, ser)


def process_output(output, input_list, ser):
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
        # Send next message
        output = output[:match.start()] + output[match.end():]
        message = input_list.pop(0)
        ser.write(message.encode())
        logger.bind(extra="INPUT").debug(message)
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
    # Parse arguments
    parser = argparse.ArgumentParser(
        prog="eCTF Replace Host Tool",
        description="Replace a component on the medical device",
    )

    parser.add_argument(
        "-a", "--application-processor", required=True, help="Serial device of the AP"
    )
    parser.add_argument(
        "-t", "--token", required=True, help="Replacement token for the AP"
    )
    parser.add_argument(
        "-i", "--component-in", required=True, help="Component ID of the new component"
    )
    parser.add_argument(
        "-o",
        "--component-out",
        required=True,
        help="Component ID of the component being replaced",
    )

    args = parser.parse_args()

    replace(args)
    

if __name__ == "__main__":

    main()
