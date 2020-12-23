/*
 *  MQTT client based on PAHO
 *
 *  Copyright (C) 2020, Hensoldt Cyber GmbH
 *
 *  This code is based on PAHO sample code Copyright (c) 2014, 2017 IBM Corp.
 */

#include "MQTT_client.h"

#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"

#define MAX_PACKET_ID   65535 // according to the MQTT specification


//------------------------------------------------------------------------------
static int getNextPacketId(
    MQTT_client_t* self
)
{
    Debug_ASSERT( self->nextPacketId <= MAX_PACKET_ID );

    if (self->nextPacketId >= MAX_PACKET_ID)
    {
        self->nextPacketId = 1;
    }
    else
    {
        self->nextPacketId++;
    }

    return self->nextPacketId;
}


//------------------------------------------------------------------------------
static void closeSession(
    MQTT_client_t* self
)
{
    self->isPingOutstanding = 0;
    self->isConnected = 0;
}



//------------------------------------------------------------------------------
// send a packet and update the keep alive mechanism if successful
static int sendPacket(
    MQTT_client_t* self,
    unsigned int length
)
{
    Timer myTimer;
    Timer* timer = NULL;
    if (self->send_timeout_ms != -1)
    {
        TimerInit(&myTimer);
        TimerCountdownMS(&myTimer, self->send_timeout_ms);
        timer = &myTimer;
    }

    return MQTT_network_sendPacket(self->net, self->sendbuf, length, timer);

    // update timer for keep-alive mechanism
    if (self->keepAliveInterval_ms != 0)
    {
        TimerCountdown(&self->timerLastSend, self->keepAliveInterval_ms);
    }

}


//==============================================================================
//
// internal helper function to send specific packets
//
//==============================================================================

//------------------------------------------------------------------------------
static int sendConnect(
    MQTT_client_t* self,
    MQTTPacket_connectData* options
)
{
    int len = MQTTSerialize_connect(self->sendbuf,
                                    self->sendbuf_size,
                                    options);
    if (len <= 0)
    {
        Debug_LOG_ERROR("%s(): MQTTSerialize_connect() failed with code %d", __func__,
                        len);
        return MQTT_FAILURE;
    }

    int ret = sendPacket(self, len);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): MQTT_client_sendPacket() failed with code %d", __func__,
                        len);
        return MQTT_FAILURE;
    }

    return MQTT_SUCCESS;
}

//------------------------------------------------------------------------------
static int sendDisconnect(
    MQTT_client_t* self
)
{
    int len = MQTTSerialize_disconnect(self->sendbuf, self->sendbuf_size);
    if (len <= 0)
    {
        Debug_LOG_ERROR("%s(): MQTTSerialize_disconnect() failed with code %d",
                        __func__, len);
        return MQTT_FAILURE;
    }

    int ret = sendPacket(self, len);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): MQTT_network_sendPacket() failed with code %d", __func__,
                        ret);
    }

    return MQTT_SUCCESS;
}


//------------------------------------------------------------------------------
static int sendPingReq(
    MQTT_client_t* self
)
{
    int len = MQTTSerialize_pingreq(self->sendbuf, self->sendbuf_size);
    if (len <= 0)
    {
        Debug_LOG_ERROR("%s(): MQTTSerialize_pingreq() failed with code %d", __func__,
                        len);
        return MQTT_FAILURE;
    }

    int ret = sendPacket(self, len);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): MQTT_client_sendPacket() failed with code %d", __func__,
                        ret);
        return MQTT_FAILURE;
    }

    self->isPingOutstanding = 1;
    return MQTT_SUCCESS;
}


