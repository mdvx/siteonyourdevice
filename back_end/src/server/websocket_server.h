#pragma once

#include "http/http_server_handler.h"

#include "http/http_server.h"
#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpServerHost;
        class Http2Client;

        class WebSocketServerHost :
                public Http2Server
        {
        public:
            WebSocketServerHost(const common::net::hostAndPort& host, ITcpLoopObserver *observer);

            virtual const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);
        };

        class WebSocketClientHost
                : public Http2Client
        {
        public:
            WebSocketClientHost(ITcpLoop* server, const common::net::socket_info& info);
        };

        class WebSocketServerHandlerHost
            : public Http2ServerHandler
        {
        public:
            typedef WebSocketServerHost server_t;
            typedef WebSocketClientHost client_t;

            WebSocketServerHandlerHost(const HttpServerInfo & info, HttpServerHost * parent);

            virtual void dataReceived(TcpClient* client);

        private:
            void processWebsocketRequest(HttpClient *hclient, const common::http::http_request& hrequest);

            HttpServerHost * parent_;
        };
    }
}
