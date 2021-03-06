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

#include "http/http_client.h"

namespace fasto {
namespace siteonyourdevice {
namespace websocket {

class WebSocketClient : public http::Http2Client {
 public:
  WebSocketClient(common::libev::IoLoop* server, const common::net::socket_info& info);
  ~WebSocketClient();

  const char* ClassName() const;
};

}  // namespace websocket
}  // namespace siteonyourdevice
}  // namespace fasto
