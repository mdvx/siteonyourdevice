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

#include <time.h>

#include <vector>

#include "tcp/tcp_client.h"

#include "http/http_streams.h"

#include "infos.h"

namespace fasto {
namespace siteonyourdevice {
namespace http {

class HttpClient : public tcp::TcpClient {
 public:
  HttpClient(tcp::ITcpLoop* server, const common::net::socket_info& info);

  common::Error send_ok(common::http::http_protocols protocol,
                        const char* extra_header,
                        const char* text,
                        bool is_keep_alive,
                        const HttpServerInfo& info) WARN_UNUSED_RESULT;
  virtual common::Error send_error(common::http::http_protocols protocol,
                                   common::http::http_status status,
                                   const char* extra_header,
                                   const char* text,
                                   bool is_keep_alive,
                                   const HttpServerInfo& info) WARN_UNUSED_RESULT;
  virtual common::Error send_file_by_fd(common::http::http_protocols protocol,
                                        int fdesc,
                                        off_t size) WARN_UNUSED_RESULT;
  virtual common::Error send_headers(common::http::http_protocols protocol,
                                     common::http::http_status status,
                                     const char* extra_header,
                                     const char* mime_type,
                                     off_t* length,
                                     time_t* mod,
                                     bool is_keep_alive,
                                     const HttpServerInfo& info) WARN_UNUSED_RESULT;

  virtual const char* ClassName() const;

  void setIsAuthenticated(bool auth);
  bool isAuthenticated() const;

 private:
  bool isAuth_;
};

class Http2Client : public HttpClient {
 public:
  typedef StreamSPtr stream_t;
  typedef std::vector<stream_t> streams_t;

  Http2Client(tcp::ITcpLoop* server, const common::net::socket_info& info);

  virtual common::Error send_error(common::http::http_protocols protocol,
                                   common::http::http_status status,
                                   const char* extra_header,
                                   const char* text,
                                   bool is_keep_alive,
                                   const HttpServerInfo& info) WARN_UNUSED_RESULT;
  virtual common::Error send_file_by_fd(common::http::http_protocols protocol,
                                        int fdesc,
                                        off_t size) WARN_UNUSED_RESULT;
  virtual common::Error send_headers(common::http::http_protocols protocol,
                                     common::http::http_status status,
                                     const char* extra_header,
                                     const char* mime_type,
                                     off_t* length,
                                     time_t* mod,
                                     bool is_keep_alive,
                                     const HttpServerInfo& info) WARN_UNUSED_RESULT;

  void processFrames(const common::http2::frames_t& frames);

  bool isSettingNegotiated() const;

  virtual const char* ClassName() const;

 private:
  bool is_http2() const;
  StreamSPtr findStreamByStreamID(IStream::stream_id_t stream_id) const;
  StreamSPtr findStreamByType(common::http2::frame_t type) const;
  streams_t streams_;
};

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
