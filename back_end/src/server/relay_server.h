#pragma once

#include "common/thread/thread.h"

#include "common/net/socket_tcp.h"

#include "commands/commands.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class InnerTcpClient;
        class TcpClient;

        class IRelayServer
                : common::net::ServerSocketTcp
        {
        public:
            typedef common::shared_ptr<TcpClient> client_t;
            IRelayServer(InnerTcpClient *parent, client_t client);
            ~IRelayServer();

            client_t client() const;
            void setClient(client_t client);

            void start();

            void addRequest(const common::buffer_type& request);

        private:
            virtual cmd_request_t createSocketCmd(const common::net::hostAndPort& host) const = 0;

            int exec();

            volatile bool stop_;
            client_t client_;
            std::shared_ptr<common::thread::Thread<int> > relayThread_;
            InnerTcpClient *parent_;
            std::vector<common::buffer_type> requests_;
        };
    }
}
