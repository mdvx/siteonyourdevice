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

#include "http/callbacks/system_callback.h"

#include <string>

#include <common/convert2string.h>
#include <common/logger.h>
#include <common/sprintf.h>
#include <common/string_util.h>
#include <common/system/system.h>

#include "http/http_client.h"

namespace common {

std::string ConvertToString(fasto::siteonyourdevice::HSCTypes t) {
  return fasto::siteonyourdevice::HSystemCallbackTypes[t];
}

bool ConvertFromString(const std::string& text, fasto::siteonyourdevice::HSCTypes* out) {
  if (!out) {
    return false;
  }

  for (uint32_t i = 0; i < SIZEOFMASS(fasto::siteonyourdevice::HSystemCallbackTypes); ++i) {
    if (text == fasto::siteonyourdevice::HSystemCallbackTypes[i]) {
      *out = static_cast<fasto::siteonyourdevice::HSCTypes>(i);
      return true;
    }
  }

  DNOTREACHED();
  return false;
}

}  // namespace common

namespace fasto {
namespace siteonyourdevice {

HttpSystemCallback::HttpSystemCallback() : HttpCallbackUrl(system) {}

bool HttpSystemCallback::handleRequest(http::HttpClient* hclient,
                                       const char* extra_header,
                                       const common::http::http_request& request,
                                       const HttpServerInfo& info) {
  return false;
}

HttpSystemShutdownCallback::HttpSystemShutdownCallback(HSCTypes type) : HttpCallbackUrl(system), type_(type) {}

bool HttpSystemShutdownCallback::handleRequest(http::HttpClient* hclient,
                                               const char* extra_header,
                                               const common::http::http_request& request,
                                               const HttpServerInfo& info) {
  // keep alive
  common::http::header_t connectionField = request.findHeaderByKey("Connection", false);
  bool isKeepAlive = common::EqualsASCII(connectionField.value, "Keep-Alive", false);
  const common::http::http_protocols protocol = request.protocol();

  common::system::shutdown_t sh_type;
  if (type_ == system_shutdown) {
    sh_type = common::system::SHUTDOWN;
  } else if (type_ == system_logout) {
    sh_type = common::system::LOGOUT;
  } else if (type_ == system_reboot) {
    sh_type = common::system::REBOOT;
  } else {
    DNOTREACHED();
  }

  common::ErrnoError errs = common::system::Shutdown(sh_type);
  if (errs) {
    DEBUG_MSG_ERROR(errs, common::logging::LOG_LEVEL_ERR);
    std::string cause = common::MemSPrintf("Shutdown failed(%s).", errs->GetDescription());
    common::Error err =
        hclient->send_error(protocol, common::http::HS_NOT_ALLOWED, extra_header, cause.c_str(), isKeepAlive, info);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    return true;
  }

  common::Error err = hclient->send_ok(protocol, extra_header, "Your device shutdowned!", isKeepAlive, info);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
  }
  return true;
}

std::shared_ptr<IHttpCallback> createSystemHttpCallback(const std::string& name) {
  HSCTypes sys_type;
  common::ConvertFromString(name, &sys_type);
  return std::shared_ptr<IHttpCallback>(new HttpSystemShutdownCallback(sys_type));
}

}  // namespace siteonyourdevice
}  // namespace fasto
