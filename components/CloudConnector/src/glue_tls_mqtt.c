/*
 *  MQTT/mbedTLS glue layer
 *
 *  Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#include "glue_tls_mqtt.h"

#include "TimeServer.h"
#include "lib_debug/Debug_OS_Error.h"

#include <camkes.h>

//------------------------------------------------------------------------------
static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(networkStack);

static const if_OS_Timer_t timer =
    IF_OS_TIMER_ASSIGN(
        timeServer_rpc,
        timeServer_notify);

static OS_Tls_Handle_t tlsContext;
static OS_Crypto_Handle_t hCrypto;
static OS_Socket_Handle_t socketHandle;

static OS_Tls_Config_t tlsCfg =
{
    .mode = OS_Tls_MODE_LIBRARY,
    .library = {
        .flags = OS_Tls_FLAG_NONE,
        .crypto = {
            .policy = NULL,
            .cipherSuites =
            OS_Tls_CIPHERSUITE_FLAGS(
                OS_Tls_CIPHERSUITE_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
                OS_Tls_CIPHERSUITE_DHE_RSA_WITH_AES_128_GCM_SHA256)
        }
    }
};

static OS_Crypto_Config_t cryptoCfg =
{
    .mode = OS_Crypto_MODE_LIBRARY,
    .entropy = IF_OS_ENTROPY_ASSIGN(
        entropy_rpc,
        entropy_port),
};

// Private static functions ----------------------------------------------------
static OS_Error_t
waitForNetworkStackInit(
    const if_OS_Socket_t* const ctx)
{
    OS_NetworkStack_State_t networkStackState;

    for (;;)
    {
        networkStackState = OS_Socket_getStatus(ctx);
        if (networkStackState == RUNNING)
        {
            // NetworkStack up and running.
            return OS_SUCCESS;
        }
        else if (networkStackState == FATAL_ERROR)
        {
            // NetworkStack will not come up.
            Debug_LOG_ERROR("A FATAL_ERROR occurred in the Network Stack component.");
            return OS_ERROR_ABORTED;
        }

        // Yield to wait until the stack is up and running.
        seL4_Yield();
    }
}

static OS_Error_t
connectSocket(
    OS_Socket_Handle_t* const socketHandle,
    const OS_Socket_Addr_t* const dstAddr)
{
    OS_Error_t ret = OS_Socket_create(
                         &networkStackCtx,
                         socketHandle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_create() failed with: %d", ret);
        return ret;
    }

    ret = OS_Socket_connect(*socketHandle, dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(*socketHandle);
        return ret;
    }

    // Wait for the event letting us know that the connection was successfully
    // established.
    for (;;)
    {
        ret = OS_Socket_wait(&networkStackCtx);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_wait() failed, code %d", ret);
            break;
        }

        char evtBuffer[128];
        const size_t evtBufferSize = sizeof(evtBuffer);
        int numberOfSocketsWithEvents;

        ret = OS_Socket_getPendingEvents(
                  &networkStackCtx,
                  evtBuffer,
                  evtBufferSize,
                  &numberOfSocketsWithEvents);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() failed, code %d",
                            ret);
            break;
        }

        // We only opened one socket, so if we get more events, this is not ok.
        if (numberOfSocketsWithEvents == 0)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned with "
                            "unexpected #events: %d", numberOfSocketsWithEvents);
            continue;
        }

        if (numberOfSocketsWithEvents != 1)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned with "
                            "unexpected #events: %d", numberOfSocketsWithEvents);
            ret = OS_ERROR_INVALID_STATE;
            break;
        }

        OS_Socket_Evt_t event;
        memcpy(&event, evtBuffer, sizeof(event));

        if (event.socketHandle != socketHandle->handleID)
        {
            Debug_LOG_ERROR("Unexpected handle received: %d, expected: %d",
                            event.socketHandle, socketHandle->handleID);
            ret = OS_ERROR_INVALID_HANDLE;
            break;
        }

        // Socket has been closed by Network stack.
        if (event.eventMask & OS_SOCK_EV_FIN)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_FIN for handle: %d",
                            event.socketHandle);
            // Socket has been closed by network stack - close socket.
            ret = OS_ERROR_NETWORK_CONN_REFUSED;
            break;
        }

        // Connection successfully established.
        if (event.eventMask & OS_SOCK_EV_CONN_EST)
        {
            Debug_LOG_DEBUG("OS_Socket_getPendingEvents() returned "
                            "connection established for handle: %d",
                            event.socketHandle);
            ret = OS_SUCCESS;
            break;
        }

        // Remote socket requested to be closed only valid for clients.
        if (event.eventMask & OS_SOCK_EV_CLOSE)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_CLOSE for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_CONNECTION_CLOSED;
            break;
        }

        // Error received - print error.
        if (event.eventMask & OS_SOCK_EV_ERROR)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_ERROR for handle: %d, code: %d",
                            event.socketHandle, event.currentError);
            ret = event.currentError;
            break;
        }
    }

    if (ret != OS_SUCCESS)
    {
        OS_Socket_close(*socketHandle);
    }

    return ret;
}

//------------------------------------------------------------------------------
OS_Error_t
glue_tls_init(
    const char* serverIpAddress,
    const char* caCert,
    size_t caCertSize,
    uint32_t serverPort)
{
    OS_Error_t ret = OS_Crypto_init(&hCrypto, &cryptoCfg);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Crypto_init() failed with: %d", ret);
        return ret;
    }

    tlsCfg.library.socket.context = &socketHandle;
    tlsCfg.library.crypto.handle = hCrypto;
    tlsCfg.library.crypto.caCerts = caCert;
    Debug_LOG_DEBUG("Assigned ServerCert: %s",
                    tlsCfg.library.crypto.caCerts);

    ret = OS_Tls_init(&tlsContext, &tlsCfg);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Tls_init() failed with: %d", ret);
        return ret;
    }

    // Check and wait until the NetworkStack component is up and running.
    ret = waitForNetworkStackInit(&networkStackCtx);
    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
        return ret;
    }

    OS_Socket_Addr_t dstAddr;

    strncpy(dstAddr.addr, serverIpAddress, sizeof(dstAddr.addr));
    dstAddr.addr[sizeof(dstAddr.addr) - 1] = '\0';

    dstAddr.port = serverPort;

    ret = connectSocket(&socketHandle, &dstAddr);
    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("connectSocket() failed with err %d", ret);
        return ret;
    }

    Debug_LOG_INFO("TCP connection established successfully");

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
OS_Error_t
glue_tls_handshake(void)
{
    OS_Error_t ret = OS_Tls_handshake(tlsContext);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Tls_handshake() failed with: %d", ret);
        return ret;
    }

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
uint64_t
glue_tls_mqtt_getTimeMs(void)
{
    uint64_t ms;

    OS_Error_t err = TimeServer_getTime(
                         &timer,
                         TimeServer_PRECISION_MSEC,
                         &ms);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("TimeServer_getTime() failed , code '%s'",
                        Debug_OS_Error_toString(err));
        ms = 0;
    }

    return ms;
}

//------------------------------------------------------------------------------
int glue_tls_mqtt_write(Network* n,
                        const unsigned char* buf,
                        int len,
                        int timeout_ms)
{
    Debug_ASSERT(buf != NULL);

    const uint64_t entryTime = glue_tls_mqtt_getTimeMs();
    if (entryTime == 0)
    {
        Debug_LOG_ERROR("glue_tls_mqtt_getTimeMs() failed to provide "
                        "entry time");
        return MQTT_FAILURE;
    }

    size_t remainingLen = len;
    size_t writtenLen = 0;

    // Loop until all data is sent or timeout.
    while ((remainingLen > 0)
           && ((glue_tls_mqtt_getTimeMs() - entryTime) < timeout_ms))
    {
        size_t actualLen = remainingLen;
        OS_Error_t ret = OS_Tls_write(
                             tlsContext,
                             (buf + writtenLen),
                             &actualLen);
        switch (ret)
        {
        case OS_SUCCESS:
            remainingLen -= actualLen;
            writtenLen += actualLen;
            break;
        case OS_ERROR_WOULD_BLOCK:
            // Donate the remaining timeslice to a thread of the same priority
            // and try to write again with the next turn.
            seL4_Yield();
            break;
        default:
            Debug_LOG_ERROR("OS_Tls_write() failed with: %d", ret);
            return MQTT_FAILURE;
        }
    };

    if (remainingLen > 0)
    {
        Debug_LOG_ERROR("OS_Tls_write() wrote only %zu bytes (of %d bytes)",
                        writtenLen, len);
        return MQTT_TIMEOUT;
    }
    return MQTT_SUCCESS;
}

//------------------------------------------------------------------------------
int glue_tls_mqtt_read(Network* n,
                       unsigned char* buf,
                       int len,
                       int timeout_ms)
{
    Debug_ASSERT(buf != NULL);
    Debug_LOG_TRACE("%s: %d bytes, %d ms", __func__, len, timeout_ms);

    const uint64_t entryTime = glue_tls_mqtt_getTimeMs();
    if (entryTime == 0)
    {
        Debug_LOG_ERROR("glue_tls_mqtt_getTimeMs() failed to provide "
                        "entry time");
        return MQTT_FAILURE;
    }

    size_t remainingLen = len;
    memset(buf, 0, len);
    size_t readLen = 0;

    // Loop until all data is read or timeout.
    while ((remainingLen > 0)
           && ((glue_tls_mqtt_getTimeMs() - entryTime) < timeout_ms))
    {
        size_t actualLen = remainingLen;
        OS_Error_t ret = OS_Tls_read(tlsContext, (buf + readLen), &actualLen);
        switch (ret)
        {
        case OS_SUCCESS:
            remainingLen -= actualLen;
            readLen += actualLen;
            break;
        case OS_ERROR_WOULD_BLOCK:
            // Donate the remaining timeslice to a thread of the same priority
            // and try to read again with the next turn.
            seL4_Yield();
            break;
        default:
            Debug_LOG_ERROR("OS_Tls_read() failed with: %d", ret);
            return MQTT_FAILURE;
        }
    };

    if (remainingLen > 0)
    {
        Debug_LOG_ERROR("OS_Tls_read() read only %zu bytes (of %d bytes)",
                        readLen, len);
        return MQTT_TIMEOUT;
    }
    return MQTT_SUCCESS;
}
