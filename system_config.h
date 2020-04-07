/**
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 *
 * System libraries configurations
 *
 */
#pragma once


//-----------------------------------------------------------------------------
// Debug
//-----------------------------------------------------------------------------
#if !defined(NDEBUG)
#   define Debug_Config_STANDARD_ASSERT
#   define Debug_Config_ASSERT_SELF_PTR
#else
#   define Debug_Config_DISABLE_ASSERT
#   define Debug_Config_NO_ASSERT_SELF_PTR
#endif

#define Debug_Config_LOG_LEVEL                  Debug_LOG_LEVEL_DEBUG
#define Debug_Config_INCLUDE_LEVEL_IN_MSG
#define Debug_Config_LOG_WITH_FILE_LINE


//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------
#define Memory_Config_USE_STDLIB_ALLOC


//-----------------------------------------------------------------------------
// ChanMUX
//-----------------------------------------------------------------------------

enum
{
    CHANMUX_CHANNEL_UNUSED_0,   // 0
    CHANMUX_CHANNEL_UNUSED_1,   // 1
    CHANMUX_CHANNEL_UNUSED_2,   // 2
    CHANMUX_CHANNEL_UNUSED_3,   // 3
    CHANMUX_CHANNEL_NIC_CTRL,   // 4
    CHANMUX_CHANNEL_NIC_DATA,   // 5
    CHANMUX_CHANNEL_MAIN_DATA,  // 6
    CHANMUX_CHANNEL_UNUSED_7,   // 7
    CHANMUX_CHANNEL_UNUSED_8,   // 8

    CHANMUX_NUM_CHANNELS        // 9
};


//-----------------------------------------------------------------------------
// COMMON
//-----------------------------------------------------------------------------
#define DATABUFFER_SIZE                         4096


//-----------------------------------------------------------------------------
// FILESYSTEM
//-----------------------------------------------------------------------------
// Max. partition per disk
#define PARTITION_COUNT                         10

// Max. file handle per partition
#define FILE_HANDLE_COUNT                       50

// FAT config
#define FILE_DIR_ENTRY_COUNT                    16      // only for (FAT12/FAT16)
#define FS_HEADER_SECTOR_COUNT                  1

#define CLUSTER_SIZE_FAT                        0x200   // size of cluster = 512 Byte
#define OFFSET_SECTORS_COUNT_FAT                3


//-----------------------------------------------------------------------------
// PARTITION MANAGER
//-----------------------------------------------------------------------------
typedef struct
{
    const char* partition_name;
    int partition_size;
    int block_size;
} Partition_config_t;

typedef struct
{
    Partition_config_t partition[2];
} Partition_cat_t;

static const Partition_cat_t partition_conf =
{
    .partition[0].partition_name = "",
    .partition[0].partition_size = 0x300000,
    .partition[0].block_size = 512,
    .partition[1].partition_name = "",
    .partition[1].partition_size = 0x300000,
    .partition[1].block_size = 512
};


// internal defines
#define PM_CONF_ARRAY_SIZE(x)                   (sizeof(x)/sizeof(x[0]))

#define PARTITION_CONFIGURATION_AT(x)           partition_conf.partition[x]

#define GET_PROPERTY_PARTITION_NAME_AT(x)       PARTITION_CONFIGURATION_AT(x).partition_name
#define GET_PROPERTY_PARTITION_SIZE_AT(x)       PARTITION_CONFIGURATION_AT(x).partition_size
#define GET_PROPERTY_BLOCK_SIZE_AT(x)           PARTITION_CONFIGURATION_AT(x).block_size

// setup disk/partition
#define GET_PROPERTY_PARTITION_COUNT            PM_CONF_ARRAY_SIZE(partition_conf.partition)
#define GET_PROPERTY_PARTITION_NAME(x)          GET_PROPERTY_PARTITION_NAME_AT(x)
#define GET_PROPERTY_PARTITION_SIZE(x)          GET_PROPERTY_PARTITION_SIZE_AT(x)
#define GET_PROPERTY_BLOCK_SIZE(x)              GET_PROPERTY_BLOCK_SIZE_AT(x)

