#pragma once

#include "http/http_client.h"

#include "infos.h"

namespace fasto
{
    namespace fastoremote
    {
        class HttpServer
                : public TcpServer
        {
        public:
            HttpServer(const common::net::hostAndPort& host, TcpServerObserver *observer);
            ~HttpServer();

            void setHttpServerInfo(const HttpServerInfo& info);
            const HttpServerInfo& info() const;

            virtual const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);

        private:
            HttpServerInfo info_;
        };

        class Http2Server
                : public HttpServer
        {
        public:
            Http2Server(const common::net::hostAndPort& host, TcpServerObserver *observer);

            virtual const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);
        };
    }
}
