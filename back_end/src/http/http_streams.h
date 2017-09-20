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

#include <memory>

#include <common/http/http2.h>
#include <common/net/socket_tcp.h>

namespace fasto {
namespace siteonyourdevice {
namespace http {

class IStream {
 public:
  typedef uint32_t stream_id_t;

  common::http2::frame_t type() const;
  stream_id_t sid() const;

  bool processFrame(const common::http2::frame_base& frame);  // true if is handled

  common::ErrnoError sendFrame(const common::http2::frame_base& frame);
  common::ErrnoError sendCloseFrame();

  virtual ~IStream();

  static IStream* createStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

 protected:
  IStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

  virtual bool processFrameImpl(const common::http2::frame_base& frame) = 0;

 private:
  common::ErrnoError sendData(const common::buffer_t& buff);
  common::net::SocketHolder sock_;
  const common::http2::frame_base init_frame_;
};

typedef std::shared_ptr<IStream> StreamSPtr;

class HTTP2DataStream : public IStream {
 public:
  HTTP2DataStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

 private:
  virtual bool processFrameImpl(const common::http2::frame_base& frame) override;
};

typedef std::shared_ptr<HTTP2DataStream> HTTP2DataStreamSPtr;

class HTTP2PriorityStream : public IStream {
 public:
  HTTP2PriorityStream(const common::net::socket_info& info, const common::http2::frame_base& frame);
  ~HTTP2PriorityStream();

 private:
  virtual bool processFrameImpl(const common::http2::frame_base& frame) override;
};

typedef std::shared_ptr<HTTP2PriorityStream> HTTP2PriorityStreamSPtr;

class HTTP2SettingsStream : public IStream {
 public:
  HTTP2SettingsStream(const common::net::socket_info& info, const common::http2::frame_base& frame);
  bool isNegotiated() const;

 private:
  virtual bool processFrameImpl(const common::http2::frame_base& frame) override;
  bool negotiated_;
};

typedef std::shared_ptr<HTTP2SettingsStream> HTTP2SettingsStreamSPtr;

class HTTP2HeadersStream : public IStream {
 public:
  HTTP2HeadersStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

 private:
  virtual bool processFrameImpl(const common::http2::frame_base& frame) override;
};

typedef std::shared_ptr<HTTP2HeadersStream> HTTP2HeadersStreamSPtr;

}  // namespace http
}  // namespace siteonyourdevice
}  // namespace fasto
