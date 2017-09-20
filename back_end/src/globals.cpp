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

#include "globals.h"

#include <string>

#include <common/convert2string.h>

namespace {
const std::string SNetworkEventTypes[] = {"InnerClientConnected", "InnerClientDisconnected", "ConfigChanged",
                                          "CountNetworkEvent"};
}

namespace common {

std::string ConvertToString(NetworkEventTypes net) {
  return SNetworkEventTypes[net];
}

bool ConvertFromString(const std::string& from, NetworkEventTypes* out) {
  if (!out) {
    return false;
  }

  for (size_t i = 0; i < SIZEOFMASS(SNetworkEventTypes); ++i) {
    if (from == SNetworkEventTypes[i]) {
      *out = static_cast<NetworkEventTypes>(i);
      return true;
    }
  }

  DNOTREACHED();
  return false;
}

}  // namespace common
