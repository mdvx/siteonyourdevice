#pragma once

#include "http/http_server_handler.h"

#include "inner/inner_server_handler.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            class RelayClient;
            class RelayClientEx;
            class ProxyRelayClient;

            class Http2InnerServerHandler
                    : public InnerServerHandler, public http::Http2ServerHandler
            {
            public:
                enum
                {
                    ping_timeout_server = 30 //sec
                };

                Http2InnerServerHandler(const HttpServerInfo& info, const common::net::hostAndPort& innerHost, const HttpConfig& config);
                ~Http2InnerServerHandler();

                virtual void preLooped(tcp::ITcpLoop* server);
                virtual void accepted(tcp::TcpClient* client);
                virtual void closed(tcp::TcpClient* client);
                virtual void dataReceived(tcp::TcpClient* client);
                virtual void dataReadyToWrite(tcp::TcpClient* client);
                virtual void postLooped(tcp::ITcpLoop* server);
                virtual void timerEmited(tcp::ITcpLoop* server, timer_id_type id);

            private:
                void innerDataReceived(InnerClient* iclient);
                void relayDataReceived(RelayClient* rclient);
                void relayExDataReceived(RelayClientEx* rclient);
                void proxyDataReceived(ProxyRelayClient* prclient);

                InnerClient* innerConnection_;
                timer_id_type ping_server_id_timer_;

                const common::net::hostAndPort innerHost_;
            };
        }
    }
}
