#pragma once

#include "infos.h"

#include "inner/inner_client.h"

#include "loop_controller.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace tcp
        {
            class TcpServer;
        }

        namespace inner
        {
            class InnerServerCommandSeqParser;
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
                    typedef std::pair<tcp::TcpClient *, common::buffer_type> request_t;
                    IInnerRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, InnerTcpClient *parent, const request_t& request);
                    ~IInnerRelayLoop();

                    bool readyForRequest() const;
                    void addRequest(const request_t& request);

                protected:
                    InnerTcpClient *const parent_;
                    fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *ihandler_;

                    const request_t request_;
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
                    UserAuthInfo serverHostInfo() const;

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
