#pragma once

#include <time.h>

#include "tcp_server.h"

#include "http/http_streams.h"

namespace fasto
{
    namespace fastoremote
    {
        class HttpClient
                : public TcpClient
        {
        public:
            HttpClient(TcpServer* server, const common::net::socket_info& info);

            virtual common::ErrnoError send_error(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* text, bool is_keep_alive);
            virtual common::ErrnoError send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size);
            virtual common::ErrnoError send_headers(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* mime_type, off_t* length, time_t* mod, bool is_keep_alive);

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

            Http2Client(TcpServer* server, const common::net::socket_info& info);

            virtual common::ErrnoError send_error(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* text, bool is_keep_alive);
            virtual common::ErrnoError send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size);
            virtual common::ErrnoError send_headers(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* mime_type, off_t* length, time_t* mod, bool is_keep_alive);

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
