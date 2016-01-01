#pragma once

#include "http/http_server_handler.h"

#include "http/http_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            class HttpServerHost;

            class WebSocketServerHost :
                    public http::Http2Server
            {
            public:
                WebSocketServerHost(const common::net::hostAndPort& host, tcp::ITcpLoopObserver *observer);

                virtual const char* className() const;

            protected:
                virtual tcp::TcpClient * createClient(const common::net::socket_info &info);
            };

            class WebSocketServerHandlerHost
                : public http::Http2ServerHandler
            {
            public:
                WebSocketServerHandlerHost(const HttpServerInfo & info, HttpServerHost * parent);

                virtual void dataReceived(tcp::TcpClient* client);

            private:
                void processWebsocketRequest(http::HttpClient *hclient, const common::http::http_request& hrequest);

                HttpServerHost * parent_;
            };
        }
    }
}
