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

#include "http/http_server_handler.h"

#include <string>
#include <utility>

#include <common/convert2string.h>
#include <common/logger.h>
#include <common/net/net.h>
#include <common/string_util.h>
#include <common/utils.h>

#include "http/http_client.h"

#include "server/server_config.h"

#include "loop_controller.h"

#include "websocket/websocket_server.h"
#include "websocket/websocket_server_handler.h"

#define BUF_SIZE 4096
#define AUTH_BASIC_METHOD "Basic"

namespace {
class WebSocketController : public fasto::siteonyourdevice::ILoopThreadController {
  const common::net::HostAndPort host_;
  const fasto::siteonyourdevice::HttpServerInfo info_;

 public:
  WebSocketController(const common::net::HostAndPort& host, const fasto::siteonyourdevice::HttpServerInfo& info)
      : host_(host), info_(info) {}

 private:
  common::libev::IoLoopObserver* CreateHandler() override {
    return new fasto::siteonyourdevice::websocket::WebSocketServerHandler(info_);
  }

  common::libev::IoLoop* CreateServer(common::libev::IoLoopObserver* handler) override {
    fasto::siteonyourdevice::websocket::WebSocketServer* serv =
        new fasto::siteonyourdevice::websocket::WebSocketServer(host_, handler);
    serv->SetName("websocket_server");

    common::ErrnoError err = serv->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return nullptr;
    }

    err = serv->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      delete serv;
      return nullptr;
    }

    return serv;
  }
};
}  // namespace

namespace fasto {
namespace siteonyourdevice {
namespace http {

IHttpAuthObserver::IHttpAuthObserver() {}

IHttpAuthObserver::~IHttpAuthObserver() {}

void HttpServerHandler::PreLooped(common::libev::IoLoop* server) {
  UNUSED(server);

  for (size_t i = 0; i < sockets_urls_.size(); ++i) {
    socket_url_t url = sockets_urls_[i];
    CHECK(url.second == nullptr);

    common::net::HostAndPort host;
    if (!common::ConvertFromString(url.first.GetHost(), &host)) {
      continue;
    }

    ILoopThreadController* loopc = new WebSocketController(host, info());
    loopc->start();
    sockets_urls_[i].second = loopc;
  }
}

void HttpServerHandler::PostLooped(common::libev::IoLoop* server) {
  UNUSED(server);

  for (size_t i = 0; i < sockets_urls_.size(); ++i) {
    socket_url_t url = sockets_urls_[i];

    if (!url.second) {
      continue;
    }

    url.second->stop();
    delete url.second;
    sockets_urls_[i].second = nullptr;
  }
}

void HttpServerHandler::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  UNUSED(server);
  UNUSED(id);
}

void HttpServerHandler::Accepted(common::libev::IoClient* client) {
  UNUSED(client);
}

void HttpServerHandler::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(client);
}

void HttpServerHandler::Closed(common::libev::IoClient* client) {
  UNUSED(client);
}

void HttpServerHandler::DataReceived(common::libev::IoClient* client) {
  char buff[BUF_SIZE] = {0};
  size_t nread = 0;
  common::Error err = client->Read(buff, BUF_SIZE, &nread);
  if (err || nread == 0) {
    client->Close();
    delete client;
    return;
  }

  HttpClient* hclient = dynamic_cast<HttpClient*>(client);
  CHECK(hclient);
  processReceived(hclient, buff, nread);
}

void HttpServerHandler::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);
}

void HttpServerHandler::registerHttpCallback(const std::string& url, http_callback_t callback) {
  httpCallbacks_[url] = callback;
}

void HttpServerHandler::registerSocketUrl(const common::uri::Url& url) {
  sockets_urls_.push_back(std::make_pair(url, nullptr));
}

void HttpServerHandler::setAuthChecker(IHttpAuthObserver* authChecker) {
  authChecker_ = authChecker;
}

const HttpServerInfo& HttpServerHandler::info() const {
  return info_;
}

bool HttpServerHandler::tryToHandleAsRegisteredCallback(HttpClient* hclient,
                                                        const std::string& uri,
                                                        const common::http::http_request& request) {
  http_callbacks_t::const_iterator it = httpCallbacks_.find(uri);
  if (it == httpCallbacks_.end()) {
    return false;
  }

  http_callback_t callback = (*it).second;
  if (!callback) {
    NOTREACHED();
    return false;
  }

  return callback->handleRequest(hclient, nullptr, request, info());
}

