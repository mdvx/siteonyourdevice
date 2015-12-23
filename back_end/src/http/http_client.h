#pragma once

#include <time.h>

#include "tcp_client.h"

#include "http/http_streams.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpClient
                : public TcpClient
        {
        public:
            HttpClient(ITcpLoop* server, const common::net::socket_info& info);

            virtual common::Error send_ok(common::http::http_protocols protocol, const char* extra_header, const char* text, bool is_keep_alive, const HttpServerInfo& info) WARN_UNUSED_RESULT;
            virtual common::Error send_error(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* text, bool is_keep_alive, const HttpServerInfo& info) WARN_UNUSED_RESULT;
            virtual common::Error send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size) WARN_UNUSED_RESULT;
            virtual common::Error send_headers(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* mime_type, off_t* length, time_t* mod, bool is_keep_alive, const HttpServerInfo& info) WARN_UNUSED_RESULT;

            virtual const char* className() const;

            void setIsAuthenticated(bool auth);
            bool isAuthenticated() const;

        private:
            using TcpClient::write;
            bool isAuth_;
        };

        class Http2Client
                : public HttpClient
        {
            friend class Http2Server;
        public:
            typedef StreamSPtr stream_t;
            typedef std::vector<stream_t> streams_t;

            Http2Client(ITcpLoop* server, const common::net::socket_info& info);

            virtual common::Error send_error(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* text, bool is_keep_alive, const HttpServerInfo& info) WARN_UNUSED_RESULT;
            virtual common::Error send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size) WARN_UNUSED_RESULT;
            virtual common::Error send_headers(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* mime_type, off_t* length, time_t* mod, bool is_keep_alive, const HttpServerInfo& info) WARN_UNUSED_RESULT;

            void processFrames(const common::http2::frames_t& frames);

            bool isSettingNegotiated() const;

            virtual const char* className() const;

        private:
            bool is_http2() const;
            StreamSPtr findStreamByStreamID(uint32_t stream_id) const;
            StreamSPtr findStreamByType(common::http2::frame_type type) const;
            streams_t streams_;
        };
    }
}
