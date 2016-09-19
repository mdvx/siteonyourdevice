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

#include <vector>
#include <utility>

#include "infos.h"

#include "inner/inner_client.h"

#include "loop_controller.h"

namespace fasto {
namespace siteonyourdevice {
namespace tcp {
class TcpServer;
}  // namespace tcp

namespace inner {
class InnerServerCommandSeqParser;
}  // namespace inner

namespace server {
namespace inner {
class InnerServerHandlerHost;
class InnerTcpServerClient;

class IInnerRelayLoop : public ILoopThreadController {
 public:
  typedef std::pair<tcp::TcpClient*, common::buffer_t> request_t;
  IInnerRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser* handler,
                  InnerTcpServerClient* parent,
                  const request_t& request);
  ~IInnerRelayLoop();

  bool readyForRequest() const;
  void addRequest(const request_t& request);

 protected:
  InnerTcpServerClient* const parent_;
  fasto::siteonyourdevice::inner::InnerServerCommandSeqParser* ihandler_;

  const request_t request_;
};

class InnerTcpServerClient : public siteonyourdevice::inner::InnerClient {
 public:
  typedef std::shared_ptr<IInnerRelayLoop> http_relay_loop_t;
  typedef std::shared_ptr<IInnerRelayLoop> websocket_relay_loop_t;

  InnerTcpServerClient(tcp::TcpServer* server, const common::net::socket_info& info);
  ~InnerTcpServerClient();

  virtual const char* className() const;

  void setServerHostInfo(const UserAuthInfo& info);
  UserAuthInfo serverHostInfo() const;

  void addHttpRelayClient(InnerServerHandlerHost* handler,
                          tcp::TcpClient* client,
                          const common::buffer_t& request);  // move ovnerships
  void addWebsocketRelayClient(InnerServerHandlerHost* handler,
                               tcp::TcpClient* client,
                               const common::buffer_t& request,
                               const common::net::HostAndPort& srcHost);  // move ovnerships

 private:
  UserAuthInfo hinfo_;
  std::vector<http_relay_loop_t> relays_http_;
  std::vector<websocket_relay_loop_t> relays_websockets_;
};

}  // namespace inner
}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