bool HttpServerHandler::tryAuthenticateIfNeeded(HttpClient* hclient,
                                                const char* extra_header,
                                                const common::http::http_request& request) {
  UNUSED(extra_header);

  if (!authChecker_) {
    return false;
  }

  if (hclient->isAuthenticated()) {
    return false;
  }

  common::http::header_t authField = request.findHeaderByKey("Authorization", false);
  if (authField.IsValid()) {
    std::string auth = authField.value;
    size_t delem_method = auth.find_first_of(' ');
    if (delem_method != std::string::npos) {
      const std::string method = auth.substr(0, delem_method);
      if (method == AUTH_BASIC_METHOD) {
        std::string enc_auth = common::utils::base64::decode64(auth.substr(delem_method + 1));

        size_t delem = enc_auth.find_first_of(':');
        if (delem != std::string::npos) {
          const std::string name = enc_auth.substr(0, delem);
          const std::string password = enc_auth.substr(delem + 1);
          if (authChecker_->userCanAuth(name, password)) {
            hclient->setIsAuthenticated(true);
            return false;
          }
        }
      }
    }

    common::Error err = hclient->send_error(
        common::http::HP_1_1, common::http::HS_UNAUTHORIZED,
        "WWW-Authenticate: " AUTH_BASIC_METHOD " realm=User or password incorrect, try again", nullptr, true, info());
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    return true;
  }

  common::Error err = hclient->send_error(
      common::http::HP_1_1, common::http::HS_UNAUTHORIZED,
      "WWW-Authenticate: " AUTH_BASIC_METHOD " realm=Private page please authenticate", nullptr, true, info());
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
  }

  return true;
}

void HttpServerHandler::processReceived(HttpClient* hclient, const char* request, size_t req_len) {
  common::http::http_request hrequest;
  std::string request_str(request, req_len);
  std::pair<common::http::http_status, common::Error> result = common::http::parse_http_request(request_str, &hrequest);

  if (result.second) {
    const std::string error_text = result.second->GetDescription();
    DEBUG_MSG_ERROR(result.second, common::logging::LOG_LEVEL_ERR);
    common::Error err =
        hclient->send_error(common::http::HP_1_1, result.first, nullptr, error_text.c_str(), false, info());
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    hclient->Close();
    delete hclient;
    return;
  }

  // keep alive
  common::http::header_t connectionField = hrequest.findHeaderByKey("Connection", false);
  bool isKeepAlive = common::EqualsASCII(connectionField.value, "Keep-Alive", false);

  common::http::header_t hostField = hrequest.findHeaderByKey("Host", false);
  bool isProxy = common::EqualsASCII(hostField.value, HTTP_PROXY_HOST_NAME, false);

  handleRequest(hclient, hrequest, isKeepAlive | isProxy);
}

void HttpServerHandler::handleRequest(HttpClient* hclient, const common::http::http_request& hrequest, bool notClose) {
  common::uri::Upath path = hrequest.path();

  if (tryAuthenticateIfNeeded(hclient, nullptr, hrequest)) {
    goto cleanup;
  }

  if (tryToHandleAsRegisteredCallback(hclient, path.GetPath(), hrequest)) {
    goto cleanup;
  }

  if (fshandler_->handleRequest(hclient, nullptr, hrequest, info())) {
    goto cleanup;
  }

  NOTREACHED();

cleanup:
  if (!notClose) {
    hclient->Close();
    delete hclient;
  }
}

HttpServerHandler::HttpServerHandler(const HttpServerInfo& info, IHttpAuthObserver* observer)
    : fshandler_(IHttpCallback::createHttpCallback(file_system)), authChecker_(observer), info_(info) {
  CHECK(fshandler_);
}

HttpServerHandler::~HttpServerHandler() {}

Http2ServerHandler::Http2ServerHandler(const HttpServerInfo& info, IHttpAuthObserver* observer)
    : HttpServerHandler(info, observer) {}

void Http2ServerHandler::processReceived(HttpClient* hclient, const char* request, size_t req_len) {
  if (common::http2::is_preface_data(request, req_len)) {
    Http2Client* h2client = dynamic_cast<Http2Client*>(hclient);
    CHECK(h2client);

    const char* settings_frame = request + PREFACE_STARTS_LEN;
    uint32_t set_lengh = req_len - PREFACE_STARTS_LEN;
    handleHttp2Request(h2client, settings_frame, set_lengh);
  } else if (common::http2::is_frame_header_data(request, req_len)) {
    Http2Client* h2client = dynamic_cast<Http2Client*>(hclient);
    CHECK(h2client);

    handleHttp2Request(h2client, request, req_len);
  } else {
    HttpServerHandler::processReceived(hclient, request, req_len);
  }
}

void Http2ServerHandler::handleHttp2Request(Http2Client* h2client, const char* request, uint32_t req_len) {
  const std::string hexstr = common::utils::hex::encode(common::StringPiece(request, req_len), false);

  common::http2::frames_t frames = common::http2::parse_frames(request, req_len);
  INFO_LOG() << "frame_header_data hex: " << hexstr;
  h2client->processFrames(frames);

  common::http2::frames_t headers_frames = common::http2::find_frames_by_type(frames, common::http2::HTTP2_HEADERS);
  for (size_t i = 0; i < headers_frames.size(); ++i) {
    common::http2::frame_headers* head = (common::http2::frame_headers*)(&headers_frames[i]);
    common::http::http_request request;
    auto result = common::http2::parse_http_request(*head, &request);
    if (result.second) {
      const std::string error_text = result.second->GetDescription();
      DEBUG_MSG_ERROR(result.second, common::logging::LOG_LEVEL_ERR);
      common::Error err =
          h2client->send_error(common::http::HP_2_0, result.first, nullptr, error_text.c_str(), false, info());
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      h2client->Close();
      delete h2client;
      return;
    }

    handleRequest(h2client, request, true);
  }
}

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
