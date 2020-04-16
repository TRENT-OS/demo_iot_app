cmake_minimum_required(VERSION 3.7.2)

seos_set_config_file(system_config.h)

project(seos_system_config C)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/include/seos_mqtt")

seos_use_libs(
    SEOS_LIBS
    SEOS_CHANMUX
    SEOS_NETWORK_DRIVER_TAP_PROXY
    SEOS_CRYPTO
    SEOS_CRYPTO_IMPL_MBEDTLS
    SEOS_NW_STACK
    SEOS_PARTITION_MANAGER
    SEOS_FILESYSTEM_CORE
    SEOS_FILESYSTEM_FAT
    SEOS_FILESYSTEM_SPIFFS
    SEOS_CONFIGURATION
    SEOS_TLS
    SEOS_TLS_IMPL_MBEDTLS
    SEOS_LOGGER
)


project(demo_iot_app C)


#-------------------------------------------------------------------------------
# Component Admin
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(Admin
    INCLUDES
        include/util
    SOURCES
        components/Admin/src/Admin.c
        components/Admin/src/set_config_backend.c
        include/util/helper_func.c
    C_FLAGS
        -Wall -Werror
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT     # indicates a camkes configuration client is being built
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
        seos_configuration
        seos_logger
)

#-------------------------------------------------------------------------------
# Component PartitionManager
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(PartitionManager
    SOURCES
        components/PartitionManager/src/PartitionManager.c
    C_FLAGS
        -Wall
        -Werror
        -DSEOS_PARTITION_MANAGER_BUILD_AS_COMPONENT
    LIBS
        seos_system_config
        seos_libs
        seos_partition_manager
        seos_logger
        chanmux_client
        proxy_nvm
)

#-------------------------------------------------------------------------------
# Component SensorTemp
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(SensorTemp
    INCLUDES
        include/util
    SOURCES
        components/Sensor/src/SensorTemp.c
        include/util/helper_func.c
    C_FLAGS
        -Wall -Werror
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT    # indicates a camkes configuration server will be used
    LIBS
        seos_system_config
        seos_libs
        seos_core_api
        seos_network_api
        seos_configuration
        seos_logger
        seos_mqtt
)

#-------------------------------------------------------------------------------
# Component ConfigServer
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(ConfigServer
    SOURCES
        components/ConfigServer/src/ConfigServer.c
        components/ConfigServer/src/create_config_backend.c
    C_FLAGS
        -Wall
        -Werror
        # filesystem
        #-DOS_CONFIG_SERVICE_BACKEND_FILESYSTEM
        -DOS_CONFIG_SERVICE_BACKEND_MEMORY
        -DSEOS_FS_BUILD_AS_LIB_BASIC_HANDLE
        # partition manager
        -DSEOS_PARTITION_MANAGER_BUILD_AS_COMPONENT
        # config server
        -DOS_CONFIG_SERVICE_CAMKES_SERVER     # indicates a camkes configuration server is being built
    LIBS
        seos_system_config
        seos_libs
        seos_core_api
        seos_configuration
        seos_partition_manager_api
        seos_filesystem_core
        seos_filesystem_fat
        seos_filesystem_spiffs
        seos_logger
)

#-------------------------------------------------------------------------------
# Component LogServer
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(LogServer
    SOURCES
        components/LogServer/src/LogServer.c
    C_FLAGS
        -Wall
        -Werror
        # filesystem
        -DSEOS_FS_BUILD_AS_LIB_BASIC_HANDLE
        # partition manager
        -DSEOS_PARTITION_MANAGER_BUILD_AS_COMPONENT
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
        seos_log_server_backend_filesystem
        seos_partition_manager_api
        seos_filesystem_core
        seos_filesystem_fat
        seos_filesystem_spiffs
)

#-------------------------------------------------------------------------------
# Component CloudConnector
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(CloudConnector
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
        include/util/helper_func.c
        ${SEOS_LIBS_DIR}/seos_nwstack/network-client-lib/seos_nw_api_interface.c
    C_FLAGS
        -Wall -Werror
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
    LIBS
        seos_core_api
        seos_libs
        seos_crypto
        seos_mqtt
        seos_tls
        seos_configuration
        seos_logger
)

#-------------------------------------------------------------------------------
# Component ChanMux
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(ChanMux
    SOURCES
        components/ChanMux/ChanMux.c
    C_FLAGS
        -Wall -Werror
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
        chanmux_server
)

#-------------------------------------------------------------------------------
# Component NwStack
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(NwStack
    INCLUDES
        include/util
    SOURCES
        components/NwStack/network_stack.c
        include/util/helper_func.c
    C_FLAGS
        -Wall -Werror
        -DSEOS_NWSTACK_AS_CLIENT
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
        -DCFG_ETH_ADDR=ETH_ADDR
        -DCFG_ETH_GATEWAY_ADDR=ETH_GATEWAY_ADDR
        -DCFG_ETH_SUBNET_MASK=ETH_SUBNET_MASK
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
        seos_nw_lib
        seos_logger
        seos_configuration
)

#-------------------------------------------------------------------------------
# Component NwDriver
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(NwDriver
    SOURCES
        components/NwDriver/nic_driver.c
    C_FLAGS
        -Wall -Werror
        -DCFG_CHANMUX_CHANNEL_CRTL=CHANMUX_CHANNEL_NIC_CTRL
        -DCFG_CHANMUX_CHANNEL_DATA=CHANMUX_CHANNEL_NIC_DATA
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
        seos_nw_driver_tap_proxy_lib
        seos_logger
        chanmux_client
)

#-------------------------------------------------------------------------------
# Component NwStack 2
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(NwStack_2
    INCLUDES
        include/util
    SOURCES
        components/NwStack/network_stack.c
        include/util/helper_func.c
    C_FLAGS
        -Wall -Werror
        -DSEOS_NWSTACK_AS_SERVER
        -DOS_CONFIG_SERVICE_CAMKES_CLIENT
        -DCFG_ETH_ADDR=ETH_ADDR
        -DCFG_ETH_GATEWAY_ADDR=ETH_GATEWAY_ADDR
        -DCFG_ETH_SUBNET_MASK=ETH_SUBNET_MASK
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
        seos_nw_lib
        seos_logger
        seos_configuration
)

#-------------------------------------------------------------------------------
# Component NwDriver 2
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(NwDriver_2
    SOURCES
        components/NwDriver/nic_driver.c
    C_FLAGS
        -DCFG_CHANMUX_CHANNEL_CRTL=CHANMUX_CHANNEL_NIC_2_CTRL
        -DCFG_CHANMUX_CHANNEL_DATA=CHANMUX_CHANNEL_NIC_2_DATA
        -Wall -Werror
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
        chanmux_client
        seos_nw_driver_tap_proxy_lib
)

#-------------------------------------------------------------------------------
# Component UART
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(UartDrv
    SOURCES
        components/UART/src/UartDrv.c
        components/UART/src/qemu_uart.c
    C_FLAGS
        -Wall -Werror
)

#-------------------------------------------------------------------------------
# Component TimerClient
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(TimerClient
    SOURCES
        components/TimerClient/src/Timer_Client.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        seos_system_config
        seos_core_api
        seos_libs
)

#-------------------------------------------------------------------------------
# Component TimeServer
#-------------------------------------------------------------------------------
DeclareCAmkESComponent(TimeServer
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


DeclareCAmkESRootserver(DemoIotApp.camkes)
GenerateCAmkESRootServer()
GenerateSimulateScript()
