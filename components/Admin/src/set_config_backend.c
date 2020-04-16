/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "set_config_backend.h"


/* Defines -------------------------------------------------------------------*/
#define PARTITION_ID            0

#define PARAMETER_FILE "PARAM.BIN"
#define DOMAIN_FILE "DOMAIN.BIN"
#define STRING_FILE "STRING.BIN"
#define BLOB_FILE "BLOB.BIN"

/* ---------------------------------------------------------------------------*/
seos_err_t
init_system_config(system_config_t* systemConfig)
{

    OS_ConfigServiceHandle_t serverLibWithFSBackend;

    // Create a handle to the remote library instance.

    seos_err_t ret = OS_ConfigService_createHandle(OS_CONFIG_HANDLE_KIND_RPC,
                                                   0,
                                                   &serverLibWithFSBackend);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s", systemConfig->mqtt_msg.config_param_name_payload);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->mqtt_msg.config_domain,
                                         systemConfig->mqtt_msg.config_param_name_payload,
                                         systemConfig->mqtt_msg.payload,
                                         systemConfig->mqtt_msg.payload_len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s", systemConfig->mqtt_msg.config_param_name_topic);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->mqtt_msg.config_domain,
                                         systemConfig->mqtt_msg.config_param_name_topic,
                                         systemConfig->mqtt_msg.topic,
                                         systemConfig->mqtt_msg.topic_len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->cloud_server_config.iot_hub.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->cloud_server_config.config_domain,
                                         systemConfig->cloud_server_config.iot_hub.config_param_name,
                                         systemConfig->cloud_server_config.iot_hub.hub_name,
                                         systemConfig->cloud_server_config.iot_hub.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->cloud_server_config.server_ip.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->cloud_server_config.config_domain,
                                         systemConfig->cloud_server_config.server_ip.config_param_name,
                                         systemConfig->cloud_server_config.server_ip.addr,
                                         systemConfig->cloud_server_config.server_ip.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->cloud_server_config.iot_hub.accessKey.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->cloud_server_config.config_domain,
                                         systemConfig->cloud_server_config.iot_hub.accessKey.config_param_name,
                                         systemConfig->cloud_server_config.iot_hub.accessKey.key,
                                         systemConfig->cloud_server_config.iot_hub.accessKey.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->cloud_server_config.server_port.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->cloud_server_config.config_domain,
                                         systemConfig->cloud_server_config.server_port.config_param_name,
                                         &systemConfig->cloud_server_config.server_port.port,
                                         sizeof(systemConfig->cloud_server_config.server_port.port));
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->cloud_server_config.iot_device.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->cloud_server_config.config_domain,
                                         systemConfig->cloud_server_config.iot_device.config_param_name,
                                         systemConfig->cloud_server_config.iot_device.device_name,
                                         systemConfig->cloud_server_config.iot_device.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->cloud_server_config.server_cert.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->cloud_server_config.config_domain,
                                         systemConfig->cloud_server_config.server_cert.config_param_name,
                                         systemConfig->cloud_server_config.server_cert.content,
                                         systemConfig->cloud_server_config.server_cert.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->network_stack_config.client_addr.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->network_stack_config.config_domain,
                                         systemConfig->network_stack_config.client_addr.config_param_name,
                                         systemConfig->network_stack_config.client_addr.addr,
                                         systemConfig->network_stack_config.client_addr.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->network_stack_config.server_addr.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->network_stack_config.config_domain,
                                         systemConfig->network_stack_config.server_addr.config_param_name,
                                         systemConfig->network_stack_config.server_addr.addr,
                                         systemConfig->network_stack_config.server_addr.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->network_stack_config.gateway.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->network_stack_config.config_domain,
                                         systemConfig->network_stack_config.gateway.config_param_name,
                                         systemConfig->network_stack_config.gateway.addr,
                                         systemConfig->network_stack_config.gateway.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting %s",
                    systemConfig->network_stack_config.subnet_mask.config_param_name);
    ret = helper_func_setConfigParameter(&serverLibWithFSBackend,
                                         systemConfig->network_stack_config.config_domain,
                                         systemConfig->network_stack_config.subnet_mask.config_param_name,
                                         systemConfig->network_stack_config.subnet_mask.addr,
                                         systemConfig->network_stack_config.subnet_mask.len);
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle failed with :%d", ret);
        return ret;
    }

    return SEOS_SUCCESS;
}