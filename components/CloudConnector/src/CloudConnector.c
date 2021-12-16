/**
 * CloudConnector component establishes an MQTT connection from the Sensor to the cloud.
 *
 * Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#include "lib_debug/Debug.h"
#include "TimeServer.h"

#include <string.h>
#include <camkes.h>

#include "glue_tls_mqtt.h"
#include "helper_func.h"

#include "MQTT_client.h"
#include "MQTTServer.h"

#include "lib_utils/managedBuffer.h"

/* Defines -------------------------------------------------------------------*/
// the following defines are the parameter names that need to match the settings
// in the configuration xml file. These will be passed to the configServer
// component to retrieve the settings for the specified parameter
#define DOMAIN_CLOUDCONNECTOR   "Domain-CloudConnector"
#define CLOUD_DEVICE_ID_NAME    "IoT-Device"
#define CLOUD_DOMAIN_NAME       "IoT-Hub"
#define SERVER_ADDRESS_NAME     "CloudServiceIP"
#define CLOUD_SAS_NAME          "SharedAccessSignature"
#define SERVER_PORT_NAME        "ServerPort"
#define SERVER_CA_CERT_NAME     "ServerCaCert"


#define PAHO_TIMEOUT_MS_LISTEN   (1000 * 60 * 5)
#define PAHO_TIMEOUT_MS_COMMAND  (1000 * 60 * 5)
#define PAHO_SEND_BUFF_SIZE      1024
#define PAHO_RECV_BUFF_SIZE      1024

// sizes chosen to at least fit the expected sizes of the parameters
static char cloudDeviceName[128];
static char cloudUsername[128];
static char cloudSAS[192];
static char serverIP[32];
static char serverCert[4096];

/* Instance variables --------------------------------------------------------*/
OS_ConfigServiceHandle_t hConfig;
typedef struct
{
    Network             net;
    unsigned char       sendBuff[PAHO_SEND_BUFF_SIZE];
    unsigned char       readBuff[PAHO_RECV_BUFF_SIZE];
} CC_FSM_PAHO_NetCtx_t;

typedef struct
{
    struct
    {
        MQTT_client_t           client;
        CC_FSM_PAHO_NetCtx_t   client_netCtx;

        MQTTServer              server;
        CC_FSM_PAHO_NetCtx_t   server_netCtx;
    } paho;

    struct
    {
        MQTT_message_t          msg;
        char*                   szTopic;
        char                    buffer[PAHO_RECV_BUFF_SIZE];
    } tmpDataPublish;

    struct
    {
        size_t                  connect;
        size_t                  pingreq;
        size_t                  publish;
        size_t                  filtered;
    } cnt;
}
CC_FSM_t;

static CC_FSM_t cc_fsm;

//==============================================================================
// external resources
//==============================================================================

OS_Error_t init_config_handle(OS_ConfigServiceHandle_t* configHandle);

//==============================================================================
// internal functions
//==============================================================================

