/*
   *  Main file of the LogServer component.
   *
   *  Copyright (C) 2020, Hensoldt Cyber GmbH
*/


#include "LibDebug/Debug.h"

#include "Logger/Server/OS_LoggerFile.h"

#include "Logger/Server/OS_LoggerConsumerChain.h"
#include "Logger/Server/OS_LoggerConsumer.h"

#include "Logger/Server/OS_LoggerOutputConsole.h"
#include "Logger/Server/OS_LoggerOutputFileSystem.h"

#include "OS_Filesystem.h"
#include "seos_pm_api.h"

#include <stdio.h>

#include <camkes.h>


/* Defines -------------------------------------------------------------------*/
#define DATABUFFER_SERVER_01    (void *)dataport_buf_admin
#define DATABUFFER_SERVER_02    (void *)dataport_buf_configServer
#define DATABUFFER_SERVER_03    (void *)dataport_buf_cloudConnector
#define DATABUFFER_SERVER_04    (void *)dataport_buf_sensorTemp
#define DATABUFFER_SERVER_05    (void *)dataport_buf_nwDriver
#define DATABUFFER_SERVER_06    (void *)dataport_buf_nwStack
#define DATABUFFER_SERVER_07    (void *)dataport_buf_nwStack_2
#define DATABUFFER_SERVER_08    (void *)dataport_buf_nwDriver_2

// log server id
#define LOG_SERVER_ID               0

// client id's
#define CLIENT_ADMIN_ID             10
#define CLIENT_CONFIGSRV_ID         20
#define CLIENT_CLOUDCON_ID          30
#define CLIENT_SENSORTEMP_ID        40
#define CLIENT_NWDRIVER_ID          50
#define CLIENT_NWSTACK_ID           60
#define CLIENT_NWSTACK2_ID          70
#define CLIENT_NWDRIVER2_ID         80

#define PARTITION_ID                1
#define LOG_FILENAME                "log.txt"



uint32_t API_LOG_SERVER_GET_SENDER_ID(void);



static OS_LoggerFilter_Handle_t filter_admin, filter_configSrv,
        filter_cloudCon, filter_sensorTemp, filter_nwDriver, filter_nwStack,
        filter_nwStack_2, filter_nwDriver_2;
static OS_LoggerConsumer_Handle_t log_consumer_admin, log_consumer_configSrv,
       log_consumer_cloudCon, log_consumer_sensorTemp,
       log_consumer_nwDriver, log_consumer_nwStack, log_consumer_nwStack_2,
       log_consumer_nwDriver_2;
static OS_LoggerConsumerCallback_t log_consumer_callback;
static OS_LoggerFormat_Handle_t format;
static OS_LoggerSubject_Handle_t subject;
static OS_LoggerOutput_Handle_t filesystem, console;
static OS_LoggerFile_Handle_t log_file;
static OS_LoggerEmitterCallback_Handle_t emitter_callback;
// Emitter configuration
static OS_LoggerFilter_Handle_t filter_log_server;
static OS_LoggerConsumer_Handle_t log_consumer_log_server;
static OS_LoggerSubject_Handle_t subject_log_server;
static OS_LoggerOutput_Handle_t console_log_server;
static char buf_log_server[DATABUFFER_SIZE];


static bool
filesystem_init(void)
{
    hPartition_t phandle;
    pm_disk_data_t pm_disk_data;
    pm_partition_data_t pm_partition_data;

    if (partition_manager_get_info_disk(&pm_disk_data) != SEOS_SUCCESS)
    {
        printf("Fail to get disk info!\n");
        return false;
    }

    if (partition_manager_get_info_partition(PARTITION_ID,
                                             &pm_partition_data) != SEOS_SUCCESS)
    {
        printf("Fail to get partition info: %d!\n", pm_partition_data.partition_id);
        return false;
    }

    if (OS_Filesystem_init(pm_partition_data.partition_id, 0) != SEOS_SUCCESS)
    {
        printf("Fail to init partition: %d!\n", pm_partition_data.partition_id);
        return false;
    }

    if ( (phandle = OS_Filesystem_open(pm_partition_data.partition_id)) < 0)
    {
        printf("Fail to open partition: %d!\n", pm_partition_data.partition_id);
        return false;
    }

    if (OS_Filesystem_create(
            phandle,
            FS_TYPE_FAT16,
            pm_partition_data.partition_size,
            0,  // default value: size of sector:   512
            0,  // default value: size of cluster:  512
            0,  // default value: reserved sectors count: FAT12/FAT16 = 1; FAT32 = 3
            0,  // default value: count file/dir entries: FAT12/FAT16 = 16; FAT32 = 0
            0,  // default value: count header sectors: 512
            FS_PARTITION_OVERWRITE_CREATE)
        != SEOS_SUCCESS)
    {
        printf("Fail to create filesystem on partition: %d!\n",
               pm_partition_data.partition_id);
        return false;
    }

    if (OS_Filesystem_close(phandle) != SEOS_SUCCESS)
    {
        printf("Fail to close partition: %d!\n", pm_partition_data.partition_id);
        return false;
    }

    return true;
}


