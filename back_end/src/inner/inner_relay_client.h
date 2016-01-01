#pragma once

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            class RelayClient
                    : public http::Http2Client
            {
            public:
                RelayClient(tcp::ITcpLoop* server, const common::net::socket_info& info);
                const char* className() const;
            };

            class ProxyRelayClient;

            class RelayClientEx
                    : public RelayClient
            {
            public:
                RelayClientEx(tcp::ITcpLoop* server, const common::net::socket_info& info, const common::net::hostAndPort& externalHost);
                const char* className() const;

                common::net::hostAndPort externalHost() const;

                ProxyRelayClient *eclient() const;
                void setEclient(ProxyRelayClient* client);

            private:
                const common::net::hostAndPort external_host_;
                ProxyRelayClient * eclient_;
            };

            class ProxyRelayClient
                    : public tcp::TcpClient
            {
            public:
                ProxyRelayClient(tcp::ITcpLoop* server, const common::net::socket_info& info, RelayClientEx * relay);
                RelayClientEx * relay() const;

            private:
                RelayClientEx * const relay_;
            };
        }
    }
}
