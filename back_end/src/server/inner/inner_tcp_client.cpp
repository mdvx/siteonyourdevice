#include "server/inner/inner_tcp_client.h"

#include "server/inner/inner_tcp_server.h"
#include "server/relay_server.h"
#include "server/server_commands.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace
        {
            class HttpRelayServer
                    : public IRelayServer
            {
            public:
                HttpRelayServer(InnerServerCommandSeqParser *handler, InnerTcpClient *parent, client_t client)
                    : IRelayServer(parent, client), handler_(handler)
                {

                }

            private:
                virtual std::string createSocketCmd(const common::net::hostAndPort& host) const
                {
                    return handler_->make_request(SERVER_PLEASE_CONNECT_HTTP_COMMAND_REQ_1S, common::convertToString(host));
                }

                InnerServerCommandSeqParser *handler_;
            };

            class WebSocketRelayServer
                    : public IRelayServer
            {
            public:
                WebSocketRelayServer(InnerServerCommandSeqParser *handler, InnerTcpClient *parent, client_t client)
                    : IRelayServer(parent, client), handler_(handler)
                {

                }

            private:
                virtual std::string createSocketCmd(const common::net::hostAndPort& host) const
                {
                    return handler_->make_request(SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND_REQ_1S, common::convertToString(host));
                }

                InnerServerCommandSeqParser *handler_;
            };
        }

        InnerTcpClient::InnerTcpClient(TcpServer* server, const common::net::socket_info& info)
            : InnerClient(server, info), hinfo_(), relays_http_(), relays_websockets_()
        {

        }

        const char* InnerTcpClient::className() const
        {
            return "InnerTcpClient";
        }

        void InnerTcpClient::addHttpRelayClient(InnerServerHandlerHost *handler, TcpClient* client, const common::buffer_type &request)
        {
            IRelayServer::client_t rrclient(client);

            for(int i = 0; i < relays_http_.size(); ++i){
                relay_server_t rserver = relays_http_[i];
                IRelayServer::client_t rclient = rserver->client();
                if(!rclient){
                    rserver->addRequest(request);
                    rserver->setClient(rrclient);
                    return;
                }

                if(rclient == rrclient){
                    rserver->addRequest(request);
                    return;
                }
            }

            std::shared_ptr<IRelayServer> tmp(new HttpRelayServer(handler, this, rrclient));
            tmp->addRequest(request);
            tmp->start();
            relays_http_.push_back(tmp);
        }

        void InnerTcpClient::addWebsocketRelayClient(InnerServerHandlerHost* handler, TcpClient* client, const common::buffer_type& request)
        {
            IRelayServer::client_t rrclient(client);

            for(int i = 0; i < relays_websockets_.size(); ++i){
                relay_server_t rserver = relays_websockets_[i];
                IRelayServer::client_t rclient = rserver->client();
                if(!rclient){
                    rserver->addRequest(request);
                    rserver->setClient(rrclient);
                    return;
                }

                if(rclient == rrclient){
                    rserver->addRequest(request);
                    return;
                }
            }

            std::shared_ptr<IRelayServer> tmp(new WebSocketRelayServer(handler, this, rrclient));
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
