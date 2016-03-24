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

#include "http/http_server_handler.h"

namespace fasto {
namespace siteonyourdevice {
namespace websocket {

class WebSocketServerHandler
  : public http::Http2ServerHandler {
 public:
  explicit WebSocketServerHandler(const HttpServerInfo& info);

 protected:
  virtual void processReceived(http::HttpClient *hclient, const char* request, size_t req_len);
  virtual void handleRequest(http::HttpClient *hclient, const common::http::http_request& hrequest,
                             bool notClose);
};

}  // namespace websocket
}  // namespace siteonyourdevice
}  // namespace fasto
