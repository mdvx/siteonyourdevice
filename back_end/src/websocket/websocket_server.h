#pragma once

#include "tcp_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class WebSocketServer
                : public TcpServer
        {
        public:
            WebSocketServer(const common::net::hostAndPort& host, ITcpLoopObserver* observer = NULL);
            const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info& info);
        };
    }
}
