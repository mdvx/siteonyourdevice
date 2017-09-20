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

#include "infos.h"

#include <string>

#include <common/convert2string.h>
#include <common/sprintf.h>

namespace fasto {
namespace siteonyourdevice {

UserAuthInfo::UserAuthInfo() : login(), password(), host() {}

UserAuthInfo::UserAuthInfo(const std::string &login,
                           const std::string &password,
                           const common::net::HostAndPort &host)
    : login(login), password(password), host(host) {}

bool UserAuthInfo::isValid() const { return !login.empty() && host.IsValid(); }

HttpServerInfo::HttpServerInfo() : server_name(), server_url() {}

HttpServerInfo::HttpServerInfo(const std::string &server_name,
                               const std::string &server_url)
    : server_name(server_name), server_url(server_url) {}

} // namespace siteonyourdevice
} // namespace fasto

namespace common {

std::string
ConvertToString(const fasto::siteonyourdevice::UserAuthInfo &uinfo) {
  return common::MemSPrintf("%s:%s:%s", uinfo.login, uinfo.password,
                            ConvertToString(uinfo.host));
}

bool ConvertFromString(const std::string &from,
                       fasto::siteonyourdevice::UserAuthInfo *out) {
  if (!out) {
    return false;
  }

  size_t up = from.find_first_of(':');
  if (up == std::string::npos) {
    return false;
  }

  size_t ph = from.find_first_of(':', up + 1);
  if (ph == std::string::npos) {
    return false;
  }

  fasto::siteonyourdevice::UserAuthInfo uinfo;
  uinfo.login = from.substr(0, up);
  uinfo.password = from.substr(up + 1, ph - up - 1);
  ConvertFromString(from.substr(ph + 1), &uinfo.host);

  *out = uinfo;
  return true;
}

} // namespace common
