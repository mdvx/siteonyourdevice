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

#include <common/net/types.h>

namespace fasto {
namespace siteonyourdevice {

struct UserAuthInfo {
  UserAuthInfo();
  UserAuthInfo(const std::string& login, const std::string& password, const common::net::HostAndPort& host);

  bool isValid() const;

  std::string login;
  std::string password;
  common::net::HostAndPort host;
};

inline bool operator==(const UserAuthInfo& lhs, const UserAuthInfo& rhs) {
  return lhs.login == rhs.login && lhs.password == rhs.password && lhs.host == rhs.host;
}

inline bool operator!=(const UserAuthInfo& x, const UserAuthInfo& y) {
  return !(x == y);
}

struct HttpServerInfo {
  HttpServerInfo();
  HttpServerInfo(const std::string& server_name, const std::string& server_url);

  std::string server_name;
  std::string server_url;
};

}  // namespace siteonyourdevice
}  // namespace fasto

namespace common {

std::string ConvertToString(const fasto::siteonyourdevice::UserAuthInfo& uinfo);
bool ConvertFromString(const std::string& from, fasto::siteonyourdevice::UserAuthInfo* out) WARN_UNUSED_RESULT;

}  // namespace common
