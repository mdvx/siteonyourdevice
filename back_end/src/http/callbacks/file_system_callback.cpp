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

#include "http/callbacks/file_system_callback.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include <common/http/http.h>
#include <common/logger.h>
#include <common/string_util.h>

#include "http/http_client.h"

namespace fasto {
namespace siteonyourdevice {

HttpFileSystemCallback::HttpFileSystemCallback()
    : HttpCallbackUrl(file_system) {}

bool HttpFileSystemCallback::handleRequest(
    http::HttpClient *hclient, const char *extra_header,
    const common::http::http_request &request, const HttpServerInfo &info) {
  std::string requeststr = common::ConvertToString(request);
  INFO_LOG() << "handleRequest:\n" << requeststr;

  // keep alive
  common::http::header_t connectionField =
      request.findHeaderByKey("Connection", false);
  bool isKeepAlive =
      common::EqualsASCII(connectionField.value, "Keep-Alive", false);
  const common::http::http_protocols protocol = request.protocol();

  if (request.method() == common::http::http_method::HM_GET ||
      request.method() == common::http::http_method::HM_HEAD) {
    common::uri::Upath path = request.path();
    if (!path.IsValid() || path.IsRoot()) {
      path = common::uri::Upath("index.html");
    }

    const std::string file_path = path.GetPath();
    int open_flags = O_RDONLY;
#ifdef COMPILER_MINGW
    open_flags |= O_BINARY;
#endif
    struct stat sb;
    if (stat(file_path.c_str(), &sb) < 0) {
      common::ErrnoError err = hclient->send_error(
          protocol, common::http::HS_NOT_FOUND, extra_header, "File not found.",
          isKeepAlive, info);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return true;
    }

    if (S_ISDIR(sb.st_mode)) {
      common::ErrnoError err =
          hclient->send_error(protocol, common::http::HS_BAD_REQUEST,
                              extra_header, "Bad filename.", isKeepAlive, info);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return true;
    }

    int file = open(file_path.c_str(), open_flags);
    if (file == INVALID_DESCRIPTOR) { /* open the file for reading */
      common::ErrnoError err =
          hclient->send_error(protocol, common::http::HS_FORBIDDEN, nullptr,
                              "File is protected.", isKeepAlive, info);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return true;
    }

    const std::string mime = path.GetMime();
    common::ErrnoError err = hclient->send_headers(
        protocol, common::http::HS_OK, nullptr, mime.c_str(), &sb.st_size,
        &sb.st_mtime, isKeepAlive, info);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      ::close(file);
      return true;
    }

    if (request.method() == common::http::http_method::HM_GET) {
      common::ErrnoError err =
          hclient->send_file_by_fd(protocol, file, sb.st_size);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
    }
    ::close(file);
    return true;
  } else {
    common::http::header_t contentTypeField =
        request.findHeaderByKey("Content-Type", false);
    if (!contentTypeField.IsValid()) {
      common::ErrnoError err =
          hclient->send_error(protocol, common::http::HS_NOT_ALLOWED, nullptr,
                              "Unsupported request.", isKeepAlive, info);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return true;
    }

    const std::string contentTypeValue = contentTypeField.value;
    if (contentTypeValue == "application/json") {
      return true;
    } else if (contentTypeValue == "application/x-www-form-urlencoded") {
      return true;
    } else if (contentTypeValue == "multipart/form-data") {
      return true;
    } else {
      common::ErrnoError err =
          hclient->send_error(protocol, common::http::HS_NOT_ALLOWED, nullptr,
                              "Unsupported content type.", isKeepAlive, info);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return true;
    }
  }
}

std::shared_ptr<IHttpCallback>
createFileSystemHttpCallback(const std::string &name) {
  return std::shared_ptr<IHttpCallback>(new HttpFileSystemCallback);
}

} // namespace siteonyourdevice
} // namespace fasto
