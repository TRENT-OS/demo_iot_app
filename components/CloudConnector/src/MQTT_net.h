/*
 *  MQTT Network helper functions
 *
 *  Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#pragma once

#if defined(MQTTCLIENT_PLATFORM_HEADER)
#define XSTR(s) STR(s)
#define STR(s) #s
#include XSTR(MQTTCLIENT_PLATFORM_HEADER)
#endif

// all failure return codes must be negative
enum
{
    MQTT_TIMEOUT = -3,
    MQTT_BUFFER_OVERFLOW = -2,
    MQTT_FAILURE = -1,
    MQTT_SUCCESS = 0
};


int MQTT_network_read(
    Network* n,
    unsigned char* buffer,
    int len,
    Timer* timer
);

int MQTT_network_write(
    Network* n,
    const unsigned char* buffer,
    int len,
    Timer* timer
);

int MQTT_network_sendPacket(
    Network* n,
    const unsigned char* buffer,
    unsigned int length,
    Timer* timer
);

int MQTT_network_readPacket(
    Network* n,
    unsigned char* buffer,
    unsigned int bufferSize,
    Timer* timer
);

int MQTT_readHeader(
    Network* n,
    unsigned char* buffer,
    unsigned int bufferSize
);
