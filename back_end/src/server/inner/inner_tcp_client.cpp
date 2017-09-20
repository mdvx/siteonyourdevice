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

#include "server/inner/inner_tcp_client.h"

#include <common/logger.h>

#include "server/inner/inner_tcp_server.h"
#include "server/server_commands.h"
#include "server/server_config.h"

#define BUF_SIZE 4096

namespace {

class RelayHandler : public common::libev::IoLoopObserver {
  common::libev::IoLoop* server_;

  common::libev::IoClient* client_primary_;
  common::libev::IoClient* client_device_;

  const fasto::siteonyourdevice::server::inner::IInnerRelayLoop::request_t request_;

 public:
  explicit RelayHandler(const fasto::siteonyourdevice::server::inner::IInnerRelayLoop::request_t& req)
      : common::libev::IoLoopObserver(), server_(NULL), client_primary_(NULL), client_device_(NULL), request_(req) {}

  bool readyForRequest() const { return !client_primary_ && client_device_; }

  void setClient(common::libev::IoClient* client, const common::buffer_t& request) {
    if (client_primary_) {
      DNOTREACHED();
      return;
    }

    client_primary_ = client;
    client_primary_->SetName("client");

    auto cb = [this, request]() {
      server_->RegisterClient(client_primary_);
      if (!client_device_ || request.empty()) {
        return;
      }

      size_t nwrite = 0;
      common::Error err = client_device_->Write((const char*)request.data(), request.size(), &nwrite);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
    };

    server_->ExecInLoopThread(cb);
  }

 private:
  void PreLooped(common::libev::IoLoop* server) { server_ = server; }

  virtual void Accepted(common::libev::IoClient* client) {
    if (client == client_primary_) {
      return;
    }

    if (!client_device_) {
      client_device_ = client;
      client_device_->SetName("device");

      setClient(request_.first, request_.second);
    }
  }

  virtual void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {}

  virtual void Closed(common::libev::IoClient* client) {
    if (client == client_primary_) {
      client_primary_ = NULL;
      return;
    }

    if (client == client_device_) {
      client_device_ = NULL;
      return;
    }
  }

  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {}

  void DataReceived(common::libev::IoClient* client) {
    char buff[BUF_SIZE] = {0};
    size_t nread = 0;
    common::Error err = client->Read(buff, BUF_SIZE, &nread);
    if (err || nread == 0) {
      client->Close();
      delete client;
      return;
    }

    size_t nwrite = 0;
    if (client == client_primary_) {
      if (client_device_) {
        err = client_device_->Write(buff, nread, &nwrite);
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
        }
      }
    } else if (client == client_device_) {
      if (client_primary_) {
        err = client_primary_->Write(buff, nread, &nwrite);
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
          client->Close();
          delete client;
        }
      }
    } else {
      NOTREACHED();
    }
  }

  virtual void DataReadyToWrite(common::libev::IoClient* client) {}

  virtual void PostLooped(common::libev::IoLoop* server) {}
};

class HttpRelayLoop : public fasto::siteonyourdevice::server::inner::IInnerRelayLoop {
 public:
  HttpRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser* handler,
                fasto::siteonyourdevice::server::inner::InnerTcpServerClient* parent,
                const request_t& request)
      : IInnerRelayLoop(handler, parent, request) {}

 private:
  virtual common::libev::IoLoopObserver* CreateHandler() override { return new RelayHandler(request_); }

  common::libev::IoLoop* CreateServer(common::libev::IoLoopObserver* handler) {
    common::libev::tcp::TcpServer* serv =
        new common::libev::tcp::TcpServer(fasto::siteonyourdevice::server::g_relay_server_host, handler);
    serv->SetName("http_proxy_relay_server");

    common::ErrnoError err = serv->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return NULL;
    }

    err = serv->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return NULL;
    }

    size_t nwrite = 0;
    fasto::siteonyourdevice::cmd_request_t createConnection =
        ihandler_->make_request(SERVER_PLEASE_CONNECT_HTTP_COMMAND_REQ_1S, common::ConvertToString(serv->GetHost()));
    common::Error err1 = parent_->Write(createConnection, &nwrite);  // inner command write
    if (err1) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return NULL;
    }

    return serv;
  }
};

