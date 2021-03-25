/*
 *  Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 *
 */

#include "system_config.h"

#include "lib_debug/Debug.h"

#include "OS_Error.h"
#include "OS_NetworkStack.h"
#include "TimeServer.h"
#include "OS_ConfigService.h"
#include "OS_Dataport.h"
#include "helper_func.h"
#include <camkes.h>

/* Defines -------------------------------------------------------------------*/
// the following defines are the parameter names that need to match the settings
// in the configuration xml file. These will be passed to the configServer
// component to retrieve the settings for the specified parameter
#define DOMAIN_NWSTACK          "Domain-NwStack"
#define ETH_ADDR                "ETH_ADDR"
#define ETH_GATEWAY_ADDR        "ETH_GATEWAY_ADDR"
#define ETH_SUBNET_MASK         "ETH_SUBNET_MASK"

// use network stack params configured in config server.
char DEV_ADDR[20];
char GATEWAY_ADDR[20];
char SUBNET_MASK[20];

static const if_OS_Timer_t timer =
    IF_OS_TIMER_ASSIGN(
        timeServer_rpc,
        timeServer_notify);

static const OS_NetworkStack_AddressConfig_t config =
{
    .dev_addr      = DEV_ADDR,
    .gateway_addr  = GATEWAY_ADDR,
    .subnet_mask   = SUBNET_MASK
};

static bool initSuccessfullyCompleted = false;

//------------------------------------------------------------------------------
static
OS_Error_t
read_ip_from_config_server(void)
{
    OS_Error_t ret;
    // Create a handle to the remote library instance.
    OS_ConfigServiceHandle_t hConfig;

    static OS_ConfigService_ClientCtx_t ctx =
    {
        .dataport = OS_DATAPORT_ASSIGN(configServer_port)
    };
    ret = OS_ConfigService_createHandleRemote(
              &ctx,
              &hConfig);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandleRemote() failed with :%d", ret);
        return ret;
    }

    // Get the needed param values one by one from config server, using below API
    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_NWSTACK,
                                         CFG_ETH_ADDR,
                                         DEV_ADDR,
                                         sizeof(DEV_ADDR));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        CFG_ETH_ADDR, ret);
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] IP ADDR: %s", get_instance_name(), DEV_ADDR);

    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_NWSTACK,
                                         CFG_ETH_GATEWAY_ADDR,
                                         GATEWAY_ADDR,
                                         sizeof(GATEWAY_ADDR));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        CFG_ETH_GATEWAY_ADDR, ret);
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] GATEWAY ADDR: %s", get_instance_name(),
                   GATEWAY_ADDR);

    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_NWSTACK,
                                         CFG_ETH_SUBNET_MASK,
                                         SUBNET_MASK,
                                         sizeof(SUBNET_MASK));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        CFG_ETH_SUBNET_MASK, ret);
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] SUBNETMASK: %s", get_instance_name(),
                   SUBNET_MASK);

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
// network stack's PicTCP OS adaption layer calls this.
uint64_t
Timer_getTimeMs(void)
{
    OS_Error_t err;
    uint64_t ms;

    if ((err = TimeServer_getTime(&timer, TimeServer_PRECISION_MSEC,
                                  &ms)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("TimeServer_getTime() failed with %d", err);
        ms = 0;
    }

    return ms;
}

//------------------------------------------------------------------------------
void
post_init(void)
{
    Debug_LOG_INFO("[NwStack '%s'] starting", get_instance_name());

    static OS_NetworkStack_SocketResources_t socks =
    {
        .notify_write       = e_write_emit,
        .wait_write         = c_write_wait,

        .notify_read        = e_read_emit,
        .wait_read          = c_read_wait,

        .notify_connection  = e_conn_emit,
        .wait_connection    = c_conn_wait,

        .buf = OS_DATAPORT_ASSIGN(network_stack_port)
    };

    static const OS_NetworkStack_CamkesConfig_t camkes_config =
    {
        .wait_loop_event         = event_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = event_internal_emit,

            .allocator_lock     = allocatorMutex_lock,
            .allocator_unlock   = allocatorMutex_unlock,

            .nwStack_lock       = nwstackMutex_lock,
            .nwStack_unlock     = nwstackMutex_unlock,

            .socketCB_lock      = socketControlBlockMutex_lock,
            .socketCB_unlock    = socketControlBlockMutex_unlock,

            .stackTS_lock       = stackThreadSafeMutex_lock,
            .stackTS_unlock     = stackThreadSafeMutex_unlock,

            .number_of_sockets  = OS_NETWORK_MAXIMUM_SOCKET_NO,
            .sockets            = &socks
        },

        .drv_nic =
        {
            // NIC -> Stack
            .from =
            {
                .io = (void**)( &(nic_port_from)),
                .size = NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS
            },
            // Stack -> NIC
            .to = OS_DATAPORT_ASSIGN(nic_port_to),

            .rpc =
            {
                .dev_read       = nic_driver_rx_data,
                .dev_write      = nic_driver_tx_data,
                .get_mac        = nic_driver_get_mac_address,
            }
        }
    };

    OS_Error_t ret;

    ret = read_ip_from_config_server();
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] Read from config failed, error %d",
                        get_instance_name(), ret);
        return;
    }

    ret = OS_NetworkStack_init(&camkes_config, &config);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] OS_NetworkStack_init() failed, error %d",
                        get_instance_name(), ret);
        return;
    }
    initSuccessfullyCompleted = true;
}

//------------------------------------------------------------------------------
int
run(void)
{
    if (!initSuccessfullyCompleted)
    {
        Debug_LOG_FATAL("[NwStack '%s'] initialization failed",
                        get_instance_name());
        return -1;
    }

    OS_Error_t ret = OS_NetworkStack_run();
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] OS_NetworkStack_run() failed, error %d",
                        get_instance_name(), ret);
        return -1;
    }

    // actually, OS_NetworkStack_run() is not supposed to return with
    // OS_SUCCESS. We have to assume this is a graceful shutdown for some
    // reason
    Debug_LOG_WARNING("[NwStack '%s'] graceful termination",
                      get_instance_name());

    return 0;
}
