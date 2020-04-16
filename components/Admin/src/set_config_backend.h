/**
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#pragma once

#include <string.h>

#include "LibDebug/Debug.h"
#include "helper_func.h"
#include "OS_ConfigService.h"


typedef struct
{
    char*  config_param_name; /**< pointer to the name of the value in the config server */
    char*  addr;
    size_t len;
}
dev_addr_t;

typedef struct
{
    char*  config_param_name;
    char*  addr;
    size_t len;
}
subnet_mask_t;

typedef struct
{
    char*  config_param_name;
    char*  addr;
    size_t len;
}
gateway_addr_t;

typedef struct
{
    char*  config_param_name;
    char*  device_name;
    size_t len;
}
iot_device_t;

typedef struct
{
    char*  config_param_name;
    char*  addr;
    size_t len;
}
server_addr_t;

typedef struct
{
    char*  config_param_name;
    uint32_t port;
}
server_port_t;

typedef struct
{
    char*  config_param_name;
    char*  content;
    size_t len;
}
server_ca_cert_t;

typedef struct
{
    char*  config_param_name;
    char*  key;
    size_t len;
}
sas_t;

typedef struct
{
    char*  config_param_name;
    char*  hub_name;
    size_t len;
    sas_t  accessKey;
}
iot_hub_t;

typedef struct
{
    char*  config_domain; /**< pointer to the domain in the config server */
    char*  config_param_name_payload;
    char*  payload;
    size_t payload_len;
    char*  config_param_name_topic;
    char*  topic;
    size_t topic_len;
}
mqtt_msg_t;

typedef struct
{
    char* config_domain; /**< pointer to the domain in the config server */
    dev_addr_t client_addr;
    dev_addr_t server_addr;
    gateway_addr_t gateway;
    subnet_mask_t subnet_mask;
} network_stack_config_t;

typedef struct
{
    char* config_domain;
    server_addr_t server_ip;
    server_port_t server_port;
    server_ca_cert_t server_cert;
    iot_hub_t iot_hub;
    iot_device_t iot_device;
}
cloud_server_config_t;

typedef struct
{
    network_stack_config_t network_stack_config;
    cloud_server_config_t  cloud_server_config;
    mqtt_msg_t             mqtt_msg;
}
system_config_t;

seos_err_t
init_system_config(system_config_t* systemConfig);