/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include <string.h>
#include <camkes.h>

#include "init_config_backend.h"


/* Defines -------------------------------------------------------------------*/
#define PARAMETER_FILE "PARAM.BIN"
#define DOMAIN_FILE "DOMAIN.BIN"
#define STRING_FILE "STRING.BIN"
#define BLOB_FILE "BLOB.BIN"


/* Private types -------------------------------------------------------------*/

static OS_FileSystem_Config_t cfg =
{
    .type = OS_FileSystem_Type_LITTLEFS,
    .size = OS_FileSystem_STORAGE_MAX,
    .storage = OS_FILESYSTEM_ASSIGN_Storage(
        storage_rpc,
        storage_dp),
};

static
void initializeName(char* buf, size_t bufSize, char const* name)
{
    memset(buf, 0, bufSize);
    strncpy(buf, name, bufSize - 1);
}

static
OS_Error_t initializeFileBackends(OS_ConfigServiceLib_t* configLib,
                                  OS_FileSystem_Handle_t hFs)
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
                         hFs);
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
              hFs);
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
              hFs);
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
              hFs);
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
    OS_FileSystem_Handle_t hFs;

    OS_Error_t err = OS_FileSystem_init(&hFs, &cfg);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_init() failed with %d.", err);
        return err;
    }

    err = OS_FileSystem_mount(hFs);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_FileSystem_mount() failed with %d.", err);
        return err;
    }

    OS_ConfigServiceInstanceStore_t* serverInstanceStore =
        OS_ConfigService_getInstances();
    OS_ConfigServiceLib_t* configLib =
        OS_ConfigServiceInstanceStore_getInstance(serverInstanceStore, 0);

    err = initializeFileBackends(configLib, hFs);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeFileBackends() failed with: %d", err);
        return err;
    }

    return OS_SUCCESS;
}
