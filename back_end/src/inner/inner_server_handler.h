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

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

#include "tcp/tcp_server.h"

#include "http_config.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

class InnerServerHandler
        : public InnerServerCommandSeqParser, public tcp::ITcpLoopObserver  {
 public:
  enum {
      ping_timeout_server = 30  // sec
  };

  explicit InnerServerHandler(const common::net::hostAndPort& innerHost, const HttpConfig& config);
  ~InnerServerHandler();

  UserAuthInfo authInfo() const;

  virtual void preLooped(tcp::ITcpLoop* server);
  virtual void accepted(tcp::TcpClient* client);
  virtual void moved(tcp::TcpClient* client);
  virtual void closed(tcp::TcpClient* client);
  virtual void dataReceived(tcp::TcpClient* client);
  virtual void dataReadyToWrite(tcp::TcpClient* client);
  virtual void postLooped(tcp::ITcpLoop* server);
  virtual void timerEmited(tcp::ITcpLoop* server, timer_id_type id);

 private:
  virtual void handleInnerRequestCommand(InnerClient *connection,
                                         cmd_seq_type id, int argc, char *argv[]);
  virtual void handleInnerResponceCommand(InnerClient *connection,
                                          cmd_seq_type id, int argc, char *argv[]);
  virtual void handleInnerApproveCommand(InnerClient *connection,
                                         cmd_seq_type id, int argc, char *argv[]);

  const HttpConfig config_;
  InnerClient* inner_connection_;
  timer_id_type ping_server_id_timer_;

  const common::net::hostAndPort innerHost_;
};

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
