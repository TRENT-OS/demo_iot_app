/*
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include "create_config_backend.h"


/* Defines -------------------------------------------------------------------*/
#define PARTITION_ID            0

#define PARAMETER_FILE "PARAM.BIN"
#define DOMAIN_FILE "DOMAIN.BIN"
#define STRING_FILE "STRING.BIN"
#define BLOB_FILE "BLOB.BIN"


/* Private types -------------------------------------------------------------*/
hPartition_t phandle;
pm_disk_data_t pm_disk_data;
pm_partition_data_t pm_partition_data;


static
void initializeName(char* buf, size_t bufSize, char const* name)
{
    memset(buf, 0, bufSize);
    strncpy(buf, name, bufSize - 1);
}

static
void
initializeDomain(OS_ConfigServiceLibTypes_Domain_t* domain, char const* name)
{
    initializeName(domain->name.name, OS_CONFIG_LIB_DOMAIN_NAME_LEN, name);
    domain->enumerator.index = 0;
}

static
seos_err_t
OS_ConfigService_writeVariableLengthBlob(
    OS_ConfigServiceBackend_t* backend,
    uint32_t index,
    uint32_t numberOfBlocks,
    void const* buffer,
    size_t bufferLength)
{
    size_t blobBlockSize = OS_ConfigServiceBackend_getSizeOfRecords(backend);
    size_t blobCapacity = blobBlockSize * numberOfBlocks;

    if (bufferLength > blobCapacity)
    {
        Debug_LOG_DEBUG("Error: function: %s - line: %d\n", __FUNCTION__, __LINE__);
        return SEOS_ERROR_GENERIC;
    }

    // We anticipate a maximum size here which should be ok to place on the stack.
    char tmpBuf[OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH];
    size_t bytesCopied = 0;

    while (bytesCopied < bufferLength)
    {
        size_t bytesToCopy;

        if ((bufferLength - bytesCopied) >= blobBlockSize)
        {
            bytesToCopy = blobBlockSize;
        }
        else
        {
            bytesToCopy = bufferLength - bytesCopied;
        }

        memcpy(tmpBuf, (char*)buffer + bytesCopied, bytesToCopy);

        seos_err_t fetchResult = OS_ConfigServiceBackend_writeRecord(
                                     backend,
                                     index,
                                     tmpBuf,
                                     sizeof(tmpBuf));

        if (SEOS_SUCCESS != fetchResult)
        {
            Debug_LOG_DEBUG("Error: function: %s - line: %d\n", __FUNCTION__, __LINE__);
            return SEOS_ERROR_GENERIC;
        }

        bytesCopied += bytesToCopy;
        index++;
    }

    return SEOS_SUCCESS;
}

