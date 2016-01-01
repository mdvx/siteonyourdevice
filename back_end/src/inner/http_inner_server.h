#pragma once

#include "http_config.h"

#include "http/http_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            class ProxyInnerServer
                    : public tcp::ITcpLoop
            {
            public:
                ProxyInnerServer(tcp::ITcpLoopObserver* observer);
                virtual const char* className() const;
            };

            class Http2InnerServer
                    : public http::Http2Server
            {
            public:
                Http2InnerServer(tcp::ITcpLoopObserver * observer, const HttpConfig& config);

                virtual const char* className() const;

            protected:
                virtual tcp::TcpClient * createClient(const common::net::socket_info &info);

            private:
                const HttpConfig config_;
            };
        }
    }
}
