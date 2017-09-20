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

#include <common/libev/io_loop_observer.h>

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

#include "http_config.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

class InnerServerHandler : public InnerServerCommandSeqParser, public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_server = 30  // sec
  };

  explicit InnerServerHandler(const common::net::HostAndPort& innerHost, const HttpConfig& config);
  ~InnerServerHandler();

  UserAuthInfo authInfo() const;
  void setConfig(const HttpConfig& config);

  virtual void PreLooped(common::libev::IoLoop* server) override;
  virtual void Accepted(common::libev::IoClient* client) override;
  virtual void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  virtual void Closed(common::libev::IoClient* client) override;
  virtual void DataReceived(common::libev::IoClient* client) override;
  virtual void DataReadyToWrite(common::libev::IoClient* client) override;
  virtual void PostLooped(common::libev::IoLoop* server) override;
  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

 private:
  virtual void handleInnerRequestCommand(InnerClient* connection, cmd_seq_t id, int argc, char* argv[]) override;
  virtual void handleInnerResponceCommand(InnerClient* connection, cmd_seq_t id, int argc, char* argv[]) override;
  virtual void handleInnerApproveCommand(InnerClient* connection, cmd_seq_t id, int argc, char* argv[]) override;

  HttpConfig config_;
  InnerClient* inner_connection_;
  common::libev::timer_id_t ping_server_id_timer_;

  const common::net::HostAndPort innerHost_;
};

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