// setup partition manager dataport
#define GET_PROPERTY_PM_DATAPORT_BUFFER         (void *)pm_dataport_buf

//-----------------------------------------------------------------------------
// LOGGER
//-----------------------------------------------------------------------------

// If the belows header is included in the config file and Logger library is
// linked, the LibDebug will forward entries to the LogServer.
#include "Logger/Client/OS_Logger.h"

// api interface name
#define API_LOG_SERVER_EMIT                     log_server_interface_emit
#define API_LOG_SERVER_GET_SENDER_ID            log_server_interface_get_sender_id
#define API_LOG_SERVER_READ_LOG_FILE            log_server_interface_read_log_file


//-----------------------------------------------------------------------------
// Demo component configuration parameters
//-----------------------------------------------------------------------------

//Sensor Component
#define DOMAIN_SENSOR           "Domain-Sensor"

#define MQTT_PAYLOAD_NAME       "MQTT_Payload" // _NAME defines are stored together with the values in the config file
#define MQTT_PAYLOAD_VALUE      "Welcome to TRENTOS-M"

#define MQTT_TOPIC_NAME         "MQTT_Topic"
#define MQTT_TOPIC_VALUE        "devices/"CLOUD_DEVICE_ID_VALUE"/messages/events/"


//CloudConnector Component
#define DOMAIN_CLOUDCONNECTOR   "Domain-CloudConnector"

#define CLOUD_DEVICE_ID_NAME    "IoT-Device"
#define CLOUD_DEVICE_ID_VALUE   "trentos15"

#define CLOUD_DOMAIN_NAME       "IoT-Hub"
#define CLOUD_DOMAIN_VALUE      "mothership.azure-devices.net/"CLOUD_DEVICE_ID_VALUE"/?api-version=2018-06-30"

#define SERVER_ADDRESS_NAME     "CloudIP"
#define SERVER_ADDRESS_VALUE    "40.113.153.50" // As string in the format "XXX.XXX.XXX.XXX"

#define CLOUD_SAS_NAME          "SharedAccessSignature"
#define CLOUD_SAS_VALUE         "SharedAccessSignature sr=mothership.azure-devices.net%2Fdevices%2Ftrentos15&sig=rfhTYo%2FP7qYcwvsDsPD%2FriuJ4T%2FuN0V32MAa0UheOdc%3D&se=1586340947"  // SharedAccessSignature sr=...

#define SERVER_PORT_NAME        "ServerPort"
#define SERVER_PORT_VALUE       8883

