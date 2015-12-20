#pragma once

#include "common/net/types.h"

#define HOST_PORT 8040
#define INNER_HOST_PORT 8020

#define INNER_HOST_NAME "siteonyourdevice.com"
#define HTTP_PROXY_HOST_NAME "proxy.siteonyourdevice.com"

#define HOST_PATH "/"

#define CHANNEL_COMMANDS_IN_NAME "COMMANDS_IN"
#define CHANNEL_COMMANDS_OUT_NAME "COMMANDS_OUT"
#define CHANNEL_CLIENTS_STATE_NAME "CLIENTS_STATE"

const common::net::hostAndPort g_inner_host(INNER_HOST_NAME, INNER_HOST_PORT);
