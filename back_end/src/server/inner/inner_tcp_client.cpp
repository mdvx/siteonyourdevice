#include "server/inner/inner_tcp_client.h"

#include "common/logger.h"

#include "server/inner/inner_tcp_server.h"
#include "server/relay_server.h"
#include "server/server_commands.h"
#include "server/server_config.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            namespace
            {
                class RelayHandlerEx
                        : public RelayHandler
                {
                    const common::buffer_type request_;
                public:
                    RelayHandlerEx(client_t client, const common::buffer_type& request)
                        : RelayHandler(), request_(request)
                    {
                        client_primary_ = client;
                        client_primary_->setName("client");
                    }

                private:
                    void closed(tcp::TcpClient* client)
                    {
                        RelayHandler::closed(client);
                    }

                    void preLooped(tcp::ITcpLoop* server)
                    {
                        server->registerClient(client_primary_);
                        RelayHandler::preLooped(server);
                    }

                    virtual void accepted(tcp::TcpClient* client)
                    {
                        if(client_primary_ != client){
                            client_secondary_ = client;
                            client_secondary_->setName("device");
                        }

                        if(!request_.empty()){
                            ssize_t nwrite = 0;
                            common::Error err = client_secondary_->write((const char*) request_.data(), request_.size(), nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                        }

                        RelayHandler::accepted(client);
                    }
                };

                class HttpRelayLoop
                        : public inner::IInnerRelayLoop
                {
                 public:
                    HttpRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, inner::InnerTcpClient *parent,
                                  tcp::TcpClient *client, const common::buffer_type& request)
                        : IInnerRelayLoop(handler, parent, client, request)
                    {

                    }

                 private:
                    virtual tcp::ITcpLoopObserver * createHandler()
                    {
                        return new RelayHandlerEx(client_, request_);
                    }

                    tcp::ITcpLoop * createServer(tcp::ITcpLoopObserver * handler)
                    {
                        tcp::TcpServer* serv = new tcp::TcpServer(g_relay_server_host, handler);
                        serv->setName("http_proxy_relay_server");

                        common::Error err = serv->bind();
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            delete serv;
                            return NULL;
                        }

                        err = serv->listen(5);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            delete serv;
                            return NULL;
                        }

                        ssize_t nwrite = 0;
                        const cmd_request_t createConnection = ihandler_->make_request(SERVER_PLEASE_CONNECT_HTTP_COMMAND_REQ_1S, common::convertToString(serv->host()));
                        err = parent_->write(createConnection, nwrite); //inner command write
                        if(err && err->isError()){;
                            DEBUG_MSG_ERROR(err);
                            delete serv;
                            return NULL;
                        }

                        return serv;
                    }
                };

                class WebSocketRelayLoop
                        : public inner::IInnerRelayLoop
                {
                    const common::net::hostAndPort srcHost_;
                public:
                    WebSocketRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, inner::InnerTcpClient *parent,
                                       tcp::TcpClient *client, const common::buffer_type& request, const common::net::hostAndPort& srcHost)
                        : IInnerRelayLoop(handler, parent, client, request), srcHost_(srcHost)
                    {

                    }

                private:
                    virtual tcp::ITcpLoopObserver * createHandler()
                    {
                        return new RelayHandlerEx(client_, request_);
                    }

                    tcp::ITcpLoop * createServer(tcp::ITcpLoopObserver * handler)
                    {
                        tcp::TcpServer* serv = new tcp::TcpServer(g_relay_server_host, handler);
                        serv->setName("websockets_proxy_relay_server");

                        common::Error err = serv->bind();
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            delete serv;
                            return NULL;
                        }

                        err = serv->listen(5);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            delete serv;
                            return NULL;
                        }

                        ssize_t nwrite = 0;
                        const cmd_request_t createConnection = ihandler_->make_request(SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND_REQ_2SS, common::convertToString(serv->host()), common::convertToString(srcHost_));
                        err = parent_->write(createConnection, nwrite); //inner command write
                        if(err && err->isError()){;
                            DEBUG_MSG_ERROR(err);
                            delete serv;
                            return NULL;
                        }

                        return serv;
                    }
                };
            }

            namespace inner
            {
                IInnerRelayLoop::IInnerRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, inner::InnerTcpClient *parent,
                                                 tcp::TcpClient *client, const common::buffer_type& request)
                    : ILoopThreadController(), parent_(parent), ihandler_(handler), client_(client), request_(request)
                {

                }

                IInnerRelayLoop::~IInnerRelayLoop()
                {
                    stop();
                    join();
                }

                InnerTcpClient::InnerTcpClient(tcp::TcpServer* server, const common::net::socket_info& info)
                    : InnerClient(server, info), hinfo_(), relays_http_(), relays_websockets_()
                {

                }

                const char* InnerTcpClient::className() const
                {
                    return "InnerTcpClient";
                }

                void InnerTcpClient::addHttpRelayClient(InnerServerHandlerHost *handler, TcpClient *client, const common::buffer_type &request)
                {
                    http_relay_loop_t tmp(new HttpRelayLoop(handler, this, client, request));
                    tmp->start();
                    relays_http_.push_back(tmp);
                }

                void InnerTcpClient::addWebsocketRelayClient(InnerServerHandlerHost* handler, TcpClient *client, const common::buffer_type& request, const common::net::hostAndPort& srcHost)
                {
                    websocket_relay_loop_t tmp(new WebSocketRelayLoop(handler, this, client, request, srcHost));
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
