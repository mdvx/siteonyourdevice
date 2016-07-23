/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "common/net/types.h"

#define INNER_HOST_PORT 8020
#define HOST_PORT 8040
#define WEBSOCKET_PORT 8060

#define INNER_HOST_NAME "siteonyourdevice.com"
#define HTTP_PROXY_HOST_NAME "proxy.siteonyourdevice.com"

#define HOST_PATH "/"

#define CHANNEL_COMMANDS_IN_NAME "COMMANDS_IN"
#define CHANNEL_COMMANDS_OUT_NAME "COMMANDS_OUT"
#define CHANNEL_CLIENTS_STATE_NAME "CLIENTS_STATE"

namespace fasto {
namespace siteonyourdevice {
namespace server {

const common::net::HostAndPort g_http_host(INNER_HOST_NAME, HOST_PORT);
const common::net::HostAndPort g_inner_host(INNER_HOST_NAME, INNER_HOST_PORT);
const common::net::HostAndPort g_websocket_host(INNER_HOST_NAME, WEBSOCKET_PORT);
const common::net::HostAndPort g_relay_server_host(INNER_HOST_NAME, RANDOM_PORT);

}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
