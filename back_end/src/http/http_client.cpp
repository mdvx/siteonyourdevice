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

#include "http/http_client.h"

#include <errno.h>
#include <inttypes.h>

#include <string>

#include <common/convert2string.h>
#include <common/file_system/file_system.h>
#include <common/logger.h>
#include <common/net/net.h>
#include <common/sprintf.h>

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

namespace {

const char* HTML_PATTERN_ISISSSS7 =
    R"(<!DOCTYPE html>
  <html>
  <head>
  <meta http-equiv="Content-type" content="text/html;charset=UTF-8">
  <title>%d %s</title>
  </head>
  <body bgcolor="#cc9999">
  <h4>%d %s</h4>%s<hr>
  <address><a href="%s">%s</a></address>
  </body>
  </html>)";

struct SendDataHelper {
  fasto::siteonyourdevice::http::StreamSPtr header_stream;
  uint32_t all_size;
};

common::ErrnoError send_data_frame(const char* buff, uint32_t buff_len, void* user_data, uint32_t* processed) {
  SendDataHelper* helper = reinterpret_cast<SendDataHelper*>(user_data);
  fasto::siteonyourdevice::http::StreamSPtr header_stream = helper->header_stream;

  uint8_t flags = 0;
  if (helper->all_size - buff_len == 0) {
    flags = common::http2::HTTP2_FLAG_END_STREAM;
  }

  common::http2::frame_hdr hdr = common::http2::frame_data::create_frame_header(flags, header_stream->sid(), buff_len);
  common::http2::frame_data fdata(hdr, buff);
  common::ErrnoError err = header_stream->sendFrame(fdata);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return err;
  }

  *processed = buff_len;
  helper->all_size -= buff_len;

  return common::ErrnoError();
}

}  // namespace

namespace fasto {
namespace siteonyourdevice {
namespace http {

HttpClient::HttpClient(common::libev::IoLoop* server, const common::net::socket_info& info)
    : TcpClient(server, info), isAuth_(false) {}

const char* HttpClient::ClassName() const {
  return "HttpClient";
}

void HttpClient::setIsAuthenticated(bool auth) {
  isAuth_ = auth;
}

bool HttpClient::isAuthenticated() const {
  return isAuth_;
}

common::Error HttpClient::send_ok(common::http::http_protocols protocol,
                                  const char* extra_header,
                                  const char* text,
                                  bool is_keep_alive,
                                  const HttpServerInfo& info) {
  return send_error(protocol, common::http::HS_OK, extra_header, text, is_keep_alive, info);
}

common::Error HttpClient::send_error(common::http::http_protocols protocol,
                                     common::http::http_status status,
                                     const char* extra_header,
                                     const char* text,
                                     bool is_keep_alive,
                                     const HttpServerInfo& info) {
  CHECK(protocol <= common::http::HP_1_1);
  const std::string title = common::ConvertToString(status);

  char err_data[1024] = {0};
  off_t err_len = common::SNPrintf(err_data, sizeof(err_data), HTML_PATTERN_ISISSSS7, status, title, status, title,
                                   text, info.server_url, info.server_name);
  common::Error err = send_headers(protocol, status, extra_header, "text/html", &err_len, nullptr, is_keep_alive, info);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
  }

  size_t nwrite = 0;
  err = Write(err_data, err_len, &nwrite);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
  }
  return err;
}

common::ErrnoError HttpClient::send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size) {
  CHECK(protocol <= common::http::HP_1_1);
  return common::net::send_file_to_fd(GetFd(), fdesc, 0, size);
}

