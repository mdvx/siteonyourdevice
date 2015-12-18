#pragma once

#include "http_config.h"

#include "http/http_server.h"
#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class ProxyRelayClient;

        class RelayClient
                : public Http2Client
        {
        public:
            RelayClient(ITcpLoop* server, const common::net::socket_info& info, const common::net::hostAndPort& externalHost);
            const char* className() const;

            common::net::hostAndPort externalHost() const;

            ProxyRelayClient *eclient() const;
            void setEclient(ProxyRelayClient* client);

        private:
            const common::net::hostAndPort external_host_;
            ProxyRelayClient * eclient_;
        };

        class ProxyRelayClient
                : public TcpClient
        {
        public:
            ProxyRelayClient(ITcpLoop* server, const common::net::socket_info& info, RelayClient * relay);
            RelayClient * relay() const;

        private:
            RelayClient * const relay_;
        };

        class ProxyInnerServer
                : public ITcpLoop
        {
        public:
            ProxyInnerServer(ITcpLoopObserver* observer, const configuration_t& config);

            virtual const char* className() const;

        private:
            const configuration_t& config_;
        };

        class Http2InnerServer
                : public Http2Server
        {
        public:
            Http2InnerServer(const common::net::hostAndPort& host, ITcpLoopObserver * observer, const configuration_t& config);

            virtual const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);

        private:
            const configuration_t& config_;
        };
    }
}
