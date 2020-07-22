/**
 * Sensor component that cyclically sends a MQTT message to the CloudConnector.
 *
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "LibDebug/Debug.h"
#include "TimeServer.h"

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

#define SECS_TO_SLEEP   5
#define S_IN_MSEC       1000

OS_ConfigServiceHandle_t serverLibWithFSBackend;

static unsigned char payload[128]; // arbitrary max expected length
static char topic[128];

static OS_Error_t
initializeSensor(void)
{
    static OS_ConfigService_ClientCtx_t ctx =
    {
        .dataport = OS_DATAPORT_ASSIGN(cfg_dataport_buf)
    };
    OS_Error_t err = OS_ConfigService_createHandleRemote(
                         0,
                         &ctx,
                         &serverLibWithFSBackend);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandleRemote() failed with :%d", err);
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
        TimeServer_sleep(TimeServer_PRECISION_SEC, SECS_TO_SLEEP);
    }

    return 0;
}
