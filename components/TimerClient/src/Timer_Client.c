/*
 *  Network Stack CAmkES App for timer client
 *
 *  Copyright (C) 2020, Hensoldt Cyber GmbH
 *
 */

#include "LibDebug/Debug.h"
#include "TimeServer.h"

#include <camkes.h>

// Triggers the NetworkStack to check for possible events.
// Value is arbitrary but shouldn't be set too high.
#define MSECS_TO_SLEEP   500
#define SIGNAL_PERIOD_MS 1000

// Tiggers Sensor to send a message every 5sec.
// Value is arbitrary but tests showed that on the current QEMU setup,
// it shouldn't be set too low.

static unsigned int counterMs = 0;


//------------------------------------------------------------------------------
int run(void)
{
    Debug_LOG_INFO("Starting TimerClient");

    for (;;)
    {
        TimeServer_sleep(TimeServer_PRECISION_MSEC, MSECS_TO_SLEEP);
        counterMs += MSECS_TO_SLEEP;
        if ((counterMs % SIGNAL_PERIOD_MS) == 0)
        {
            e_timeout_nwstacktick_emit();
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
unsigned int
TimerClient_getTimeMs(void)
{
    return counterMs;
}