//------------------------------------------------------------------------------
static
OS_Error_t
do_tls_handshake(void)
{

    OS_Error_t ret;

    if ((ret = glue_tls_handshake() != OS_SUCCESS))
    {
        Debug_LOG_WARNING("TLS handshake failed with Errno=%i\n", ret);
        return ret;
    }

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
static
OS_Error_t
set_mqtt_options(MQTTPacket_connectData* options)
{

    OS_Error_t ret = helper_func_getConfigParameter(&hConfig,
                                                    DOMAIN_CLOUDCONNECTOR,
                                                    CLOUD_DOMAIN_NAME,
                                                    cloudUsername,
                                                    sizeof(cloudUsername));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        CLOUD_DOMAIN_NAME, ret);
        return ret;
    }
    Debug_LOG_DEBUG("Retrieved CloudDomain: %s", cloudUsername);

    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_CLOUDCONNECTOR,
                                         CLOUD_SAS_NAME,
                                         cloudSAS,
                                         sizeof(cloudSAS));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        CLOUD_SAS_NAME, ret);
        return ret;
    }
    Debug_LOG_DEBUG("Retrieved CloudSAS: %s", cloudSAS);

    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_CLOUDCONNECTOR,
                                         CLOUD_DEVICE_ID_NAME,
                                         cloudDeviceName,
                                         sizeof(cloudDeviceName));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        CLOUD_DEVICE_ID_NAME, ret);
        return ret;
    }
    Debug_LOG_DEBUG("Retrieved DeviceName: %s", cloudDeviceName);

    options->willFlag           = 0;
    options->MQTTVersion        = 4;
    options->clientID.cstring   = cloudDeviceName;
    options->username.cstring = cloudUsername;
    options->password.cstring = cloudSAS;
    options->keepAliveInterval  = 0; // disable keep alive
    options->cleansession       = 1;

    options->will.message.cstring  = "Famous last words";
    options->will.qos               = 0;
    options->will.retained          = 0;
    options->will.topicName.cstring = "will topic";

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
static int do_mqtt_connect(MQTT_client_t* client,
                           MQTTPacket_connectData* options)
{

    MQTT_connackData_t data;

    int ret = MQTT_client_connect(client, options, &data, NULL);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("MQTT_client_connect() failed with code %d", ret);
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
static int do_process_publish(CC_FSM_t* self,
                              void* inputBuf,
                              size_t inputBufLen)
{
    MQTT_message_t* msg = &(self->tmpDataPublish.msg);

    MQTTString topicObj;
    MQTTLenString* topic = &(topicObj.lenstring);

    // deserialize the packet.
    int ret = MQTTDeserialize_publish(&(msg->dup),
                                      (int*) & (msg->qos),
                                      &(msg->retained),
                                      &(msg->id),
                                      &topicObj,
                                      (unsigned char**) & (msg->payload),
                                      (int*) & (msg->payloadlen),
                                      inputBuf,
                                      inputBufLen);
    if (ret != 1)
    {
        Debug_LOG_ERROR("Malformed PUBLISH received!");
        return -1;
    }

    // sanity check: topic and payload must be in input buffer. Actually, there
    // should be no need to check this, as MQTTDeserialize_publish() should
    // do such kind of checks already.
    Debug_ASSERT( is_buffer_in_buffer( topic->data, topic->len,
                                       inputBuf, inputBufLen) );
    Debug_ASSERT( is_buffer_in_buffer( msg->payload, msg->payloadlen,
                                       inputBuf, inputBufLen) );

    Debug_LOG_DEBUG("Deserialized PUBLISH: dup=%d, qos=%d, retained=%d, packetid=%d, topicName (len=%d):'%.*s' , payload (len=%d):'%.*s' ",
                    (int) msg->dup,
                    (int) msg->qos,
                    (int) msg->retained,
                    (int) msg->id,
                    topic->len,
                    topic->len,
                    (char*)topic->data,
                    msg->payloadlen,
                    msg->payloadlen,
                    (char*)msg->payload);

    if (msg->qos != 1)
    {
        Debug_LOG_WARNING("incoming PUBLISH has QoS=%d, will set to 1", msg->qos);
        msg->qos = 1;
    }

    if (msg->dup != 0)
    {
        Debug_LOG_ERROR("incoming PUBLISH has DUP=%d, will ignore it.", msg->dup);
        return -1;
    }

    managedBuffer_t mb;
    managedBuffer_init( &mb,
                        self->tmpDataPublish.buffer,
                        sizeof(self->tmpDataPublish.buffer) );

    // copy topic into tmp buffer and make it a NULL-terminated string. Note
    // that the topic itself in the packet is not NULL-terminated, but the MQTT
    // API function to create the packet wants a NULL-terminated string with
    // the topic name as parameter. Thus we need to add a NULL char here.
    size_t lenTopicStr = topic->len + 1;
    if (managedBuffer_getFreeSpace(&mb) < lenTopicStr)
    {
        Debug_LOG_ERROR("tmp buffer too small for topic");
        return -1;
    }

    self->tmpDataPublish.szTopic = (char*)managedBuffer_getFreeSpacePtr(&mb);
    managedBuffer_append(&mb, topic->data, topic->len);
    managedBuffer_appendChar(&mb, '\0');

    void* payloadBuffer = (char*)managedBuffer_getFreeSpacePtr(&mb);

    if (managedBuffer_getFreeSpace(&mb) < msg->payloadlen)
    {
        Debug_LOG_ERROR("tmp buffer too small for payload");
        return -1;
    }

    managedBuffer_append(&mb, msg->payload, msg->payloadlen);

    // make payload point to the temp buffer content
    msg->payload = payloadBuffer;

    return 0;
}

//==============================================================================
// MQTT packet handlers
//==============================================================================

//------------------------------------------------------------------------------
static int handle_MQTT_CONNECT(CC_FSM_t* self)
{
    self->cnt.connect++;
    Debug_LOG_DEBUG("received MQTT CONNECT #%u", self->cnt.connect);
    MQTTServer_sendConnAck(&self->paho.server, 0, 0);

    return 0;
}

//------------------------------------------------------------------------------
static int handle_MQTT_PINGREQ(CC_FSM_t* self)
{
    // For the scope of this demo, we will ignore the MQTT Subscribe events
    Debug_LOG_WARNING("received MQTT PINGREQ, ignored");

    return 0;
}

//------------------------------------------------------------------------------
static int handle_MQTT_SUBSCRIBE(CC_FSM_t* self)
{
    // For the scope of this demo, we will ignore the MQTT Subsribe events
    Debug_LOG_WARNING("received MQTT SUBSCRIBE, ignored");

    return 0;
}

//------------------------------------------------------------------------------
static int handle_MQTT_unsupportedPacket(CC_FSM_t* self,
                                         int packet_type)
{
    Debug_LOG_WARNING("received unsupported packet type %d, ignored", packet_type);

    // we've received a unsupported packet, ignore this

    return 0;
}

//------------------------------------------------------------------------------
static int handle_MQTT_PUBLISH(CC_FSM_t* self)
{
    // in case of error we wait for the next packet. This is ok, as there is
    // no channel to the sender of the packets to report errors.

    self->cnt.publish++;
    Debug_LOG_DEBUG("received MQTT PUBLISH #%u", self->cnt.publish);

    // the buffer from the MQTT server connection holds the packet. Process it
    // and populate a message that is send out on the WAN
    CC_FSM_PAHO_NetCtx_t* netCtx_server = &(self->paho.server_netCtx);

    int ret = do_process_publish(self,
                                 netCtx_server->readBuff,
                                 sizeof(netCtx_server->readBuff));
    if (ret != 0)
    {
        Debug_LOG_ERROR("do_process_publish() failed with code %d", ret);
        // don't report the error to caller, just listen for the next package
        return 0;
    }

    //

    ret = MQTT_client_publish(&(self->paho.client),
                              self->tmpDataPublish.szTopic,
                              &(self->tmpDataPublish.msg),
                              NULL);
    if (ret != MQTT_SUCCESS)
    {
        Debug_LOG_ERROR("MQTTPublish() failed with code %d", ret);
        MQTT_client_disconnect(&self->paho.client);
        return -1;
    }
    Debug_LOG_INFO("MQTT publish on WAN successful");

    return 0;
}

//------------------------------------------------------------------------------
static int handle_CC_FSM_INIT(CC_FSM_t* self)
{

    OS_Error_t ret = helper_func_getConfigParameter(&hConfig,
                                                    DOMAIN_CLOUDCONNECTOR,
                                                    SERVER_ADDRESS_NAME,
                                                    &serverIP,
                                                    sizeof(serverIP));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        SERVER_ADDRESS_NAME, ret);
        return ret;
    }

    uint32_t serverPort;
    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_CLOUDCONNECTOR,
                                         SERVER_PORT_NAME,
                                         &serverPort,
                                         sizeof(serverPort));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        SERVER_PORT_NAME, ret);
        return ret;
    }

    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_CLOUDCONNECTOR,
                                         SERVER_CA_CERT_NAME,
                                         &serverCert,
                                         sizeof(serverCert));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s failed with :%d",
                        SERVER_CA_CERT_NAME, ret);
        return ret;
    }

    Debug_LOG_DEBUG("Setting MQTT options ..." );
    MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
    ret = set_mqtt_options(&options);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("set_mqtt_options() failed with code %d", ret);
        return ret;
    }

    Debug_LOG_INFO("Setting TLS to IP:%s Port:%u ...", serverIP, serverPort);
    ret = glue_tls_init(serverIP, serverCert, sizeof(serverCert), serverPort);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("glue_tls_init() failed with code %d", ret);
        return ret;
    }

    Debug_LOG_INFO("Establishing TLS session... ");
    ret = do_tls_handshake();
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("do_tls_handshake() failed with code %d", ret);
        return ret;
    }
    Debug_LOG_INFO("TLS session established successfully");

    Debug_LOG_INFO("Establishing MQTT connection... ");
    ret = do_mqtt_connect(&self->paho.client, &options);
    if (ret != 0)
    {
        Debug_LOG_ERROR("do_mqtt_connect() failed with code %d", ret);
        return ret;
    }

    Debug_LOG_INFO("CloudConnector initialized" );

    //Unblock the cloudConnector_rpc_write
    ret = sem_post();
    if (ret != 0)
    {
        Debug_LOG_ERROR("sem_post() failed with code %d", ret);
        return ret;
    }

    return 0;
}

