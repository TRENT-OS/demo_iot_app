/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "init_config_backend.h"


/* Defines -------------------------------------------------------------------*/
#define PARTITION_ID            0

#define PARAMETER_FILE "PARAM.BIN"
#define DOMAIN_FILE "DOMAIN.BIN"
#define STRING_FILE "STRING.BIN"
#define BLOB_FILE "BLOB.BIN"


/* Private types -------------------------------------------------------------*/
hPartition_t phandle;
OS_PartitionManagerDataTypes_PartitionData_t pm_partition_data;

static
void initializeName(char* buf, size_t bufSize, char const* name)
{
    memset(buf, 0, bufSize);
    strncpy(buf, name, bufSize - 1);
}

static
OS_Error_t initializeFileBackends(OS_ConfigServiceLib_t* configLib,
                                  hPartition_t phandle)
{
    OS_ConfigServiceBackend_t parameterBackend;
    OS_ConfigServiceBackend_t domainBackend;
    OS_ConfigServiceBackend_t stringBackend;
    OS_ConfigServiceBackend_t blobBackend;
    OS_ConfigServiceBackend_FileName_t name;

    Debug_LOG_INFO("Initializing file backends...");

    // Initialize the backends in the config library object.
    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, DOMAIN_FILE);
    OS_Error_t err = OS_ConfigServiceBackend_initializeFileBackend(
                         &domainBackend,
                         name,
                         phandle);
    Debug_LOG_DEBUG("Domain name: %s", name.buffer);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_initializeFileBackend() for file %s failed with: %d",
                        name.buffer, err);
        return err;
    }

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN,
                   PARAMETER_FILE);
    err = OS_ConfigServiceBackend_initializeFileBackend(
              &parameterBackend,
              name,
              phandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_initializeFileBackend() for file %s failed with: %d",
                        name.buffer, err);
        return err;
    }
    Debug_LOG_DEBUG("Parameter backend initialized.");

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, STRING_FILE);
    err = OS_ConfigServiceBackend_initializeFileBackend(
              &stringBackend,
              name,
              phandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_initializeFileBackend() for file %s failed with: %d",
                        name.buffer, err);
        return err;
    }
    Debug_LOG_DEBUG("String backend initialized.");

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, BLOB_FILE);
    err = OS_ConfigServiceBackend_initializeFileBackend(
              &blobBackend,
              name,
              phandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_initializeFileBackend() for file %s failed with: %d",
                        name.buffer, err);
        return err;
    }
    Debug_LOG_DEBUG("Blob backend initialized.");

    err = OS_ConfigServiceLib_Init(
              configLib,
              &parameterBackend,
              &domainBackend,
              &stringBackend,
              &blobBackend);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceLib_Init() failed with: %d", err);
        return err;
    }

    Debug_LOG_INFO("File backends initialized.");

    return OS_SUCCESS;
}

OS_Error_t
init_system_config_backend(void)
{
    OS_Error_t pm_result = OS_PartitionManager_getInfoPartition(PARTITION_ID,
                                                     &pm_partition_data);
    if (pm_result != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to get partition info: %d!",
                        pm_partition_data.partition_id);
        return OS_ERROR_GENERIC;
    }

    OS_Error_t fs_result = OS_Filesystem_init(pm_partition_data.partition_id, 0);
    if (fs_result != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to init partition: %d!", fs_result);
        return fs_result;
    }

    if ( (phandle = OS_Filesystem_open(pm_partition_data.partition_id)) < 0)
    {
        Debug_LOG_ERROR("Fail to open partition: %d!", pm_partition_data.partition_id);
        return OS_ERROR_GENERIC;
    }

    if (OS_Filesystem_mount(phandle) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to mount filesystem on partition: %d!",
                        pm_partition_data.partition_id);
        return OS_ERROR_GENERIC;
    }

    OS_ConfigServiceInstanceStore_t* serverInstanceStore =
        OS_ConfigService_getInstances();
    OS_ConfigServiceLib_t* configLib =
        OS_ConfigServiceInstanceStore_getInstance(serverInstanceStore, 0);

    OS_Error_t err = initializeFileBackends(configLib, phandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeFileBackends() failed with: %d", err);
        return err;
    }

    return OS_SUCCESS;
}
