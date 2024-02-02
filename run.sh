#!/bin/bash

# Get the absolute path of the current directory
current_directory=$(pwd)
ap_device="tty.usbmodem11302"
compA_device="tty.usbmodem11202"
compB_device="tty.usbmodem11402"
bootloader_file="$current_directory/insecure.bin"
DAPLINK_VOLUME="/Volumes/DAPLINK/"
DAPLINK1_VOLUME="/Volumes/DAPLINK 1/"
DAPLINK2_VOLUME="/Volumes/DAPLINK 2/"

# Print the result
echo "The absolute path of the current directory is: $current_directory"

# Load Bootloader to DAPLINK interfaces
echo "Loading Bootloader to DAPLINK Interfaces...."
sudo cp "$bootloader_file" "$DAPLINK_VOLUME"
sudo cp "$bootloader_file" "$DAPLINK1_VOLUME"
sudo cp "$bootloader_file" "$DAPLINK2_VOLUME"

# Build Deployment and create global_secrets.h
echo "Building Deployment to generate global_secrets.h...."
sudo poetry run ectf_build_depl -d "$current_directory" || { echo "ERROR: Failed to build deployment."; exit 1; }

# Build Application processor
echo "Building Application Processor...."
sudo poetry run ectf_build_ap -d "$current_directory" -on ap --p 123456 -c 2 -ids "0x11111124, 0x11111125" -b "Test boot message" -t 0123456789abcdef -od build || { echo "ERROR: Failed to build Application processor."; exit 1; }

# Flash updated AP img file to AP device
echo "Flashing updated ap.img file onto Application Processor $ap_device...."
sudo poetry run ectf_update --infile "$current_directory/build/ap.img" --port "/dev/$ap_device"

# Build Component A
echo "Building Component A...."
sudo poetry run ectf_build_comp -d "$current_directory" -on comp -od build -id 0x11111124 -b "Component boot" -al "McLean" -ad "08/08/08" -ac "Fritz" || { echo "ERROR: Failed to build Component A."; exit 1; }

# Flash updated Component A img file to AP device
echo "Flashing updated comp.img file onto Component A $compA_device...."
sudo poetry run ectf_update --infile "$current_directory/build/comp.img" --port "/dev/$compA_device"

# Build Component B
echo "Building Component B...."
sudo poetry run ectf_build_comp -d "$current_directory" -on comp -od build -id 0x11111125 -b "Component boot" -al "Doe" -ad "03/04/05" -ac "John" || { echo "ERROR: Failed to build Component B."; exit 1; }

# Flash updated Component B img file to AP device
echo "Flashing updated comp.img file onto Component B $compB_device...."
sudo poetry run ectf_update --infile "$current_directory/build/comp.img" --port "/dev/$compB_device"

echo "SUCCESS: Script executed successfully."