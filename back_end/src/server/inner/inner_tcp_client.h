#pragma once

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class IRelayServer;
        class TcpServer;
        class InnerServerHandlerHost;

        class InnerTcpClient
                : public InnerClient
        {
        public:
            typedef std::shared_ptr<IRelayServer> relay_server_t;

            InnerTcpClient(TcpServer* server, const common::net::socket_info& info);
            ~InnerTcpClient();

            virtual const char* className() const;

            void setServerHostInfo(const UserAuthInfo& info);
            const UserAuthInfo& serverHostInfo() const;

            void addHttpRelayClient(InnerServerHandlerHost* handler, TcpClient* client, const common::buffer_type& request); //move ovnerships
            void addWebsocketRelayClient(InnerServerHandlerHost* handler, TcpClient* client, const common::buffer_type& request); //move ovnerships

        private:
            UserAuthInfo hinfo_;
            std::vector<relay_server_t> relays_http_;
            std::vector<relay_server_t> relays_websockets_;
        };
    }
}
