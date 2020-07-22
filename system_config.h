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

#define Debug_Config_LOG_LEVEL                  Debug_LOG_LEVEL_INFO
#define Debug_Config_INCLUDE_LEVEL_IN_MSG
#define Debug_Config_LOG_WITH_FILE_LINE


//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------
#define Memory_Config_USE_STDLIB_ALLOC


//-----------------------------------------------------------------------------
// StorageServer
//-----------------------------------------------------------------------------
#define CONFIGSERVER_STORAGE_OFFSET 0
#define CONFIGSERVER_STORAGE_SIZE   (1024*1024)

#define LOGSERVER_STORAGE_OFFSET    (1024*1024)
#define LOGSERVER_STORAGE_SIZE      (1024*1024)


//-----------------------------------------------------------------------------
// ChanMUX
//-----------------------------------------------------------------------------

#define CHANMUX_CHANNEL_NIC_CTRL      4
#define CHANMUX_CHANNEL_NIC_DATA      5
#define CHANMUX_CHANNEL_NVM           6


//-----------------------------------------------------------------------------
// ChanMUX clients
//-----------------------------------------------------------------------------

#define CHANMUX_ID_NIC        101
#define CHANMUX_ID_STORAGE    102


//-----------------------------------------------------------------------------
// LOGGER
//-----------------------------------------------------------------------------

#if !defined(CAMKES_TOOL_PROCESSING)

// If the belows header is included in the config file and Logger library is
// linked, the LibDebug will forward entries to the LogServer.
#include "Logger/Client/OS_Logger.h"

// api interface name
#define API_LOG_SERVER_EMIT                     logServer_rpc_emit
#define API_LOG_SERVER_GET_SENDER_ID            logServer_rpc_get_sender_id
#define API_LOG_SERVER_READ_LOG_FILE            logServer_rpc_read_log_file

#endif // !defined(CAMKES_TOOL_PROCESSING)


//-----------------------------------------------------------------------------
// IDS
//-----------------------------------------------------------------------------

#define CONFIGSERVER_LOGGER_ID      10
#define CLOUDCONNECTOR_LOGGER_ID    20
#define SENSOR_LOGGER_ID            30
#define NWDRIVER_LOGGER_ID          40
#define NWSTACK_LOGGER_ID           50

#define CONFIGSERVER_STORAGE_ID     1
#define LOGGER_STORAGE_ID           2

#define NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS 16
#define NIC_DRIVER_RINGBUFFER_SIZE                                             \
    (NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS * 4096)

