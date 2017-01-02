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

#include <utility>
#include <vector>
#include <string>

#include <common/net/types.h>
#include <common/url.h>

#define SERVER_SETTINGS_SECTION_LABEL "http_server"

//
#define LOGIN_SETTING_LABEL "login"
#define PASSWORD_SETTING_LABEL "password"
//
#define SERVER_TYPE_SETTING_LABEL "server_type"
#define PRIVATE_SITE_SETTING_LABEL "private_site"
// local
#define CONTENT_PATH_SETTING_LABEL "content_path"
#define LOCAL_HOST_SETTING_LABEL "local_host"
// external
#define EXTERNAL_HOST_SETTING_LABEL "external_host"

#define HANDLERS_URLS_SECTION_LABEL "http_handlers_utls"

#define SERVER_SOCKETS_SECTION_LABEL "http_server_sockets"

namespace fasto {
namespace siteonyourdevice {

enum http_server_t { FASTO_SERVER = 0, EXTERNAL_SERVER = 1 };

struct HttpConfig {
  std::string content_path;
  common::net::HostAndPort local_host;
  std::string login;
  std::string password;
  bool is_private_site;

  common::net::HostAndPort external_host;
  http_server_t server_type;

  typedef std::pair<std::string, std::string> handlers_url_t;
  std::vector<handlers_url_t> handlers_urls;

  typedef std::pair<std::string, common::uri::Uri> server_sockets_url_t;
  std::vector<server_sockets_url_t> server_sockets_urls;
};

}  // namespace siteonyourdevice
}  // namespace fasto
