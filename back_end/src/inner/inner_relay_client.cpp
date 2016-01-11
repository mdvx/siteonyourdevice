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

#include "inner/inner_relay_client.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

RelayClient::RelayClient(tcp::ITcpLoop *server, const common::net::socket_info& info)
  : Http2Client(server, info) {
}

const char* RelayClient::className() const {
  return "RelayClient";
}

RelayClientEx::RelayClientEx(tcp::ITcpLoop* server, const common::net::socket_info& info,
                             const common::net::hostAndPort& externalHost)
  : RelayClient(server, info), external_host_(externalHost), eclient_(NULL) {
}

common::net::hostAndPort RelayClientEx::externalHost() const {
  return external_host_;
}

ProxyRelayClient *RelayClientEx::eclient() const {
  return eclient_;
}

void RelayClientEx::setEclient(ProxyRelayClient *client) {
  eclient_ = client;
}

const char* RelayClientEx::className() const {
  return "RelayClientEx";
}

ProxyRelayClient::ProxyRelayClient(tcp::ITcpLoop *server, const common::net::socket_info& info,
                                   RelayClientEx *relay)
  : TcpClient(server, info), relay_(relay) {
}

RelayClientEx *ProxyRelayClient::relay() const {
  return relay_;
}

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
