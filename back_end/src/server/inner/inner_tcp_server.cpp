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

#include "server/inner/inner_tcp_server.h"

#include <string>
#include <vector>

#include <common/convert2string.h>
#include <common/logger.h>
#include <common/net/net.h>
#include <common/threads/thread_manager.h>

#include "server/http_server_host.h"
#include "server/server_commands.h"

#include "server/inner/inner_tcp_client.h"

extern "C" {
#include "sds.h"
}

namespace fasto {
namespace siteonyourdevice {
namespace server {
namespace inner {

class InnerServerHandlerHost::InnerSubHandler : public RedisSubHandler {
 public:
  explicit InnerSubHandler(InnerServerHandlerHost* parent) : parent_(parent) {}

  void processSubscribed(cmd_seq_t request_id, int argc, char* argv[]) {
    sds join = sdsempty();
    join = sdscatfmt(join, "%s ", request_id.c_str());
    for (int i = 0; i < argc; ++i) {
      join = sdscat(join, argv[i]);
      if (i != argc - 1) {
        join = sdscatlen(join, " ", 1);
      }
    }

    publish_command_out(join, sdslen(join));
    sdsfree(join);
  }

  void publish_command_out(const std::string& msg) { publish_command_out(msg.c_str(), msg.length()); }

  void publish_command_out(const char* msg, size_t msg_len) {
    bool res = parent_->sub_commands_in_->publish_command_out(msg, msg_len);
    if (!res) {
      std::string err_str =
          common::MemSPrintf("publish_command_out with args: msg = %s, msg_len = " PRIu64 " failed!", msg, msg_len);
      ERROR_LOG() << err_str;
    }
  }

  virtual void handleMessage(char* channel, size_t channel_len, char* msg, size_t msg_len) {
    // [std::string]site [hex_string]seq [std::string]command args ...
    // [hex_string]seq OK/FAIL [std::string]command args ..
    INFO_LOG() << "InnerSubHandler channel: " << channel << ", msg: " << msg;
    char* space = strchr(msg, ' ');
    if (!space) {
      const std::string resp = common::MemSPrintf("UNKNOWN COMMAND: %s", msg);
      WARNING_LOG() << resp;
      publish_command_out(resp);
      return;
    }

    *space = 0;

    char buff[MAX_COMMAND_SIZE] = {0};
    int len = snprintf(buff, sizeof(buff), STRINGIZE(REQUEST_COMMAND) " %s" END_OF_COMMAND,
                       space + 1);  // only REQUEST_COMMAND

    char* star_seq = NULL;
    cmd_id_t seq = strtoul(buff, &star_seq, 10);
    if (*star_seq != ' ') {
      std::string resp = common::MemSPrintf("PROBLEM EXTRACTING SEQUENCE: %s", space + 1);
      WARNING_LOG() << resp;
      publish_command_out(resp);
      return;
    }

    const char* id_ptr = strchr(star_seq + 1, ' ');
    if (!id_ptr) {
      std::string resp = common::MemSPrintf("PROBLEM EXTRACTING ID: %s", space + 1);
      ;
      WARNING_LOG() << resp;
      publish_command_out(resp);
      return;
    }

    size_t len_seq = id_ptr - (star_seq + 1);
    cmd_seq_t id = std::string(star_seq + 1, len_seq);
    const char* cmd = id_ptr;

    InnerTcpServerClient* fclient = parent_->parent_->findInnerConnectionByHost(msg);
    if (!fclient) {
      int argc;
      sds* argv = sdssplitargs(cmd, &argc);
      char* command = argv[0];

      std::string resp = common::MemSPrintf(SERVER_COMMANDS_OUT_FAIL_2US(CAUSE_NOT_CONNECTED), id, command);
      publish_command_out(resp);
      sdsfreesplitres(argv, argc);
      return;
    }

    size_t nwrite = 0;
    common::Error err = fclient->TcpClient::Write(buff, len, &nwrite);
    if (err) {
      int argc;
      sds* argv = sdssplitargs(cmd, &argc);
      char* command = argv[0];

      std::string resp = common::MemSPrintf(SERVER_COMMANDS_OUT_FAIL_2US(CAUSE_NOT_HANDLED), id, command);
      publish_command_out(resp);
      sdsfreesplitres(argv, argc);
      return;
    }

    auto cb = std::bind(&InnerSubHandler::processSubscribed, this, std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3);
    fasto::siteonyourdevice::inner::RequestCallback rc(id, cb);
    parent_->subscribeRequest(rc);
  }

