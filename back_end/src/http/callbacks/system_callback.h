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

#include <string>

#include "http/callbacks/http_callbacks.h"

namespace fasto {
namespace siteonyourdevice {

enum HSCTypes {
  system_shutdown,
  system_logout,
  system_reboot
};

const std::string HSystemCallbackTypes[] = { "shutdown", "logout", "reboot" };

class HttpSystemCallback
  : public HttpCallbackUrl {
 public:
  HttpSystemCallback();
  virtual bool handleRequest(http::HttpClient* hclient, const char* extra_header,
                             const common::http::http_request& request,
                             const HttpServerInfo& info);
};

class HttpSystemShutdownCallback
  : public HttpCallbackUrl {
 public:
  explicit HttpSystemShutdownCallback(HSCTypes type);
  virtual bool handleRequest(http::HttpClient* hclient, const char* extra_header,
                             const common::http::http_request& request,
                             const HttpServerInfo& info);

 private:
  const HSCTypes type_;
};

common::shared_ptr<IHttpCallback> createSystemHttpCallback(const std::string& name);

}  // namespace siteonyourdevice
}  // namespace fasto

namespace common {
    std::string ConvertToString(fasto::siteonyourdevice::HSCTypes t);
}  // namespace common
