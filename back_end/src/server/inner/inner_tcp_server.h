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
        namespace server
        {
            class HttpServerHost;

            namespace inner
            {
                class InnerServerHandlerHost
                        : public fasto::siteonyourdevice::inner::InnerServerCommandSeqParser, public tcp::ITcpLoopObserver
                {
                public:
                    enum
                    {
                        ping_timeout_clients = 60 //sec
                    };

                    InnerServerHandlerHost(HttpServerHost * parent);

                    virtual void preLooped(tcp::ITcpLoop* server);

                    virtual void accepted(tcp::TcpClient* client);
                    virtual void moved(tcp::TcpClient* client);
                    virtual void closed(tcp::TcpClient* client);

                    virtual void dataReceived(tcp::TcpClient* client);
                    virtual void dataReadyToWrite(tcp::TcpClient* client);
                    virtual void postLooped(tcp::ITcpLoop* server);
                    virtual void timerEmited(tcp::ITcpLoop* server, timer_id_type id);

                    virtual ~InnerServerHandlerHost();

                    void setStorageConfig(const redis_sub_configuration_t &config);

                private:
                    virtual void handleInnerRequestCommand(fasto::siteonyourdevice::inner::InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
                    virtual void handleInnerResponceCommand(fasto::siteonyourdevice::inner::InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
                    virtual void handleInnerApproveCommand(fasto::siteonyourdevice::inner::InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);

                    HttpServerHost* const parent_;

                    class InnerSubHandler;
                    RedisSub *sub_commands_in_;
                    InnerSubHandler *handler_;
                    std::shared_ptr<common::thread::Thread<void> > redis_subscribe_command_in_thread_;
                    timer_id_type ping_client_id_timer_;
                };

                class InnerTcpServer
                        : public tcp::TcpServer
                {
                public:
                    InnerTcpServer(const common::net::hostAndPort& host, tcp::ITcpLoopObserver* observer);

                    virtual const char* className() const;

                private:
                    virtual tcp::TcpClient * createClient(const common::net::socket_info& info);
                };
            }
        }
    }
}
