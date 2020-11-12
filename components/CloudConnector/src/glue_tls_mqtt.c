/*
 *  MQTT/mbedTLS glue layer
 *
 *  Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "glue_tls_mqtt.h"

#include <camkes.h>

// decrease send/recv max length to increase robustness
#define MAX_NW_SIZE 2048

//------------------------------------------------------------------------------
static OS_Tls_Handle_t tlsContext;
static OS_Crypto_Handle_t hCrypto;
static OS_NetworkSocket_Handle_t socket;

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
init_socket_config(OS_Network_Socket_t* socketConfig,
                   const char* serverIpAddress,
                   uint32_t serverPort)
{
    if (NULL == socketConfig || NULL == serverIpAddress)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    socketConfig->domain = OS_AF_INET;
    socketConfig->type   = OS_SOCK_STREAM;
    socketConfig->port   = serverPort;
    strncpy(socketConfig->name, serverIpAddress, sizeof(socketConfig->name)-1);

    return OS_SUCCESS;
}


static int
sendFunc(
    void*                ctx,
    const unsigned char* buf,
    size_t               len)
{
    OS_Error_t err;
    OS_NetworkSocket_Handle_t* sockHandle = (OS_NetworkSocket_Handle_t*) ctx;
    size_t n;

    n = len > MAX_NW_SIZE ? MAX_NW_SIZE : len;
    if ((err = OS_NetworkSocket_write(*sockHandle, buf, n, &n)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Error during socket write...error:%d", err);
        return -1;
    }

    return n;
}

static int
recvFunc(
    void*          ctx,
    unsigned char* buf,
    size_t         len)
{
    OS_Error_t err;
    OS_NetworkSocket_Handle_t* sockHandle = (OS_NetworkSocket_Handle_t*) ctx;
    size_t n;

    n = len > MAX_NW_SIZE ? MAX_NW_SIZE : len;
    if ((err = OS_NetworkSocket_read(*sockHandle, buf, n, &n)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Error during socket read...error:%d", err);
        return -1;
    }

    return n;
}

//------------------------------------------------------------------------------
OS_Error_t
glue_tls_init(const char* serverIpAddress,
              const char* caCert,
              size_t caCertSize,
              uint32_t serverPort)
{

    OS_Error_t ret;

    OS_Network_Socket_t socketCfg;

    ret = init_socket_config(&socketCfg, serverIpAddress, serverPort);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("init_socket_config() failed with: %d", ret);
        return ret;
    }

    ret = OS_Crypto_init(&hCrypto, &cryptoCfg);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Crypto_init() failed with: %d", ret);
        return ret;
    }

    tlsCfg.library.socket.context = &socket;
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

    ret = OS_NetworkSocket_create(NULL, &socketCfg, &socket);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed with: %d", ret);
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
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Tls_read() failed with: %d", ret);
        return MQTT_FAILURE;
    }

    return lengthRead;
}
