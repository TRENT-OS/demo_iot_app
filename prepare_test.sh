#!/bin/bash -ue

#-------------------------------------------------------------------------------
#
# Prepare test script
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
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
    # configuration and create an image with the output filename provided
    ${DIR_SDK}/bin/cpt -i ${CFG_XML} -o ${IMG_OUT}
}


#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
DIR_SDK=$1
shift 1

echo "Running Configuration Provisining Tool from SDK directory: ${DIR_SDK}"

sdk_cpt ${DIR_SDK} ${TEST_SYSTEM_DIR}/configuration/config.xml nvm_06