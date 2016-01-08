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

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

#include "http_config.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

class InnerServerHandler
        : public InnerServerCommandSeqParser {
 public:
  explicit InnerServerHandler(const HttpConfig& config);
  UserAuthInfo authInfo() const;

 protected:
  const HttpConfig config_;

 private:
  virtual void handleInnerRequestCommand(InnerClient *connection,
                                         cmd_seq_type id, int argc, char *argv[]);
  virtual void handleInnerResponceCommand(InnerClient *connection,
                                          cmd_seq_type id, int argc, char *argv[]);
  virtual void handleInnerApproveCommand(InnerClient *connection,
                                         cmd_seq_type id, int argc, char *argv[]);
};

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
