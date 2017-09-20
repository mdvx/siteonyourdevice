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

#include <common/threads/event_bus.h>

#include "network/network_controller.h"
#include "network/network_event_handler.h"

#include "fasto_remote_application.h"

namespace fasto {
namespace siteonyourdevice {
namespace application {

FastoRemoteApplication::FastoRemoteApplication(int argc, char* argv[])
    : common::application::IApplication(argc, argv),
      controller_(nullptr),
      network_handler_(nullptr),
      thread_(EVENT_BUS()->CreateEventThread<NetworkEventTypes>()) {}

FastoRemoteApplication::~FastoRemoteApplication() {
  EVENT_BUS()->Stop();
}

int FastoRemoteApplication::PreExecImpl() {
  controller_ = new network::NetworkController(fApp->GetArgc(), fApp->GetArgv());
  network_handler_ = new network::NetworkEventHandler(controller_);
  controller_->connect();
  return EXIT_SUCCESS;
}

int FastoRemoteApplication::ExecImpl() {
  EVENT_BUS()->JoinEventThread(thread_);
  return EXIT_SUCCESS;
}

int FastoRemoteApplication::PostExecImpl() {
  delete network_handler_;
  delete controller_;
  return EXIT_SUCCESS;
}

void FastoRemoteApplication::PostEvent(event_t* event) {}

void FastoRemoteApplication::SendEvent(event_t* event) {}

void FastoRemoteApplication::Subscribe(listener_t* listener, common::events_size_t id) {}

void FastoRemoteApplication::UnSubscribe(listener_t* listener, common::events_size_t id) {}

void FastoRemoteApplication::UnSubscribe(listener_t* listener) {}

void FastoRemoteApplication::ShowCursor() {}

void FastoRemoteApplication::HideCursor() {}

bool FastoRemoteApplication::IsCursorVisible() const {
  return false;
}

common::application::timer_id_t FastoRemoteApplication::AddTimer(uint32_t interval,
                                                                 common::application::timer_callback_t cb,
                                                                 void* user_data) {
  return -1;
}

bool FastoRemoteApplication::RemoveTimer(common::application::timer_id_t id) {
  return true;
}

void FastoRemoteApplication::ExitImpl(int result) {
  controller_->exit(result);
  EVENT_BUS()->StopEventThread(thread_);
}

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