//------------------------------------------------------------------------------
static int handle_CC_FSM_NEW_MESSAGE(CC_FSM_t* self)
{
    const char* receivedBuf = (const char*) sensor_port;
    CC_FSM_PAHO_NetCtx_t* netCtx_server = &(self->paho.server_netCtx);

    Debug_LOG_INFO("New message received from client", __func__);

    memcpy(netCtx_server->readBuff, receivedBuf, sizeof(netCtx_server->readBuff));

    int packet_type = MQTTServer_readType(&self->paho.server);

    int ret;
    switch (packet_type)
    {
    //------------------------------------------------
    case CONNECT:
        ret = handle_MQTT_CONNECT(self);
        break;
    //------------------------------------------------
    case PINGREQ:
        ret = handle_MQTT_PINGREQ(self);
        break;
    //------------------------------------------------
    case PUBLISH:
        ret = handle_MQTT_PUBLISH(self);
        break;
    //------------------------------------------------
    case SUBSCRIBE:
        ret = handle_MQTT_SUBSCRIBE(self);
        break;
    //------------------------------------------------
    case CONNACK:
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP:
    case SUBACK:
    case UNSUBSCRIBE:
    case UNSUBACK:
    case PINGRESP:
    case DISCONNECT:
        ret = handle_MQTT_unsupportedPacket(self, packet_type);
        break;
    //------------------------------------------------
    default:
        Debug_LOG_ERROR("received invalid data, found packet type %d, abort",
                        packet_type);

        // we've received unknown data. The packet parser might be out of
        // sync now, because it reads two byte at once at least. So we
        // could be out of sync now and never re-sync.
        ret = -1;
        break;
    }

    ret = sem_post();
    if (ret != 0)
    {
        Debug_LOG_ERROR("sem_post() failed with code %d", ret);
        return ret;
    }

    return 0;
}

