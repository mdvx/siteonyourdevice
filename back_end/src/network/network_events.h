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

#include "http_config.h"
#include "infos.h"

namespace fasto {
namespace siteonyourdevice {
namespace network {

typedef common::event_traits<NetworkEventTypes> NetworkEventTraits;
typedef NetworkEventTraits::event_t NetworkEvent;
typedef NetworkEventTraits::listener_t NetworkEventListener;

template <NetworkEventTypes event_t, typename inf_t>
class NetworkEventBaseInfo : public common::Event<NetworkEventTypes, event_t> {
 public:
  typedef inf_t info_t;
  typedef common::Event<NetworkEventTypes, event_t> base_class_t;
  typedef typename base_class_t::senders_t senders_t;

  NetworkEventBaseInfo(senders_t* sender, info_t info) : base_class_t(sender), info_(info) {}

  info_t info() const { return info_; }

 private:
  const info_t info_;
};

template <NetworkEventTypes event_t>
class NetworkEventBaseInfo<event_t, void> : public common::Event<NetworkEventTypes, event_t> {
 public:
  typedef void info_t;
  typedef common::Event<NetworkEventTypes, event_t> base_class_t;
  typedef typename base_class_t::senders_t senders_t;

  explicit NetworkEventBaseInfo(senders_t* sender) : base_class_t(sender) {}
};

typedef NetworkEventBaseInfo<InnerClientConnected, UserAuthInfo> InnerClientConnectedEvent;
typedef NetworkEventBaseInfo<InnerClientDisconnected, UserAuthInfo> InnerClientDisconnectedEvent;
typedef NetworkEventBaseInfo<ConfigChanged, HttpConfig> ConfigChangedEvent;

}  // namespace network
}  // namespace siteonyourdevice
}  // namespace fasto