/* The server cert can depend on the cloud server location your Hub ressource is
running on. The Azure Region "West Europe" works with the TLS_CA_4 already provided
here.
*/
#define SERVER_CA_CERT_NAME     "ServerCaCert"
#define Microsoft_IT_TLS_CA_4_PEM                                         \
    "-----BEGIN CERTIFICATE-----\r\n"                                     \
    "MIIFtDCCBJygAwIBAgIQC2qzsD6xqfbEYJJqqM3+szANBgkqhkiG9w0BAQsFADBa\r\n" \
    "MQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl\r\n" \
    "clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTE2\r\n" \
    "MDUyMDEyNTIzOFoXDTI0MDUyMDEyNTIzOFowgYsxCzAJBgNVBAYTAlVTMRMwEQYD\r\n" \
    "VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy\r\n" \
    "b3NvZnQgQ29ycG9yYXRpb24xFTATBgNVBAsTDE1pY3Jvc29mdCBJVDEeMBwGA1UE\r\n" \
    "AxMVTWljcm9zb2Z0IElUIFRMUyBDQSA0MIICIjANBgkqhkiG9w0BAQEFAAOCAg8A\r\n" \
    "MIICCgKCAgEAq+XrXaNrOZ71NIgSux1SJl19CQvGeY6rtw7fGbLd7g/27vRW5Ebi\r\n" \
    "kg/iZwvjHHGk1EFztMuZFo6/d32wrx5s7XEuwwh3Sl6Sruxa0EiB0MXpoPV6jx6N\r\n" \
    "XtOtksDaxpE1MSC5OQTNECo8lx0AnpkYGAnPS5fkyfwA8AxanTboskDBSqyEKKo9\r\n" \
    "Rhgrp4qs9K9LqH5JQsdiIMDmpztd65Afu4rYnJDjOrFswpTOPjJry3GzQS65xeFd\r\n" \
    "2FkngvvhSA1+6ATx+QEnQfqUWn3FMLu2utcRm4j6AcxuS5K5+Hg8y5xomhZmiNCT\r\n" \
    "sCqDLpcRHX6BIGHksLmbnG5TlZUixtm9dRC62XWMPD8d0Jb4M0V7ex9UM+VIl6cF\r\n" \
    "JKLb0dyVriAqfZaJSHuSetAksd5IEfdnPLTf+Fhg9U97NGjm/awmCLbzLEPbT8QW\r\n" \
    "0JsMcYexB2uG3Y+gsftm2tjL6fLwZeWO2BzqL7otZPFe0BtQsgyFSs87yC4qanWM\r\n" \
    "wK5c2enAfH182pzjvUqwYAeCK31dyBCvLmKM3Jr94dm5WUiXQhrDUIELH4Mia+Sb\r\n" \
    "vCkigv2AUVx1Xw41wt1/L3pnnz2OW4y7r530zAz7qB+dIcHz51IaXc4UV21QuEnu\r\n" \
    "sQsn0uJpJxJuxsAmPuekKxuLUzgG+hqHOuBLf5kWTlk9WWnxcadlZRsCAwEAAaOC\r\n" \
    "AUIwggE+MB0GA1UdDgQWBBR6e4zBz+egyhzUa/r74TPDDxqinTAfBgNVHSMEGDAW\r\n" \
    "gBTlnVkwgkdYzKz6CFQ2hns6tQRN8DASBgNVHRMBAf8ECDAGAQH/AgEAMA4GA1Ud\r\n" \
    "DwEB/wQEAwIBhjAnBgNVHSUEIDAeBggrBgEFBQcDAQYIKwYBBQUHAwIGCCsGAQUF\r\n" \
    "BwMJMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGln\r\n" \
    "aWNlcnQuY29tMDoGA1UdHwQzMDEwL6AtoCuGKWh0dHA6Ly9jcmwzLmRpZ2ljZXJ0\r\n" \
    "LmNvbS9PbW5pcm9vdDIwMjUuY3JsMD0GA1UdIAQ2MDQwMgYEVR0gADAqMCgGCCsG\r\n" \
    "AQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMA0GCSqGSIb3DQEB\r\n" \
    "CwUAA4IBAQAR/nIGOiEKN27I9SkiAmKeRQ7t+gaf77+eJDUX/jmIsrsB4Xjf0YuX\r\n" \
    "/bd38YpyT0k66LMp13SH5LnzF2CHiJJVgr3ZfRNIfwaQOolm552W95XNYA/X4cr2\r\n" \
    "du76mzVIoZh90pMqT4EWx6iWu9El86ZvUNoAmyqo9DUA4/0sO+3lFZt/Fg/Hjsk2\r\n" \
    "IJTwHQG5ElBQmYHgKEIsjnj/7cae1eTK6aCqs0hPpF/kixj/EwItkBE2GGYoOiKa\r\n" \
    "3pXxWe6fbSoXdZNQwwUS1d5ktLa829d2Wf6l1uVW4f5GXDuK+OwO++8SkJHOIBKB\r\n" \
    "ujxS43/jQPQMQSBmhxjaMmng9tyPKPK9\r\n"                                 \
    "-----END CERTIFICATE-----\r\n"


//NwStack
#define DOMAIN_NWSTACK          "Domain-NwStack"

#define ETH_ADDR                "ETH_ADDR"
#define ETH_ADDR_VALUE          "10.0.0.11"

#define ETH_GATEWAY_ADDR        "ETH_GATEWAY_ADDR"
#define ETH_GATEWAY_ADDR_VALUE  "10.0.0.1"

#define ETH_SUBNET_MASK         "ETH_SUBNET_MASK"
#define ETH_SUBNET_MASK_VALUE   "255.255.255.0"