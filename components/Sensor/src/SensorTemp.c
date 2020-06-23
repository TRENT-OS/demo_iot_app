/**
 * Sensor component that cyclically sends a MQTT message to the CloudConnector.
 *
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "LibDebug/Debug.h"

#include "OS_ConfigService.h"

#include "helper_func.h"

#include "MQTTPacket.h"

#include <string.h>
#include <camkes.h>
#include "time.h"

/* Defines -------------------------------------------------------------------*/
#define SECS_TO_SLEEP   5
#define S_IN_MSEC       1000

OS_ConfigServiceHandle_t serverLibWithFSBackend;

static unsigned char payload[128]; // arbitrary max expected length
static char topic[128];

static OS_Error_t
initializeSensor(void)
{
    OS_Error_t err = OS_ConfigService_createHandle(
                         OS_CONFIG_HANDLE_KIND_RPC,
                         0,
                         &serverLibWithFSBackend);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle() failed with :%d", err);
        return err;
    }

    return OS_SUCCESS;
}

static OS_Error_t
CloudConnector_write(unsigned char* msg, void* dataPort, size_t len)
{
    memcpy(dataPort, msg, len);
    OS_Error_t err = cloudConnector_interface_write();
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

    ret = helper_func_getConfigParameter(&serverLibWithFSBackend,
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


    ret = helper_func_getConfigParameter(&serverLibWithFSBackend,
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
                                    0,
                                    0,
                                    1,
                                    mqttTopic,
                                    (unsigned char*)payload,
                                    strlen((const char*)payload));


    for (;;)
    {
        CloudConnector_write(serializedMsg, (void*)cloudConnectorDataPort,
                             len);
        api_time_server_sleep(SECS_TO_SLEEP * S_IN_MSEC);
    }

    return 0;
}