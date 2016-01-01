#pragma once

#include "http/http_server_handler.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace websocket
        {
            class WebSocketServerHandler
                    : public http::Http2ServerHandler
            {
            public:
                WebSocketServerHandler(const HttpServerInfo &info);

            protected:
                virtual void processReceived(http::HttpClient *hclient, const char* request, size_t req_len);
                virtual void handleRequest(http::HttpClient *hclient, const common::http::http_request& hrequest, bool notClose);
            };
        }
    }
}