  InnerServerHandlerHost* parent_;
};

InnerServerHandlerHost::InnerServerHandlerHost(HttpServerHost* parent)
    : parent_(parent), sub_commands_in_(NULL), handler_(NULL), ping_client_id_timer_(INVALID_TIMER_ID) {
  handler_ = new InnerSubHandler(this);
  sub_commands_in_ = new RedisSub(handler_);
  redis_subscribe_command_in_thread_ = THREAD_MANAGER()->CreateThread(&RedisSub::listen, sub_commands_in_);
}

InnerServerHandlerHost::~InnerServerHandlerHost() {
  sub_commands_in_->stop();
  redis_subscribe_command_in_thread_->Join();
  delete sub_commands_in_;
  delete handler_;
}

void InnerServerHandlerHost::PreLooped(common::libev::IoLoop* server) {
  ping_client_id_timer_ = server->CreateTimer(ping_timeout_clients, ping_timeout_clients);
}

void InnerServerHandlerHost::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {}

void InnerServerHandlerHost::PostLooped(common::libev::IoLoop* server) {}

void InnerServerHandlerHost::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  if (ping_client_id_timer_ == id) {
    auto online_clients = server->GetClients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      auto client = online_clients[i];
      const cmd_request_t ping_request = make_request(PING_COMMAND_REQ);
      size_t nwrite = 0;
      common::Error err = client->Write(ping_request.data(), ping_request.size(), &nwrite);
      INFO_LOG() << "Pinged sended " << nwrite << " byte, client[" << client->GetFormatedName() << "], from server["
                 << server->GetFormatedName() << "], " << online_clients.size() << " client(s) connected.";
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
        client->Close();
        delete client;
      }
    }
  }
}

void InnerServerHandlerHost::Accepted(common::libev::IoClient* client) {
  size_t nwrite = 0;
  cmd_request_t whoareyou = make_request(SERVER_WHO_ARE_YOU_COMMAND_REQ);
  client->Write(whoareyou.data(), whoareyou.size(), &nwrite);
}

void InnerServerHandlerHost::Closed(common::libev::IoClient* client) {
  bool isOk = parent_->unRegisterInnerConnectionByHost(client);
  if (isOk) {
    InnerTcpServerClient* iconnection = dynamic_cast<InnerTcpServerClient*>(client);
    if (iconnection) {
      UserAuthInfo hinf = iconnection->serverHostInfo();
      std::string hoststr = hinf.host.GetHost();
      std::string connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_DISCONNECTED_1S, hoststr);
      bool res = sub_commands_in_->publish_clients_state(connected_resp);
      if (!res) {
        std::string err_str =
            common::MemSPrintf("publish_clients_state with args: connected_resp = %s failed!", connected_resp);
        ERROR_LOG() << err_str;
      }
    }
  }
}

void InnerServerHandlerHost::DataReceived(common::libev::IoClient* client) {
  char buff[MAX_COMMAND_SIZE] = {0};
  size_t nread = 0;
  common::Error err = client->Read(buff, MAX_COMMAND_SIZE, &nread);
  if (err || nread == 0) {
    client->Close();
    delete client;
    return;
  }

  InnerTcpServerClient* iclient = dynamic_cast<InnerTcpServerClient*>(client);
  CHECK(iclient);

  handleInnerDataReceived(iclient, buff, nread);
}

void InnerServerHandlerHost::DataReadyToWrite(common::libev::IoClient* client) {}

void InnerServerHandlerHost::setStorageConfig(const redis_sub_configuration_t& config) {
  sub_commands_in_->setConfig(config);
  redis_subscribe_command_in_thread_->Start();
}

void InnerServerHandlerHost::handleInnerRequestCommand(siteonyourdevice::inner::InnerClient* connection,
                                                       cmd_seq_t id,
                                                       int argc,
                                                       char* argv[]) {
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, PING_COMMAND)) {
    cmd_responce_t pong = make_responce(id, PING_COMMAND_RESP_SUCCESS);
    size_t nwrite = 0;
    connection->Write(pong, &nwrite);
  } else {
    WARNING_LOG() << "UNKNOWN COMMAND: " << command;
  }
}

