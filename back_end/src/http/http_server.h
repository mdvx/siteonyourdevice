#pragma once

#include "tcp_server.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpServer
                : public TcpServer
        {
        public:
            HttpServer(const common::net::hostAndPort& host, ITcpLoopObserver *observer);
            ~HttpServer();

            virtual const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);
        };

        class Http2Server
                : public HttpServer
        {
        public:
            Http2Server(const common::net::hostAndPort& host, ITcpLoopObserver *observer);

            virtual const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);
        };
    }
}
