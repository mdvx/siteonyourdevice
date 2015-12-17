#pragma once

#include "http_config.h"

#include "http/http_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class RelayClient
                : public Http2Client
        {
        public:
            RelayClient(TcpServer* server, const common::net::socket_info& info);
            const char* className() const;
        };

        class Http2InnerServer
                : public Http2Server
        {
        public:
            Http2InnerServer(const common::net::hostAndPort& host, TcpServerObserver * observer, const configuration_t& config);

            RelayClient* createRelayClient(const common::net::socket_info &info);

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);

        private:
            const configuration_t& config_;
        };
    }
}
