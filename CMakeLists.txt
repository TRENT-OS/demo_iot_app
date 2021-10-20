#
# Demo IoT
#
# Copyright (C) 2019-2021, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

os_set_config_file(
    "system_config.h"
    CONFIG_PROJECT_NAME "system_config"
)

add_subdirectory("include/mqtt")

#-------------------------------------------------------------------------------
project(demo_iot_app C)

DeclareCAmkESComponent(
    SensorTemp
    INCLUDES
        include/util
    SOURCES
        components/Sensor/src/SensorTemp.c
        components/common/common.c
        include/util/helper_func.c
    C_FLAGS
        -Wall -Werror
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
    LIBS
        system_config
        lib_debug
        os_core_api
        os_configuration
        os_logger
        os_mqtt
)

DeclareCAmkESComponent(
    ConfigServer
    SOURCES
        components/ConfigServer/src/ConfigServer.c
        components/ConfigServer/src/init_config_backend.c
        components/common/common.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_CONFIG_SERVICE_BACKEND_FILESYSTEM
        -DOS_CONFIG_SERVICE_CAMKES_SERVER
    LIBS
        system_config
        lib_debug
        os_core_api
        os_configuration
        os_filesystem
        os_logger
)

DeclareCAmkESComponent(
    LogServer
    SOURCES
        components/LogServer/src/LogServer.c
        components/LogServer/src/custom_log_format.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        os_core_api
        lib_debug
        os_log_server_backend_filesystem
        os_filesystem
        TimeServer_client
)

DeclareCAmkESComponent(
    CloudConnector
    INCLUDES
        include
        include/util
    SOURCES
        components/CloudConnector/src/CloudConnector.c
        components/CloudConnector/src/init_CloudConnector.c
        components/CloudConnector/src/MQTT_net.c
        components/CloudConnector/src/MQTTServer.c
        components/CloudConnector/src/MQTT_client.c
        components/CloudConnector/src/glue_tls_mqtt.c
        components/common/common.c
        include/util/helper_func.c
    C_FLAGS
        -Wall -Werror
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
    LIBS
        os_core_api
        lib_compiler
        lib_debug
        lib_utils
        os_crypto
        os_mqtt
        os_tls
        os_configuration
        os_logger
        os_socket_client
        TimeServer_client
)

DeclareCAmkESComponent(
    NwStackConfigurator
    INCLUDES
        include/util
    SOURCES
        components/NwStackConfigurator/NwStackConfigurator.c
        include/util/helper_func.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
    LIBS
        system_config
        os_core_api
        os_configuration
        lib_debug
        networkStack_PicoTcp_api
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NetworkStack_PicoTcp
    C_FLAGS
        -DUSE_LOGSERVER
        -DLOGSERVER_CLIENT_LEVEL_FILTER=Debug_LOG_LEVEL_INFO
        -DLOGSERVER_DATAPORT=logServer_port
    LIBS
        os_logger
)

NIC_ChanMux_DeclareCAmkESComponent(
    NwDriver
    CHANMUX_CHANNEL_NIC_CTRL
    CHANMUX_CHANNEL_NIC_DATA
)

StorageServer_DeclareCAmkESComponent(
    StorageServer
)

Storage_ChanMux_DeclareCAmkESComponent(
    Storage_ChanMux
)

ChanMux_UART_DeclareCAmkESComponents(
    ChanMux_UART
    components/ChanMux/ChanMux_config.c
    system_config
)

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

EntropySource_DeclareCAmkESComponent(
    EntropySource
)

DeclareAndCreateCamkESSystem(DemoIotApp.camkes)
GenerateSimulateScript()
