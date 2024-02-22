/*
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#include "lib_debug/Debug.h"
#include <camkes.h>

#define DATABUFFER_CLIENT       (void *)logServer_port

static OS_LoggerFilter_Handle_t filter;

void pre_init(void)
{
    OS_LoggerFilter_ctor(&filter, Debug_LOG_LEVEL_DEBUG);

    OS_LoggerEmitter_getInstance(
        DATABUFFER_CLIENT,
        &filter,
        API_LOG_SERVER_EMIT);
}
