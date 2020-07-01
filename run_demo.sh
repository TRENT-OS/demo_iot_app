#!/bin/bash -em

#-------------------------------------------------------------------------------
#
# Script to run the IoT Demo
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
USAGE_STRING="run_demo.sh <path-to-project-build> <path-to-proxy>\n
This script starts the Mosquitto Broker and runs the IoT Demo app\n"
OPT_INTERACTIVE="-it"

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
echo ${SCRIPTPATH}

if [ "$1" == "" ]; then
    echo -e "${USAGE_STRING}"
    exit 1
fi

PROJECT_PATH=$1
PROXY_PATH=$2
CPT_PATH=$3

if [ -z ${PROJECT_PATH} ]; then
    echo "ERROR: missing project path"
    exit 1
fi

# default is the zynq7000 platform
IMAGE_PATH=${PROJECT_PATH}/images/capdl-loader-image-arm-zynq7000
if [ ! -f ${IMAGE_PATH} ]; then
    echo "ERROR: missing project image ${IMAGE_PATH}"
    exit 1
fi

if [ ! -f ${PROXY_PATH}/proxy_app ]; then
    echo "ERROR: proxy application path missing!"
    exit 1
fi

if [ ! -f ${CPT_PATH}/cpt ]; then
    echo "ERROR: config provisioning tool path missing!"
    exit 1
fi

shift 3

# create provisioned partition from XML file
echo "Creating provisioned partition"
${CPT_PATH}/cpt -i ${SCRIPTPATH}/configuration/config.xml -o nvm_06
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

#run the Mosquitto MQTT broker
mosquitto -c /etc/mosquitto/mosquitto.conf > mosquitto_log.txt &
sleep 1

# run QEMU
qemu-system-arm  ${QEMU_PARAMS[@]} $@ 2> qemu_stderr.txt &
sleep 1

# start proxy app
${PROXY_PATH}/proxy_app -c TCP:4444 -t 1  > proxy_app.out &
sleep 1

fg
