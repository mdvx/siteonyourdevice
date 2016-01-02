#pragma once

#include "common/thread/thread.h"

#include "common/net/socket_tcp.h"

#include "commands/commands.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            namespace inner
            {
                class InnerTcpClient;
            }

            class IRelayServer
                : common::net::ServerSocketTcp
            {
            public:
                typedef common::net::socket_descr_type client_t;
                IRelayServer(inner::InnerTcpClient *parent, client_t client);
                ~IRelayServer();

                client_t client() const;
                void setClient(client_t client);

                void start();

                void addRequest(const common::buffer_type& request);

            private:
                virtual cmd_request_t createSocketCmd(const common::net::hostAndPort& host) const = 0;

                int exec();

                volatile bool stop_;
                client_t client_fd_;
                std::shared_ptr<common::thread::Thread<int> > relayThread_;
                inner::InnerTcpClient *parent_;
                std::vector<common::buffer_type> requests_;
            };
        }
    }
}
