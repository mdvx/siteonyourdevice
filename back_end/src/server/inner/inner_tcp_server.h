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

#include <memory>

#include <common/libev/io_loop_observer.h>
#include <common/libev/tcp/tcp_server.h>
#include <common/threads/thread.h>
#include <common/types.h>

#include "inner/inner_server_command_seq_parser.h"

#include "server/redis_helpers.h"

#include "infos.h"

namespace fasto {
namespace siteonyourdevice {
namespace server {
class HttpServerHost;
namespace inner {

class InnerServerHandlerHost : public fasto::siteonyourdevice::inner::InnerServerCommandSeqParser,
                               public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_clients = 60  // sec
  };

  explicit InnerServerHandlerHost(HttpServerHost* parent);

  virtual void PreLooped(common::libev::IoLoop* server);

  virtual void Accepted(common::libev::IoClient* client);
  virtual void Moved(common::libev::IoLoop* server, common::libev::IoClient* client);
  virtual void Closed(common::libev::IoClient* client);

  virtual void DataReceived(common::libev::IoClient* client);
  virtual void DataReadyToWrite(common::libev::IoClient* client);
  virtual void PostLooped(common::libev::IoLoop* server);
  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id);

  virtual ~InnerServerHandlerHost();

  void setStorageConfig(const redis_sub_configuration_t& config);

 private:
  virtual void handleInnerRequestCommand(siteonyourdevice::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]);
  virtual void handleInnerResponceCommand(siteonyourdevice::inner::InnerClient* connection,
                                          cmd_seq_t id,
                                          int argc,
                                          char* argv[]);
  virtual void handleInnerApproveCommand(siteonyourdevice::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]);

  HttpServerHost* const parent_;

  class InnerSubHandler;
  RedisSub* sub_commands_in_;
  InnerSubHandler* handler_;
  std::shared_ptr<common::threads::Thread<void>> redis_subscribe_command_in_thread_;
  common::libev::timer_id_t ping_client_id_timer_;
};

class InnerTcpServer : public common::libev::tcp::TcpServer {
 public:
  InnerTcpServer(const common::net::HostAndPort& host, common::libev::IoLoopObserver* observer);
  virtual const char* ClassName() const override;

 private:
  virtual common::libev::tcp::TcpClient* CreateClient(const common::net::socket_info& info) override;
};

}  // namespace inner
}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
