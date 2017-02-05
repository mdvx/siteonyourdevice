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

#include "server/http_server_host.h"

#include <unistd.h>

#include <string>

#include <common/thread/thread_manager.h>
#include <common/logger.h>

#include "inner/inner_tcp_client.h"

#include "http/http_client.h"

#include "loop_controller.h"

#define BUF_SIZE 4096

namespace fasto {
namespace siteonyourdevice {
namespace server {

namespace {

int exec_http_server(http::Http2Server* server) {
  common::Error err = server->bind();
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  err = server->listen(5);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  return server->exec();
}

int exec_inner_server(inner::InnerTcpServer* server) {
  common::Error err = server->bind();
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  err = server->listen(5);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  return server->exec();
}

int exec_websocket_server(websocket::WebSocketServerHost* server) {
  common::Error err = server->bind();
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  err = server->listen(5);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  return server->exec();
}

}  // namespace

HttpInnerServerHandlerHost::HttpInnerServerHandlerHost(const HttpServerInfo& info,
                                                       HttpServerHost* parent)
    : Http2ServerHandler(info, NULL), parent_(parent) {}

HttpInnerServerHandlerHost::~HttpInnerServerHandlerHost() {}

void HttpInnerServerHandlerHost::accepted(tcp::TcpClient* client) {}

void HttpInnerServerHandlerHost::closed(tcp::TcpClient* client) {}

void HttpInnerServerHandlerHost::dataReceived(tcp::TcpClient* client) {
  char buff[BUF_SIZE] = {0};
  ssize_t nread = 0;
  common::Error err = client->read(buff, BUF_SIZE, &nread);
  if ((err && err->isError()) || nread == 0) {
    client->close();
    delete client;
    return;
  }

  client_t* hclient = dynamic_cast<client_t*>(client);
  CHECK(hclient);

  std::string request(buff, nread);
  common::http::http_request hrequest;
  auto result = parse_http_request(request, &hrequest);

  if (result.second && result.second->isError()) {
    const std::string error_text = result.second->description();
    hclient->send_error(common::http::HP_1_1, result.first, NULL, error_text.c_str(), false,
                        info());
    hclient->close();
    delete hclient;
    return;
  }

  processHttpRequest(hclient, hrequest);
}

void HttpInnerServerHandlerHost::processHttpRequest(http::HttpClient* hclient,
                                                    const common::http::http_request& hrequest) {
  const common::http::http_protocols protocol = hrequest.protocol();
  common::uri::Upath path = hrequest.path();
  std::string hpath = path.hpath();
  std::string fpath = path.filename();

  inner::InnerTcpServerClient* innerConnection = parent_->findInnerConnectionByHost(hpath);
  /*if (!innerConnection) {
    common::http::header_t refererField = hrequest.findHeaderByKey("Referer", false);
    if (refererField.isValid()) {
      common::uri::Uri refpath(refererField.value);
      common::uri::Upath rpath = refpath.path();
      DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "hpath: %s, fpath %s", hpath, fpath);
      hpath = rpath.path();
      fpath = path.path();
      DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "after hpath: %s, fpath %s", hpath, fpath);
    }

    innerConnection = parent_->findInnerConnectionByHost(hpath);
  }*/

  if (!innerConnection) {
    WARNING_LOG() << "HttpInnerServerHandlerHost not found host " << hpath << ", request str:\n"
                  << common::ConvertToString(hrequest);
    std::string msg = common::MemSPrintf(
        "Not registered host(%s) or it is offline(if it is your host, please build server "
        "installer(button in your profile page) and run it on your device!).",
        hpath);
    hclient->send_error(protocol, common::http::HS_NOT_FOUND, NULL, msg.c_str(), false, info());
    hclient->close();
    delete hclient;
    return;
  }

  hclient->setName(hpath);

  common::http::http_request chrequest = hrequest;
  path.setPath(fpath);
  chrequest.setPath(path);

  tcp::ITcpLoop* server = hclient->server();
  server->unregisterClient(hclient);
  innerConnection->addHttpRelayClient(parent_->innerHandler(), hclient,
                                      common::ConvertToBytes(chrequest));
}

HttpServerHost::HttpServerHost(const common::net::HostAndPort& httpHost,
                               const common::net::HostAndPort& innerHost,
                               const common::net::HostAndPort& webSocketHost)
    : httpHandler_(NULL),
      httpServer_(NULL),
      http_thread_(),
      innerHandler_(NULL),
      innerServer_(NULL),
      inner_thread_(),
      websocketHandler_(NULL),
      websocketServer_(NULL),
      websocket_thread_(),
      connections_(),
      rstorage_() {
  HttpServerInfo hinf(PROJECT_NAME_SERVER_TITLE, PROJECT_DOMAIN);

  httpHandler_ = new HttpInnerServerHandlerHost(hinf, this);
  httpServer_ = new http::Http2Server(httpHost, httpHandler_);
  httpServer_->setName("proxy_http_server");
  http_thread_ = THREAD_MANAGER()->createThread(&exec_http_server, httpServer_);

  innerHandler_ = new inner::InnerServerHandlerHost(this);
  innerServer_ = new inner::InnerTcpServer(innerHost, innerHandler_);
  innerServer_->setName("inner_server");
  inner_thread_ = THREAD_MANAGER()->createThread(&exec_inner_server, innerServer_);

  websocketHandler_ = new websocket::WebSocketServerHandlerHost(hinf, this);
  websocketServer_ = new websocket::WebSocketServerHost(webSocketHost, websocketHandler_);
  websocketServer_->setName("websocket_server");
  websocket_thread_ = THREAD_MANAGER()->createThread(&exec_websocket_server, websocketServer_);
}

inner::InnerServerHandlerHost* HttpServerHost::innerHandler() const {
  return innerHandler_;
}

bool HttpServerHost::unRegisterInnerConnectionByHost(tcp::TcpClient* connection) {
  inner::InnerTcpServerClient* iconnection = dynamic_cast<inner::InnerTcpServerClient*>(connection);
  if (!iconnection) {
    return false;
  }

  UserAuthInfo hinf = iconnection->serverHostInfo();
  if (!hinf.isValid()) {
    return false;
  }

  std::string host = hinf.host.host;
  connections_.erase(host);
  return true;
}

bool HttpServerHost::registerInnerConnectionByUser(const UserAuthInfo& user,
                                                   tcp::TcpClient* connection) {
  CHECK(user.isValid());
  inner::InnerTcpServerClient* iconnection = dynamic_cast<inner::InnerTcpServerClient*>(connection);
  if (!iconnection) {
    return false;
  }

  iconnection->setServerHostInfo(user);

  std::string host = user.host.host;
  connections_[host] = iconnection;
  return true;
}

bool HttpServerHost::findUser(const UserAuthInfo& user) const {
  return rstorage_.findUser(user);
}

inner::InnerTcpServerClient* HttpServerHost::findInnerConnectionByHost(
    const std::string& host) const {
  inner_connections_type::const_iterator hs = connections_.find(host);
  if (hs == connections_.end()) {
    return nullptr;
  }

  return (*hs).second;
}

void HttpServerHost::setStorageConfig(const redis_sub_configuration_t& config) {
  rstorage_.setConfig(config);
  innerHandler_->setStorageConfig(config);
}

HttpServerHost::~HttpServerHost() {
  delete httpServer_;
  delete innerServer_;
  delete websocketServer_;

  delete websocketHandler_;
  delete innerHandler_;
  delete httpHandler_;
}

int HttpServerHost::exec() {
  http_thread_->start();
  inner_thread_->start();
  websocket_thread_->start();

  int res = http_thread_->joinAndGet();
  res |= inner_thread_->joinAndGet();
  res |= websocket_thread_->joinAndGet();
  return res;
}

void HttpServerHost::stop() {
  httpServer_->stop();
  innerServer_->stop();
  websocketServer_->stop();
}

}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
