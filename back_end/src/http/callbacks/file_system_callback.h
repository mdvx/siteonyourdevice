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

class HttpFileSystemCallback
        : public HttpCallbackUrl {
 public:
  HttpFileSystemCallback();
  virtual bool handleRequest(http::HttpClient* hclient, const char* extra_header,
                             const common::http::http_request& request, const HttpServerInfo& info);
};

common::shared_ptr<IHttpCallback> createFileSystemHttpCallback(const std::string& name);

}  // namespace siteonyourdevice
}  // namespace fasto
