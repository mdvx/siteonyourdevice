#include "server/inner/inner_tcp_client.h"

#include "server/inner/inner_tcp_server.h"
#include "server/relay_server.h"
#include "server/server_commands.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            namespace
            {
                class HttpRelayServer
                        : public IRelayServer
                {
                public:
                    HttpRelayServer(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, inner::InnerTcpClient *parent, client_t client)
                        : IRelayServer(parent, client), handler_(handler)
                    {

                    }

                private:
                    virtual cmd_request_t createSocketCmd(const common::net::hostAndPort& host) const
                    {
                        return handler_->make_request(SERVER_PLEASE_CONNECT_HTTP_COMMAND_REQ_1S, common::convertToString(host));
                    }

                    fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler_;
                };

                class WebSocketRelayServer
                        : public IRelayServer
                {
                   const common::net::hostAndPort srcHost_;
                public:
                    WebSocketRelayServer(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, inner::InnerTcpClient *parent, client_t client, const common::net::hostAndPort& srcHost)
                        : IRelayServer(parent, client), handler_(handler), srcHost_(srcHost)
                    {

                    }

                private:
                    virtual cmd_request_t createSocketCmd(const common::net::hostAndPort& host) const
                    {
                        return handler_->make_request(SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND_REQ_2SS, common::convertToString(host), common::convertToString(srcHost_));
                    }

                    fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler_;
                };
            }

            namespace inner
            {
                InnerTcpClient::InnerTcpClient(tcp::TcpServer* server, const common::net::socket_info& info)
                    : InnerClient(server, info), hinfo_(), relays_http_(), relays_websockets_()
                {

                }

                const char* InnerTcpClient::className() const
                {
                    return "InnerTcpClient";
                }

                void InnerTcpClient::addHttpRelayClient(InnerServerHandlerHost *handler, client_t client, const common::buffer_type &request)
                {
                    for(size_t i = 0; i < relays_http_.size(); ++i){
                        relay_server_t rserver = relays_http_[i];
                        IRelayServer::client_t rclient = rserver->client();
                        if(rclient == INVALID_DESCRIPTOR){
                            rserver->addRequest(request);
                            rserver->setClient(client);
                            return;
                        }

                        if(rclient == client){
                            rserver->addRequest(request);
                            return;
                        }
                    }

                    std::shared_ptr<IRelayServer> tmp(new HttpRelayServer(handler, this, client));
                    tmp->addRequest(request);
                    tmp->start();
                    relays_http_.push_back(tmp);
                }

                void InnerTcpClient::addWebsocketRelayClient(InnerServerHandlerHost* handler, client_t client, const common::buffer_type& request, const common::net::hostAndPort& srcHost)
                {
                    for(size_t i = 0; i < relays_websockets_.size(); ++i){
                        relay_server_t rserver = relays_websockets_[i];
                        IRelayServer::client_t rclient = rserver->client();
                        if(rclient == INVALID_DESCRIPTOR){
                            rserver->addRequest(request);
                            rserver->setClient(client);
                            return;
                        }

                        if(rclient == client){
                            rserver->addRequest(request);
                            return;
                        }
                    }

                    std::shared_ptr<IRelayServer> tmp(new WebSocketRelayServer(handler, this, client, srcHost));
                    tmp->addRequest(request);
                    tmp->start();
                    relays_websockets_.push_back(tmp);
                }

                InnerTcpClient::~InnerTcpClient()
                {
                    relays_http_.clear();
                    relays_websockets_.clear();
                }

                void InnerTcpClient::setServerHostInfo(const UserAuthInfo &info)
                {
                    hinfo_ = info;
                }

                const UserAuthInfo &InnerTcpClient::serverHostInfo() const
                {
                    return hinfo_;
                }
            }
        }
    }
}
