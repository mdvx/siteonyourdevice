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
namespace inner {

class RelayClient
        : public http::Http2Client {
 public:
  RelayClient(tcp::ITcpLoop* server, const common::net::socket_info& info);
  const char* ClassName() const;
};

class ProxyRelayClient;

class RelayClientEx
        : public RelayClient {
 public:
  RelayClientEx(tcp::ITcpLoop* server, const common::net::socket_info& info,
                const common::net::HostAndPort& externalHost);
  const char* ClassName() const;

  common::net::HostAndPort externalHost() const;

  ProxyRelayClient *eclient() const;
  void setEclient(ProxyRelayClient* client);

 private:
  const common::net::HostAndPort external_host_;
  ProxyRelayClient * eclient_;
};

class ProxyRelayClient
        : public tcp::TcpClient {
 public:
  ProxyRelayClient(tcp::ITcpLoop* server, const common::net::socket_info& info,
                   RelayClientEx* relay);
  RelayClientEx * relay() const;

 private:
  RelayClientEx * const relay_;
};

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
