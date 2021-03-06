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

#include <string>

#include "server/websocket/websocket_server.h"

#include <common/convert2string.h>
#include <common/logger.h>

#include "server/http_server_host.h"
#include "server/inner/inner_tcp_client.h"
#include "server/websocket/websocket_client.h"

#define BUF_SIZE 4096

namespace fasto {
namespace siteonyourdevice {
namespace server {
namespace websocket {

WebSocketServerHost::WebSocketServerHost(const common::net::HostAndPort& host, common::libev::IoLoopObserver* observer)
    : Http2Server(host, observer) {}

const char* WebSocketServerHost::ClassName() const {
  return "WebSocketServerHost";
}

common::libev::tcp::TcpClient* WebSocketServerHost::CreateClient(const common::net::socket_info& info) {
  return new WebSocketClientHost(this, info);
}

WebSocketServerHandlerHost::WebSocketServerHandlerHost(const HttpServerInfo& info, HttpServerHost* parent)
    : Http2ServerHandler(info, NULL), parent_(parent) {}

void WebSocketServerHandlerHost::DataReceived(common::libev::IoClient* client) {
  char buff[BUF_SIZE] = {0};
  size_t nread = 0;
  common::Error err = client->Read(buff, BUF_SIZE, &nread);
  if (err || nread == 0) {
    client->Close();
    delete client;
    return;
  }

  WebSocketClientHost* hclient = dynamic_cast<WebSocketClientHost*>(client);
  CHECK(hclient);

  std::string request(buff, nread);
  common::http::http_request hrequest;
  auto result = parse_http_request(request, &hrequest);

  if (result.second) {
    const std::string error_text = result.second->GetDescription();
    hclient->send_error(common::http::HP_1_1, result.first, NULL, error_text.c_str(), false, info());
    hclient->Close();
    delete hclient;
    return;
  }

  processWebsocketRequest(hclient, hrequest);
}

void WebSocketServerHandlerHost::processWebsocketRequest(http::HttpClient* hclient,
                                                         const common::http::http_request& hrequest) {
  const common::http::http_protocols protocol = hrequest.protocol();
  common::uri::Upath path = hrequest.path();
  std::string hpath = path.GetHpath();
  std::string fpath = path.GetFileName();
  common::net::HostAndPort host;
  common::ConvertFromString(hpath, &host);
  std::string hpath_without_port = host.GetHost();

  inner::InnerTcpServerClient* innerConnection = parent_->findInnerConnectionByHost(hpath_without_port);
  if (!innerConnection) {
    WARNING_LOG() << "WebSocketServerHandlerHost not found host " << hpath << " request str:\n"
                  << common::ConvertToString(hrequest);
    hclient->send_error(protocol, common::http::HS_NOT_FOUND, NULL, "Not registered host.", false, info());
    hclient->Close();
    delete hclient;
    return;
  }

  hclient->SetName(common::ConvertToString(host));

  common::http::http_request chrequest = hrequest;
  path.SetPath(fpath);
  chrequest.setPath(path);

  common::buffer_t res = common::ConvertToBytes(chrequest);

  common::libev::IoLoop* server = hclient->GetServer();
  server->UnRegisterClient(hclient);
  innerConnection->addWebsocketRelayClient(parent_->innerHandler(), hclient, res,
                                           common::net::HostAndPort::CreateLocalHost(host.GetPort()));
}

}  // namespace websocket
}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
