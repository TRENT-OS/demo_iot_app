/*
 *  MQTT Network helper functions
 *
 *  Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#include "MQTT_net.h"

#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"

#include "MQTTPacket.h"

//------------------------------------------------------------------------------
// lowest layer of the MQTT network interface, it just depends on the actual
// Network object and nothing else. It may read less data than requested.
int MQTT_network_read(
    Network* n,
    unsigned char* buffer,
    int len,
    Timer* timer
)
{

    int timeout_ms = timer ? TimerLeftMS(timer) : -1;

    // The interface here is a bit odd. We pass (and thus expose) the complete
    // Network object, as it does not have a opaque context pointer field that
    // could hold all that is needed. Currently we can't change the signature,
    // as the code from paho.mqtt.embedded-c/MQTTClient-C/src/MQTTClient.c also
    // use this and - even worse - duplicates the definitions instead of using
    // a shared header file. It would be much better if we create our own
    // MQTTClient.c and use a shared header - and fix the interface to just
    // pass a context.
    return n->mqttread(n, buffer, len, timeout_ms);
}


//------------------------------------------------------------------------------
// lowest layer of the MQTT network interface, it just depends on the actual
// Network object and nothing else. It may write less data than requested.
int MQTT_network_write(
    Network* n,
    const unsigned char* buffer,
    int len,
    int timeout_ms
)
{
    // see comment in network_read() above, why the interface is the way it is.
    int ret = n->mqttwrite(n, buffer, len, timeout_ms);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("mqttwrite() failed with: %d", ret);
        return MQTT_FAILURE; // ToDo: return MQTT_TIMEOUT on timeout
    }

    return ret;
}


//------------------------------------------------------------------------------
// send a packet from given buffer to a Network object.
int MQTT_network_sendPacket(
    Network* n,
    const unsigned char* buffer,
    unsigned int length,
    Timer* timer
)
{

    int timeout_ms = -1;
    timeout_ms = TimerLeftMS(timer);

    int ret = MQTT_network_write(n, buffer, length, timeout_ms);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("network_write() for packet failed with: %d", ret);
        return ret;
    }

    return ret;
}


//------------------------------------------------------------------------------
// read a packet from the Network and get the decoded length. Return a positive
// value with the number of length bytes read or a negative value indicating an
// error. The current implementation never returns 0, but this should be
// considered as an error condition where the underlying transport layer died
// unexpectedly.
static int MQTT_network_readAndDecodePacketLength(
    Network* n,
    unsigned int* value,
    Timer* timer
)
{
    // ensure we have a sane return value in case of error
    *value = 0;

    int shift = 0;
    int bytesLeft = 4; // read max 4 bytes, ie max encoded length is 2^28.
    unsigned int tmpPacketLen = 0;
    unsigned int cntBytes = 0;

    for (;;)
    {
        unsigned char lenByte;
        // this will also check internally if the timer has expired
        int rc = MQTT_network_read(n, &lenByte, 1, timer);
        if (rc != 1)
        {
            Debug_LOG_ERROR("network_read() for a length byte failed with: %d", rc);
            return MQTT_FAILURE; // ToDo: return MQTT_TIMEOUT on timeout
        }

        cntBytes++;
        // bit 0-6 hold another 7 bit of the length
        unsigned int mask = ((1U << 7) - 1);
        tmpPacketLen |= (lenByte & mask) << shift;
        shift += 7;

        // bit 7 indicates if more length bytes follow.
        if ((lenByte & (1U << 7) ) == 0)
        {
            *value = tmpPacketLen;
            return cntBytes;
        }

        if (--bytesLeft <= 0)
        {
            Debug_LOG_ERROR("%s(): too many length bytes", __func__);
            return MQTT_FAILURE;
        }

    } // end for(;;)
}


//------------------------------------------------------------------------------
int MQTT_network_readPacket(
    Network* n,
    unsigned char* buffer,
    unsigned int bufferSize,
    Timer* timer
)
{
    // read the header byte. This has the packet type in it

    int rc = MQTT_network_read(n, buffer, 1, timer);
    if (rc != 1)
    {
        Debug_LOG_ERROR("MQTT_network_read() for header byte failed with: %d", rc);
        return MQTT_FAILURE; // ToDo: return MQTT_TIMEOUT on timeout
    }

    // read the remaining length, 0 is a valid length
    unsigned int payloadLen = 0;
    rc = MQTT_network_readAndDecodePacketLength(n, &payloadLen, timer);
    if ((rc <= 0) || (payloadLen < 0))
    {
        Debug_LOG_ERROR("MQTT_network_readAndDecodePacketLength failed with: %d", rc);
        return MQTT_FAILURE; // ToDo: return MQTT_TIMEOUT on timeout
    }

    // put the original remaining length back into the buffer
    unsigned int offset = 1 + MQTTPacket_encode(&buffer[1], payloadLen);

    // if the packet has payload, then read it. If there is no payload, then
    // it's better to avoid trying to read 0 byte form the network stack, as
    // this may be seen as an undefined operation
    if (payloadLen > 0)
    {
        // ToDo: somebody could try to send huge packets, which we would reject
        //       when this function is used. Chunked reading requires a smarter
        //       implementation.
        if (payloadLen > (bufferSize - offset))
        {
            Debug_LOG_ERROR("%s(): buffer too small", __func__);
            return  MQTT_BUFFER_OVERFLOW;
        }

        // ToDo: here we should loop over MQTT_network_read(), as it may read
        //       less data than expected.
        rc = MQTT_network_read(n, &buffer[offset], payloadLen, timer);
        if (rc != payloadLen)
        {
            Debug_LOG_ERROR("MQTT_network_read for payload failed with: %d", rc);
            return MQTT_FAILURE; // ToDo: return MQTT_TIMEOUT on timeout
        }
    }

    // the packet type is encoded in certain bits of the first byte
    MQTTHeader header = {0};
    header.byte = buffer[0];
    return header.bits.type;
}

//------------------------------------------------------------------------------
int MQTT_readHeader(
    Network* n,
    unsigned char* buffer,
    unsigned int bufferSize
)
{
    // the packet type is encoded in certain bits of the first byte
    MQTTHeader header = {0};
    header.byte = buffer[0];

    Debug_LOG_TRACE("%s(): Parsed header byte: %d\n", __func__, header.bits.type);

    return header.bits.type;
}
