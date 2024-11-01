/*
 * Main file of the ConfigServer component.
 *
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */


#include <camkes.h>

#include "lib_debug/Debug.h"
#include "init_config_backend.h"

void post_init(void)
{
    Debug_LOG_INFO("Starting ConfigServer...");

    OS_Error_t err = init_system_config_backend();
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("init_system_config_backend() failed with:%d", err);
        return;
    }

    Debug_LOG_INFO("Config Server initialized.");

    return;
}
