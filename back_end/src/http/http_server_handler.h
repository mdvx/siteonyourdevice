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

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <common/http/http.h>
#include <common/libev/io_loop_observer.h>
#include <common/uri/url.h>

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

class HttpServerHandler : public common::libev::IoLoopObserver {
 public:
  typedef std::shared_ptr<IHttpCallback> http_callback_t;
  typedef std::map<std::string, http_callback_t> http_callbacks_t;

  typedef std::pair<common::uri::Url, ILoopController*> socket_url_t;
  typedef std::vector<socket_url_t> sockets_url_t;

  HttpServerHandler(const HttpServerInfo& info, IHttpAuthObserver* observer);
  virtual void PreLooped(common::libev::IoLoop* server) override;
  virtual void Accepted(common::libev::IoClient* client) override;
  virtual void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  virtual void Closed(common::libev::IoClient* client) override;
  virtual void DataReceived(common::libev::IoClient* client) override;
  virtual void DataReadyToWrite(common::libev::IoClient* client) override;
  virtual void PostLooped(common::libev::IoLoop* server) override;
  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

  virtual ~HttpServerHandler();

  void registerHttpCallback(const std::string& url, http_callback_t callback);
  void registerSocketUrl(const common::uri::Url& url);

  void setAuthChecker(IHttpAuthObserver* authChecker);

  const HttpServerInfo& info() const;

 protected:
  virtual void processReceived(HttpClient* hclient, const char* request, size_t req_len);
  virtual void handleRequest(HttpClient* hclient, const common::http::http_request& hrequest, bool notClose);

 private:
  bool tryToHandleAsRegisteredCallback(HttpClient* hclient,
                                       const std::string& uri,
                                       const common::http::http_request& request);
  bool tryAuthenticateIfNeeded(HttpClient* hclient,
                               const char* extra_header,
                               const common::http::http_request& request);

  http_callbacks_t httpCallbacks_;
  const std::shared_ptr<IHttpCallback> fshandler_;
  IHttpAuthObserver* authChecker_;

  sockets_url_t sockets_urls_;

  const HttpServerInfo info_;
};

class Http2ServerHandler : public HttpServerHandler {
 public:
  Http2ServerHandler(const HttpServerInfo& info, IHttpAuthObserver* observer);

 protected:
  virtual void processReceived(HttpClient* hclient, const char* request, size_t req_len) override;

 private:
  void handleHttp2Request(Http2Client* h2client, const char* request, uint32_t req_len);
};

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
