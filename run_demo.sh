#!/bin/bash -em

#-------------------------------------------------------------------------------
#
# Script to run the IoT Demo
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
PROJECT_PATH=$1
SDK_PATH=$2

CURRENT_SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"
DIR_BIN_SDK=${SDK_PATH}/bin
# Be Aware! Any change to the usage of the script is likely supposed to be kept
# in sync with the official documentation (wiki handbook) like for the pahe at
# https://wiki.hensoldt-cyber.systems/display/HEN/12.+TRENTOS-M+DEMO+-+IoT+Demo
USAGE_STRING="Usage: ./run_demo.sh <path-to-project-build> <path-to-sdk>"

if [ -z ${PROJECT_PATH} ]; then
    echo "ERROR: missing path to project build!"
    echo ${USAGE_STRING}
    exit 1
fi

CMAKE_FILE_PATH=${SDK_PATH}/CMakeLists.txt
BUILD_FILE_PATH=${SDK_PATH}/build-system.sh
if [ ! -f ${CMAKE_FILE_PATH} ] || [ ! -f ${BUILD_FILE_PATH} ]; then
    echo "ERROR: missing (or wrong) path to sdk!"
    echo ${USAGE_STRING}
    exit 1
fi

shift 2

IMAGE_PATH=${PROJECT_PATH}/images/capdl-loader-image-arm-zynq7000

# create provisioned partition from XML file
echo "Creating configuration provisioned partition"
# run the tool with the configuration file provided by the system. The created
# image needs to be named "nvm_06", since the system makes use of the Proxy App
# which expects the NVM file name to follow the naming convention
# "nvm_[channelID]". The system makes use of the first NVM channel of the Proxy,
# which maps to the channel number six of the App -> nvm_06.
# Since the demo is using a FAT filesystem, the option is set accordingly.
${DIR_BIN_SDK}/cpt -i ${CURRENT_SCRIPT_DIR}/configuration/config.xml -o nvm_06 -t FAT
sleep 1

QEMU_PARAMS=(
    -machine xilinx-zynq-a9
    -m size=512M
    -nographic
    -s
    -serial tcp:localhost:4444,server
    -serial mon:stdio
    -kernel ${IMAGE_PATH}
)

# run QEMU
qemu-system-arm  ${QEMU_PARAMS[@]} $@ 2> qemu_stderr.txt &
sleep 1

# start proxy app
${DIR_BIN_SDK}/proxy_app -c TCP:4444 -t 1  > proxy_app.out &
sleep 1

fg
