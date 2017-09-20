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

#include <memory>
#include <string>

#include <common/http/http.h>

#include "infos.h"

namespace fasto {
namespace siteonyourdevice {

namespace http {
class HttpClient;
} // namespace http

enum HCTypes {
  system,
  file_system // handle all requests
};

const std::string HCallbackTypes[] = {"system", "file_system"};

class IHttpCallback {
public:
  IHttpCallback();
  virtual ~IHttpCallback();
  virtual bool handleRequest(http::HttpClient *hclient,
                             const char *extra_header,
                             const common::http::http_request &request,
                             const HttpServerInfo &info) = 0;

  static std::shared_ptr<IHttpCallback> createHttpCallback(HCTypes type);
  static std::shared_ptr<IHttpCallback>
  createHttpCallback(const std::string &ns_name, const std::string &name);
};

class HttpCallbackUrl : public IHttpCallback {
public:
  explicit HttpCallbackUrl(HCTypes type);
  HCTypes type() const;

private:
  const HCTypes type_;
};

} // namespace siteonyourdevice
} // namespace fasto

namespace common {
std::string ConvertToString(fasto::siteonyourdevice::HCTypes t);
bool ConvertFromString(const std::string &from,
                       fasto::siteonyourdevice::HCTypes *out);
} // namespace common
