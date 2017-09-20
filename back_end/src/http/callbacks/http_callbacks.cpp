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

#include <string>

#include <common/convert2string.h>

#include "http/callbacks/http_callbacks.h"

#include "http/callbacks/file_system_callback.h"
#include "http/callbacks/system_callback.h"

namespace common {
std::string ConvertToString(fasto::siteonyourdevice::HCTypes t) {
  return fasto::siteonyourdevice::HCallbackTypes[t];
}

bool ConvertFromString(const std::string &from,
                       fasto::siteonyourdevice::HCTypes *out) {
  if (!out) {
    return false;
  }

  for (uint32_t i = 0; i < SIZEOFMASS(fasto::siteonyourdevice::HCallbackTypes);
       ++i) {
    if (from == fasto::siteonyourdevice::HCallbackTypes[i]) {
      *out = static_cast<fasto::siteonyourdevice::HCTypes>(i);
      return true;
    }
  }

  DNOTREACHED();
  return false;
}

} // namespace common

namespace fasto {
namespace siteonyourdevice {

IHttpCallback::IHttpCallback() {}

IHttpCallback::~IHttpCallback() {}

std::shared_ptr<IHttpCallback> IHttpCallback::createHttpCallback(HCTypes type) {
  if (type == file_system) {
    return std::shared_ptr<IHttpCallback>(new HttpFileSystemCallback);
  } else if (type == system) {
    return std::shared_ptr<IHttpCallback>(new HttpSystemCallback);
  } else {
    DNOTREACHED();
    return std::shared_ptr<IHttpCallback>();
  }
}

std::shared_ptr<IHttpCallback>
IHttpCallback::createHttpCallback(const std::string &ns_name,
                                  const std::string &name) {
  HCTypes htype;
  if (common::ConvertFromString(ns_name, &htype)) {
    if (htype == file_system) {
      return createFileSystemHttpCallback(name);
    } else if (htype == system) {
      return createSystemHttpCallback(name);
    }
  }
  DNOTREACHED();
  return std::shared_ptr<IHttpCallback>();
}

HttpCallbackUrl::HttpCallbackUrl(HCTypes type) : IHttpCallback(), type_(type) {}

HCTypes HttpCallbackUrl::type() const { return type_; }

} // namespace siteonyourdevice
} // namespace fasto
