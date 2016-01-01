#include "http/http_streams.h"

#include "common/portable_endian.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace http
        {
            common::http2::frame_type IStream::type() const
            {
                return init_frame_.type();
            }

            uint32_t IStream::sid() const
            {
                return init_frame_.stream_id();
            }

            IStream::IStream(const common::net::socket_info& info, const common::http2::frame_base& frame)
                : sock_(info), init_frame_(frame)
            {
            }

            bool IStream::processFrame(const common::http2::frame_base& frame)
            {
                if(!frame.isValid()){
                    NOTREACHED();
                    return false;
                }

                return processFrameImpl(frame);
            }

            common::ErrnoError IStream::sendData(const common::buffer_type& buff)
            {
                ssize_t nwrite = 0;
                return sock_.write(buff.data(), buff.size(), nwrite);
            }

            common::ErrnoError IStream::sendFrame(const common::http2::frame_base& frame)
            {
                using namespace common;
                CHECK(sid() == frame.stream_id());
                buffer_type raw = frame.raw_data();
                return sendData(raw);
            }

            common::ErrnoError IStream::sendCloseFrame()
            {
                common::http2::frame_hdr hdr = common::http2::frame_rst::create_frame_header(0, sid());
                uint32_t er = be32toh(common::http2::HTTP2_STREAM_CLOSED);
                common::http2::frame_rst rst(hdr, &er);
                return sendFrame(rst);
            }

            IStream* IStream::createStream(const common::net::socket_info& info, const common::http2::frame_base &frame)
            {
                if(!frame.isValid()){
                    NOTREACHED();
                    return NULL;
                }

                common::http2::frame_type type = frame.type();

                switch(type){
                case common::http2::HTTP2_DATA:
                    return new HTTP2DataStream(info, frame);
                case common::http2::HTTP2_HEADERS:
                    return new HTTP2HeadersStream(info, frame);
                case common::http2::HTTP2_PRIORITY:
                {
                    IStream* res = new HTTP2PriorityStream(info, frame);
                    return res;
                }
                case common::http2::HTTP2_RST_STREAM:
                    NOTREACHED();
                    return NULL;
                case common::http2::HTTP2_SETTINGS:
                    return new HTTP2SettingsStream(info, frame);
                case common::http2::HTTP2_PUSH_PROMISE:
                    NOTREACHED();
                    return NULL;
                case common::http2::HTTP2_PING:
                    NOTREACHED();
                    return NULL;
                case common::http2::HTTP2_GOAWAY:
                    NOTREACHED();
                    return NULL;
                case common::http2::HTTP2_WINDOW_UPDATE:
                    NOTREACHED();
                    return NULL;
                case common::http2::HTTP2_CONTINUATION:
                    NOTREACHED();
                    return NULL;

                default:
                    NOTREACHED();
                    return NULL;
                }
            }

            IStream::~IStream()
            {

            }

            HTTP2DataStream::HTTP2DataStream(const common::net::socket_info& info, const common::http2::frame_base &frame)
                : IStream(info, frame)
            {
                CHECK(common::http2::HTTP2_DATA == frame.type());
            }

            bool HTTP2DataStream::processFrameImpl(const common::http2::frame_base& frame)
            {
                return true;
            }

            HTTP2PriorityStream::HTTP2PriorityStream(const common::net::socket_info& info, const common::http2::frame_base &frame)
                : IStream(info, frame)
            {
                CHECK(common::http2::HTTP2_PRIORITY == frame.type());
            }

            HTTP2PriorityStream::~HTTP2PriorityStream()
            {

            }

            bool HTTP2PriorityStream::processFrameImpl(const common::http2::frame_base& frame)
            {
                sendCloseFrame();
                return true;
            }

            HTTP2SettingsStream::HTTP2SettingsStream(const common::net::socket_info& info, const common::http2::frame_base &frame)
                : IStream(info, frame), negotiated_(false)
            {
                CHECK(common::http2::HTTP2_SETTINGS == frame.type());
            }

            bool HTTP2SettingsStream::isNegotiated() const
            {
                return negotiated_;
            }

            bool HTTP2SettingsStream::processFrameImpl(const common::http2::frame_base& frame)
            {
                if(frame.type() == common::http2::HTTP2_SETTINGS){
                    sendFrame(frame);
                    if(frame.flags() & common::http2::HTTP2_FLAG_ACK){
                        negotiated_ = true;
                    }
                }
                else if(frame.type() != common::http2::HTTP2_GOAWAY){
                    sendCloseFrame();
                }
                return true;
            }

            HTTP2HeadersStream::HTTP2HeadersStream(const common::net::socket_info& info, const common::http2::frame_base &frame)
                : IStream(info, frame)
            {
                CHECK(common::http2::HTTP2_HEADERS == frame.type());
            }

            bool HTTP2HeadersStream::processFrameImpl(const common::http2::frame_base& frame)
            {
                return false;
            }
        }
    }
}
