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
    static OS_ConfigService_ClientCtx_t ctx =
    {
        .dataport = OS_DATAPORT_ASSIGN(cfg_dataport_buf)
    };
    OS_Error_t err = OS_ConfigService_createHandleRemote(
                         &ctx,
                         configHandle);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandleRemote() failed with :%d", err);
        return err;
    }

    return OS_SUCCESS;
}
