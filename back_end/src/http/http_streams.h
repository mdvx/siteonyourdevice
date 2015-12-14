#pragma once

#include "common/http/http2.h"
#include "common/net/socket_tcp.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class IStream
        {
        public:
            common::http2::frame_type type() const;
            uint32_t sid() const;

            bool processFrame(const common::http2::frame_base& frame); //true if is handled

            common::ErrnoError sendFrame(const common::http2::frame_base& frame);
            common::ErrnoError sendCloseFrame();

            virtual ~IStream();

            static IStream* createStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

        protected:
            IStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

            virtual bool processFrameImpl(const common::http2::frame_base& frame) = 0;

        private:
            common::ErrnoError sendData(const common::buffer_type& buff);
            common::net::SocketHolder sock_;
            const common::http2::frame_base init_frame_;
        };

        typedef std::shared_ptr<IStream> StreamSPtr;

        class HTTP2DataStream
                : public IStream
        {
        public:
            HTTP2DataStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

        private:
            virtual bool processFrameImpl(const common::http2::frame_base& frame);
        };

        typedef std::shared_ptr<HTTP2DataStream> HTTP2DataStreamSPtr;

        class HTTP2PriorityStream
                : public IStream
        {
        public:
            HTTP2PriorityStream(const common::net::socket_info& info, const common::http2::frame_base& frame);
            ~HTTP2PriorityStream();

        private:
            virtual bool processFrameImpl(const common::http2::frame_base& frame);
        };

        typedef std::shared_ptr<HTTP2PriorityStream> HTTP2PriorityStreamSPtr;

        class HTTP2SettingsStream
                : public IStream
        {
        public:
            HTTP2SettingsStream(const common::net::socket_info& info, const common::http2::frame_base& frame);
            bool isNegotiated() const;

        private:
            virtual bool processFrameImpl(const common::http2::frame_base& frame);
            bool negotiated_;
        };

        typedef std::shared_ptr<HTTP2SettingsStream> HTTP2SettingsStreamSPtr;

        class HTTP2HeadersStream
                : public IStream
        {
        public:
            HTTP2HeadersStream(const common::net::socket_info& info, const common::http2::frame_base& frame);

        private:
            virtual bool processFrameImpl(const common::http2::frame_base& frame);
        };

        typedef std::shared_ptr<HTTP2HeadersStream> HTTP2HeadersStreamSPtr;
    }
}
