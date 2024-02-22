/*
 * MQTT client based on PAHO
 *
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#if defined(MQTTCLIENT_PLATFORM_HEADER)
#define XSTR(s) STR(s)
#define STR(s) #s
#include XSTR(MQTTCLIENT_PLATFORM_HEADER)
#endif

#include "MQTT_net.h"
#include <stddef.h>

#include "MQTTPacket.h"

#define NUM_MQTT_CLIENT_MESSAGE_HANDLERS    1


typedef struct
{
    unsigned char qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void* payload;
    size_t payloadlen;
} MQTT_message_t;


typedef struct
{
    MQTT_message_t* message;
    MQTTString* topicName;
} MQTT_messageData_t;


typedef struct MQTTConnackData
{
    unsigned char rc;
    unsigned char sessionPresent;
} MQTT_connackData_t;


typedef struct
{
    Network* net;
    unsigned char* sendbuf;
    size_t sendbuf_size;
    unsigned char* readbuf;
    size_t readbuf_size;

    unsigned int send_timeout_ms;
    unsigned int keepAliveInterval_ms;
    unsigned int nextPacketId;
    int isPingOutstanding;
    int isConnected;
    Timer timerLastSend;
} MQTT_client_t;


void MQTT_client_init(
    MQTT_client_t* self,
    Network* net,
    unsigned int send_timeout_ms,
    void* sendbuf,
    size_t sendbuf_size,
    void* readbuf,
    size_t readbuf_size
);


int MQTT_client_connect(
    MQTT_client_t* self,
    MQTTPacket_connectData* options,
    MQTT_connackData_t* data,
    Timer* timer
);

int MQTT_client_publish(
    MQTT_client_t* self,
    const char* topic,
    MQTT_message_t* msg,
    Timer* timer
);

void MQTT_client_disconnect(
    MQTT_client_t* self);
