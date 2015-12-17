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

            common::net::hostAndPort host() const;

            void close();

            void setName(const std::string& name);
            std::string name() const;

            common::id_counter<TcpClient>::type_t id() const;
            const char* className() const;
            std::string formatedName() const;

            void execInServerThread(function_type func);

        public:
            void closeClient(TcpClient *client);

        protected:
            virtual TcpClient * createClient(const common::net::socket_info& info);

        private:
            static void read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);
            static void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);

            virtual void preLooped(EvLoop* loop);
            virtual void stoped(EvLoop* loop);
            virtual void postLooped(EvLoop* loop);

            common::ErrnoError accept(common::net::socket_info& info) WARN_UNUSED_RESULT;

            common::net::ServerSocketTcp sock_;
            uint32_t total_clients_;

            TcpServerObserver* const observer_;
            EvLoop impl_;

            fasto_s_sync * accept_io_;

            std::string name_;
            const common::id_counter<TcpServer> id_;
        };

        class TcpClient
                : common::IMetaClassInfo
        {
        public:
            friend class TcpServer;
            TcpClient(TcpServer* server, const common::net::socket_info& info);
            virtual ~TcpClient();

            TcpServer* server() const;

            common::net::socket_info info() const;
            int fd() const;

            virtual common::ErrnoError write(const char *data, uint16_t size, ssize_t &nwrite) WARN_UNUSED_RESULT;
            virtual common::ErrnoError read(char* outData, uint16_t maxSize, ssize_t &nread) WARN_UNUSED_RESULT;

            void close();

            void setName(const std::string& name);
            std::string name() const;

            common::id_counter<TcpClient>::type_t id() const;
            virtual const char *className() const;
            std::string formatedName() const;

        private:
            TcpServer* server_;
            fasto_cs_sync * read_io_;
            common::net::SocketHolder sock_;
            std::string name_;
            const common::id_counter<TcpClient> id_;
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
