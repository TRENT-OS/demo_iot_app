/*
 *  MQTT/mbedTLS glue layer
 *
 *  Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#include "glue_tls_mqtt.h"

#include <camkes.h>

//------------------------------------------------------------------------------
static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(networkStack);

static OS_Tls_Handle_t tlsContext;
static OS_Crypto_Handle_t hCrypto;
static OS_NetworkSocket_Handle_t socketHandle;

static int
sendFunc(
    void*                ctx,
    const unsigned char* buf,
    size_t               len);

static int
recvFunc(
    void*          ctx,
    unsigned char* buf,
    size_t         len);

static OS_Tls_Config_t tlsCfg =
{
    .mode = OS_Tls_MODE_LIBRARY,
    .library = {
        .socket = {
            .recv   = recvFunc,
            .send   = sendFunc,
        },
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
        networkStackState = OS_NetworkSocket_getStatus(ctx);
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

static int
sendFunc(
    void*                ctx,
    const unsigned char* buf,
    size_t               len)
{
    OS_NetworkSocket_Handle_t* hSocket = (OS_NetworkSocket_Handle_t*) ctx;
    size_t n;

    OS_Error_t ret = OS_NetworkSocket_write(*hSocket, buf, len, &n);
    if (ret == OS_ERROR_TRY_AGAIN)
    {
        return OS_Tls_SOCKET_WRITE_WOULD_BLOCK;
    }
    else if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_write() failed with %d", ret);
        return ret;
    }

    return n;
}

static int
recvFunc(
    void*          ctx,
    unsigned char* buf,
    size_t         len)
{
    OS_NetworkSocket_Handle_t* hSocket = (OS_NetworkSocket_Handle_t*) ctx;
    size_t n;

    OS_Error_t ret = OS_NetworkSocket_read(*hSocket, buf, len, &n);
    if (ret == OS_ERROR_TRY_AGAIN)
    {
        // Avoid polling here and rather wait until we get notified by the
        // NetworkStack about pending events. After it gets unblocked, try
        // again.
        ret = OS_NetworkSocket_wait(&networkStackCtx);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_wait() failed, code %d", ret);
            return ret;
        }

        return OS_Tls_SOCKET_READ_WOULD_BLOCK;
    }
    else if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_read() failed with %d", ret);
        return ret;
    }

    return n;
}

static OS_Error_t
connectSocket(
    OS_NetworkSocket_Handle_t* const socketHandle,
    const OS_NetworkSocket_Addr_t* const dstAddr)
{
    OS_Error_t ret = OS_NetworkSocket_create(
                         &networkStackCtx,
                         socketHandle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed with: %d", ret);
        return ret;
    }

    ret = OS_NetworkSocket_connect(*socketHandle, dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_connect() failed, code %d", ret);
        OS_NetworkSocket_close(*socketHandle);
        return ret;
    }

    // Wait for the event letting us know that the connection was successfully
    // established.
    for (;;)
    {
        ret = OS_NetworkSocket_wait(&networkStackCtx);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_wait() failed, code %d", ret);
            break;
        }

        char evtBuffer[128];
        const size_t evtBufferSize = sizeof(evtBuffer);
        int numberOfSocketsWithEvents;

        ret = OS_NetworkSocket_getPendingEvents(
                  &networkStackCtx,
                  evtBuffer,
                  evtBufferSize,
                  &numberOfSocketsWithEvents);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_getPendingEvents() failed, code %d",
                            ret);
            break;
        }

        // We only opened one socket, so if we get more events, this is not ok.
        if (numberOfSocketsWithEvents == 0)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_getPendingEvents() returned with "
                            "unexpected #events: %d", numberOfSocketsWithEvents);
            continue;
        }

        if (numberOfSocketsWithEvents != 1)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_getPendingEvents() returned with "
                            "unexpected #events: %d", numberOfSocketsWithEvents);
            ret = OS_ERROR_INVALID_STATE;
            break;
        }

        OS_NetworkSocket_Evt_t event;
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
            Debug_LOG_ERROR("OS_NetworkSocket_getPendingEvents() returned "
                            "OS_SOCK_EV_FIN for handle: %d",
                            event.socketHandle);
            // Socket has been closed by network stack - close socket.
            ret = OS_ERROR_NETWORK_CONN_REFUSED;
            break;
        }

        // Connection successfully established.
        if (event.eventMask & OS_SOCK_EV_CONN_EST)
        {
            Debug_LOG_DEBUG("OS_NetworkSocket_getPendingEvents() returned "
                            "connection established for handle: %d",
                            event.socketHandle);
            ret = OS_SUCCESS;
            break;
        }

        // Remote socket requested to be closed only valid for clients.
        if (event.eventMask & OS_SOCK_EV_CLOSE)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_getPendingEvents() returned "
                            "OS_SOCK_EV_CLOSE for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_CONNECTION_CLOSED;
            break;
        }

        // Error received - print error.
        if (event.eventMask & OS_SOCK_EV_ERROR)
        {
            Debug_LOG_ERROR("OS_NetworkSocket_getPendingEvents() returned "
                            "OS_SOCK_EV_ERROR for handle: %d, code: %d",
                            event.socketHandle, event.currentError);
            ret = event.currentError;
            break;
        }
    }

    if (ret != OS_SUCCESS)
    {
        OS_NetworkSocket_close(*socketHandle);
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

    OS_NetworkSocket_Addr_t dstAddr;

    strncpy(dstAddr.addr, serverIpAddress, sizeof(dstAddr.addr));
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
int glue_tls_mqtt_write(Network* n,
                        const unsigned char* buf,
                        int len,
                        int timeout_ms)
{
    Debug_ASSERT(buf    != NULL);

    size_t to_write = len;
    OS_Error_t ret = OS_Tls_write(tlsContext, buf, &to_write);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Tls_write() failed with: %d", ret);
        return MQTT_FAILURE;
    }
    if (to_write != len)
    {
        Debug_LOG_ERROR("OS_Tls_write() wrote only %zd bytes (of %d bytes)",
                        to_write, len);
        return MQTT_FAILURE;
    }

    return MQTT_SUCCESS;
}

//------------------------------------------------------------------------------
int glue_tls_mqtt_read(Network* n,
                       unsigned char* buf,
                       int len,
                       int timeout_ms)
{
    Debug_ASSERT(buf    != NULL);

    Timer t;
    size_t lengthRead = len;
    Debug_LOG_TRACE("%s: %d bytes, %d ms", __func__, len, timeout_ms);

    TimerInit(&t);
    TimerCountdownMS(&t, timeout_ms);

    memset ( buf, 0, len );

    OS_Error_t ret = OS_Tls_read(tlsContext, buf, &lengthRead);
    if (ret != OS_SUCCESS && ret != OS_ERROR_CONNECTION_CLOSED)
    {
        Debug_LOG_ERROR("OS_Tls_read() failed with: %d", ret);
        return MQTT_FAILURE;
    }

    return lengthRead;
}