class WebSocketRelayLoop : public fasto::siteonyourdevice::server::inner::IInnerRelayLoop {
  const common::net::HostAndPort srcHost_;

 public:
  WebSocketRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser* handler,
                     fasto::siteonyourdevice::server::inner::InnerTcpServerClient* parent,
                     const request_t& request,
                     const common::net::HostAndPort& srcHost)
      : IInnerRelayLoop(handler, parent, request), srcHost_(srcHost) {}

 private:
  virtual common::libev::IoLoopObserver* CreateHandler() { return new RelayHandler(request_); }

  common::libev::IoLoop* CreateServer(common::libev::IoLoopObserver* handler) {
    common::libev::tcp::TcpServer* serv =
        new common::libev::tcp::TcpServer(fasto::siteonyourdevice::server::g_relay_server_host, handler);
    serv->SetName("websockets_proxy_relay_server");

    common::ErrnoError err = serv->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return NULL;
    }

    err = serv->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return NULL;
    }

    size_t nwrite = 0;
    fasto::siteonyourdevice::cmd_request_t createConnection =
        ihandler_->make_request(SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND_REQ_2SS,
                                common::ConvertToString(serv->GetHost()), common::ConvertToString(srcHost_));
    common::Error err1 = parent_->Write(createConnection, &nwrite);  // inner command write
    if (err1) {
      DEBUG_MSG_ERROR(err1, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return NULL;
    }

    return serv;
  }
};
}  // namespace

namespace fasto {
namespace siteonyourdevice {
namespace server {
namespace inner {

IInnerRelayLoop::IInnerRelayLoop(siteonyourdevice::inner::InnerServerCommandSeqParser* handler,
                                 InnerTcpServerClient* parent,
                                 const request_t& request)
    : ILoopThreadController(), parent_(parent), ihandler_(handler), request_(request) {}

bool IInnerRelayLoop::readyForRequest() const {
  RelayHandler* hand = dynamic_cast<RelayHandler*>(handler_);
  CHECK(hand);

  return hand->readyForRequest();
}

void IInnerRelayLoop::addRequest(const request_t& request) {
  RelayHandler* hand = dynamic_cast<RelayHandler*>(handler_);
  CHECK(hand);

  hand->setClient(request.first, request.second);
}

IInnerRelayLoop::~IInnerRelayLoop() {
  stop();
  join();
}

InnerTcpServerClient::InnerTcpServerClient(common::libev::IoLoop* server, const common::net::socket_info& info)
    : InnerClient(server, info), hinfo_(), relays_http_(), relays_websockets_() {}

const char* InnerTcpServerClient::ClassName() const {
  return "InnerTcpServerClient";
}

void InnerTcpServerClient::addHttpRelayClient(InnerServerHandlerHost* handler,
                                              common::libev::tcp::TcpClient* client,
                                              const common::buffer_t& request) {
  for (size_t i = 0; i < relays_http_.size(); ++i) {
    http_relay_loop_t loop = relays_http_[i];
    if (loop->readyForRequest()) {
      loop->addRequest(std::make_pair(client, request));
      return;
    }
  }

  http_relay_loop_t tmp(new HttpRelayLoop(handler, this, std::make_pair(client, request)));
  tmp->start();

  relays_http_.push_back(tmp);
}

void InnerTcpServerClient::addWebsocketRelayClient(InnerServerHandlerHost* handler,
                                                   common::libev::tcp::TcpClient* client,
                                                   const common::buffer_t& request,
                                                   const common::net::HostAndPort& srcHost) {
  websocket_relay_loop_t tmp(new WebSocketRelayLoop(handler, this, std::make_pair(client, request), srcHost));
  tmp->start();

  relays_websockets_.push_back(tmp);
}

InnerTcpServerClient::~InnerTcpServerClient() {}

void InnerTcpServerClient::setServerHostInfo(const UserAuthInfo& info) {
  hinfo_ = info;
}

UserAuthInfo InnerTcpServerClient::serverHostInfo() const {
  return hinfo_;
}

}  // namespace inner
}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