common::Error HttpClient::send_headers(common::http::http_protocols protocol,
                                       common::http::http_status status,
                                       const char* extra_header,
                                       const char* mime_type,
                                       off_t* length,
                                       time_t* mod,
                                       bool is_keep_alive,
                                       const HttpServerInfo& info) {
  CHECK(protocol <= common::http::HP_1_1);
  const std::string title = common::ConvertToString(status);

  time_t now = time(nullptr);
  char timebuf[100];
  strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));

  char header_data[1024] = {0};
  int cur_pos = common::SNPrintf(header_data, sizeof(header_data),
                                 protocol == common::http::HP_2_0 ? HTTP_2_0_PROTOCOL_NAME
                                     " %d %s\r\n"
                                     "Server: %s\r\n"
                                     "Date: %s\r\n"
                                                                  : HTTP_1_1_PROTOCOL_NAME
                                     " %d %s\r\n"
                                     "Server: %s\r\n"
                                     "Date: %s\r\n",
                                 status, title, info.server_name, timebuf);

  if (extra_header) {
    int exlen = common::SNPrintf(header_data + cur_pos, sizeof(header_data) - cur_pos, "%s\r\n", extra_header);
    cur_pos += exlen;
  }
  if (mime_type) {
    int mim_t =
        common::SNPrintf(header_data + cur_pos, sizeof(header_data) - cur_pos, "Content-Type: %s\r\n", mime_type);
    cur_pos += mim_t;
  }
  if (length) {
    int len = common::SNPrintf(header_data + cur_pos, sizeof(header_data) - cur_pos, "Content-Length: %" PRId32 "\r\n",
                               *length);
    cur_pos += len;
  }
  if (mod) {
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(mod));
    int mlen = common::SNPrintf(header_data + cur_pos, sizeof(header_data) - cur_pos, "Last-Modified: %s\r\n", timebuf);
    cur_pos += mlen;
  }

  if (!is_keep_alive) {
#define CONNECTION_CLOSE "Connection: close\r\n\r\n"
    const int last_len = sizeof(CONNECTION_CLOSE) - 1;
    memcpy(header_data + cur_pos, CONNECTION_CLOSE, last_len);
    cur_pos += last_len;
  } else {
#define CONNECTION_KEEP_ALIVE "Keep-Alive: timeout=15, max=100\r\n\r\n"
    const int last_len = sizeof(CONNECTION_KEEP_ALIVE) - 1;
    memcpy(header_data + cur_pos, CONNECTION_KEEP_ALIVE, last_len);
    cur_pos += last_len;
  }

  DCHECK(strlen(header_data) == cur_pos);
  size_t nwrite = 0;
  common::Error err = Write(header_data, cur_pos, &nwrite);
  DCHECK(!err);
  return err;
}

Http2Client::Http2Client(common::libev::IoLoop* server, const common::net::socket_info& info)
    : HttpClient(server, info), streams_() {}

const char* Http2Client::ClassName() const {
  return "Http2Client";
}

bool Http2Client::is_http2() const {
  StreamSPtr main_stream = findStreamByStreamID(0);
  return main_stream.get();
}

common::Error Http2Client::send_error(common::http::http_protocols protocol,
                                      common::http::http_status status,
                                      const char* extra_header,
                                      const char* text,
                                      bool is_keep_alive,
                                      const HttpServerInfo& info) {
  if (is_http2() && protocol == common::http::HP_2_0) {
    const std::string title = common::ConvertToString(status);
    char err_data[1024] = {0};
    off_t err_len = common::SNPrintf(err_data, sizeof(err_data), HTML_PATTERN_ISISSSS7, status, title, status, title,
                                     text, info.server_url, info.server_name);
    common::Error err =
        send_headers(protocol, status, extra_header, "text/html", &err_len, nullptr, is_keep_alive, info);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return err;
    }

    StreamSPtr header_stream = findStreamByType(common::http2::HTTP2_HEADERS);
    if (!header_stream) {
      common::ErrnoError errr = DEBUG_MSG_PERROR("findStreamByType", EAGAIN, common::logging::LOG_LEVEL_ERR);
      return common::make_error_from_errno(errr);
    }

    common::http2::frame_hdr hdr = common::http2::frame_data::create_frame_header(common::http2::HTTP2_FLAG_END_STREAM,
                                                                                  header_stream->sid(), err_len);
    common::http2::frame_data fdata(hdr, err_data);
    return common::make_error_from_errno(header_stream->sendFrame(fdata));
  }

  return HttpClient::send_error(protocol, status, extra_header, text, is_keep_alive, info);
}

common::ErrnoError Http2Client::send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size) {
  if (is_http2() && protocol == common::http::HP_2_0) {
    StreamSPtr header_stream = findStreamByType(common::http2::HTTP2_HEADERS);
    if (!header_stream) {
      return DEBUG_MSG_PERROR("findStreamByType", EAGAIN, common::logging::LOG_LEVEL_ERR);
    }

    SendDataHelper help;
    help.header_stream = header_stream;
    help.all_size = size;

    common::ErrnoError err = common::file_system::read_file_cb(fdesc, NULL, size, &send_data_frame, &help);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return err;
    }

    return common::ErrnoError();
  }

  return HttpClient::send_file_by_fd(protocol, fdesc, size);
}

