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

#include "http/http_streams.h"

#include <common/portable_endian.h>

namespace fasto {
namespace siteonyourdevice {
namespace http {

common::http2::frame_t IStream::type() const {
  return init_frame_.type();
}

IStream::stream_id_t IStream::sid() const {
  return init_frame_.stream_id();
}

IStream::IStream(const common::net::socket_info& info, const common::http2::frame_base& frame)
    : sock_(info), init_frame_(frame) {}

bool IStream::processFrame(const common::http2::frame_base& frame) {
  if (!frame.IsValid()) {
    NOTREACHED();
    return false;
  }

  return processFrameImpl(frame);
}

common::ErrnoError IStream::sendData(const common::buffer_t& buff) {
  size_t nwrite = 0;
  common::ErrnoError err = sock_.Write((const char*)buff.data(), buff.size(), &nwrite);
  return err;
}

common::ErrnoError IStream::sendFrame(const common::http2::frame_base& frame) {
  CHECK(sid() == frame.stream_id());
  common::buffer_t raw = frame.raw_data();
  return sendData(raw);
}

common::ErrnoError IStream::sendCloseFrame() {
  common::http2::frame_hdr hdr = common::http2::frame_rst::create_frame_header(0, sid());
  uint32_t er = be32toh(common::http2::HTTP2_STREAM_CLOSED);
  common::http2::frame_rst rst(hdr, &er);
  return sendFrame(rst);
}

IStream* IStream::createStream(const common::net::socket_info& info, const common::http2::frame_base& frame) {
  if (!frame.IsValid()) {
    NOTREACHED();
    return nullptr;
  }

  common::http2::frame_t type = frame.type();

  switch (type) {
    case common::http2::HTTP2_DATA:
      return new HTTP2DataStream(info, frame);
    case common::http2::HTTP2_HEADERS:
      return new HTTP2HeadersStream(info, frame);
    case common::http2::HTTP2_PRIORITY: {
      IStream* res = new HTTP2PriorityStream(info, frame);
      return res;
    }
    case common::http2::HTTP2_RST_STREAM:
      NOTREACHED();
      return nullptr;
    case common::http2::HTTP2_SETTINGS:
      return new HTTP2SettingsStream(info, frame);
    case common::http2::HTTP2_PUSH_PROMISE:
      NOTREACHED();
      return nullptr;
    case common::http2::HTTP2_PING:
      NOTREACHED();
      return nullptr;
    case common::http2::HTTP2_GOAWAY:
      NOTREACHED();
      return nullptr;
    case common::http2::HTTP2_WINDOW_UPDATE:
      NOTREACHED();
      return nullptr;
    case common::http2::HTTP2_CONTINUATION:
      NOTREACHED();
      return nullptr;

    default:
      NOTREACHED();
      return nullptr;
  }
}

IStream::~IStream() {}

HTTP2DataStream::HTTP2DataStream(const common::net::socket_info& info, const common::http2::frame_base& frame)
    : IStream(info, frame) {
  CHECK(common::http2::HTTP2_DATA == frame.type());
}

bool HTTP2DataStream::processFrameImpl(const common::http2::frame_base& frame) {
  return true;
}

HTTP2PriorityStream::HTTP2PriorityStream(const common::net::socket_info& info, const common::http2::frame_base& frame)
    : IStream(info, frame) {
  CHECK(common::http2::HTTP2_PRIORITY == frame.type());
}

HTTP2PriorityStream::~HTTP2PriorityStream() {}

bool HTTP2PriorityStream::processFrameImpl(const common::http2::frame_base& frame) {
  sendCloseFrame();
  return true;
}

HTTP2SettingsStream::HTTP2SettingsStream(const common::net::socket_info& info, const common::http2::frame_base& frame)
    : IStream(info, frame), negotiated_(false) {
  CHECK(common::http2::HTTP2_SETTINGS == frame.type());
}

bool HTTP2SettingsStream::isNegotiated() const {
  return negotiated_;
}

bool HTTP2SettingsStream::processFrameImpl(const common::http2::frame_base& frame) {
  if (frame.type() == common::http2::HTTP2_SETTINGS) {
    sendFrame(frame);
    if (frame.flags() & common::http2::HTTP2_FLAG_ACK) {
      negotiated_ = true;
    }
  } else if (frame.type() != common::http2::HTTP2_GOAWAY) {
    sendCloseFrame();
  }
  return true;
}

HTTP2HeadersStream::HTTP2HeadersStream(const common::net::socket_info& info, const common::http2::frame_base& frame)
    : IStream(info, frame) {
  CHECK(common::http2::HTTP2_HEADERS == frame.type());
}

bool HTTP2HeadersStream::processFrameImpl(const common::http2::frame_base& frame) {
  return false;
}

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
