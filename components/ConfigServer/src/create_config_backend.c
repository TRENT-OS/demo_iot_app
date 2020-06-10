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
static uint8_t parameterIndex = 0;
static uint8_t blobIndex = 0;
static uint8_t stringIndex = 0;

hPartition_t phandle;
OS_PartitionManagerDataTypes_PartitionData_t pm_partition_data;


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
void
initializeParameter(OS_ConfigServiceLibTypes_Parameter_t* parameter)
{
    memset(parameter, 0, sizeof(OS_ConfigServiceLibTypes_Parameter_t));
}

static
OS_Error_t
addInt32Parameter(
    OS_ConfigServiceLib_t* configLib,
    OS_ConfigServiceLibTypes_Parameter_t* parameter,
    unsigned int domainIndex,
    const char* parameterName,
    uint32_t parameterValue)
{
    parameter->domain.index = domainIndex;
    parameter->parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_INTEGER32;
    initializeName(parameter->parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   parameterName);

    parameter->parameterValue.valueInteger32 = parameterValue;

    OS_Error_t err = OS_ConfigServiceBackend_writeRecord(
                         &configLib->parameterBackend,
                         parameterIndex,
                         parameter,
                         sizeof(OS_ConfigServiceLibTypes_Parameter_t));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_writeRecord() failed with: %d", err);
        return err;
    }

    parameterIndex++;

    return OS_SUCCESS;
}

static
OS_Error_t
addStringParameter(
    OS_ConfigServiceLib_t* configLib,
    OS_ConfigServiceLibTypes_Parameter_t* parameter,
    unsigned int domainIndex,
    const char* parameterName)
{
    parameter->domain.index = domainIndex;
    parameter->parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_STRING;
    initializeName(parameter->parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   parameterName);

    parameter->parameterValue.valueString.index = stringIndex;
    parameter->parameterValue.valueString.size =
        OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH;

    OS_Error_t err = OS_ConfigServiceBackend_writeRecord(
                         &configLib->parameterBackend,
                         parameterIndex,
                         parameter,
                         sizeof(OS_ConfigServiceLibTypes_Parameter_t));;
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_writeRecord() failed with: %d", err);
        return err;
    }

    parameterIndex++;
    stringIndex++;

    return OS_SUCCESS;
}

static
OS_Error_t
addBlobParameter(
    OS_ConfigServiceLib_t* configLib,
    OS_ConfigServiceLibTypes_Parameter_t* parameter,
    unsigned int domainIndex,
    const char* parameterName,
    size_t parameterSize)
{
    parameter->domain.index = domainIndex;
    parameter->parameterType = OS_CONFIG_LIB_PARAMETER_TYPE_BLOB;
    initializeName(parameter->parameterName.name, OS_CONFIG_LIB_PARAMETER_NAME_LEN,
                   parameterName);

    uint32_t calcNumberOfBlocks;

    if (parameterSize <= OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH)
    {
        calcNumberOfBlocks = 1;
    }

    if (parameterSize > OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH)
    {
        calcNumberOfBlocks =
            (parameterSize / OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH);

        if ((parameterSize % OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH) != 0)
        {
            calcNumberOfBlocks++;
        }
    }

    parameter->parameterValue.valueBlob.index = blobIndex;
    parameter->parameterValue.valueBlob.numberOfBlocks = calcNumberOfBlocks;
    parameter->parameterValue.valueBlob.size =
        (OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH *
         parameter->parameterValue.valueBlob.numberOfBlocks);

    OS_Error_t err = OS_ConfigServiceBackend_writeRecord(
                         &configLib->parameterBackend,
                         parameterIndex,
                         parameter,
                         sizeof(OS_ConfigServiceLibTypes_Parameter_t));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_writeRecord() failed with: %d", err);
        return err;
    }

    blobIndex += calcNumberOfBlocks;
    parameterIndex++;

    return OS_SUCCESS;
}

