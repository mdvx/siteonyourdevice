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

#include "network/network_event_handler.h"

#include <common/threads/event_bus.h>
#include <common/logger.h>

#include "network/network_controller.h"

namespace fasto {
namespace siteonyourdevice {
namespace network {

class NetworkEventHandler::NetworkListener : public common::IListenerEx<NetworkEventTypes> {
  NetworkEventHandler* const app_;

 public:
  explicit NetworkListener(NetworkEventHandler* app)
      : common::IListenerEx<NetworkEventTypes>(), app_(app) {
    EVENT_BUS()->Subscribe<InnerClientConnectedEvent>(this);
    EVENT_BUS()->Subscribe<InnerClientDisconnectedEvent>(this);
    EVENT_BUS()->Subscribe<ConfigChangedEvent>(this);
  }

  ~NetworkListener() {
    EVENT_BUS()->UnSubscribe<ConfigChangedEvent>(this);
    EVENT_BUS()->UnSubscribe<InnerClientDisconnectedEvent>(this);
    EVENT_BUS()->UnSubscribe<InnerClientConnectedEvent>(this);
  }

  virtual void HandleEvent(event_t* event) { app_->handleEvent(event); }

  virtual void HandleExceptionEvent(event_t* event, common::Error err) {
    app_->handleExceptionEvent(event, err);
  }
};

NetworkEventHandler::NetworkEventHandler(NetworkController* controller)
    : network_listener_(nullptr), controller_(controller) {
  network_listener_ = new NetworkListener(this);
}

NetworkEventHandler::~NetworkEventHandler() {
  delete network_listener_;
}

void NetworkEventHandler::handleEvent(NetworkEvent* event) {
  if (event->GetEventType() == InnerClientConnectedEvent::EventType) {
  } else if (event->GetEventType() == InnerClientDisconnectedEvent::EventType) {
  } else if (event->GetEventType() == ConfigChangedEvent::EventType) {
    ConfigChangedEvent* new_config_event = static_cast<ConfigChangedEvent*>(event);
    HttpConfig config = new_config_event->info();
    controller_->disConnect();
    controller_->setConfig(config);
    controller_->connect();
  }
}

void NetworkEventHandler::handleExceptionEvent(NetworkEvent* event, common::Error err) {
  WARNING_LOG() << "Exception event: " << common::ConvertToString(event->GetEventType())
                << " msg: " << err->description();
}

}  // namespace network
}  // namespace siteonyourdevice
}  // namespace fasto
