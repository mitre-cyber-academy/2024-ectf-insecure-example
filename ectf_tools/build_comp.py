# @file build_comp.py
# @author Frederich Stine
# @brief Tool for building component firmware
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

from ectf_tools.utils import run_shell, package_binary


def build_component(
    design: Path,
    output_name,
    output_dir: Path,
    component_id,
    boot_message,
    attestation_location,
    attestation_date,
    attestation_customer
):
    """
    Build an application processor.
    """

    try:
        os.makedirs(output_dir, exist_ok=True)
    except Exception as e:
        print(e)
        raise

    logger.info("Creating parameters for build")
    fh = open(design / Path("component/inc/ectf_params.h"), "w")
    fh.write("#ifndef __ECTF_PARAMS__\n")
    fh.write("#define __ECTF_PARAMS__\n")
    fh.write(f"#define COMPONENT_ID {component_id}\n")
    fh.write(f"#define COMPONENT_BOOT_MSG \"{boot_message}\"\n")
    
    
    # 2.txt K, K2
    
    # define k_share k2 
    
    fh.write(f"#define ATTESTATION_LOC \"{attestation_location}\"\n") #encyrpt with K 
    fh.write(f"#define ATTESTATION_DATE \"{attestation_date}\"\n")   #encyrpt with K 
    fh.write(f"#define ATTESTATION_CUSTOMER \"{attestation_customer}\"\n")  #encyrpt with K 
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
        f"\"cd component && "
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
        "-id", "--component-id", required=True,
        help="Component ID for the provisioned component"
    )

    parser.add_argument(
        "-b", "--boot-message", required=True,
        help="Component boot message"
    )

    parser.add_argument(
        "-al", "--attestation-location", required=True,
        help="Attestation data location field"
    )
    
    parser.add_argument(
        "-ad", "--attestation-date", required=True,
        help="Attestation data date field"
    )

    parser.add_argument(
        "-ac", "--attestation-customer", required=True,
        help="Attestation data customer field"
    )
    
    args = parser.parse_args()

    build_component(
        args.design,
        args.output_name,
        args.output_dir,
        args.component_id,
        args.boot_message,
        args.attestation_location,
        args.attestation_date,
        args.attestation_customer
    )
 

if __name__ == '__main__':
    main()
    
