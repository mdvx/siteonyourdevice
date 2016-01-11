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

#include <string>

#include "common/event.h"

#define NETWORK_EVENT_LOOP_ID 1

enum NetworkEventTypes {
  InnerClientConnected = 0,
  InnerClientDisconnected,

  CountNetworkEvent
};

const std::string SNetworkEventTypes[] = {
  "InnerClientConnected",
  "InnerClientDisconnected",
  "CountNetworkEvent"
};

namespace common {

template<>
struct event_traits<NetworkEventTypes> {
  typedef IEvent<NetworkEventTypes> event_t;
  typedef IExceptionEvent<NetworkEventTypes> ex_event_t;
  typedef IListener<NetworkEventTypes> listener_t;
  static const unsigned max_count = CountNetworkEvent;
  static const unsigned id = NETWORK_EVENT_LOOP_ID;
};

std::string convertToString(NetworkEventTypes net);

}  // namespace common