seos_err_t
initializeDomainsAndParameters(OS_ConfigServiceLib_t* configLib)
{
    int result;

    Debug_LOG_INFO("Initializing Domains and Parameters...");

    // initialize CloudConnector Domain
    Debug_LOG_INFO("Initializing %s...", DOMAIN_SENSOR);
    OS_ConfigServiceLibTypes_Domain_t domain;
    initializeDomain(&domain, DOMAIN_SENSOR);
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->domainBackend,
                 0,
                 &domain,
                 sizeof(domain));
    if (result != 0)
    {
        return result;
    }

    // Initialize the parameters
    OS_ConfigServiceLibTypes_Parameter_t parameter;
    OS_ConfigServiceAccessRights_SetAll(&parameter.readAccess);
    OS_ConfigServiceAccessRights_SetAll(&parameter.writeAccess);

    // Initialize an array large enough to store the biggest blob of the config
    char largeBlob[3072];
    memset(largeBlob, 0, sizeof(largeBlob));

    /* MQTT Message Payload  -------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", MQTT_PAYLOAD_NAME, DOMAIN_SENSOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_BLOB;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   MQTT_PAYLOAD_NAME);
    parameter.parameterValue.valueBlob.index = 0;
    parameter.parameterValue.valueBlob.numberOfBlocks = 2;
    parameter.parameterValue.valueBlob.size =
        (OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH *
         parameter.parameterValue.valueBlob.numberOfBlocks);
    parameter.domain.index = 0;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 0,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        Debug_LOG_ERROR("SeosConfigBackend_writeRecord1 failed with: %d\n", result);
        return result;
    }
    result = OS_ConfigService_writeVariableLengthBlob(
                 &configLib->blobBackend,
                 parameter.parameterValue.valueBlob.index,
                 parameter.parameterValue.valueBlob.numberOfBlocks,
                 largeBlob,
                 parameter.parameterValue.valueBlob.size);
    if (result != 0)
    {
        return result;
    }

    /* MQTT Message Topic  ---------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", MQTT_TOPIC_NAME, DOMAIN_SENSOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_BLOB;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   MQTT_TOPIC_NAME);
    parameter.parameterValue.valueBlob.index = 2;
    parameter.parameterValue.valueBlob.numberOfBlocks = 2;
    parameter.parameterValue.valueBlob.size =
        (OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH *
         parameter.parameterValue.valueBlob.numberOfBlocks);
    parameter.domain.index = 0;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 1,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        Debug_LOG_ERROR("SeosConfigBackend_writeRecord1 failed with: %d\n", result);
        return result;
    }
    result = OS_ConfigService_writeVariableLengthBlob(
                 &configLib->blobBackend,
                 parameter.parameterValue.valueBlob.index,
                 parameter.parameterValue.valueBlob.numberOfBlocks,
                 largeBlob,
                 parameter.parameterValue.valueBlob.size);
    if (result != 0)
    {
        return result;
    }

    // initialize CloudConnector Domain
    Debug_LOG_INFO("Initializing %s", DOMAIN_CLOUDCONNECTOR);
    initializeDomain(&domain, DOMAIN_CLOUDCONNECTOR);
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->domainBackend,
                 1,
                 &domain,
                 sizeof(domain));
    if (result != 0)
    {
        return result;
    }

    /* Server Port  ----------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", SERVER_PORT_NAME, DOMAIN_CLOUDCONNECTOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_INTEGER32;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   SERVER_PORT_NAME);
    parameter.parameterValue.valueInteger32 = 0;
    parameter.domain.index = 1;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 2,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }

    /* Cloud SharedAccessSignature -------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", CLOUD_SAS_NAME, DOMAIN_CLOUDCONNECTOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_BLOB;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   CLOUD_SAS_NAME);
    parameter.parameterValue.valueBlob.index = 4;
    parameter.parameterValue.valueBlob.numberOfBlocks = 3;
    parameter.parameterValue.valueBlob.size =
        (OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH *
         parameter.parameterValue.valueBlob.numberOfBlocks);
    parameter.domain.index = 1;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 3,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigService_writeVariableLengthBlob(
                 &configLib->blobBackend,
                 parameter.parameterValue.valueBlob.index,
                 parameter.parameterValue.valueBlob.numberOfBlocks,
                 largeBlob,
                 parameter.parameterValue.valueBlob.size);
    if (result != 0)
    {
        return result;
    }

    /* Azure Cloud Domain ----------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", CLOUD_DOMAIN_NAME, DOMAIN_CLOUDCONNECTOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_BLOB;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   CLOUD_DOMAIN_NAME);

    parameter.parameterValue.valueBlob.index = 7;
    parameter.parameterValue.valueBlob.numberOfBlocks = 1;
    parameter.parameterValue.valueBlob.size =
        (OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH *
         parameter.parameterValue.valueBlob.numberOfBlocks);
    parameter.domain.index = 1;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 4,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigService_writeVariableLengthBlob(
                 &configLib->blobBackend,
                 parameter.parameterValue.valueBlob.index,
                 parameter.parameterValue.valueBlob.numberOfBlocks,
                 largeBlob,
                 parameter.parameterValue.valueBlob.size);
    if (result != 0)
    {
        return result;
    }

    /* Server Address --------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", SERVER_ADDRESS_NAME, DOMAIN_CLOUDCONNECTOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   SERVER_ADDRESS_NAME);

    char str[OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH];
    memset(str, 0, sizeof(str));

    parameter.parameterValue.valueString.index = 0;
    parameter.parameterValue.valueString.size =
        OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH;
    parameter.domain.index = 1;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 5,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->stringBackend,
                 parameter.parameterValue.valueString.index,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }

    /* Device Name -----------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", CLOUD_DEVICE_ID_NAME, DOMAIN_CLOUDCONNECTOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   CLOUD_DEVICE_ID_NAME);

    memset(str, 0, sizeof(str));

    parameter.parameterValue.valueString.index = 1;
    parameter.parameterValue.valueString.size =
        OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH;
    parameter.domain.index = 1;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 6,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->stringBackend,
                 parameter.parameterValue.valueString.index,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }

    /* TLS Cert --------------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", SERVER_CA_CERT_NAME, DOMAIN_CLOUDCONNECTOR);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_BLOB;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   SERVER_CA_CERT_NAME);
    parameter.parameterValue.valueBlob.index = 8;
    parameter.parameterValue.valueBlob.numberOfBlocks = 48;
    parameter.parameterValue.valueBlob.size =
        (OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH *
         parameter.parameterValue.valueBlob.numberOfBlocks);
    parameter.domain.index = 1;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 7,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigService_writeVariableLengthBlob(
                 &configLib->blobBackend,
                 parameter.parameterValue.valueBlob.index,
                 parameter.parameterValue.valueBlob.numberOfBlocks,
                 largeBlob,
                 parameter.parameterValue.valueBlob.size);
    if (result != 0)
    {
        return result;
    }

    // initialize NwStack Domain
    Debug_LOG_INFO("Initializing %s", DOMAIN_NWSTACK);
    initializeDomain(&domain, DOMAIN_NWSTACK);
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->domainBackend,
                 2,
                 &domain,
                 sizeof(domain));
    if (result != 0)
    {
        return result;
    }

    // Initialize the parameters
    Debug_LOG_INFO("Initializing %s in %s...", ETH_ADDR, DOMAIN_NWSTACK);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   ETH_ADDR);

    parameter.parameterValue.valueString.index = 2;
    parameter.parameterValue.valueString.size =
        OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH;
    parameter.domain.index = 2;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 8,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->stringBackend,
                 parameter.parameterValue.valueString.index,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }

    Debug_LOG_INFO("Initializing %s in %s...", ETH_GATEWAY_ADDR, DOMAIN_NWSTACK);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   ETH_GATEWAY_ADDR);

    parameter.parameterValue.valueString.index = 3;
    parameter.parameterValue.valueString.size =
        OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH;
    parameter.domain.index = 2;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 9,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->stringBackend,
                 parameter.parameterValue.valueString.index,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }

    Debug_LOG_INFO("Initializing %s in %s...", ETH_SUBNET_MASK, DOMAIN_NWSTACK);
    parameter.parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter.parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   ETH_SUBNET_MASK);

    parameter.parameterValue.valueString.index = 4;
    parameter.parameterValue.valueString.size =
        OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH;
    parameter.domain.index = 2;
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->parameterBackend,
                 10,
                 &parameter,
                 sizeof(parameter));
    if (result != 0)
    {
        return result;
    }
    result = OS_ConfigServiceBackend_writeRecord(
                 &configLib->stringBackend,
                 parameter.parameterValue.valueString.index,
                 str,
                 sizeof(str));
    if (result != 0)
    {
        return result;
    }

    Debug_LOG_INFO("Domains and parameters initialized.");

    return 0;
}

seos_err_t
create_system_config_backend(void)
{

    seos_err_t pm_result = partition_manager_get_info_disk(&pm_disk_data);
    if (pm_result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to get disk info: %d", pm_result);
        return SEOS_ERROR_GENERIC;
    }

    pm_result = partition_manager_get_info_partition(PARTITION_ID,
                                                     &pm_partition_data);
    if (pm_result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to get partition info: %d!",
                        pm_partition_data.partition_id);
        return SEOS_ERROR_GENERIC;
    }

    seos_err_t fs_result = OS_Filesystem_init(pm_partition_data.partition_id, 0);
    if (fs_result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to init partition: %d!", fs_result);
        return fs_result;
    }

    if ( (phandle = OS_Filesystem_open(pm_partition_data.partition_id)) < 0)
    {
        Debug_LOG_ERROR("Fail to open partition: %d!", pm_partition_data.partition_id);
        return SEOS_ERROR_GENERIC;
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
        Debug_LOG_ERROR("Fail to create filesystem on partition: %d!",
                        pm_partition_data.partition_id);
        return SEOS_ERROR_GENERIC;
    }

    if (OS_Filesystem_mount(phandle) != SEOS_SUCCESS)
    {
        return SEOS_ERROR_GENERIC;
    }

    OS_ConfigServiceInstanceStore_t* serverInstanceStore =
        OS_ConfigService_getInstances();
    OS_ConfigServiceLib_t* configLib =
        OS_ConfigServiceInstanceStore_getInstance(serverInstanceStore, 0);

    // Create the file backends
    Debug_LOG_INFO("Setting up filesystem backend...");

    seos_err_t result = createFileBackends(phandle);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("createFileBackends failed with: %d", result);
        return result;
    }

    result = initializeFileBackends(configLib, phandle);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeFileBackends failed with: %d", result);
        return result;
    }

    // Create the empty parameters in the instance.
    result = initializeDomainsAndParameters(configLib);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeDomainsAndParameters failed with: %d\n", result);
        return result;
    }

    return SEOS_SUCCESS;
}

seos_err_t
createFileBackends(hPartition_t phandle)
{
    seos_err_t result = 0;
    OS_ConfigServiceBackend_FileName_t name;

    // Create the file backends.
    Debug_LOG_DEBUG("Size of ConfigLib_Domain: %d", sizeof(OS_ConfigServiceLibTypes_Domain_t));
    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, DOMAIN_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    result = OS_ConfigServiceBackend_createFileBackend(name, phandle, 3,
                                                 sizeof(OS_ConfigServiceLibTypes_Domain_t));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    Debug_LOG_DEBUG("Size of ConfigLib_Parameter: %d",
                    sizeof(OS_ConfigServiceLibTypes_Parameter_t));
    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN,
                   PARAMETER_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    result = OS_ConfigServiceBackend_createFileBackend(name, phandle, 11,
                                                 sizeof(OS_ConfigServiceLibTypes_Parameter_t));
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, STRING_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    result = OS_ConfigServiceBackend_createFileBackend(name, phandle, 5,
                                                 OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, BLOB_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    result = OS_ConfigServiceBackend_createFileBackend(name, phandle, 56,
                                                 OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    Debug_LOG_INFO("File backends created.");

    return SEOS_SUCCESS;
}

seos_err_t initializeFileBackends(OS_ConfigServiceLib_t* configLib,
                                  hPartition_t phandle)
{
    seos_err_t result = SEOS_SUCCESS;

    OS_ConfigServiceBackend_t parameterBackend;
    OS_ConfigServiceBackend_t domainBackend;
    OS_ConfigServiceBackend_t stringBackend;
    OS_ConfigServiceBackend_t blobBackend;
    OS_ConfigServiceBackend_FileName_t name;

    Debug_LOG_INFO("Initializing file backends...");

    // Initialize the backends in the config library object.
    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, DOMAIN_FILE);
    result = OS_ConfigServiceBackend_initializeFileBackend(&domainBackend, name, phandle);
    Debug_LOG_DEBUG("Domain name: %s", name.buffer);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN,
                   PARAMETER_FILE);
    result = OS_ConfigServiceBackend_initializeFileBackend(&parameterBackend, name,
                                                     phandle);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }
    Debug_LOG_DEBUG("Parameter backend initialized.");

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, STRING_FILE);
    result = OS_ConfigServiceBackend_initializeFileBackend(&stringBackend, name, phandle);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }
    Debug_LOG_DEBUG("String backend initialized.");

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, BLOB_FILE);
    result = OS_ConfigServiceBackend_initializeFileBackend(&blobBackend, name, phandle);
    if (result != SEOS_SUCCESS)
    {
        return result;
    }
    Debug_LOG_DEBUG("Blob backend initialized.");

    result = OS_ConfigServiceLib_Init(
                 configLib,
                 &parameterBackend,
                 &domainBackend,
                 &stringBackend,
                 &blobBackend);
    if (result != SEOS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceLib_Init failed with: %d", result);
        return result;
    }

    Debug_LOG_INFO("File backends initialized.");

    return result;
}
