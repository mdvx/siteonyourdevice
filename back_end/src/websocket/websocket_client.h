#pragma once

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace websocket
        {
            class WebSocketClient
                    : public http::Http2Client
            {
            public:
                WebSocketClient(tcp::ITcpLoop* server, const common::net::socket_info& info);
                ~WebSocketClient();

                const char* className() const;
            };
        }
    }
}