void InnerServerHandlerHost::handleInnerResponceCommand(siteonyourdevice::inner::InnerClient* connection,
                                                        cmd_seq_t id,
                                                        int argc,
                                                        char* argv[]) {
  size_t nwrite = 0;
  char* state_command = argv[0];

  if (IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1) {
    char* command = argv[1];
    if (IS_EQUAL_COMMAND(command, PING_COMMAND)) {
      if (argc > 2) {
        const char* pong = argv[2];
        if (!pong) {
          cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
          connection->Write(resp, &nwrite);
          goto fail;
        }

        cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_SUCCESS);
        common::Error err = connection->Write(resp, &nwrite);
        if (err) {
          goto fail;
        }
      } else {
        cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
        connection->Write(resp, &nwrite);
      }
    } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {
      if (argc > 2) {
        const char* uauthstr = argv[2];
        if (!uauthstr) {
          cmd_approve_t resp =
              make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
          connection->Write(resp, &nwrite);
          goto fail;
        }

        UserAuthInfo uauth;
        common::ConvertFromString(uauthstr, &uauth);
        if (!uauth.isValid()) {
          cmd_approve_t resp =
              make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_USER);
          connection->Write(resp, &nwrite);
          goto fail;
        }

        bool isOk = parent_->findUser(uauth);
        if (!isOk) {
          cmd_approve_t resp =
              make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_UNREGISTERED_USER);
          connection->Write(resp, &nwrite);
          goto fail;
        }

        std::string hoststr = uauth.host.GetHost();
        InnerTcpServerClient* fclient = parent_->findInnerConnectionByHost(hoststr);
        if (fclient) {
          cmd_approve_t resp =
              make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_DOUBLE_CONNECTION_HOST);
          connection->Write(resp, &nwrite);
          goto fail;
        }

        cmd_approve_t resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_SUCCESS);
        common::Error err = connection->Write(resp, &nwrite);
        if (err) {
          cmd_approve_t resp2 =
              make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, err->GetDescription());
          connection->Write(resp2, &nwrite);
          goto fail;
        }

        isOk = parent_->registerInnerConnectionByUser(uauth, connection);
        if (isOk) {
          std::string connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_CONNECTED_1S, hoststr);
          bool res = sub_commands_in_->publish_clients_state(connected_resp);
          if (!res) {
            std::string err_str =
                common::MemSPrintf("publish_clients_state with args: connected_resp = %s failed!", connected_resp);
            ERROR_LOG() << err_str;
          }
        }
      } else {
        cmd_approve_t resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
        connection->Write(resp, &nwrite);
      }
    } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_HTTP_COMMAND)) {
    } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND)) {
    } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_SYSTEM_INFO_COMMAND)) {
    } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONFIG_COMMAND)) {
    } else {
      WARNING_LOG() << "UNKNOWN RESPONCE COMMAND: " << command;
    }
  } else if (IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1) {
  } else {
    WARNING_LOG() << "UNKNOWN STATE COMMAND: " << state_command;
  }

  return;

fail:
  connection->Close();
  delete connection;
}

void InnerServerHandlerHost::handleInnerApproveCommand(siteonyourdevice::inner::InnerClient* connection,
                                                       cmd_seq_t id,
                                                       int argc,
                                                       char* argv[]) {
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, SUCCESS_COMMAND)) {
    if (argc > 1) {
      const char* okrespcommand = argv[1];
      if (IS_EQUAL_COMMAND(okrespcommand, PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(okrespcommand, SERVER_WHO_ARE_YOU_COMMAND)) {
      }
    }
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
  } else {
    WARNING_LOG() << "UNKNOWN COMMAND: " << command;
  }
}

InnerTcpServer::InnerTcpServer(const common::net::HostAndPort& host, common::libev::IoLoopObserver* observer)
    : TcpServer(host, observer) {}

const char* InnerTcpServer::ClassName() const {
  return "InnerTcpServer";
}

common::libev::tcp::TcpClient* InnerTcpServer::CreateClient(const common::net::socket_info& info) {
  return new InnerTcpServerClient(this, info);
}

}  // namespace inner
}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
