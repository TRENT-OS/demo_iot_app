/*
 *  MQTT server based on PAHO
 *
 *  Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#pragma once

#if defined(MQTTCLIENT_PLATFORM_HEADER)
#define XSTR(s) STR(s)
#define STR(s) #s
#include XSTR(MQTTCLIENT_PLATFORM_HEADER)
#endif

#include <stddef.h>

typedef struct
{
    unsigned char* sendbuf;
    size_t sendbuf_size;

    unsigned char* readbuf;
    size_t readbuf_size;

    Network* net;
    unsigned int socket_timeout_ms;
}
MQTTServer;

void MQTTServerInit(MQTTServer* self,
                    Network* network,
                    unsigned int socket_timeout_ms,
                    void* sendbuf,
                    size_t sendbuf_size,
                    void* readbuf,
                    size_t readbuf_size);

int MQTTServer_readPacket(MQTTServer* self,
                          Timer* timer);


int MQTTServer_readType(MQTTServer* self);

int MQTTServer_sendConnAck(MQTTServer* self,
                           unsigned char connack_rc,
                           unsigned char sessionPresent);

int MQTTServer_sendPingRes(MQTTServer* self);
