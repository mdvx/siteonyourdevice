#pragma once

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            class WebSocketClientHost
                    : public http::Http2Client
            {
            public:
                WebSocketClientHost(tcp::ITcpLoop* server, const common::net::socket_info& info);

                virtual const char* className() const;
            };
        }
    }
}
