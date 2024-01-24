# @file utils.py
# @author Frederich Stine
# @brief Helper functions
# @date 2024
#
# This source file is part of an example system for MITRE's 2024 Embedded CTF (eCTF).
# This code is being provided only for educational purposes for the 2024 MITRE eCTF
# competition, and may not meet MITRE standards for quality. Use this code at your
# own risk!
#
# @copyright Copyright (c) 2024 The MITRE Corporation

import asyncio
import subprocess
from loguru import logger 
from typing import Tuple, Callable, Awaitable
import shlex
from time import sleep
import os
from pathlib import Path

HandlerRet = Tuple[bytes, bytes]
HandlerTy = Callable[..., Awaitable[Tuple[bytes, bytes]]]

class CmdFailedError(Exception):
    pass

"""
Run shell command
"""
async def run_shell(cmd: str) -> HandlerRet:
    logger.debug(f"Running command {repr(cmd)}")
    proc = await asyncio.create_subprocess_shell(
        cmd,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )

    stdout_raw, stderr_raw = await proc.communicate()
    stdout = stdout_raw.decode(errors="backslashreplace")
    stderr = stderr_raw.decode(errors="backslashreplace")
    stdout_msg = f"STDOUT:\n{stdout}" if stdout else "NO STDOUT"
    stderr_msg = f"STDERR:\n{stderr}" if stderr else "NO STDERR"
    #if proc.returncode:
    #    logger.error(stdout_msg)
    #    logger.error(stderr_msg)
    #    raise CmdFailedError(
    #        f"Tool build failed with return code {proc.returncode}", stdout, stderr
    #    )
    logger.info(stdout_msg)
    logger.info(stderr_msg)
    return stdout_raw, stderr_raw


PAGE_SIZE = 8192

APP_PAGES = 28
TOTAL_SIZE = APP_PAGES * PAGE_SIZE

"""
Package a device image for use with the bootstrapper
"""
def package_binary(bin_path, image_path):

    # Read input binaries
    with open(bin_path, "rb") as fp:
        bl_data = fp.read()

    # Pad bootloader to max size
    image_bl_pad_len = TOTAL_SIZE - len(bl_data)
    image_bl_padding = b"\xff" * image_bl_pad_len
    image_bl_data = bl_data + image_bl_padding

    # Write output binary
    with open(image_path, "wb") as fp:
        fp.write(image_bl_data)



def i2c_address_is_blacklisted(addr):
    addr &= 0xFF
    if 0 <= addr <= 7: 
        return True
    elif 0x78 <= addr <= 0x7F:
        return True
    elif addr in (0x18, 0x28, 0x36):
        return True
    else:
        return False