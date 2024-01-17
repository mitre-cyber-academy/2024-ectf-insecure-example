# @file build_depl.py
# @author Frederich Stine
# @brief Tool for building deployment
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

from ectf_tools.utils import run_shell


def build_depl(
    design: Path
):
    """
    Build a deployment
    """

    logger.info("Running build")
    output = asyncio.run(run_shell(
        f"cd {design} && "
        f"pwd && "
        f"nix-shell --command "
        f"\"cd deployment && "
        f" make clean && "
        f" make\""
    ))

    logger.info("Built deployment")

    return output


def main():
    parser = argparse.ArgumentParser(
        prog="eCTF Build Deployment Tool",
        description="Build a deployment using Nix"
    )

    parser.add_argument(
        "-d", "--design", required=True, type=Path,
        help="Path to the root directory of the included design"
    )

    args = parser.parse_args()

    build_depl(
        args.design,
    )
 

if __name__ == '__main__':
    main()
    
