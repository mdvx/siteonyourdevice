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

#include "inner/http_inner_server_handler.h"

#include <string>

#include <common/logger.h>
#include <common/net/net.h>
#include <common/threads/event_bus.h>

#include "network/network_events.h"

#include "inner/inner_client.h"
#include "inner/inner_relay_client.h"

#define GB (1024 * 1024 * 1024)
#define BUF_SIZE 4096

namespace fasto {
namespace siteonyourdevice {
namespace inner {

Http2ClientServerHandler::Http2ClientServerHandler(const HttpServerInfo &info)
    : Http2ServerHandler(info, nullptr) {}

Http2ClientServerHandler::~Http2ClientServerHandler() {}

void Http2ClientServerHandler::preLooped(tcp::ITcpLoop *server) {
  Http2ServerHandler::preLooped(server);
}

void Http2ClientServerHandler::accepted(tcp::TcpClient *client) {}

void Http2ClientServerHandler::closed(tcp::TcpClient *client) {
  ProxyRelayClient *prclient =
      dynamic_cast<ProxyRelayClient *>(client); // proxyrelay connection
  if (prclient) {
    RelayClientEx *rclient = prclient->relay();
    rclient->setEclient(nullptr);
  }
}

void Http2ClientServerHandler::postLooped(tcp::ITcpLoop *server) {
  Http2ServerHandler::postLooped(server);
}

void Http2ClientServerHandler::timerEmited(tcp::ITcpLoop *server,
                                           timer_id_t id) {}

void Http2ClientServerHandler::relayDataReceived(inner::RelayClient *rclient) {
  char buff[BUF_SIZE] = {0};
  size_t nread = 0;
  common::ErrnoError err = rclient->read(buff, BUF_SIZE, &nread);
  if (err || nread == 0) {
    rclient->close();
    delete rclient;
    return;
  }

  Http2ServerHandler::processReceived(rclient, buff, nread);
}

void Http2ClientServerHandler::relayExDataReceived(
    inner::RelayClientEx *rclient) {
  char buff[BUF_SIZE] = {0};
  size_t nread = 0;
  common::ErrnoError err = rclient->read(buff, BUF_SIZE, &nread);
  if (err || nread == 0) {
    rclient->close();
    delete rclient;
    return;
  }

  const common::net::HostAndPort externalHost = rclient->externalHost();

  common::net::socket_info client_info;
  common::http::http_protocols protocol = common::http::HP_1_1;
  if (common::http2::is_preface_data(buff, nread)) {
    protocol = common::http::HP_2_0;
  } else if (common::http2::is_frame_header_data(buff, nread)) {
    protocol = common::http::HP_2_0;
  }

  if (externalHost.IsValid()) {
    ProxyRelayClient *eclient = rclient->eclient();
    if (!eclient) {
      common::ErrnoError err = common::net::connect(
          externalHost, common::net::ST_SOCK_STREAM, 0, &client_info);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
        const std::string error_text = err->GetDescription();
        err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR,
                                  nullptr, error_text.c_str(), false, info());
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
        }
        return;
      }

      tcp::ITcpLoop *server = rclient->server();
      eclient = new ProxyRelayClient(server, client_info, rclient);
      server->registerClient(eclient);
      rclient->setEclient(eclient);
    }

    CHECK(eclient);

    size_t nwrite = 0;
    err = eclient->write(buff, nread, &nwrite);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      const std::string error_text = err->GetDescription();
      err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR,
                                nullptr, error_text.c_str(), false, info());
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return;
    }
  } else {
    err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR,
                              nullptr, "Invalid external host!", false, info());
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
  }
}

void Http2ClientServerHandler::proxyDataReceived(ProxyRelayClient *prclient) {
  char buff[BUF_SIZE] = {0};
  size_t nread = 0;
  common::ErrnoError err = prclient->read(buff, BUF_SIZE, &nread);
  if (err || nread == 0) {
    prclient->close();
    delete prclient;
    return;
  }

  RelayClient *rclient = prclient->relay();
  size_t nwrite = 0;
  err = rclient->TcpClient::write(buff, nread, &nwrite);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
  }

  return;
}

void Http2ClientServerHandler::dataReceived(tcp::TcpClient *client) {
  RelayClientEx *rexclient =
      dynamic_cast<RelayClientEx *>(client); // relay external connection
  if (rexclient) {
    relayExDataReceived(rexclient);
    return;
  }

  RelayClient *rclient =
      dynamic_cast<RelayClient *>(client); // relay connection
  if (rclient) {
    relayDataReceived(rclient);
    return;
  }

  ProxyRelayClient *prclient =
      dynamic_cast<ProxyRelayClient *>(client); // proxyrelay connection
  if (prclient) {
    proxyDataReceived(prclient);
    return;
  }

  Http2ServerHandler::dataReceived(client); // direct connection
}

void Http2ClientServerHandler::dataReadyToWrite(tcp::TcpClient *client) {}

} // namespace inner
} // namespace siteonyourdevice
} // namespace fasto
