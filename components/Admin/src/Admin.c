/*
   *  Admin component that writes the system config file based on the set
   *  parameters.
   *
   *  Copyright (C) 2020, Hensoldt Cyber GmbH
*/


#include <camkes.h>

#include "LibDebug/Debug.h"

#include "set_config_backend.h"

/* Defines -------------------------------------------------------------------*/
#define DATABUFFER_CLIENT       (void *)logServer_dataport_buf


/* Private types -------------------------------------------------------------*/
static OS_LoggerFilter_Handle_t filter;
static OS_LoggerEmitterCallback_Handle_t reg;

static system_config_t systemConfig =
{
    .network_stack_config = {
        .config_domain = DOMAIN_NWSTACK,
        .client_addr = {
            .config_param_name = ETH_ADDR_CLIENT,
            .addr = ETH_ADDR_CLIENT_VALUE,
            .len = sizeof(ETH_ADDR_CLIENT_VALUE)
        },
        .server_addr = {
            .config_param_name = ETH_ADDR_SERVER,
            .addr = ETH_ADDR_SERVER_VALUE,
            .len = sizeof(ETH_ADDR_SERVER_VALUE)
        },
        .gateway = {
            .config_param_name = ETH_GATEWAY_ADDR,
            .addr = ETH_GATEWAY_ADDR_VALUE,
            .len = sizeof(ETH_GATEWAY_ADDR_VALUE)
        },
        .subnet_mask = {
            .config_param_name = ETH_SUBNET_MASK,
            .addr = ETH_SUBNET_MASK_VALUE,
            .len = sizeof(ETH_SUBNET_MASK_VALUE)
        }
    },
    .cloud_server_config = {
        .config_domain = DOMAIN_CLOUDCONNECTOR,
        .server_ip = {
            .config_param_name = SERVER_ADDRESS_NAME,
            .addr = SERVER_ADDRESS_VALUE,
            .len = sizeof(SERVER_ADDRESS_VALUE)
        },
        .server_port = {
            .config_param_name = SERVER_PORT_NAME,
            .port = SERVER_PORT_VALUE,
        },
        .server_cert = {
            .config_param_name = SERVER_CA_CERT_NAME,
            .content = SERVER_CA_CERT_PEM_VALUE,
            .len     = sizeof(SERVER_CA_CERT_PEM_VALUE)
        },
        .iot_hub = {
            .config_param_name = CLOUD_DOMAIN_NAME,
            .hub_name = CLOUD_DOMAIN_VALUE,
            .len  = sizeof(CLOUD_DOMAIN_VALUE),
            .accessKey = {
                .config_param_name = CLOUD_SAS_NAME,
                .key = CLOUD_SAS_VALUE,
                .len = sizeof(CLOUD_SAS_VALUE)
            }
        },
        .iot_device = {
            .config_param_name = CLOUD_DEVICE_ID_NAME,
            .device_name = CLOUD_DEVICE_ID_VALUE,
            .len  = sizeof(CLOUD_DEVICE_ID_VALUE)
        }
    },
    .mqtt_msg = {
        .config_domain             = DOMAIN_SENSOR,
        .config_param_name_payload = MQTT_PAYLOAD_NAME,
        .payload                   = MQTT_PAYLOAD_VALUE,
        .payload_len               = sizeof(MQTT_PAYLOAD_VALUE),
        .config_param_name_topic   = MQTT_TOPIC_NAME,
        .topic                     = MQTT_TOPIC_VALUE,
        .topic_len                 = sizeof(MQTT_TOPIC_VALUE),
    }
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


int run(void)
{
    if (logServer_init() == false)
    {
        printf("Failed to init logServer connection!\n");
        return -1;
    }

    // wait for the ConfigServer to prepare the configuration backend
    config_backend_ready_wait();

    Debug_LOG_INFO("Setting system configuration...");
    seos_err_t err = init_system_config(&systemConfig);
    if (err != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("create_system_config_backend failed with:%d", err);
        return -1;
    }

    Debug_LOG_INFO("System configuration set.");

    // notify apps that configuration can now be retrieved
    system_config_set_emit();

    return 0;
}
