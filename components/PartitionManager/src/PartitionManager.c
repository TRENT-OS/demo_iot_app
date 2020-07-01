/*
   *  Partition manager init file for the use of the partition manager as a component.
   *
   *  Copyright (C) 2020, Hensoldt Cyber GmbH
*/

#include "api_pm.h"                 // include path to partition_manager must be set in cmakelists.txt
#include "ChanMuxNvmDriver.h"
#include "LibDebug/Debug.h"         // needs seos_libs

#include <camkes.h>


static const ChanMuxClientConfig_t chanMuxClientConfig =
{
    .port  = CHANMUX_DATAPORT_ASSIGN(chanMux_chan_portRead,
                                     chanMux_chan_portWrite),
    .wait  = chanMux_chan_EventHasData_wait,
    .write = chanMux_Rpc_write,
    .read  = chanMux_Rpc_read
};

static ChanMuxNvmDriver chanMuxNvmDriver;

void api_pm_component__init(void)
{
    OS_Error_t pm_stat;

    if (!ChanMuxNvmDriver_ctor(
            &chanMuxNvmDriver,
            &chanMuxClientConfig))
    {
        Debug_LOG_ERROR("Failed to construct ChanMuxNvmDriver!");
        return;
    }

    pm_stat = api_pm_partition_manager_init(
                  ChanMuxNvmDriver_get_nvm(&chanMuxNvmDriver));
    if (pm_stat != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Fail to init partition manager, ret: %d", pm_stat);
        return;
    }

}