OS_Error_t
initializeDomainsAndParameters(OS_ConfigServiceLib_t* configLib)
{
    Debug_LOG_INFO("Initializing Domains and Parameters...");

    OS_ConfigServiceLibTypes_Domain_t domain;

    // initialize Sensor Domain
    Debug_LOG_DEBUG("initializing Domain: %s", DOMAIN_SENSOR);
    initializeDomain(&domain, DOMAIN_SENSOR);

    const uint8_t sensorDomainIndex = 0;

    OS_Error_t err = OS_ConfigServiceBackend_writeRecord(
                         &configLib->domainBackend,
                         sensorDomainIndex,
                         &domain,
                         sizeof(domain));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_writeRecord() failed with: %d", err);
        return err;
    }

    // Initialize the parameters
    OS_ConfigServiceLibTypes_Parameter_t parameter;
    initializeParameter(&parameter);
    OS_ConfigServiceAccessRights_SetAll(&parameter.readAccess);
    OS_ConfigServiceAccessRights_SetAll(&parameter.writeAccess);

    /* MQTT Message Payload  -------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", MQTT_PAYLOAD_NAME, DOMAIN_SENSOR);
    err = addBlobParameter(
              configLib,
              &parameter,
              sensorDomainIndex,
              MQTT_PAYLOAD_NAME,
              sizeof(MQTT_PAYLOAD_VALUE));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, MQTT_PAYLOAD_NAME);
        return err;
    }

    /* MQTT Message Topic  ---------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", MQTT_TOPIC_NAME, DOMAIN_SENSOR);
    err = addBlobParameter(
              configLib,
              &parameter,
              sensorDomainIndex,
              MQTT_TOPIC_NAME,
              sizeof(MQTT_TOPIC_VALUE));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, MQTT_TOPIC_NAME);
        return err;
    }

    // initialize CloudConnector Domain
    Debug_LOG_INFO("Initializing %s", DOMAIN_CLOUDCONNECTOR);
    initializeDomain(&domain, DOMAIN_CLOUDCONNECTOR);

    const uint8_t cloudConnectorDomainIndex = 1;

    err = OS_ConfigServiceBackend_writeRecord(
              &configLib->domainBackend,
              cloudConnectorDomainIndex,
              &domain,
              sizeof(domain));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_writeRecord() failed with: %d", err);
        return err;
    }

    /* Server Port  ----------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", SERVER_PORT_NAME, DOMAIN_CLOUDCONNECTOR);
    err = addInt32Parameter(
              configLib,
              &parameter,
              cloudConnectorDomainIndex,
              SERVER_PORT_NAME,
              0);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, SERVER_PORT_NAME);
        return err;
    }

    /* Cloud SharedAccessSignature -------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", CLOUD_SAS_NAME, DOMAIN_CLOUDCONNECTOR);
    err = addBlobParameter(
              configLib,
              &parameter,
              cloudConnectorDomainIndex,
              CLOUD_SAS_NAME,
              sizeof(CLOUD_SAS_VALUE));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, CLOUD_SAS_NAME);
        return err;
    }

    /* Azure Cloud Domain ----------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", CLOUD_DOMAIN_NAME, DOMAIN_CLOUDCONNECTOR);
    err = addBlobParameter(
              configLib,
              &parameter,
              cloudConnectorDomainIndex,
              CLOUD_DOMAIN_NAME,
              sizeof(CLOUD_DOMAIN_VALUE));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, CLOUD_DOMAIN_NAME);
        return err;
    }

    /* Server Address --------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", SERVER_ADDRESS_NAME, DOMAIN_CLOUDCONNECTOR);
    err = addStringParameter(
              configLib,
              &parameter,
              cloudConnectorDomainIndex,
              SERVER_ADDRESS_NAME);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, SERVER_ADDRESS_NAME);
        return err;
    }

    /* Device Name -----------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", CLOUD_DEVICE_ID_NAME, DOMAIN_CLOUDCONNECTOR);
    err = addStringParameter(
              configLib,
              &parameter,
              cloudConnectorDomainIndex,
              CLOUD_DEVICE_ID_NAME);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, CLOUD_DEVICE_ID_NAME);
        return err;
    }

    /* TLS Cert --------------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", SERVER_CA_CERT_NAME, DOMAIN_CLOUDCONNECTOR);
    err = addBlobParameter(
              configLib,
              &parameter,
              cloudConnectorDomainIndex,
              SERVER_CA_CERT_NAME,
              sizeof(SERVER_CA_CERT_PEM_VALUE));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, SERVER_CA_CERT_NAME);
        return err;
    }

    // initialize NwStack Domain
    Debug_LOG_INFO("Initializing %s", DOMAIN_NWSTACK);
    initializeDomain(&domain, DOMAIN_NWSTACK);

    const uint8_t nwStackDomainIndex = 2;

    err = OS_ConfigServiceBackend_writeRecord(
              &configLib->domainBackend,
              nwStackDomainIndex,
              &domain,
              sizeof(domain));
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigServiceBackend_writeRecord() failed with: %d", err);
        return err;
    }

    /* IP Address  -----------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", ETH_ADDR, DOMAIN_NWSTACK);
    err = addStringParameter(
              configLib,
              &parameter,
              nwStackDomainIndex,
              ETH_ADDR);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, ETH_ADDR);
        return err;
    }

    /* Gateway Address  ------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", ETH_GATEWAY_ADDR, DOMAIN_NWSTACK);
    err = addStringParameter(
              configLib,
              &parameter,
              nwStackDomainIndex,
              ETH_GATEWAY_ADDR);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, ETH_GATEWAY_ADDR);
        return err;
    }

    /* Subnet Address  -------------------------------------------------------*/
    Debug_LOG_INFO("Initializing %s in %s...", ETH_SUBNET_MASK, DOMAIN_NWSTACK);
    err = addStringParameter(
              configLib,
              &parameter,
              nwStackDomainIndex,
              ETH_SUBNET_MASK);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR(
            "OS_ConfigServiceBackend_writeRecord() failed \
             with: %d for the parameter: %s", err, ETH_SUBNET_MASK);
        return err;
    }

    Debug_LOG_INFO("Domains and parameters initialized.");

    return OS_SUCCESS;

}

