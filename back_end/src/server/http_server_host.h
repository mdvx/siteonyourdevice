#pragma once

#include "http/http_server.h"

#include "common/thread/thread.h"

#include "server/inner_tcp_server.h"

#include "http/http_server_handler.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpServerHost;

        class HttpInnerServerHandlerHost
                : public Http2ServerHandler
        {
        public:
            typedef Http2Server server_t;
            typedef HttpClient client_t;

            HttpInnerServerHandlerHost(const HttpServerInfo & info, HttpServerHost * parent);
            virtual void accepted(TcpClient* client);
            virtual void closed(TcpClient* client);
            virtual void dataReceived(TcpClient* client);

            virtual ~HttpInnerServerHandlerHost();

        private:
            HttpServerHost* const parent_;
        };

        class HttpServerHost
        {
        public:
            typedef std::unordered_map<std::string, InnerTcpClient*> inner_connections_type;

            HttpServerHost(const common::net::hostAndPort& httpHost, const common::net::hostAndPort& innerHost);
            ~HttpServerHost();

            void setStorageConfig(const redis_sub_configuration_t &config);

            bool unRegisterInnerConnectionByHost(TcpClient* connection) WARN_UNUSED_RESULT;
            bool registerInnerConnectionByUser(const UserAuthInfo& user, TcpClient* connection) WARN_UNUSED_RESULT;
            bool findUser(const UserAuthInfo& user) const;
            InnerTcpClient* const findInnerConnectionByHost(const std::string& host) const;

            int exec() WARN_UNUSED_RESULT;
            void stop();

            InnerServerHandlerHost * innerHandler() const;

        private:
            HttpInnerServerHandlerHost* httpHandler_;
            Http2Server* httpServer_;
            std::shared_ptr<common::thread::Thread<int> > http_thread_;

            InnerServerHandlerHost* innerHandler_;
            InnerTcpServer * innerServer_;
            std::shared_ptr<common::thread::Thread<int> > inner_thread_;

            inner_connections_type connections_;
            RedisStorage rstorage_;
        };
    }
}
