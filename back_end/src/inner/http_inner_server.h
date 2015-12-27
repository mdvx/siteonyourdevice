#pragma once

#include "http_config.h"

#include "http/http_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class ProxyInnerServer
                : public ITcpLoop
        {
        public:
            ProxyInnerServer(ITcpLoopObserver* observer);
            virtual const char* className() const;
        };

        class Http2InnerServer
                : public Http2Server
        {
        public:
            Http2InnerServer(ITcpLoopObserver * observer, const HttpConfig& config);

            virtual const char* className() const;

        protected:
            virtual TcpClient * createClient(const common::net::socket_info &info);

        private:
            const HttpConfig config_;
        };
    }
}
