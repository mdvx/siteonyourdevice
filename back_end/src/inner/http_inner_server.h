#pragma once

#include "http_config.h"

#include "http/http_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class Http2InnerServerHandler;

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
            Http2InnerServer(const common::net::hostAndPort& host, Http2InnerServerHandler * handler, const configuration_t& config);

            common::Error innerConnect();
            common::Error innerDisConnect();

            RelayClient* createRelayClient(const common::net::socket_info &info);

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);

        private:
            Http2InnerServerHandler * handler_;
            const configuration_t& config_;
        };
    }
}
