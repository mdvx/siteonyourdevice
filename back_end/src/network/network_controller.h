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

#include "common/multi_threading/types.h"

#include <string>

#include "globals.h"
#include "http_config.h"
#include "infos.h"

namespace common {
namespace thread {
template<typename type_t>
class EventThread;
}
}

namespace fasto {
namespace siteonyourdevice {
class ILoopThreadController;
namespace network {

class NetworkController {
 public:
  NetworkController(int argc, char *argv[]);
  ~NetworkController();

  int exec() SYNC_CALL();
  void exit(int result) SYNC_CALL();

  void connect() ASYNC_CALL(InnerClientConnectedEvent);
  void disConnect() ASYNC_CALL(InnerClientDisconnectedEvent);

  UserAuthInfo authInfo() const;
  HttpConfig config() const;
  void setConfig(const HttpConfig& config);

 private:
  void readConfig();
  void saveConfig();

  common::multi_threading::mutex_t server_mutex_;
  ILoopThreadController * server_;

  std::string config_path_;
  HttpConfig config_;

  common::thread::EventThread<NetworkEventTypes>* const thread_;  // event thread handle
};

}  // namespace network
}  // namespace siteonyourdevice
}  // namespace fasto
