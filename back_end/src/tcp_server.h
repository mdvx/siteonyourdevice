#pragma once

#include "common/patterns/crtp_pattern.h"
#include "common/net/socket_tcp.h"

#include "event_loop.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class TcpServerObserver;
        class TcpClient;

        class TcpServer
                : public EvLoopObserver, common::IMetaClassInfo
        {
        public:
            TcpServer(const common::net::hostAndPort& host, TcpServerObserver* observer = NULL);
            virtual ~TcpServer();

            common::ErrnoError bind() WARN_UNUSED_RESULT;
            common::ErrnoError listen(int backlog) WARN_UNUSED_RESULT;

            int exec() WARN_UNUSED_RESULT;
            void stop();

            void registerClient(const common::net::socket_info& info);
            void registerClient(TcpClient * client);
            void unregisterClient(TcpClient * client);
            virtual void closeClient(TcpClient *client);

            common::net::hostAndPort host() const;

            void close();

            void setName(const std::string& name);
            std::string name() const;

            common::id_counter<TcpServer>::type_t id() const;
            const char* className() const;
            std::string formatedName() const;

            void execInServerThread(function_type func);

        protected:
            virtual TcpClient * createClient(const common::net::socket_info& info);

        private:
            static void read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);
            static void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);

            virtual void preLooped(LibEvLoop* loop);
            virtual void stoped(LibEvLoop* loop);
            virtual void postLooped(LibEvLoop* loop);

            common::ErrnoError accept(common::net::socket_info& info) WARN_UNUSED_RESULT;

            common::net::ServerSocketTcp sock_;
            uint32_t total_clients_;

            TcpServerObserver* const observer_;
            LibEvLoop* const loop_;

            ev_io * accept_io_;

            std::string name_;
            const common::id_counter<TcpServer> id_;
        };

        class TcpServerObserver
        {
        public:
            virtual void preLooped(TcpServer* server) = 0;

            virtual void accepted(TcpClient* client) = 0;
            virtual void moved(TcpClient* client) = 0;
            virtual void closed(TcpClient* client) = 0;

            virtual void dataReceived(TcpClient* client) = 0;
            virtual void postLooped(TcpServer* server) = 0;
            virtual ~TcpServerObserver();
        };
    }
}
