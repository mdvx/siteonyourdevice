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

#include "tcp/tcp_client.h"

#include "commands/commands.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

class InnerClient : public tcp::TcpClient {
public:
  InnerClient(tcp::ITcpLoop *server, const common::net::socket_info &info);
  const char *ClassName() const override;

  common::ErrnoError write(const cmd_request_t &request,
                           size_t *nwrite) WARN_UNUSED_RESULT;
  common::ErrnoError write(const cmd_responce_t &responce,
                           size_t *nwrite) WARN_UNUSED_RESULT;
  common::ErrnoError write(const cmd_approve_t &approve,
                           size_t *nwrite) WARN_UNUSED_RESULT;

private:
  using tcp::TcpClient::write;
};

} // namespace inner
} // namespace siteonyourdevice
} // namespace fasto
