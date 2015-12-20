#pragma once

#include "http/http_server.h"

#include "common/thread/thread.h"

#include "server/inner_tcp_server.h"

#include "http/http_server_handler.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpServerHandlerHost;

        class HttpInnerServerHandlerHost
                : public Http2ServerHandler
        {
        public:
            typedef Http2Server server_t;
            typedef HttpClient client_t;

            HttpInnerServerHandlerHost(const HttpServerInfo & info, HttpServerHandlerHost * parent);
            virtual void accepted(TcpClient* client);
            virtual void closed(TcpClient* client);
            virtual void dataReceived(TcpClient* client);

            virtual ~HttpInnerServerHandlerHost();

        private:
            HttpServerHandlerHost* const parent_;
        };

        class HttpServerHandlerHost
        {
        public:
            typedef std::unordered_map<std::string, InnerTcpClient*> inner_connections_type;

            HttpServerHandlerHost(const HttpServerInfo& info);
            ~HttpServerHandlerHost();

            bool unRegisterInnerConnectionByHost(TcpClient* connection) WARN_UNUSED_RESULT;
            bool registerInnerConnectionByUser(const UserAuthInfo& user, TcpClient* connection) WARN_UNUSED_RESULT;
            bool findUser(const UserAuthInfo& user) const;
            InnerTcpClient* const findInnerConnectionByHost(const std::string& host) const;

            InnerServerHandlerHost* innerHandler() const;
            HttpInnerServerHandlerHost* httpHandler() const;

            void setStorageConfig(const redis_sub_configuration_t &config);

        private:
            inner_connections_type connections_;

            InnerServerHandlerHost* innerHandler_;
            HttpInnerServerHandlerHost* httpHandler_;

            RedisStorage rstorage_;
        };

        class HttpServerHost
        {
        public:
            HttpServerHost(const common::net::hostAndPort& httpHost, const common::net::hostAndPort& innerHost, HttpServerHandlerHost *handler);
            virtual ~HttpServerHost();

            common::Error bind() WARN_UNUSED_RESULT;
            common::Error listen(int backlog) WARN_UNUSED_RESULT;
            int exec() WARN_UNUSED_RESULT;
            void stop();

        private:
            Http2Server* httpServer_;
            std::shared_ptr<common::thread::Thread<int> > httpThread_;

            InnerTcpServer* innerServer_;
            std::shared_ptr<common::thread::Thread<int> > innerThread_;
        };
    }
}
