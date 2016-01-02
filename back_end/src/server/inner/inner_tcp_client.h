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
                class InnerTcpClient;

                class IInnerRelayLoop
                        : public ILoopThreadController
                {
                public:
                    IInnerRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, InnerTcpClient *parent,
                                    tcp::TcpClient *client, const common::buffer_type& request);
                    ~IInnerRelayLoop();

                protected:
                    InnerTcpClient *const parent_;
                    fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *ihandler_;
                    tcp::TcpClient * const client_;
                    const common::buffer_type request_;
                };

                class InnerTcpClient
                        : public fasto::siteonyourdevice::inner::InnerClient
                {
                public:
                    typedef std::shared_ptr<IInnerRelayLoop> http_relay_loop_t;
                    typedef std::shared_ptr<IInnerRelayLoop> websocket_relay_loop_t;

                    InnerTcpClient(tcp::TcpServer* server, const common::net::socket_info& info);
                    ~InnerTcpClient();

                    virtual const char* className() const;

                    void setServerHostInfo(const UserAuthInfo& info);
                    const UserAuthInfo& serverHostInfo() const;

                    void addHttpRelayClient(InnerServerHandlerHost* handler, tcp::TcpClient* client, const common::buffer_type& request); //move ovnerships
                    void addWebsocketRelayClient(InnerServerHandlerHost* handler, tcp::TcpClient* client, const common::buffer_type& request,
                                                 const common::net::hostAndPort &srcHost); //move ovnerships

                private:
                    UserAuthInfo hinfo_;
                    std::vector<http_relay_loop_t> relays_http_;
                    std::vector<websocket_relay_loop_t> relays_websockets_;
                };
            }
        }
    }
}
