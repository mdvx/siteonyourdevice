#pragma once

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class WebSocketClient
                : public Http2Client
        {
        public:
            WebSocketClient(ITcpLoop* server, const common::net::socket_info& info);
            ~WebSocketClient();

            const char* className() const;
        };
    }
}
