/*
 *  MQTT server based on PAHO
 *
 *  Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "MQTTServer.h"

#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"

#include "MQTT_net.h"
#include "MQTTPacket.h"

//==============================================================================
//
// MQTT Server helper functions
//
//==============================================================================


//------------------------------------------------------------------------------
// send a packet. Returns SUCCESS if this worked, everything else is an error.
static int MQTTServer_sendPacket(
    MQTTServer* self,
    unsigned int length
)
{
    Timer myTimer;
    Timer* timer = NULL;
    if (self->socket_timeout_ms != -1)
    {
        TimerInit(&myTimer);
        TimerCountdownMS(&myTimer, self->socket_timeout_ms);
        timer = &myTimer;
    }

    return MQTT_network_sendPacket(self->net, self->sendbuf, length, timer);
}


//==============================================================================
//
// Public Functions
//
//==============================================================================

//------------------------------------------------------------------------------
void MQTTServerInit(
    MQTTServer* self,
    Network* network,
    unsigned int socket_timeout_ms,
    void* sendbuf,
    size_t sendbuf_size,
    void* readbuf,
    size_t readbuf_size
)
{
    Debug_ASSERT_SELF(self);
    Debug_ASSERT(network != NULL);
    Debug_ASSERT(sendbuf != NULL);
    Debug_ASSERT(readbuf != NULL);

    self->net               = network;
    self->socket_timeout_ms = socket_timeout_ms;

    self->sendbuf       = (unsigned char*)sendbuf;
    self->sendbuf_size  = sendbuf_size;
    self->readbuf       = (unsigned char*)readbuf;
    self->readbuf_size  = readbuf_size;
}


//------------------------------------------------------------------------------
int MQTTServer_readPacket(
    MQTTServer* self,
    Timer* timer
)
{
    return MQTT_network_readPacket( self->net,
                                    self->readbuf,
                                    self->readbuf_size,
                                    timer );
}

//------------------------------------------------------------------------------
int MQTTServer_readType(
    MQTTServer* self
)
{
    return MQTT_readHeader( self->net,
                            self->readbuf,
                            self->readbuf_size
                          );
}


//------------------------------------------------------------------------------
int MQTTServer_sendConnAck(
    MQTTServer* self,
    unsigned char connack_rc,
    unsigned char sessionPresent
)
{
    int len = MQTTSerialize_connack(self->sendbuf,
                                    self->sendbuf_size,
                                    connack_rc,
                                    sessionPresent);
    if (len <= 0)
    {
        Debug_LOG_ERROR("%s(): MQTTSerialize_connack() failed with code %d", __func__,
                        len);
        return MQTT_FAILURE;
    }

    return MQTTServer_sendPacket(self, len);
}


//------------------------------------------------------------------------------
int MQTTServer_sendPingRes(
    MQTTServer* self
)
{
    // we know that the code below will write 2 bytes
    if (self->sendbuf_size < 2)
    {
        Debug_LOG_ERROR("%s(): send buffer too short", __func__);
        return MQTTPACKET_BUFFER_TOO_SHORT;
    }

    unsigned char* ptr = self->sendbuf;
    int len = 0;


    MQTTHeader header = {0};
    header.byte = 0;
    header.bits.type = PINGRESP;
    writeChar(&ptr, header.byte); /* write header */
    len++;

    len += MQTTPacket_encode(ptr, 0); /* write remaining length */

    Debug_ASSERT(len == 2);

    return MQTTServer_sendPacket(self, len);
}
