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

#include <string>

#include "http/http_server.h"

#include <common/thread/thread.h>

#include "server/inner/inner_tcp_server.h"

#include "server/websocket/websocket_server.h"

#include "http/http_server_handler.h"

namespace fasto {
namespace siteonyourdevice {
namespace server {

class HttpServerHost;

namespace inner {
class InnerTcpServerClient;
}  // namespace inner

class HttpInnerServerHandlerHost : public http::Http2ServerHandler {
 public:
  typedef http::Http2Server server_t;
  typedef http::HttpClient client_t;

  HttpInnerServerHandlerHost(const HttpServerInfo& info, HttpServerHost* parent);
  virtual void accepted(tcp::TcpClient* client);
  virtual void closed(tcp::TcpClient* client);
  virtual void dataReceived(tcp::TcpClient* client);

  virtual ~HttpInnerServerHandlerHost();

 private:
  void processHttpRequest(http::HttpClient* hclient, const common::http::http_request& hrequest);
  HttpServerHost* const parent_;
};

class HttpServerHost {
 public:
  typedef std::unordered_map<std::string, inner::InnerTcpServerClient*> inner_connections_type;

  HttpServerHost(const common::net::HostAndPort& httpHost,
                 const common::net::HostAndPort& innerHost,
                 const common::net::HostAndPort& webSocketHost);
  ~HttpServerHost();

  void setStorageConfig(const redis_sub_configuration_t& config);

  bool unRegisterInnerConnectionByHost(tcp::TcpClient* connection) WARN_UNUSED_RESULT;
  bool registerInnerConnectionByUser(const UserAuthInfo& user,
                                     tcp::TcpClient* connection) WARN_UNUSED_RESULT;
  bool findUser(const UserAuthInfo& user) const;
  inner::InnerTcpServerClient* findInnerConnectionByHost(const std::string& host) const;

  int exec() WARN_UNUSED_RESULT;
  void stop();

  inner::InnerServerHandlerHost* innerHandler() const;

 private:
  HttpInnerServerHandlerHost* httpHandler_;
  http::Http2Server* httpServer_;
  std::shared_ptr<common::thread::Thread<int> > http_thread_;

  inner::InnerServerHandlerHost* innerHandler_;
  inner::InnerTcpServer* innerServer_;
  std::shared_ptr<common::thread::Thread<int> > inner_thread_;

  websocket::WebSocketServerHandlerHost* websocketHandler_;
  websocket::WebSocketServerHost* websocketServer_;
  std::shared_ptr<common::thread::Thread<int> > websocket_thread_;

  inner_connections_type connections_;
  RedisStorage rstorage_;
};

}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
