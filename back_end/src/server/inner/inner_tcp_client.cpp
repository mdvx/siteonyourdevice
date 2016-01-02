#include "server/inner/inner_tcp_client.h"

#include "common/logger.h"

#include "server/inner/inner_tcp_server.h"
#include "server/server_commands.h"
#include "server/server_config.h"

#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            namespace inner
            {
                namespace
                {
                    class RelayHandler
                            : public tcp::ITcpLoopObserver
                    {
                        tcp::ITcpLoop* server_;

                        tcp::TcpClient* client_primary_;
                        tcp::TcpClient* client_device_;

                        const IInnerRelayLoop::request_t request_;

                    public:
                        RelayHandler(const IInnerRelayLoop::request_t& req)
                            : tcp::ITcpLoopObserver(), server_(NULL), client_primary_(NULL), client_device_(NULL), request_(req)
                        {
                        }

                        bool readyForRequest() const
                        {
                            return !client_primary_ && client_device_;
                        }

                        void setClient(tcp::TcpClient* client, const common::buffer_type &request)
                        {
                            if(client_primary_){
                                NOTREACHED();
                                return;
                            }

                            client_primary_ = client;
                            client_primary_->setName("client");

                            auto cb = [this, request]()
                            {
                                server_->registerClient(client_primary_);
                                if(!client_device_ || request.empty()){
                                    return;
                                }

                                ssize_t nwrite = 0;
                                common::Error err = client_device_->write((const char*) request.data(), request.size(), nwrite);
                                if(err && err->isError()){
                                    DEBUG_MSG_ERROR(err);
                                }
                            };

                            server_->execInLoopThread(cb);
                        }

                    private:
                        void preLooped(tcp::ITcpLoop* server)
                        {
                            server_ = server;

                            setClient(request_.first, request_.second);
                        }

                        virtual void accepted(tcp::TcpClient* client)
                        {
                            if(client == client_primary_){
                                return;
                            }

                            if(!client_device_){
                                client_device_ = client;
                                client_device_->setName("device");
                            }
                        }

                        virtual void moved(tcp::TcpClient* client)
                        {

                        }

                        virtual void closed(tcp::TcpClient* client)
                        {
                            if(client == client_primary_){
                                client_primary_ = NULL;
                                return;
                            }

                            if(client == client_device_){
                                client_device_ = NULL;
                                return;
                            }
                        }

                        virtual void timerEmited(tcp::ITcpLoop* server, timer_id_type id)
                        {

                        }

                        void dataReceived(tcp::TcpClient* client)
                        {
                            char buff[BUF_SIZE] = {0};
                            ssize_t nread = 0;
                            common::Error err = client->read(buff, BUF_SIZE, nread);
                            if((err && err->isError()) || nread == 0){
                                client->close();
                                delete client;
                                return;
                            }

                            ssize_t nwrite = 0;
                            if(client == client_primary_){
                                if(client_device_){
                                    err = client_device_->write(buff, nread, nwrite);
                                    if(err){
                                        DEBUG_MSG_ERROR(err);
                                    }
                                }
                            }
                            else if(client == client_device_){
                                if(client_primary_){
                                    err = client_primary_->write(buff, nread, nwrite);
                                    if(err){
                                        DEBUG_MSG_ERROR(err);
                                        client->close();
                                        delete client;
                                    }
                                }
                            }
                            else{
                                NOTREACHED();
                            }
                        }

                        virtual void dataReadyToWrite(tcp::TcpClient* client)
                        {

                        }

                        virtual void postLooped(tcp::ITcpLoop* server)
                        {

                        }
                    };

                    class HttpRelayLoop
                            : public inner::IInnerRelayLoop
                    {
                     public:
                        HttpRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, inner::InnerTcpClient *parent, const request_t& request)
                            : IInnerRelayLoop(handler, parent, request)
                        {

                        }

                     private:
                        virtual tcp::ITcpLoopObserver * createHandler()
                        {
                            return new RelayHandler(request_);
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
                        WebSocketRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler,
                                           inner::InnerTcpClient *parent, const request_t& request, const common::net::hostAndPort& srcHost)
                            : IInnerRelayLoop(handler, parent, request), srcHost_(srcHost)
                        {

                        }

                    private:
                        virtual tcp::ITcpLoopObserver * createHandler()
                        {
                            return new RelayHandler(request_);
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

                IInnerRelayLoop::IInnerRelayLoop(fasto::siteonyourdevice::inner::InnerServerCommandSeqParser *handler, inner::InnerTcpClient *parent, const request_t &request)
                    : ILoopThreadController(), parent_(parent), ihandler_(handler), request_(request)
                {

                }

                bool IInnerRelayLoop::readyForRequest() const
                {
                    RelayHandler * hand = dynamic_cast<RelayHandler*>(handler_);
                    CHECK(hand);

                    return hand->readyForRequest();
                }

                void IInnerRelayLoop::addRequest(const request_t& request)
                {
                    RelayHandler * hand = dynamic_cast<RelayHandler*>(handler_);
                    CHECK(hand);

                    hand->setClient(request.first, request.second);
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

                void InnerTcpClient::addHttpRelayClient(InnerServerHandlerHost* handler, TcpClient *client, const common::buffer_type &request)
                {
                    for(size_t i = 0; i < relays_http_.size(); ++i){
                        http_relay_loop_t loop = relays_http_[i];
                        if(loop->readyForRequest()){
                            loop->addRequest(std::make_pair(client, request));
                            return;
                        }
                    }

                    http_relay_loop_t tmp(new HttpRelayLoop(handler, this, std::make_pair(client, request)));
                    tmp->start();

                    relays_http_.push_back(tmp);
                }

                void InnerTcpClient::addWebsocketRelayClient(InnerServerHandlerHost* handler, TcpClient *client, const common::buffer_type& request, const common::net::hostAndPort& srcHost)
                {
                    websocket_relay_loop_t tmp(new WebSocketRelayLoop(handler, this,std::make_pair(client, request), srcHost));
                    tmp->start();

                    relays_websockets_.push_back(tmp);
                }

                InnerTcpClient::~InnerTcpClient()
                {
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
