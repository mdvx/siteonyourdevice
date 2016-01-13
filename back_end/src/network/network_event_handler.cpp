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

#include "common/thread/event_bus.h"
#include "common/logger.h"

#include "network/network_controller.h"

namespace fasto {
namespace siteonyourdevice {
namespace network {

void NetworkEventHandler::handleEvent(NetworkEvent *event) {
  if (event->eventType() == InnerClientConnectedEvent::EventType) {
      // InnerClientConnectedEvent * ev = static_cast<InnerClientConnectedEvent*>(event);
  } else if (event->eventType() == InnerClientDisconnectedEvent::EventType) {
      // InnerClientDisconnectedEvent * ev = static_cast<InnerClientDisconnectedEvent*>(event);
  }
}

void NetworkEventHandler::handleExceptionEvent(NetworkEvent* event, common::Error err) {
  DEBUG_MSG_FORMAT<512>(common::logging::L_WARNING,
                        "Exception event %s, %s",
                        common::convertToString(event->eventType()), err->description());
}

class NetworkEventHandler::NetworkListener
        : public common::IListener<NetworkEventTypes> {
  NetworkEventHandler * const app_;
 public:
  explicit NetworkListener(NetworkEventHandler * app)
    : common::IListener<NetworkEventTypes>(), app_(app) {
    EVENT_BUS()->subscribe<InnerClientConnectedEvent>(this);
    EVENT_BUS()->subscribe<InnerClientDisconnectedEvent>(this);
  }

  ~NetworkListener() {
    EVENT_BUS()->unsubscribe<InnerClientDisconnectedEvent>(this);
    EVENT_BUS()->unsubscribe<InnerClientConnectedEvent>(this);
  }

  virtual void handleEvent(event_t* event) {
    app_->handleEvent(event);
  }

  virtual void handleExceptionEvent(event_t* event, common::Error err) {
    app_->handleExceptionEvent(event, err);
  }
};

NetworkEventHandler::NetworkEventHandler(NetworkController *controller)
  : controller_(controller) {
  networkListener_ = new NetworkListener(this);
}

NetworkEventHandler::~NetworkEventHandler() {
  delete networkListener_;
}

void NetworkEventHandler::start() {
  controller_->connect();
}

}  // namespace network
}  // namespace siteonyourdevice
}  // namespace fasto
