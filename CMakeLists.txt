#
# IoT Demo
#
# Copyright (C) 2019-2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

os_set_config_file(system_config.h)

add_subdirectory("include/mqtt")


#-------------------------------------------------------------------------------
project(system_config C)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})


#-------------------------------------------------------------------------------
project(demo_iot_app C)

DeclareCAmkESComponent(
    PartitionManager
    SOURCES
        components/PartitionManager/src/PartitionManager.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_PARTITION_MANAGER_BUILD_AS_COMPONENT
    LIBS
        system_config
        os_libs
        os_partition_manager
        os_logger
        proxy_nvm
)

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
        #-DCONFIG_SERVER_BACKEND_MEMORY
        -DOS_FS_BUILD_AS_LIB_BASIC_HANDLE
        -DOS_PARTITION_MANAGER_BUILD_AS_COMPONENT
        -DOS_CONFIG_SERVICE_CAMKES_SERVER
    LIBS
        system_config
        os_libs
        os_core_api
        os_configuration
        os_partition_manager_api
        os_filesystem_core
        os_filesystem_fat
        os_filesystem_spiffs
        os_logger
)

DeclareCAmkESComponent(
    LogServer
    SOURCES
        components/LogServer/src/LogServer.c
    C_FLAGS
        -Wall
        -Werror
        -DOS_FS_BUILD_AS_LIB_BASIC_HANDLE
        -DOS_PARTITION_MANAGER_BUILD_AS_COMPONENT
    LIBS
        system_config
        os_core_api
        os_libs
        os_log_server_backend_filesystem
        os_partition_manager_api
        os_filesystem_core
        os_filesystem_fat
        os_filesystem_spiffs
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
)

DeclareCAmkESComponent_ChanMux(
    ChanMux
    components/ChanMux/ChanMux.c
    system_config
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
)

DeclareCAmkESComponent_NIC_ChanMux(
    NwDriver
    CHANMUX_CHANNEL_NIC_CTRL
    CHANMUX_CHANNEL_NIC_DATA
)

DeclareCAmkESComponent_UART(
    UART
)

DeclareCAmkESComponent(
    TimerClient
    SOURCES
        components/TimerClient/src/Timer_Client.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        os_core_api
        os_libs
)

DeclareCAmkESComponent(
    TimeServer
    INCLUDES
        components/TimeServer/include
        components/TimeServer/timestamp
    SOURCES
        components/TimeServer/src/main.c
        components/TimeServer/src/time_server.c
        components/TimeServer/timestamp/timestamp.c
    C_FLAGS
        -Wall
        -Werror
)

DeclareCAmkESComponent_EntropySource(
    EntropySource
)

DeclareAndCreateCamkESSystem(DemoIotApp.camkes)
GenerateSimulateScript()
