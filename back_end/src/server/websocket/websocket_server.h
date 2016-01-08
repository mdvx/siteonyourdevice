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

#include "http/http_server.h"

namespace fasto {
namespace siteonyourdevice {
namespace server {

class HttpServerHost;

namespace websocket {

class WebSocketServerHost :
        public http::Http2Server {
 public:
  WebSocketServerHost(const common::net::hostAndPort& host, tcp::ITcpLoopObserver *observer);

  virtual const char* className() const;

 protected:
  virtual tcp::TcpClient * createClient(const common::net::socket_info &info);
};

class WebSocketServerHandlerHost
    : public http::Http2ServerHandler {
 public:
  WebSocketServerHandlerHost(const HttpServerInfo & info, HttpServerHost * parent);

  virtual void dataReceived(tcp::TcpClient* client);

 private:
  void processWebsocketRequest(http::HttpClient *hclient,
                               const common::http::http_request& hrequest);

  HttpServerHost * parent_;
};

}  // namespace websocket
}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