//------------------------------------------------------------------------------
static int sendAck(
    MQTT_client_t* self,
    unsigned int ackType,
    unsigned char dup,
    unsigned short packetId
)
{
    int len = MQTTSerialize_ack(self->sendbuf,
                                self->sendbuf_size,
                                ackType,
                                dup,
                                packetId);
    if (len <= 0)
    {
        Debug_LOG_ERROR("%s(): MQTTSerialize_ack(%d) failed with code %d", __func__,
                        ackType, len);
        return MQTT_FAILURE;
    }

    int ret = sendPacket(self, len);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): MQTT_client_sendPacket() failed with code %d", __func__,
                        ret);
        return MQTT_FAILURE;
    }

    return MQTT_SUCCESS;
}


//------------------------------------------------------------------------------
static int sendPublish(
    MQTT_client_t* self,
    unsigned char dup,
    int qos,
    unsigned char retained,
    unsigned short packetId,
    const char* topicName,
    unsigned char* payload,
    int payloadLen
)
{
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char*)topicName;

    int len = MQTTSerialize_publish(self->sendbuf,
                                    self->sendbuf_size,
                                    dup,
                                    qos,
                                    retained,
                                    packetId,
                                    topic,
                                    payload,
                                    payloadLen);
    if (len <= 0)
    {
        Debug_LOG_ERROR("%s(): MQTTSerialize_publish() failed with code %d", __func__,
                        len);
        return MQTT_FAILURE;
    }

    int ret = sendPacket(self, len);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): MQTT_client_sendPacket() failed with code %d", __func__,
                        len);
        return MQTT_FAILURE;
    }

    return MQTT_SUCCESS;
}


//==============================================================================
//
// handle incoming packets
//
//==============================================================================


//------------------------------------------------------------------------------
int waitForNextPacket(
    MQTT_client_t* self,
    Timer* timer
)
{
    int packetType = -1;
    int ret = MQTT_network_readPacket( self->net,
                                       self->readbuf,
                                       self->readbuf_size,
                                       timer);
    if (ret < 0)
    {
        Debug_LOG_WARNING("MQTT_network_readPacket() failed with: %d", ret);
    }
    else
    {
        packetType = ret;

        // ToDo: we could remember the timestamp, as the time when we've last
        //       received something from the server. Currently there is no need
        //       for this, as the keep-alive if only about the packets we send.

        switch (packetType)
        {
        case PINGRESP:
            self->isPingOutstanding = 0;
            break;

        default:
            break;
        }
    }

    if (    (self->keepAliveInterval_ms != 0)
            && TimerIsExpired(&self->timerLastSend) )
    {
        // seems we've reached the timeout already and expect a response,
        // but it did not arrive. So the connection is considered dead
        if (self->isPingOutstanding)
        {
            return MQTT_FAILURE;
        }

        // send a new ping packet to show we are alive
        int ret = sendPingReq(self);
        if (ret != MQTT_SUCCESS)
        {
            Debug_LOG_ERROR("%s(): MQTT_client_sendPingReq() failed with code %d", __func__,
                            ret);

            if (packetType == -1)
            {
                // sending the PING failed and we did not receive a packet
                // either. Tell the caller that something is wrong. It should
                // terminate the session
                return MQTT_FAILURE;
            }

            // sending the PING failed, but we have a packet. So better
            // give the packet to the caller without reporting an error and
            // let it process it. It may detect an error later.
        }
    }

    return (packetType == -1) ? MQTT_SUCCESS : packetType;
}


//------------------------------------------------------------------------------
int waitForSpecificPacket(
    MQTT_client_t* self,
    int packetType,
    Timer* timer
)
{
    for (;;)
    {
        if (timer && TimerIsExpired(timer))
        {
            return MQTT_TIMEOUT;
        }

        int ret = waitForNextPacket(self, timer);
        if (ret < 0)
        {
            Debug_LOG_ERROR("%s(): waitForNextPacket() failed with code %d", __func__, ret);
            return MQTT_FAILURE;
        }

        if (ret == packetType)
        {
            return MQTT_SUCCESS;
        }

        // ignore all other packets
    }
}


//------------------------------------------------------------------------------
typedef struct
{
    unsigned short packetId;
    unsigned char dup;
    unsigned char type;
} ackData_t;

