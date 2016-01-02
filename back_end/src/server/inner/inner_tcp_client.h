#pragma once

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

#include "server/relay_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace tcp
        {
            class TcpServer;
        }

        namespace server
        {
            namespace inner
            {
                class InnerServerHandlerHost;

                class InnerTcpClient
                        : public fasto::siteonyourdevice::inner::InnerClient
                {
                public:
                    typedef IRelayServer::client_t client_t;
                    typedef std::shared_ptr<IRelayServer> relay_server_t;

                    InnerTcpClient(tcp::TcpServer* server, const common::net::socket_info& info);
                    ~InnerTcpClient();

                    virtual const char* className() const;

                    void setServerHostInfo(const UserAuthInfo& info);
                    const UserAuthInfo& serverHostInfo() const;

                    void addHttpRelayClient(InnerServerHandlerHost* handler, client_t client, const common::buffer_type& request); //move ovnerships
                    void addWebsocketRelayClient(InnerServerHandlerHost* handler, client_t client, const common::buffer_type& request,
                                                 const common::net::hostAndPort &srcHost); //move ovnerships

                private:
                    UserAuthInfo hinfo_;
                    std::vector<relay_server_t> relays_http_;
                    std::vector<relay_server_t> relays_websockets_;
                };
            }
        }
    }
}