void log_server_interface__init()
{
    // set up consumer chain
    OS_LoggerConsumerChain_getInstance();

    // set up log format layer
    OS_LoggerFormat_ctor(&format);

    // register objects to observe
    OS_LoggerSubject_ctor(&subject);
    // Emitter configuration
    OS_LoggerSubject_ctor(&subject_log_server);

    // set up log file
    OS_LoggerFile_ctor(&log_file, PARTITION_ID, LOG_FILENAME);

    // set up backend
    OS_LoggerOutputFileSystem_ctor(&filesystem, &format);
    OS_LoggerOutputConsole_ctor(&console, &format);
    // Emitter configuration
    OS_LoggerOutputConsole_ctor(&console_log_server, &format);

    // attach observed object to subject
    OS_LoggerSubject_attach(
        (OS_LoggerAbstractSubject_Handle_t*)&subject,
        (OS_LoggerAbstractObserver_Handle_t*)&filesystem);

    OS_LoggerSubject_attach(
        (OS_LoggerAbstractSubject_Handle_t*)&subject,
        (OS_LoggerAbstractObserver_Handle_t*)&console);

    // Emitter configuration
    OS_LoggerSubject_attach(
        (OS_LoggerAbstractSubject_Handle_t*)&subject_log_server,
        (OS_LoggerAbstractObserver_Handle_t*)&console_log_server);

    // Emitter configuration: set up registered functions layer
    OS_LoggerEmitterCallback_ctor(&emitter_callback, NULL, API_LOG_SERVER_EMIT);

    // set up log filter layer
    OS_LoggerFilter_ctor(&filter_admin,          Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_configSrv,      Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_cloudCon,       Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_sensorTemp,     Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_nwDriver,       Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_nwStack,        Debug_LOG_LEVEL_INFO);
    OS_LoggerFilter_ctor(&filter_nwStack_2,      Debug_LOG_LEVEL_INFO);
    // Emitter configuration
    OS_LoggerFilter_ctor(&filter_log_server,     Debug_LOG_LEVEL_DEBUG);

    // set up registered functions layer
    OS_LoggerConsumerCallback_ctor(&log_consumer_callback, logServer_ready_emit,
                               API_LOG_SERVER_GET_SENDER_ID, api_time_server_get_timestamp);

    // set up log consumer layer
    OS_LoggerConsumer_ctor(&log_consumer_admin,          DATABUFFER_SERVER_01, &filter_admin,          &log_consumer_callback, &subject, NULL, CLIENT_ADMIN_ID, "ADMIN");
    OS_LoggerConsumer_ctor(&log_consumer_configSrv,      DATABUFFER_SERVER_02, &filter_configSrv,      &log_consumer_callback, &subject, NULL, CLIENT_CONFIGSRV_ID, "CONFIG-SERVER");
    OS_LoggerConsumer_ctor(&log_consumer_cloudCon,       DATABUFFER_SERVER_03, &filter_cloudCon,       &log_consumer_callback, &subject, NULL, CLIENT_CLOUDCON_ID, "CLOUDCONNECTOR");
    OS_LoggerConsumer_ctor(&log_consumer_sensorTemp,     DATABUFFER_SERVER_04, &filter_sensorTemp,     &log_consumer_callback, &subject, NULL, CLIENT_SENSORTEMP_ID, "SENSOR-TEMP");
    OS_LoggerConsumer_ctor(&log_consumer_nwDriver,       DATABUFFER_SERVER_05, &filter_nwDriver,       &log_consumer_callback, &subject, NULL, CLIENT_NWDRIVER_ID, "NWDRIVER");
    OS_LoggerConsumer_ctor(&log_consumer_nwStack,        DATABUFFER_SERVER_06, &filter_nwStack,        &log_consumer_callback, &subject, NULL, CLIENT_NWSTACK_ID, "NWSTACK");
    OS_LoggerConsumer_ctor(&log_consumer_nwStack_2,      DATABUFFER_SERVER_07, &filter_nwStack_2,      &log_consumer_callback, &subject, NULL, CLIENT_NWSTACK2_ID, "NWSTACK2");
    OS_LoggerConsumer_ctor(&log_consumer_nwDriver_2,     DATABUFFER_SERVER_08, &filter_nwDriver_2,     &log_consumer_callback, &subject, NULL, CLIENT_NWDRIVER2_ID, "NWDRIVER2");

    // Emitter configuration
    OS_LoggerConsumer_ctor(&log_consumer_log_server, buf_log_server, &filter_log_server, &log_consumer_callback, &subject_log_server, &log_file, LOG_SERVER_ID, "LOG-SERVER");

    // Emitter configuration: set up log emitter layer
    OS_LoggerEmitter_getInstance(buf_log_server, &filter_log_server, &emitter_callback);

    // set up consumer chain layer
    OS_LoggerConsumerChain_append(&log_consumer_admin);
    OS_LoggerConsumerChain_append(&log_consumer_configSrv);
    OS_LoggerConsumerChain_append(&log_consumer_cloudCon);
    OS_LoggerConsumerChain_append(&log_consumer_sensorTemp);
    OS_LoggerConsumerChain_append(&log_consumer_nwDriver);
    OS_LoggerConsumerChain_append(&log_consumer_nwStack);
    OS_LoggerConsumerChain_append(&log_consumer_nwStack_2);
    // Emitter configuration
    OS_LoggerConsumerChain_append(&log_consumer_log_server);

    // create filesystem
    if(filesystem_init() == false){
        printf("Fail to init filesystem!\n");
        return;
    }

    // create log file
    OS_LoggerFile_create(&log_file);

    // start polling
    OS_LoggerConsumerChain_poll();
}

