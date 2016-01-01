#pragma once

#include "tcp/tcp_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace websocket
        {
            class WebSocketServer
                    : public tcp::TcpServer
            {
            public:
                WebSocketServer(const common::net::hostAndPort& host, tcp::ITcpLoopObserver* observer = NULL);
                const char* className() const;

            protected:
                virtual tcp::TcpClient * createClient(const common::net::socket_info& info);
            };
        }
    }
}
