/*
 *  Network Stack CAmkES wrapper
 *
 *  Copyright (C) 2020, Hensoldt Cyber GmbH
 *
 */

#include "system_config.h"

#include "LibDebug/Debug.h"
#include "SeosError.h"
#include "seos_api_network_stack.h"
#include "OS_ConfigService.h"
#include "helper_func.h"
#include <camkes.h>


/* Defines -------------------------------------------------------------------*/
#define DATABUFFER_CLIENT       (void *)logServer_dataport_buf


/* Private types -------------------------------------------------------------*/
static OS_LoggerFilter_Handle_t filter;
static OS_LoggerEmitterCallback_Handle_t reg;

// use network stack params configured in config server.
char DEV_ADDR[20];
char GATEWAY_ADDR[20];
char SUBNET_MASK[20];

static seos_network_stack_config_t param_config =
{
    .dev_addr      =   DEV_ADDR,
    .gateway_addr  =   GATEWAY_ADDR,
    .subnet_mask   =   SUBNET_MASK
};

static bool
logServer_init(void)
{
    // Wait until LogServer is ready to process logs.
    logServer_ready_wait();

    // set up registered functions layer
    if (OS_LoggerEmitterCallback_ctor(&reg, logServer_ready_wait,
                                  API_LOG_SERVER_EMIT) == false)
    {
        Debug_LOG_ERROR("Failed to set up registered functions layer");
        return false;
    }

    // set up log filter layer
    if (OS_LoggerFilter_ctor(&filter, Debug_LOG_LEVEL_DEBUG) == false)
    {
        Debug_LOG_ERROR("Failed to set up log filter layer");
        return false;
    }

    OS_LoggerEmitter_getInstance(DATABUFFER_CLIENT, &filter, &reg);

    return true;
}


//------------------------------------------------------------------------------
int run()
{
    // Initialize LogServer connection
    if (logServer_init() == false)
    {
        printf("Failed to init logServer connection!\n\n\n");
    }

    // wait for the init of the admin component to set the config
    admin_system_config_set_wait();

    Debug_LOG_INFO("[NwStack '%s'] starting", get_instance_name());

    // can't make this "static const" or even "static" because the data ports
    // are allocated at runtime
    seos_camkes_network_stack_config_t camkes_config =
    {
        .notify_init_done        = event_network_init_done_emit,
        .wait_loop_event         = event_tick_or_data_wait,

        .internal =
        {
            .notify_loop        = event_internal_emit,

            .notify_write       = e_write_emit,
            .wait_write         = c_write_wait,

            .notify_read        = e_read_emit,
            .wait_read          = c_read_wait,

            .notify_connection  = e_conn_emit,
            .wait_connection    = c_conn_wait,
        },

        .drv_nic =
        {
            .wait_init_done     = event_nic_init_done_wait,

            .from = // NIC -> stack
            {
                .buffer         = port_nic_from,
                .len            = PAGE_SIZE
            },

            .to = // stack -> NIC
            {
                .buffer         = port_nic_to,
                .len            = PAGE_SIZE
            },

            .rpc =
            {
                .dev_write      = nic_driver_tx_data,
                .get_mac        = nic_driver_get_mac,
            }
        },

        .app =
        {
            .notify_init_done   = event_network_init_done_emit,

            .port =
            {
                .buffer         = port_app_io,
                .len            = PAGE_SIZE
            },
        }
    };

    seos_err_t ret;

    // Create a handle to the remote library instance.
    OS_ConfigServiceHandle_t serverLibWithFSBackend;

    ret = OS_ConfigService_createHandle(OS_CONFIG_HANDLE_KIND_RPC,
                                          0,
                                          &serverLibWithFSBackend);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    // Get the needed param values one by one from config server, using below API
    ret = helper_func_getConfigParameter(&serverLibWithFSBackend,
                                         DOMAIN_NWSTACK,
#ifdef SEOS_NWSTACK_AS_CLIENT
                                         ETH_ADDR_CLIENT,
#endif
#ifdef SEOS_NWSTACK_AS_SERVER
                                         ETH_ADDR_SERVER,
#endif
                                         DEV_ADDR,
                                         sizeof(DEV_ADDR));
    if (ret != SEOS_SUCCESS)
    {
#ifdef SEOS_NWSTACK_AS_CLIENT
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        ETH_ADDR_CLIENT, ret);
#endif
#ifdef SEOS_NWSTACK_AS_SERVER
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        ETH_ADDR_SERVER, ret);
#endif
        return ret;
    }
    Debug_LOG_INFO("[NwStack '%s'] IP ADDR: %s", get_instance_name(), DEV_ADDR);

    ret = helper_func_getConfigParameter(&serverLibWithFSBackend,
                                         DOMAIN_NWSTACK,
                                         ETH_GATEWAY_ADDR,
                                         GATEWAY_ADDR,
                                         sizeof(GATEWAY_ADDR));
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        ETH_GATEWAY_ADDR, ret);
        return ret;
    }
    Debug_LOG_INFO("Retrieved TAP 0 GATEWAY ADDR: %s", GATEWAY_ADDR);

    ret = helper_func_getConfigParameter(&serverLibWithFSBackend,
                                         DOMAIN_NWSTACK,
                                         ETH_SUBNET_MASK,
                                         SUBNET_MASK,
                                         sizeof(SUBNET_MASK));
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter for param %s failed with :%d",
                        ETH_SUBNET_MASK, ret);
        return ret;
    }
    Debug_LOG_INFO("Retrieved TAP  0 SUBNETMASK: %s", SUBNET_MASK);


    ret = seos_network_stack_run(&camkes_config, &param_config);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_FATAL("[NwStack '%s'] seos_network_stack_run() failed, error %d",
                        get_instance_name(), ret);
        return ret;
    }


    // actually, seos_network_stack_run() is not supposed to return with
    // SEOS_SUCCESS. We have to assume this is a graceful shutdown for some
    // reason
    Debug_LOG_WARNING("[NwStack '%s'] graceful termination",
                      get_instance_name());

    return 0;
}
