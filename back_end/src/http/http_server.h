#pragma once

#include "tcp/tcp_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace http
        {
            class HttpServer
                    : public tcp::TcpServer
            {
            public:
                HttpServer(const common::net::hostAndPort& host, tcp::ITcpLoopObserver *observer);
                ~HttpServer();

                virtual const char* className() const;

            protected:
                virtual tcp::TcpClient * createClient(const common::net::socket_info &info);
            };

            class Http2Server
                    : public HttpServer
            {
            public:
                Http2Server(const common::net::hostAndPort& host, tcp::ITcpLoopObserver *observer);

                virtual const char* className() const;

            protected:
                virtual tcp::TcpClient * createClient(const common::net::socket_info &info);
            };
        }
    }
}
