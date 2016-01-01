#pragma once

#include "http/http_server.h"

#include "common/thread/thread.h"

#include "server/inner/inner_tcp_server.h"

#include "server/websocket/websocket_server.h"

#include "http/http_server_handler.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            class HttpServerHost;

            namespace inner
            {
                class InnerTcpClient;
            }

            class HttpInnerServerHandlerHost
                    : public http::Http2ServerHandler
            {
            public:
                typedef http::Http2Server server_t;
                typedef http::HttpClient client_t;

                HttpInnerServerHandlerHost(const HttpServerInfo & info, HttpServerHost * parent);
                virtual void accepted(tcp::TcpClient* client);
                virtual void closed(tcp::TcpClient* client);
                virtual void dataReceived(tcp::TcpClient* client);

                virtual ~HttpInnerServerHandlerHost();

            private:
                void processHttpRequest(http::HttpClient *hclient, const common::http::http_request& hrequest);
                HttpServerHost* const parent_;
            };

            class HttpServerHost
            {
            public:
                typedef std::unordered_map<std::string, inner::InnerTcpClient*> inner_connections_type;

                HttpServerHost(const common::net::hostAndPort& httpHost, const common::net::hostAndPort& innerHost, const common::net::hostAndPort& webSocketHost);
                ~HttpServerHost();

                void setStorageConfig(const redis_sub_configuration_t &config);

                bool unRegisterInnerConnectionByHost(tcp::TcpClient* connection) WARN_UNUSED_RESULT;
                bool registerInnerConnectionByUser(const UserAuthInfo& user, tcp::TcpClient* connection) WARN_UNUSED_RESULT;
                bool findUser(const UserAuthInfo& user) const;
                inner::InnerTcpClient* const findInnerConnectionByHost(const std::string& host) const;

                int exec() WARN_UNUSED_RESULT;
                void stop();

                inner::InnerServerHandlerHost * innerHandler() const;

            private:
                HttpInnerServerHandlerHost* httpHandler_;
                http::Http2Server* httpServer_;
                std::shared_ptr<common::thread::Thread<int> > http_thread_;

                inner::InnerServerHandlerHost* innerHandler_;
                inner::InnerTcpServer * innerServer_;
                std::shared_ptr<common::thread::Thread<int> > inner_thread_;

                WebSocketServerHandlerHost* websocketHandler_;
                WebSocketServerHost * websocketServer_;
                std::shared_ptr<common::thread::Thread<int> > websocket_thread_;

                inner_connections_type connections_;
                RedisStorage rstorage_;
            };
        }
    }
}
