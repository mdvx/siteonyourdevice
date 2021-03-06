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

#include "server/websocket/websocket_client.h"

namespace fasto {
namespace siteonyourdevice {
namespace server {
namespace websocket {

WebSocketClientHost::WebSocketClientHost(common::libev::IoLoop* server, const common::net::socket_info& info)
    : Http2Client(server, info) {}

const char* WebSocketClientHost::ClassName() const {
  return "WebSocketClientHost";
}

}  // namespace websocket
}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