common::Error Http2Client::send_headers(common::http::http_protocols protocol,
                                        common::http::http_status status,
                                        const char* extra_header,
                                        const char* mime_type,
                                        off_t* length,
                                        time_t* mod,
                                        bool is_keep_alive,
                                        const HttpServerInfo& info) {
  if (is_http2() && protocol == common::http::HP_2_0) {
    StreamSPtr header_stream = findStreamByType(common::http2::HTTP2_HEADERS);
    if (!header_stream) {
      common::ErrnoError err = DEBUG_MSG_PERROR("findStreamByType", EAGAIN, common::logging::LOG_LEVEL_ERR);
      return common::make_error_from_errno(err);
    }

    common::http2::http2_nvs_t nvs;

    common::http2::http2_nv nvstatus;
    nvstatus.name = MAKE_BUFFER(":status");
    nvstatus.value = common::ConvertToBytes((uint32_t)status);
    nvs.push_back(nvstatus);

    char timebuf[100];
    time_t now = time(nullptr);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
    common::http2::http2_nv nvdate;
    nvdate.name = MAKE_BUFFER("date");
    nvdate.value = common::ConvertToBytes(timebuf);
    nvs.push_back(nvdate);

    common::http2::http2_nv nvserver;
    nvserver.name = MAKE_BUFFER("server");
    nvserver.value = common::ConvertToBytes(info.server_name);
    nvs.push_back(nvserver);

    /*http2::http2_nv nvenc;
    nvenc.name = MAKE_buffer_t("content-encoding");
    nvenc.value = MAKE_buffer_t("deflate");
    nvs.push_back(nvenc);*/

    if (mime_type) {
      common::http2::http2_nv nvmime;
      nvmime.name = MAKE_BUFFER("content-type");
      nvmime.value = common::ConvertToBytes(mime_type);
      nvs.push_back(nvmime);
    }
    if (length) {
      common::http2::http2_nv nvlen;
      nvlen.name = MAKE_BUFFER("content-length");
      nvlen.value = common::ConvertToBytes(*length);
      nvs.push_back(nvlen);
    }

    if (mod) {
      strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(mod));
      common::http2::http2_nv nvmod;
      nvmod.name = MAKE_BUFFER("last-modified");
      nvmod.value = common::ConvertToBytes(timebuf);
      nvs.push_back(nvmod);
    }

    common::http2::http2_deflater hd;
    common::buffer_t buff;
    hd.http2_deflate_hd_bufs(buff, nvs);

    common::http2::frame_hdr hdr = common::http2::frame_headers::create_frame_header(
        common::http2::HTTP2_FLAG_END_HEADERS, header_stream->sid(), buff.size());
    common::http2::frame_headers fhdr(hdr, buff);

    return common::make_error_from_errno(header_stream->sendFrame(fhdr));
  }

  return HttpClient::send_headers(protocol, status, extra_header, mime_type, length, mod, is_keep_alive, info);
}

StreamSPtr Http2Client::findStreamByStreamID(IStream::stream_id_t stream_id) const {
  for (size_t i = 0; i < streams_.size(); ++i) {
    StreamSPtr stream = streams_[i];
    if (stream->sid() == stream_id) {
      return stream;
    }
  }

  return StreamSPtr();
}

StreamSPtr Http2Client::findStreamByType(common::http2::frame_t type) const {
  for (size_t i = 0; i < streams_.size(); ++i) {
    StreamSPtr stream = streams_[i];
    if (stream->type() == type) {
      return stream;
    }
  }

  return StreamSPtr();
}

bool Http2Client::isSettingNegotiated() const {
  StreamSPtr settings = findStreamByStreamID(0);
  if (!settings) {
    return false;
  }

  HTTP2SettingsStreamSPtr rsettings = std::dynamic_pointer_cast<HTTP2SettingsStream>(settings);
  CHECK(rsettings);
  return rsettings->isNegotiated();
}

void Http2Client::processFrames(const common::http2::frames_t& frames) {
  common::net::socket_info inf = GetInfo();
  for (int i = 0; i < frames.size(); ++i) {
    common::http2::frame_base frame = frames[i];
    StreamSPtr stream = findStreamByStreamID(frame.stream_id());
    if (!stream) {
      IStream* nstream = IStream::createStream(inf, frame);
      stream = StreamSPtr(nstream);
      streams_.push_back(stream);
    }

    stream->processFrame(frame);
  }
}

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
