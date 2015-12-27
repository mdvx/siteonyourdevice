#pragma once

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class WebSocketClientHost
                : public Http2Client
        {
        public:
            WebSocketClientHost(ITcpLoop* server, const common::net::socket_info& info);

            virtual const char* className() const;
        };
    }
}
