/*
   *  Main file of the LogServer component.
   *
   *  Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
*/


#include "lib_debug/Debug.h"
#include "TimeServer.h"

#include "Logger/Server/OS_LoggerConsumerChain.h"
#include "Logger/Server/OS_LoggerConsumer.h"

#include "Logger/Server/OS_LoggerOutputConsole.h"

#include "Logger/Client/OS_LoggerEmitter.h"

#include "custom_log_format.h"

#include <stdio.h>

#include <camkes.h>


/* Defines -------------------------------------------------------------------*/
#define DATABUFFER_SERVER_01    (void *)configServer_dp
#define DATABUFFER_SERVER_02    (void *)cloudConnector_dp
#define DATABUFFER_SERVER_03    (void *)sensorTemp_dp
#define DATABUFFER_SERVER_04    (void *)nwDriver_dp
#define DATABUFFER_SERVER_05    (void *)nwStack_dp

// log server id
#define LOG_SERVER_ID               0

// client id's
#define CLIENT_CONFIGSRV_ID         10
#define CLIENT_CLOUDCON_ID          20
#define CLIENT_SENSORTEMP_ID        30
#define CLIENT_NWDRIVER_ID          40
#define CLIENT_NWSTACK_ID           50

#define PARTITION_ID                1

uint32_t API_LOG_SERVER_GET_SENDER_ID(void);

static OS_LoggerFilter_Handle_t filter_configSrv,
       filter_cloudCon, filter_sensorTemp, filter_nwDriver, filter_nwStack;
static OS_LoggerConsumer_Handle_t log_consumer_configSrv,
       log_consumer_cloudCon, log_consumer_sensorTemp,
       log_consumer_nwDriver, log_consumer_nwStack;
static OS_LoggerConsumerCallback_t log_consumer_callback;
static OS_LoggerSubject_Handle_t subject;
static OS_LoggerOutput_Handle_t console;

// Emitter configuration
static OS_LoggerFilter_Handle_t filter_log_server;
static OS_LoggerConsumer_Handle_t log_consumer_log_server;
static OS_LoggerSubject_Handle_t subject_log_server;
static OS_LoggerOutput_Handle_t console_log_server;
static char buf_log_server[DATABUFFER_SIZE];

static const if_OS_Timer_t timer =
    IF_OS_TIMER_ASSIGN(
        timeServer_rpc,
        timeServer_notify);

// Private functions -----------------------------------------------------------

static uint64_t
get_time_sec(
    void)
{
    OS_Error_t err;
    uint64_t sec;

    if ((err = TimeServer_getTime(&timer, TimeServer_PRECISION_SEC,
                                  &sec)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("TimeServer_getTime() failed with %d", err);
        sec = 0;
    }

    return sec;
}

// Public functions ------------------------------------------------------------

void pre_init(void)
{
    // set up consumer chain
    OS_LoggerConsumerChain_getInstance();

    // register objects to observe
    OS_LoggerSubject_ctor(&subject);
    // Emitter configuration
    OS_LoggerSubject_ctor(&subject_log_server);

    // set up backend
    OS_LoggerOutputConsole_ctor(&console, &custom_log_format);
    // Emitter configuration
    OS_LoggerOutputConsole_ctor(&console_log_server, &custom_log_format);

    // attach observed object to subject
    OS_LoggerSubject_attach(
        (OS_LoggerAbstractSubject_Handle_t*)&subject,
        &console);

    // Emitter configuration
    OS_LoggerSubject_attach(
        (OS_LoggerAbstractSubject_Handle_t*)&subject_log_server,
        &console_log_server);

    // set up log filter layer
    OS_LoggerFilter_ctor(&filter_configSrv,      Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_cloudCon,       Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_sensorTemp,     Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_nwDriver,       Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_nwStack,        Debug_LOG_LEVEL_INFO);
    // Emitter configuration
    OS_LoggerFilter_ctor(&filter_log_server,     Debug_LOG_LEVEL_INFO);

    // set up registered functions layer
    OS_LoggerConsumerCallback_ctor(
        &log_consumer_callback,
        API_LOG_SERVER_GET_SENDER_ID,
        get_time_sec);

    // set up log consumer layer
    OS_LoggerConsumer_ctor(&log_consumer_configSrv,      DATABUFFER_SERVER_01,
                           &filter_configSrv,      &log_consumer_callback, &subject, NULL,
                           CLIENT_CONFIGSRV_ID, "CONFIG-SERVER");
    OS_LoggerConsumer_ctor(&log_consumer_cloudCon,       DATABUFFER_SERVER_02,
                           &filter_cloudCon,       &log_consumer_callback, &subject, NULL,
                           CLIENT_CLOUDCON_ID, "CLOUDCONNECTOR");
    OS_LoggerConsumer_ctor(&log_consumer_sensorTemp,     DATABUFFER_SERVER_03,
                           &filter_sensorTemp,     &log_consumer_callback, &subject, NULL,
                           CLIENT_SENSORTEMP_ID, "SENSOR-TEMP");
    OS_LoggerConsumer_ctor(&log_consumer_nwDriver,       DATABUFFER_SERVER_04,
                           &filter_nwDriver,       &log_consumer_callback, &subject, NULL,
                           CLIENT_NWDRIVER_ID, "NWDRIVER");
    OS_LoggerConsumer_ctor(&log_consumer_nwStack,        DATABUFFER_SERVER_05,
                           &filter_nwStack,        &log_consumer_callback, &subject, NULL,
                           CLIENT_NWSTACK_ID, "NWSTACK");

    // Emitter configuration
    OS_LoggerConsumer_ctor(&log_consumer_log_server, buf_log_server,
                           &filter_log_server, &log_consumer_callback, &subject_log_server, NULL,
                           LOG_SERVER_ID, "LOG-SERVER");

    // Emitter configuration: set up log emitter layer
    OS_LoggerEmitter_getInstance(
        buf_log_server,
        &filter_log_server,
        API_LOG_SERVER_EMIT);

    // set up consumer chain layer
    OS_LoggerConsumerChain_append(&log_consumer_configSrv);
    OS_LoggerConsumerChain_append(&log_consumer_cloudCon);
    OS_LoggerConsumerChain_append(&log_consumer_sensorTemp);
    OS_LoggerConsumerChain_append(&log_consumer_nwDriver);
    OS_LoggerConsumerChain_append(&log_consumer_nwStack);
    // Emitter configuration
    OS_LoggerConsumerChain_append(&log_consumer_log_server);
}
