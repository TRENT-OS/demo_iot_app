/**
 * Sensor component that cyclically sends a MQTT message to the CloudConnector.
 *
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "LibDebug/Debug.h"

#include "OS_ConfigService.h"
#include "OS_Network.h"

#include "helper_func.h"

#include "MQTTPacket.h"

#include <string.h>
#include <camkes.h>
#include "time.h"

/* Defines -------------------------------------------------------------------*/
#define SECS_TO_SLEEP   5
#define S_IN_MSEC       1000

#define DATABUFFER_CLIENT       (void *)logServer_dataport_buf

/* External ressources -------------------------------------------------------*/
extern seos_err_t OS_NetworkAPP_RT(OS_Network_Context_t ctx);

/* Instance variables --------------------------------------------------------*/
static OS_LoggerFilter_Handle_t filter;
static OS_LoggerEmitterCallback_Handle_t reg;

OS_ConfigServiceHandle_t serverLibWithFSBackend;
#if 0
static unsigned char payload[128]; // arbitrary max expected length
static char topic[128];
#endif
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

static seos_err_t
initializeSensor(void)
{
    seos_err_t err = SEOS_SUCCESS;

    if (logServer_init() == false)
    {
        printf("Failed to init logServer connection!\n\n\n");
    }

    err = OS_ConfigService_createHandle(
              OS_CONFIG_HANDLE_KIND_RPC,
              0,
              &serverLibWithFSBackend);
    if (err != SEOS_SUCCESS)
    {
        return err;
    }

    return SEOS_SUCCESS;
}
#if 0
static seos_err_t
CloudConnector_write(unsigned char* msg, void* dataPort, size_t len)
{
    memcpy(dataPort, msg, len);
    seos_err_t err = cloudConnector_interface_write();
    return err;
}
#endif

int run()
{
    seos_err_t ret = SEOS_SUCCESS;

    ret = initializeSensor();
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeSensor() failed with:%d", ret);
        return ret;
    }

    // wait for the init of the admin component
    admin_system_config_set_wait();

    Debug_LOG_INFO("Starting TemperatureSensor...");

    char buffer[4096];
    OS_NetworkAPP_RT(NULL);    /* Must be actually called by SEOS Runtime */

    OS_NetworkServer_Socket_t  srv_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = 5555,
        .backlog = 1,
    };

    /* Gets filled when accept is called */
    OS_NetworkSocket_Handle_t  seos_socket_handle ;
    /* Gets filled when server socket create is called */
    OS_NetworkServer_Handle_t  seos_nw_server_handle ;

    seos_err_t err = OS_NetworkServerSocket_create(NULL, &srv_socket,
                                                   &seos_nw_server_handle);

    if (err != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("server_socket_create() failed, code %d", err);
        return -1;
    }

#if 0
    ret = helper_func_getConfigParameter(&serverLibWithFSBackend,
                                         DOMAIN_SENSOR,
                                         MQTT_PAYLOAD_NAME,
                                         &payload,
                                         sizeof(payload));
    if (ret != SEOS_SUCCESS)
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
    if (ret != SEOS_SUCCESS)
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
#endif
    Debug_LOG_INFO("launching echo server");

    for (;;)
    {
        err = OS_NetworkServerSocket_accept(seos_nw_server_handle, &seos_socket_handle);
        if (err != SEOS_SUCCESS)
        {
            Debug_LOG_ERROR("socket_accept() failed, error %d", err);
            return -1;
        }

        /*
            As of now the nw stack behavior is as below:
            Keep reading data until you receive one of the return values:
             a. err = SEOS_ERROR_CONNECTION_CLOSED and length = 0 indicating end of data read
                      and connection close
             b. err = SEOS_ERROR_GENERIC  due to error in read
             c. err = SEOS_SUCCESS and length = 0 indicating no data to read but there is still
                      connection
             d. err = SEOS_SUCCESS and length >0 , valid data

            Take appropriate actions based on the return value rxd.


            Only a single socket is supported and no multithreading !!!
            Once we accept an incoming connection, start reading data from the client and echo back
            the data rxd.
        */
        memset(buffer, 0, sizeof(buffer));

        Debug_LOG_INFO("starting server read loop");
        /* Keep calling read until we receive CONNECTION_CLOSED from the stack */
        for (;;)
        {
            Debug_LOG_DEBUG("read...");
            size_t n = 1;
            err = OS_NetworkSocket_read(seos_socket_handle, buffer, &n);
            if (SEOS_SUCCESS != err)
            {
                Debug_LOG_ERROR("socket_read() failed, error %d", err);
                break;
            }

            Debug_ASSERT(n == 1);
            Debug_LOG_DEBUG("Got a byte %02x, send it back", buffer[0]);

            err = OS_NetworkSocket_write(seos_socket_handle, buffer, &n);
            if (err != SEOS_SUCCESS)
            {
                Debug_LOG_ERROR("socket_write() failed, error %d", err);
                break;
            }
        }

        switch (err)
        {
        /* This means end of read as socket was closed. Exit now and close handle*/
        case SEOS_ERROR_CONNECTION_CLOSED:
            // the test runner checks for this string
            Debug_LOG_INFO("connection closed by server");
            break;
        /* Any other value is a failure in read, hence exit and close handle  */
        default :
            Debug_LOG_ERROR("server socket failure, error %d", err);
            break;
        } //end of switch
    }
    return -1;
#if 0
    for (;;)
    {
        CloudConnector_write(serializedMsg, (void*)cloudConnectorDataPort,
                             len);
        api_time_server_sleep(SECS_TO_SLEEP * S_IN_MSEC);
    }
#endif
    sensor_init_emit();
    if (ret != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("sensor_init_emit() failed with: %d",
                        ret);
        return 0;
    }

    return 0;
}