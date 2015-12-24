#pragma once

#include "http/http_server_handler.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class WebSocketServerHandler
                : public Http2ServerHandler
        {
        public:
            WebSocketServerHandler(const HttpServerInfo &info);

        protected:
            virtual void processReceived(HttpClient *hclient, const char* request, size_t req_len);
            virtual void handleRequest(HttpClient *hclient, const common::http::http_request& hrequest, bool notClose);
        };
    }
}
