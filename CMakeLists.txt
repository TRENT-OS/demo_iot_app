#
# Demo IoT
#
# Copyright (C) 2019-2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

os_set_config_file(system_config.h CREATE_CONFIG_PROJECT)

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
        os_libs
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
        os_libs
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
        os_libs
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
        os_libs
        os_crypto
        os_mqtt
        os_tls
        os_configuration
        os_logger
        os_network_api
        TimeServer_client
)

DeclareCAmkESComponent(
    NwStack
    INCLUDES
        include/util
    SOURCES
        components/NwStack/network_stack.c
        components/common/common.c
        include/util/helper_func.c
    C_FLAGS
        -Wall -Werror
        -DOS_NWSTACK_AS_CLIENT
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
        -DCFG_ETH_ADDR=ETH_ADDR
        -DCFG_ETH_GATEWAY_ADDR=ETH_GATEWAY_ADDR
        -DCFG_ETH_SUBNET_MASK=ETH_SUBNET_MASK
    LIBS
        system_config
        os_core_api
        os_libs
        os_network_lib
        os_logger
        os_configuration
        TimeServer_client
)

DeclareCAmkESComponent_NIC_ChanMux(
    NwDriver
    CHANMUX_CHANNEL_NIC_CTRL
    CHANMUX_CHANNEL_NIC_DATA
)

DeclareCAmkESComponent_StorageServer(
    StorageServer
)

DeclareCAmkESComponent_ChanMux_Storage(
    ChanMux_Storage
)

DeclareCAmkESComponents_ChanMux_UART(
    ChanMux
    components/ChanMux/ChanMux_config.c
    system_config
)

DeclareCAmkESComponent(
    Ticker
    SOURCES
        components/Ticker/Ticker.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        os_core_api
        os_libs
)

DeclareCAmkESComponent_TimeServer(
    TimeServer
)

DeclareCAmkESComponent_EntropySource(
    EntropySource
)

DeclareAndCreateCamkESSystem(DemoIotApp.camkes)
GenerateSimulateScript()
