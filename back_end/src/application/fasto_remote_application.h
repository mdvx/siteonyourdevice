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

#include "globals.h"

#include "application/fasto_application.h"

namespace common {
namespace thread {
template<typename type_t>
class EventThread;
}
}

namespace fasto {
namespace siteonyourdevice {
namespace network {

class NetworkEventHandler;
class NetworkController;
}  // namespace network

namespace application {

class FastoRemoteApplication
        : public IFastoApplicationImpl {
 public:
  FastoRemoteApplication(int argc, char* argv[]);
  virtual ~FastoRemoteApplication();

  virtual int exec();
  virtual void exit(int result);

 private:
  virtual int preExec();
  virtual int postExec();

  network::NetworkController* controller_;
  network::NetworkEventHandler* network_handler_;
  common::thread::EventThread<NetworkEventTypes>* const thread_;  // event thread handle
};

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
