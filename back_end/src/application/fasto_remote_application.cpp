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

#include "application/fasto_remote_application.h"

#include <stdlib.h>

#include "network/network_controller.h"
#include "network/network_event_handler.h"

#include "fasto_remote_application.h"

namespace fasto {
namespace siteonyourdevice {
namespace application {

network::NetworkEventHandler* createHandlerImpl(network::NetworkController * controler) {
  return new fasto::siteonyourdevice::network::NetworkEventHandler(controler);
}

FastoRemoteApplication::FastoRemoteApplication(int argc, char *argv[])
    : IFastoApplicationImpl(argc, argv), controller_(NULL), network_handler_(NULL) {
}

FastoRemoteApplication::~FastoRemoteApplication() {
}

int FastoRemoteApplication::preExec() {
  controller_ = new network::NetworkController(fApp->argc(), fApp->argv());
  network_handler_ = createHandlerImpl(controller_);
  network_handler_->start();
  return EXIT_SUCCESS;
}

int FastoRemoteApplication::exec() {
  return controller_->exec();
}

int FastoRemoteApplication::postExec() {
  delete network_handler_;
  delete controller_;
  return EXIT_SUCCESS;
}

void FastoRemoteApplication::exit(int result) {
  controller_->exit(result);
}

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
