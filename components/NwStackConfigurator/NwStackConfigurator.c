/*
 * NwStackConfigurator component that retrieves the stack configuration from the
 * ConfigServer and configures the NetworkStack component with it.
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "OS_ConfigService.h"
#include "OS_Dataport.h"
#include "OS_Error.h"
#include "OS_Socket.h"
#include "if_NetworkStack_PicoTcp_Config.h"
#include "lib_debug/Debug.h"

#include "helper_func.h"

#include <camkes.h>

/* Defines -------------------------------------------------------------------*/
// the following defines are the parameter names that need to match the settings
// in the configuration xml file. These will be passed to the configServer
// component to retrieve the settings for the specified parameter
#define DOMAIN_NWSTACK   "Domain-NwStack"
#define ETH_ADDR         "ETH_ADDR"
#define ETH_GATEWAY_ADDR "ETH_GATEWAY_ADDR"
#define ETH_SUBNET_MASK  "ETH_SUBNET_MASK"

//------------------------------------------------------------------------------
static const if_NetworkStack_PicoTcp_Config_t networkStackConfig =
    if_NetworkStack_PicoTcp_Config_ASSIGN(networkStack_PicoTcp_Config);

//------------------------------------------------------------------------------
static
OS_Error_t
read_ip_from_config_server(
    OS_NetworkStack_AddressConfig_t* const ipAddrConfig,
    const char* devAddrParamName,
    const char* gatewayAddrParamName,
    const char* subnetMaskParamName)
{
    OS_Error_t ret;
    // Create a handle to the remote library instance.
    OS_ConfigServiceHandle_t hConfig;

    static OS_ConfigService_ClientCtx_t ctx =
    {
        .dataport = OS_DATAPORT_ASSIGN(configServer_port)
    };

    ret = OS_ConfigService_createHandleRemote(
              &ctx,
              &hConfig);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_ConfigService_createHandleRemote() failed with :%d",
                        ret);
        return ret;
    }

    // Get the needed param values one by one from config server, using the
    // helper library wrapping around the ConfigServer API.
    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_NWSTACK,
                                         devAddrParamName,
                                         ipAddrConfig->dev_addr,
                                         sizeof(ipAddrConfig->dev_addr));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s "
                        "failed with :%d", devAddrParamName, ret);
        return ret;
    }
    Debug_LOG_INFO("Retrieved IP ADDR: %s", ipAddrConfig->dev_addr);

    ret = helper_func_getConfigParameter(&hConfig,
                                         DOMAIN_NWSTACK,
                                         gatewayAddrParamName,
                                         ipAddrConfig->gateway_addr,
                                         sizeof(ipAddrConfig->gateway_addr));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s "
                        "failed with :%d", gatewayAddrParamName, ret);
        return ret;
    }
    Debug_LOG_INFO("Retrieved GATEWAY ADDR: %s", ipAddrConfig->gateway_addr);

    ret = helper_func_getConfigParameter(
              &hConfig,
              DOMAIN_NWSTACK,
              subnetMaskParamName,
              ipAddrConfig->subnet_mask,
              sizeof(ipAddrConfig->subnet_mask));
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("helper_func_getConfigParameter() for param %s "
                        "failed with :%d", subnetMaskParamName, ret);
        return ret;
    }
    Debug_LOG_INFO("Retrieved SUBNETMASK: %s", ipAddrConfig->subnet_mask);

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
void
post_init(void)
{
    OS_NetworkStack_AddressConfig_t ipAddrConfig;

    OS_Error_t ret = read_ip_from_config_server(
                         &ipAddrConfig,
                         ETH_ADDR,
                         ETH_GATEWAY_ADDR,
                         ETH_SUBNET_MASK);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_FATAL("Read from config failed, error %d", ret);
        return;
    }

    ret = networkStackConfig.configIpAddr(&ipAddrConfig);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("Failed to configure NetworkStack instance, error %d",
                        ret);
        return;
    }
}
