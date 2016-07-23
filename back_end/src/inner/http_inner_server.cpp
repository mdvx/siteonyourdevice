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

#include "inner/http_inner_server.h"

#include "http/http_client.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

ProxyInnerServer::ProxyInnerServer(tcp::ITcpLoopObserver* observer)
  : ITcpLoop(observer) {
}

const char* ProxyInnerServer::ClassName() const {
  return "ProxyInnerServer";
}

tcp::TcpClient * ProxyInnerServer::createClient(const common::net::socket_info& info) {
  return new tcp::TcpClient(this, info);
}

Http2InnerServer::Http2InnerServer(tcp::ITcpLoopObserver* observer, const HttpConfig& config)
  : Http2Server(config.local_host, observer), config_(config) {
}

const char* Http2InnerServer::ClassName() const {
  return "Http2InnerServer";
}

tcp::TcpClient * Http2InnerServer::createClient(const common::net::socket_info& info) {
  http::Http2Client *cl = new http::Http2Client(this, info);
  cl->setIsAuthenticated(!config_.is_private_site);
  return cl;
}

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