int waitAndReadSpecificAckPacket(
    MQTT_client_t* self,
    int ackPacketType,
    ackData_t* data,
    Timer* timer
)
{
    Debug_ASSERT(NULL != data);

    int ret = waitForSpecificPacket(self, ackPacketType, timer);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): waitForPacket(%d) failed with code %d",
                        __func__, ackPacketType, ret);
        return MQTT_FAILURE;
    }

    int len = MQTTDeserialize_ack(&data->type,
                                  &data->dup,
                                  &data->packetId,
                                  self->readbuf,
                                  self->readbuf_size);
    if (len != 1)
    {
        Debug_LOG_ERROR("%s(): MQTTDeserialize_ack(%d) failed with code %d",
                        __func__, ackPacketType, len);
        return MQTT_FAILURE;
    }

    return MQTT_SUCCESS;
}


//==============================================================================
//
// QoS levels handling
//
//==============================================================================

//------------------------------------------------------------------------------
int checkPublishQos(
    MQTT_client_t* self,
    unsigned char qos,
    Timer* timer
)
{
    int ret;
    ackData_t ackData;

    switch (qos)
    {
    //-----------------------------------------------------------
    case 0:
        // no ACK
        return MQTT_SUCCESS;

    //-----------------------------------------------------------
    case 1:
        ret = waitAndReadSpecificAckPacket(self, PUBACK, &ackData, timer);
        if (ret != MQTT_SUCCESS)
        {
            Debug_LOG_ERROR("%s(): waitForPacket(PUBACK) failed with code %d", __func__,
                            ret);
            return MQTT_FAILURE;
        }

        Debug_ASSERT(PUBACK == ackData.type);
        Debug_LOG_INFO("%s(): got PUBACK", __func__);

        return MQTT_SUCCESS;

    //-----------------------------------------------------------
    case 2:
        ret = waitAndReadSpecificAckPacket(self, PUBREC, &ackData, timer);
        if (ret != MQTT_SUCCESS)
        {
            Debug_LOG_ERROR("%s(): waitAndReadSpecificAckPacket(PUBACK) failed with code %d",
                            __func__, ret);
            return MQTT_FAILURE;
        }

        Debug_ASSERT(PUBREC == ackData.type);
        Debug_LOG_INFO("%s(): got PUBREC", __func__);

        ret = sendAck(self, PUBREL, 0, ackData.packetId);
        if (ret != MQTT_SUCCESS)
        {
            Debug_LOG_ERROR("%s(): sendAck(PUBREL) failed with code %d", __func__, ret);
            return MQTT_FAILURE;
        }

        ret = waitAndReadSpecificAckPacket(self, PUBCOMP, &ackData, timer);
        if (ret != MQTT_SUCCESS)
        {
            Debug_LOG_ERROR("%s(): waitAndReadSpecificAckPacket(PUBCOMP) failed with code %d",
                            __func__, ret);
            return MQTT_FAILURE;
        }

        Debug_ASSERT(PUBCOMP == ackData.type);
        Debug_LOG_INFO("%s(): got PUBCOMP", __func__);

        return MQTT_SUCCESS;

    //-----------------------------------------------------------
    default:
        // unsupported QoS level
        break;
    }

    return MQTT_FAILURE;
}


//==============================================================================
//
// Public Functions
//
//==============================================================================


