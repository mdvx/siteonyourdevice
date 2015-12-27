#pragma once

#include "common/thread/thread.h"

#include "inner/inner_server_command_seq_parser.h"

#include "server/redis_helpers.h"

#include "tcp/tcp_server.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpServerHost;

        class InnerServerHandlerHost
                : public InnerServerCommandSeqParser, public ITcpLoopObserver
        {
        public:
            InnerServerHandlerHost(HttpServerHost * parent);

            virtual void preLooped(ITcpLoop* server);

            virtual void accepted(TcpClient* client);
            virtual void moved(TcpClient* client);
            virtual void closed(TcpClient* client);

            virtual void dataReceived(TcpClient* client);
            virtual void postLooped(ITcpLoop* server);
            virtual ~InnerServerHandlerHost();

            void setStorageConfig(const redis_sub_configuration_t &config);

        private:
            virtual void handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
            virtual void handleInnerResponceCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
            virtual void handleInnerApproveCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);

            HttpServerHost* const parent_;

            class InnerSubHandler;
            RedisSub *sub_commands_in_;
            InnerSubHandler *handler_;
            std::shared_ptr<common::thread::Thread<void> > redis_subscribe_command_in_thread_;
        };

        class IRelayServer;

        class InnerTcpServer
                : public TcpServer
        {
        public:
            InnerTcpServer(const common::net::hostAndPort& host, ITcpLoopObserver* observer);

            virtual const char* className() const;

        private:
            virtual TcpClient * createClient(const common::net::socket_info& info);
        };
    }
}
