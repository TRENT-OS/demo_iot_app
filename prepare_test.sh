#!/bin/bash -ue

#-------------------------------------------------------------------------------
#
# Prepare test script
#
# Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
#
#-------------------------------------------------------------------------------

# This script assumes it is located in the test systems root folder and should be invoked
# from the desired test output directory.
TEST_SYSTEM_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"


#-------------------------------------------------------------------------------
# Configuration Provisioning Tool
function sdk_cpt()
{
    local DIR_SDK=$1
    local CFG_XML=$2
    local IMG_OUT=$3

    # Run the tool with the provided path to the XML file holding the
    # configuration and create an image with the output filename provided.
    # Since the demo is using a FAT filesystem, the option is set accordingly.
    ${DIR_SDK}/bin/cpt -i ${CFG_XML} -o ${IMG_OUT} -t FAT
}


#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
DIR_SDK=$1
shift 1

echo "Running Configuration Provisining Tool from SDK directory: ${DIR_SDK}"

# run the tool with the configuration file provided by the system. The created
# image needs to be named "nvm_06", since the system makes use of the Proxy App
# which expects the NVM file name to follow the naming convention
# "nvm_[channelID]". The system makes use of the first NVM channel of the Proxy,
# which maps to the channel number six of the App -> nvm_06
sdk_cpt ${DIR_SDK} ${TEST_SYSTEM_DIR}/configuration/config.xml nvm_06