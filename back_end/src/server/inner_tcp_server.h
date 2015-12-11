#pragma once

#include "common/thread/thread.h"

#include "inner/inner_server_command_seq_parser.h"

#include "server/redis_helpers.h"

#include "infos.h"

namespace fasto
{
    namespace fastoremote
    {
        typedef UserAuthInfo HostInfo;

        class HttpServerHandlerHost;

        class InnerServerHandlerHost
                : public InnerServerCommandSeqParser, public TcpServerObserver
        {
        public:
            InnerServerHandlerHost(HttpServerHandlerHost * parent);

            virtual void preLooped(TcpServer* server);

            virtual void accepted(TcpClient* client);
            virtual void moved(TcpClient* client);
            virtual void closed(TcpClient* client);

            virtual void dataReceived(TcpClient* client);
            virtual void postLooped(TcpServer* server);
            virtual ~InnerServerHandlerHost();

            void setStorageConfig(const redis_sub_configuration_t &config);

        private:
            virtual void handleInnerRequestCommand(InnerClient *connection, uint64_t id, int argc, char *argv[]);
            virtual void handleInnerResponceCommand(InnerClient *connection, uint64_t id, int argc, char *argv[]);
            virtual void handleInnerApproveCommand(InnerClient *connection, uint64_t id, int argc, char *argv[]);

            HttpServerHandlerHost* const parent_;

            class InnerSubHandler;
            RedisSub *sub_commands_in_;
            InnerSubHandler *handler_;
            std::shared_ptr<common::thread::Thread<void> > redis_subscribe_command_in_thread_;
        };

        class RelayServer;

        class InnerTcpClient
                : public InnerClient
        {
        public:
            typedef std::shared_ptr<RelayServer> relay_server_t;

            InnerTcpClient(TcpServer* server, const common::net::socket_info& info);
            ~InnerTcpClient();

            virtual const char* className() const;

            void setServerHostInfo(const HostInfo& info);
            const HostInfo& serverHostInfo() const;

            void addClient(InnerServerHandlerHost* handler, TcpClient* client, const common::buffer_type& request); //move ovnerships

        private:
            HostInfo hinfo_;
            std::vector<relay_server_t> relays_;
        };

        class InnerTcpServer
                : public TcpServer
        {
        public:
            InnerTcpServer(const common::net::hostAndPort& host, TcpServerObserver* observer);

            virtual const char* className() const;

        private:
            virtual TcpClient * createClient(const common::net::socket_info& info);
        };


        class RelayServer
                : common::net::ServerSocketTcp
        {
        public:
            typedef common::shared_ptr<TcpClient> client_t;
            RelayServer(InnerServerHandlerHost *handler, InnerTcpClient *parent, const common::net::hostAndPort& host, client_t client);
            ~RelayServer();

            client_t client() const;
            void setClient(client_t client);

            void start();

            void addRequest(const common::buffer_type& request);

        private:
            int exec();
            common::ErrnoError bindAvailible();

            volatile bool stop_;
            client_t client_;
            std::shared_ptr<common::thread::Thread<int> > relayThread_;
            InnerTcpClient *parent_;
            InnerServerHandlerHost *handler_;
            std::vector<common::buffer_type> requests_;
        };
    }
}