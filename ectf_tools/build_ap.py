# @file build_ap.py
# @author Frederich Stine
# @brief Tool for building application processor firmware
# @date 2024
#
# This source file is part of an example system for MITRE's 2024 Embedded CTF (eCTF).
# This code is being provided only for educational purposes for the 2024 MITRE eCTF
# competition, and may not meet MITRE standards for quality. Use this code at your
# own risk!
#
# @copyright Copyright (c) 2024 The MITRE Corporation

from loguru import logger
import argparse
import asyncio
from pathlib import Path
import os

from ectf_tools.utils import run_shell, package_binary, i2c_address_is_blacklisted


def build_ap(
    design: Path,
    output_name,
    output_dir: Path,
    pin,
    token,
    component_cnt,
    component_ids,
    boot_message
):
    """
    Build an application processor.
    """

    try:
        for component_id in component_ids.split(","):
            component_id = int(component_id.strip(), 0)
            if i2c_address_is_blacklisted(component_id):
                logger.error(f"Invalid component ID {component_id:x}")
                logger.error(f"IDs ending in 0x00-0x07, 0x78-0x7F, and 0x18, 0x28, or 0x36 are reserved due to I2C conflicts")
                exit(1)
    except ValueError:
        logger.warning("Cannot parse component IDs to enforce I2C blacklist")

    try:
        os.makedirs(output_dir, exist_ok=True)
    except Exception as e:
        print(e)
        raise

    logger.info("Creating parameters for build")
    fh = open(design / Path("application_processor/inc/ectf_params.h"), "w")
    fh.write("#ifndef __ECTF_PARAMS__\n")
    fh.write("#define __ECTF_PARAMS__\n")
    fh.write(f"#define AP_PIN \"{pin}\"\n")
    fh.write(f"#define AP_TOKEN \"{token}\"\n")
    fh.write(f"#define COMPONENT_IDS {component_ids}\n")
    fh.write(f"#define COMPONENT_CNT {component_cnt}\n")
    fh.write(f"#define AP_BOOT_MSG \"{boot_message}\"\n")
    fh.write("#endif\n")
    fh.close()

    output_dir = os.path.abspath(output_dir)
    output_elf = f"{output_dir}/{output_name}.elf"
    output_bin = f"{output_dir}/{output_name}.bin"
    output_img = f"{output_dir}/{output_name}.img"

    logger.info("Checking output paths")
    if os.path.exists(output_elf):
        logger.info("Removing old .elf output")
        os.remove(output_elf)
    if os.path.exists(output_bin):
        logger.info("Removing old .bin output")
        os.remove(output_bin)
    if os.path.exists(output_img):
        logger.info("Removing old .img output")
        os.remove(output_img)

    logger.info("Running build")
    output = asyncio.run(run_shell(
        f"cd {design} && "
        f"pwd && "
        f"nix-shell --command "
        f"\"cd application_processor && "
        f" make clean && "
        f" make && make release && "
        f" cp build/max78000.elf {output_elf} && "
        f" cp build/max78000.bin {output_bin}\""
    ))

    if not os.path.exists(output_bin):
        logger.error("Error: tool did not build properly")
        exit(1)

    logger.info("Built application processor")

    logger.info("Packaging binary")
    package_binary(output_bin, output_img)
    logger.info("Binary packaged")

    return output


def main():
    parser = argparse.ArgumentParser(
        prog="eCTF Build Application Processor Tool",
        description="Build an Application Processor using Nix"
    )

    parser.add_argument(
        "-d", "--design", required=True, type=Path,
        help="Path to the root directory of the included design"
    )

    parser.add_argument(
        "-on", "--output-name", required=True,
        help=("Output prefix of the built application processor binary \n"
              "Example 'ap' -> ap.bin, ap.elf, ap.img" 
        )
    )

    parser.add_argument(
        "-od", "--output-dir", required=False, type=Path,
        default=Path('.'), help=f"Output name of the directory to store the result: default: %(default)s"
    )

    parser.add_argument(
        "-p", "--pin", required=True,
        help="PIN for built application processor"
    )

    parser.add_argument(
        "-t", "--token", required=True,
        help="Token for built application processor"
    )

    parser.add_argument(
        "-c", "--component-cnt", required=True,
        help="Number of components to provision Application Processor for"    
    )

    parser.add_argument(
        "-ids", "--component-ids", required=True,
        help="Component IDs to provision the Application Processor for"
    )

    parser.add_argument(
        "-b", "--boot-message", required=True,
        help="Application Processor boot message"
    )

    args = parser.parse_args()

    build_ap(
        args.design,
        args.output_name,
        args.output_dir,
        args.pin,
        args.token,
        args.component_cnt,
        args.component_ids,
        args.boot_message
    )
 

if __name__ == '__main__':
    main()
    