//------------------------------------------------------------------------------
int MQTT_client_connect(
    MQTT_client_t* self,
    MQTTPacket_connectData* options,
    MQTT_connackData_t* data,
    Timer* timer
)
{
    int ret;

    // don't send connect packet again if we are already connected
    if (self->isConnected)
    {
        Debug_LOG_ERROR("%s(): already connected", __func__);
        return MQTT_FAILURE;
    }

    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    if (options == NULL)
    {
        options = &default_options;
    }

    // Via options->cleansession a clean session can be requested and any
    // previous persistent sessoin data is dropped by the broker. Clean session
    // are enabled in the defaults. For persistent sessions, all information
    // and messages are preserved by the broker when the client disconnects for
    // any reason. They are queued until the client connects again (not
    // requesting a clean session) and then send.


    // The keep-alive time is set to 60 seconds by default, using value of 0
    // disables it. MQTT rules are, that this is the maximum time that shall
    // elapse between two packets send from client to broker. The client must
    // ensure this and send PINGREQ packet regularly if nothing else if send.
    // If the broker does not receive anything from a client withing the
    // keep-alive time, it closes the connection and sends the LWT message.
    self->keepAliveInterval_ms = options->keepAliveInterval;
    TimerCountdown(&self->timerLastSend, self->keepAliveInterval_ms);

    ret = sendConnect(self, options);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): MQTT_client_sendConnect() failed with code %d", __func__,
                        ret);
        return MQTT_FAILURE;
    }

    ret = waitForSpecificPacket(self, CONNACK, timer);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): waitForPacket(CONNACK) failed with code %d", __func__,
                        ret);
        return MQTT_FAILURE;
    }

    data->rc = 0;
    data->sessionPresent = 0;

    ret = MQTTDeserialize_connack(&data->sessionPresent,
                                  &data->rc,
                                  self->readbuf,
                                  self->readbuf_size);
    if (ret != 1)
    {
        Debug_LOG_ERROR("%s(): MQTTDeserialize_connack() failed with code %d", __func__,
                        ret);
        return MQTT_FAILURE;
    }

    self->isConnected = 1;
    self->isPingOutstanding = 0;

    return MQTT_SUCCESS;
}


//------------------------------------------------------------------------------
int MQTT_client_publish(
    MQTT_client_t* self,
    const char* topicName,
    MQTT_message_t* msg,
    Timer* timer
)
{
    int ret;

    if (!self->isConnected)
    {
        Debug_LOG_ERROR("%s(): not connected", __func__);
        // lay safe and ensure here is no connection
        closeSession(self);
        return MQTT_FAILURE;
    }

    if ((msg->qos == 1) || (msg->qos == 2))
    {
        msg->id = getNextPacketId(self);
    }

    ret = sendPublish(self,
                      0,
                      msg->qos,
                      msg->retained,
                      msg->id,
                      topicName,
                      (unsigned char*)msg->payload,
                      msg->payloadlen);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): sendPublish() failed with code %d", __func__, ret);
        closeSession(self);
        return MQTT_FAILURE;
    }

    ret = checkPublishQos(self, msg->qos, timer);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): checkPublishQOS() failed with code %d", __func__, ret);
        closeSession(self);
        return MQTT_FAILURE;
    }

    return MQTT_SUCCESS;
}


//------------------------------------------------------------------------------
void MQTT_client_disconnect(
    MQTT_client_t* self
)
{
    int ret = sendDisconnect(self);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("%s(): sendDisconnect() failed with code %d", __func__, ret);

        // there is nothing we can do here, but we are disconnected from our
        // side. For the caller it never fails.
    }

    closeSession(self);
}


//------------------------------------------------------------------------------
void MQTT_client_init(
    MQTT_client_t* self,
    Network* net,
    unsigned int send_timeout_ms,
    void* sendbuf,
    size_t sendbuf_size,
    void* readbuf,
    size_t readbuf_size
)
{
    Debug_ASSERT_SELF(self);
    Debug_ASSERT(net != NULL);
    Debug_ASSERT(sendbuf != NULL);
    Debug_ASSERT(readbuf != NULL);

    self->net             = net;

    self->sendbuf       = (unsigned char*)sendbuf;
    self->sendbuf_size  = sendbuf_size;
    self->readbuf       = (unsigned char*)readbuf;
    self->readbuf_size  = readbuf_size;

    self->send_timeout_ms = send_timeout_ms;

    self->keepAliveInterval_ms = 0;
    TimerInit(&self->timerLastSend);

    self->isConnected = 0;
    self->isPingOutstanding = 0;
    self->nextPacketId = 1;
}
