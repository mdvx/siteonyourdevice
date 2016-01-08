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

#include <utility>
#include <map>
#include <string>
#include <vector>

#include "common/http/http.h"

#include "tcp/tcp_server.h"
#include "http/callbacks/http_callbacks.h"

#include "infos.h"

namespace fasto {
namespace siteonyourdevice {
class ILoopController;
namespace http {
class Http2Client;

class IHttpAuthObserver {
 public:
  IHttpAuthObserver();
  virtual bool userCanAuth(const std::string& user, const std::string& password) = 0;
  virtual ~IHttpAuthObserver();
};

class HttpServerHandler
        : public tcp::ITcpLoopObserver {
 public:
  typedef common::shared_ptr<IHttpCallback> http_callback_t;
  typedef std::map<std::string, http_callback_t> http_callbacks_t;

  typedef std::pair<common::uri::Uri, ILoopController*> socket_url_t;
  typedef std::vector<socket_url_t> sockets_url_t;

  HttpServerHandler(const HttpServerInfo& info, IHttpAuthObserver * observer);
  virtual void preLooped(tcp::ITcpLoop* server);
  virtual void accepted(tcp::TcpClient* client);
  virtual void moved(tcp::TcpClient* client);
  virtual void closed(tcp::TcpClient* client);
  virtual void dataReceived(tcp::TcpClient* client);
  virtual void dataReadyToWrite(tcp::TcpClient* client);
  virtual void postLooped(tcp::ITcpLoop* server);
  virtual void timerEmited(tcp::ITcpLoop* server, timer_id_type id);

  virtual ~HttpServerHandler();

  void registerHttpCallback(const std::string& url, http_callback_t callback);
  void registerSocketUrl(const common::uri::Uri& url);

  void setAuthChecker(IHttpAuthObserver * authChecker);

  const HttpServerInfo& info() const;

 protected:
  virtual void processReceived(HttpClient *hclient, const char* request, size_t req_len);
  virtual void handleRequest(HttpClient *hclient, const common::http::http_request& hrequest,
                             bool notClose);

 private:
  bool tryToHandleAsRegisteredCallback(HttpClient* hclient, const std::string& uri,
                                       const common::http::http_request& request);
  bool tryAuthenticateIfNeeded(HttpClient* hclient, const char* extra_header,
                               const common::http::http_request& request);


  http_callbacks_t httpCallbacks_;
  const common::shared_ptr<IHttpCallback> fshandler_;
  IHttpAuthObserver * authChecker_;

  sockets_url_t sockets_urls_;

  const HttpServerInfo info_;
};

class Http2ServerHandler
        : public HttpServerHandler {
 public:
  Http2ServerHandler(const HttpServerInfo &info, IHttpAuthObserver * observer);

 protected:
  virtual void processReceived(HttpClient *hclient, const char* request, size_t req_len);

 private:
  void handleHttp2Request(Http2Client* h2client, const char* request, uint32_t req_len);
};

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
