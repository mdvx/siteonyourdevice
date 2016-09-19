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

#include "http/http_server.h"

#include "http/http_client.h"

namespace fasto {
namespace siteonyourdevice {
namespace http {

HttpServer::HttpServer(const common::net::HostAndPort& host, tcp::ITcpLoopObserver* observer)
    : TcpServer(host, observer) {}

HttpServer::~HttpServer() {}

tcp::TcpClient* HttpServer::createClient(const common::net::socket_info& info) {
  return new HttpClient(this, info);
}

const char* HttpServer::ClassName() const {
  return "HttpServer";
}

Http2Server::Http2Server(const common::net::HostAndPort& host, tcp::ITcpLoopObserver* observer)
    : HttpServer(host, observer) {}

tcp::TcpClient* Http2Server::createClient(const common::net::socket_info& info) {
  return new Http2Client(this, info);
}

const char* Http2Server::ClassName() const {
  return "Http2Server";
}

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