//==============================================================================
// public functions
//==============================================================================


//------------------------------------------------------------------------------
int CC_FSM_ctor()
{
    CC_FSM_t* self = &cc_fsm;

    // Initialize the memory in self
    memset(self, 0, sizeof(*self));

    OS_Error_t err = init_config_handle(&hConfig);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("init_config_handle() failed with: %d", err);
        return -1;
    }

    //--------------------------------------------------------------------------
    // Setup PAHO MQTT
    //--------------------------------------------------------------------------

    // setup network layer for MQTT, WAN uses TLS socket
    CC_FSM_PAHO_NetCtx_t* netCtx_client = &(self->paho.client_netCtx);

    Network* net_wan = &(netCtx_client->net);
    net_wan->mqttread  = glue_tls_mqtt_read;
    net_wan->mqttwrite = glue_tls_mqtt_write;

    MQTT_client_init(&self->paho.client,
                     net_wan,
                     PAHO_TIMEOUT_MS_COMMAND,
                     netCtx_client->sendBuff,
                     sizeof(netCtx_client->sendBuff),
                     netCtx_client->readBuff,
                     sizeof(netCtx_client->readBuff) );

    CC_FSM_PAHO_NetCtx_t* netCtx_server = &(self->paho.server_netCtx);

    Network* net_lan = &(netCtx_server->net);

    MQTTServerInit(&self->paho.server,
                   net_lan,
                   PAHO_TIMEOUT_MS_COMMAND,
                   netCtx_server->sendBuff,
                   sizeof(netCtx_server->sendBuff),
                   netCtx_server->readBuff,
                   sizeof(netCtx_server->readBuff) );

    return 0;
}

OS_Error_t
cloudConnector_rpc_write()
{
    CC_FSM_t* self = &cc_fsm;

    OS_Error_t ret = sem_wait();
    if (ret)
    {
        Debug_LOG_ERROR("Failed to wait on semaphore, error %d", ret);
    }

    ret = handle_CC_FSM_NEW_MESSAGE(self);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("handle_CC_FSM_NEW_MESSAGE() failed with %d", ret);
        return OS_ERROR_GENERIC;
    }

    Debug_LOG_INFO("Waiting for new message from client...");

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------

int run()
{
    Debug_LOG_INFO("Starting CloudConnector...");

    CC_FSM_t* self = &cc_fsm;


    int ret = CC_FSM_ctor();
    if (ret != 0)
    {
        Debug_LOG_ERROR("CC_FSM_ctor() failed with: %d", ret);
        return -1;
    }

    ret = handle_CC_FSM_INIT(self);
    if (ret != 0)
    {
        Debug_LOG_ERROR("handle_CC_FSM_INIT() failed with: %d", ret);
        return -1;
    }

    return 0;
}