OS_Error_t
create_system_config_backend(void)
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
        != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to create filesystem on partition: %d!",
                        pm_partition_data.partition_id);
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

    // Create the file backends
    Debug_LOG_INFO("Setting up filesystem backend...");

    OS_Error_t err = createFileBackends(phandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("createFileBackends() failed with: %d", err);
        return err;
    }

    err = initializeFileBackends(configLib, phandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeFileBackends() failed with: %d", err);
        return err;
    }

    // Create the empty parameters in the instance.
    err = initializeDomainsAndParameters(configLib);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("initializeDomainsAndParameters() failed with: %d", err);
        return err;
    }

    return OS_SUCCESS;
}

OS_Error_t
createFileBackends(hPartition_t phandle)
{
    OS_ConfigServiceBackend_FileName_t name;

    /* Create the file backends. The following defined amounts are all specifically
     * tailored to fit the requirements of the demo application parameters and need
     * to be adapter for any other use case.
    */
    Debug_LOG_DEBUG("Size of ConfigLib_Domain: %d",
                    sizeof(OS_ConfigServiceLibTypes_Domain_t));
    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, DOMAIN_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    OS_Error_t err = OS_ConfigServiceBackend_createFileBackend(
                         name,
                         phandle,
                         3, // Total amount of domains used
                         sizeof(OS_ConfigServiceLibTypes_Domain_t));
    if (err != OS_SUCCESS)
    {
        return err;
    }

    Debug_LOG_DEBUG("Size of ConfigLib_Parameter: %d",
                    sizeof(OS_ConfigServiceLibTypes_Parameter_t));
    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN,
                   PARAMETER_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    err = OS_ConfigServiceBackend_createFileBackend(
              name,
              phandle,
              11, // Total amount of parameters in the backend
              sizeof(OS_ConfigServiceLibTypes_Parameter_t));
    if (err != OS_SUCCESS)
    {
        return err;
    }

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, STRING_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    err = OS_ConfigServiceBackend_createFileBackend(
              name,
              phandle,
              5, // Total amount of string parameters in the backend
              OS_CONFIG_LIB_PARAMETER_MAX_STRING_LENGTH);
    if (err != OS_SUCCESS)
    {
        return err;
    }

    initializeName(name.buffer, OS_CONFIG_BACKEND_MAX_FILE_NAME_LEN, BLOB_FILE);
    Debug_LOG_DEBUG("Name.buffer: %s", name.buffer);
    err = OS_ConfigServiceBackend_createFileBackend(
              name,
              phandle,
              60,  // Total amount of blob blocks needed to store all the blob type parameters
              OS_CONFIG_LIB_PARAMETER_MAX_BLOB_BLOCK_LENGTH);
    if (err != OS_SUCCESS)
    {
        return err;
    }

    Debug_LOG_INFO("File backends created.");

    return OS_SUCCESS;
}

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
