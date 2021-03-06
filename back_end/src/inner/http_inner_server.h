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

#include "http_config.h"

#include "http/http_server.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

class ProxyInnerServer : public common::libev::IoLoop {
 public:
  typedef common::libev::IoLoop base_class;
  explicit ProxyInnerServer(common::libev::IoLoopObserver* observer);
  virtual const char* ClassName() const;

 protected:
  common::libev::tcp::TcpClient* CreateClient(const common::net::socket_info& info) override;
};

class Http2InnerServer : public http::Http2Server {
 public:
  Http2InnerServer(common::libev::IoLoopObserver* observer, const HttpConfig& config);

  virtual const char* ClassName() const;

 protected:
  virtual common::libev::tcp::TcpClient* CreateClient(const common::net::socket_info& info) override;

 private:
  const HttpConfig config_;
};

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
