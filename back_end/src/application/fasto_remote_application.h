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

#include <common/application/application.h>

#include "globals.h"

namespace common {
namespace threads {
template <typename type_t> class EventThread;
}
} // namespace common

namespace fasto {
namespace siteonyourdevice {
namespace network {

class NetworkEventHandler;
class NetworkController;
} // namespace network

namespace application {

class FastoRemoteApplication : public common::application::IApplication {
public:
  FastoRemoteApplication(int argc, char *argv[]);
  virtual ~FastoRemoteApplication();

  virtual void PostEvent(event_t *event) override;
  virtual void SendEvent(event_t *event) override;

  virtual void Subscribe(listener_t *listener,
                         common::events_size_t id) override;
  virtual void UnSubscribe(listener_t *listener,
                           common::events_size_t id) override;
  virtual void UnSubscribe(listener_t *listener) override;

  virtual void ShowCursor() override;
  virtual void HideCursor() override;
  virtual bool IsCursorVisible() const override;

  virtual common::application::timer_id_t
  AddTimer(uint32_t interval, common::application::timer_callback_t cb,
           void *user_data) override;
  virtual bool RemoveTimer(common::application::timer_id_t id) override;

private:
  virtual int PreExecImpl() override;
  virtual int ExecImpl() override;
  virtual int PostExecImpl() override;
  virtual void ExitImpl(int result) override;

  network::NetworkController *controller_;
  network::NetworkEventHandler *network_handler_;
  common::threads::EventThread<NetworkEventTypes>
      *const thread_; // event thread handle
};

} // namespace application
} // namespace siteonyourdevice
} // namespace fasto
