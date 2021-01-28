/**
 * Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 */

#pragma once

#include "lib_debug/Debug.h"

#include "OS_ConfigService.h"


//------------------------------------------------------------------------------
OS_Error_t
helper_func_getConfigParameter(
    OS_ConfigServiceHandle_t* handle,
    const char* DomainName,
    const char* ParameterName,
    void*       parameterBuffer,
    size_t      parameterLength);
