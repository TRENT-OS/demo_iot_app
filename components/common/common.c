/*  Copyright (C) 2020, Hensoldt Cyber GmbH */
#include "lib_debug/Debug.h"
#include <camkes.h>

#define DATABUFFER_CLIENT       (void *)logServer_dp

static OS_LoggerFilter_Handle_t filter;

void pre_init(void)
{
    OS_LoggerFilter_ctor(&filter, Debug_LOG_LEVEL_DEBUG);

    OS_LoggerEmitter_getInstance(
        DATABUFFER_CLIENT,
        &filter,
        API_LOG_SERVER_EMIT);
}
