/*
   *  Init file for the CloudConnector.
   *
   *  Copyright (C) 2020, Hensoldt Cyber GmbH
*/


#include <camkes.h>

#include "LibDebug/Debug.h"
#include "OS_ConfigService.h"

/* Instance variables --------------------------------------------------------*/
OS_ConfigServiceHandle_t serverLibWithMemBackend;

OS_Error_t
init_config_handle(OS_ConfigServiceHandle_t* configHandle)
{
    OS_Error_t err = OS_ConfigService_createHandle(
                         OS_CONFIG_HANDLE_KIND_RPC,
                         0,
                         configHandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandle() failed with :%d", err);
        return err;
    }

    return OS_SUCCESS;
}