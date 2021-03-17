/**
 * Sensor component that cyclically sends a MQTT message to the CloudConnector.
 *
 * Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#include "lib_debug/Debug.h"

#include "OS_ConfigService.h"

#include "helper_func.h"

#include "MQTTPacket.h"

#include <string.h>
#include <camkes.h>
#include "time.h"

/* Defines -------------------------------------------------------------------*/
// the following defines are the parameter names that need to match the settings
// in the configuration xml file. These will be passed to the configServer
// component to retrieve the settings for the specified parameter
#define DOMAIN_SENSOR           "Domain-Sensor"
#define MQTT_PAYLOAD_NAME       "MQTT_Payload" // _NAME defines are stored together with the values in the config file
#define MQTT_TOPIC_NAME         "MQTT_Topic"

// send a new message to the cloudConnector every five seconds
#define SEC_TO_SLEEP   5

OS_ConfigServiceHandle_t hConfig;

static unsigned char payload[128]; // arbitrary max expected length
static char topic[128];

static OS_Error_t
initializeSensor(void)
{
    static OS_ConfigService_ClientCtx_t ctx =
    {
        .dataport = OS_DATAPORT_ASSIGN(configServer_port)
    };
    OS_Error_t err = OS_ConfigService_createHandleRemote(
                         &ctx,
                         &hConfig);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandleRemote() failed with :%d", err);
        return err;
    }

    // set up a tick with the local timer ID 1. The local timer ID 0 is used for
    // the sleep() function of the TimeServer
    int ret = timeServer_rpc_periodic(1, (NS_IN_S*SEC_TO_SLEEP));
    if (0 != ret)
    {
        Debug_LOG_ERROR("timeServer_rpc_periodic() failed, code %d", ret);
        return -1;
    }

    return OS_SUCCESS;
}

static OS_Error_t
CloudConnector_write(unsigned char* msg, void* dataPort, size_t len)
{
    memcpy(dataPort, msg, len);
    OS_Error_t err = cloudConnector_rpc_write();
    return err;
}


int run()
{
    OS_Error_t ret = initializeSensor();
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeSensor() failed with:%d", ret);
        return ret;
    }

    Debug_LOG_INFO("Starting TemperatureSensor...");

    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_SENSOR,
                                         MQTT_PAYLOAD_NAME,
                                         &payload,
                                         sizeof(payload));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        MQTT_PAYLOAD_NAME, ret);
        return ret;
    }
    Debug_LOG_INFO("Retrieved MQTT Payload: %s", payload);


    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_SENSOR,
                                         MQTT_TOPIC_NAME,
                                         &topic,
                                         sizeof(topic));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        MQTT_TOPIC_NAME, ret);
        return ret;
    }

    MQTTString mqttTopic = MQTTString_initializer;
    mqttTopic.cstring = topic;
    Debug_LOG_INFO("Retrieved MQTT Topic: %s", mqttTopic.cstring);

    unsigned char serializedMsg[320]; //arbitrary size
    int len = MQTTSerialize_publish(serializedMsg,
                                    sizeof(serializedMsg),
                                    0,
                                    1,
                                    0,
                                    1,
                                    mqttTopic,
                                    (unsigned char*)payload,
                                    strlen((const char*)payload));

    for (;;)
    {
        CloudConnector_write(serializedMsg, (void*)cloudConnector_port,
                             len);

        timeServer_notify_wait();
    }

    return 0;
}
