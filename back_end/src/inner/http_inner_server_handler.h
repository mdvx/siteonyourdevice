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
namespace inner {

class RelayClient;
class RelayClientEx;
class ProxyRelayClient;

class Http2ClientServerHandler : public http::Http2ServerHandler {
 public:
  Http2ClientServerHandler(const HttpServerInfo& info);
  ~Http2ClientServerHandler();

  virtual void PreLooped(common::libev::IoLoop* server) override;
  virtual void Accepted(common::libev::IoClient* client) override;
  virtual void Closed(common::libev::IoClient* client) override;
  virtual void DataReceived(common::libev::IoClient* client) override;
  virtual void DataReadyToWrite(common::libev::IoClient* client) override;
  virtual void PostLooped(common::libev::IoLoop* server) override;
  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

 private:
  void relayDataReceived(RelayClient* rclient);
  void relayExDataReceived(RelayClientEx* rclient);
  void proxyDataReceived(ProxyRelayClient* prclient);
};

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
