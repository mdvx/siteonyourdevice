#include "server/http_server_host.h"

#include <unistd.h>

#include "common/thread/thread_manager.h"
#include "common/logger.h"

#include "inner/inner_tcp_client.h"

#include "http/http_client.h"

#include "loop_controller.h"

#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    {
        HttpInnerServerHandlerHost::HttpInnerServerHandlerHost(const HttpServerInfo &info, HttpServerHost *parent)
            : Http2ServerHandler(info, NULL), parent_(parent)
        {

        }

        HttpInnerServerHandlerHost::~HttpInnerServerHandlerHost()
        {

        }

        void HttpInnerServerHandlerHost::accepted(TcpClient* client)
        {
        }

        void HttpInnerServerHandlerHost::closed(TcpClient* client)
        {
        }

        void HttpInnerServerHandlerHost::dataReceived(TcpClient* client)
        {
            char buff[BUF_SIZE] = {0};
            ssize_t nread = 0;
            common::Error err = client->read(buff, BUF_SIZE, nread);
            if((err && err->isError()) || nread == 0){
                /*if(nread != 0){
                    hclient->send_error(HS_FORBIDDEN, NULL, "Not allowed.", false);
                }*/
                client->close();
                delete client;
                return;
            }

            client_t* hclient = dynamic_cast<client_t*>(client);
            CHECK(hclient);
            using namespace common;

            std::string request(buff, nread);
            http::http_request hrequest;
            std::pair<http::http_status, common::Error> result = parse_http_request(request, hrequest);

            if(result.second && result.second->isError()){
                const std::string error_text = result.second->description();
                hclient->send_error(http::HP_1_1, result.first, NULL, error_text.c_str(), false, info());
                hclient->close();
                delete hclient;
                return;
            }

            const http::http_protocols protocol = hrequest.protocol();
            common::uri::Upath path = hrequest.path_;
            std::string hpath = path.hpath();
            std::string fpath = path.fpath();           

            InnerTcpClient * innerConnection = parent_->findInnerConnectionByHost(hpath);
            if(!innerConnection){
                common::http::http_request::header_t refererField = hrequest.findHeaderByKey("Referer", false);
                if(refererField.isValid()){
                    common::uri::Uri refpath(refererField.value_);
                    common::uri::Upath rpath = refpath.path();
                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "hpath: %s, fpath %s", hpath, fpath);
                    hpath = rpath.path();
                    fpath = path.path();
                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "after hpath: %s, fpath %s", hpath, fpath);
                }

                innerConnection = parent_->findInnerConnectionByHost(hpath);
            }

            if(!innerConnection){
                DEBUG_MSG_FORMAT<1024>(common::logging::L_WARNING, "not found host %s, request str:\n%s", hpath, convertToString(hrequest));
                hclient->send_error(protocol, http::HS_NOT_FOUND, NULL, "Not registered host.", false, info());
                hclient->close();
                delete hclient;
                return;
            }

            hclient->setName(hpath);
            http::http_request chrequest = hrequest;
            chrequest.path_.setPath(fpath);

            common::buffer_type res = common::convertToBytes(chrequest);

            innerConnection->addHttpRelayClient(parent_->innerHandler(), hclient, res);
            ITcpLoop *server = hclient->server();
            server->unregisterClient(hclient);
        }

        namespace
        {
            int exec_http_server(Http2Server * server)
            {
                common::Error err = server->bind();
                if(err && err->isError()){
                    return EXIT_FAILURE;
                }

                err = server->listen(5);
                if(err && err->isError()){
                    return EXIT_FAILURE;
                }

                return server->exec();
            }

            int exec_inner_server(InnerTcpServer * server)
            {
                common::Error err = server->bind();
                if(err && err->isError()){
                    return EXIT_FAILURE;
                }

                err = server->listen(5);
                if(err && err->isError()){
                    return EXIT_FAILURE;
                }

                return server->exec();
            }

            int exec_websocket_server(WebSocketServerHost * server)
            {
                common::Error err = server->bind();
                if(err && err->isError()){
                    return EXIT_FAILURE;
                }

                err = server->listen(5);
                if(err && err->isError()){
                    return EXIT_FAILURE;
                }

                return server->exec();
            }
        }

        HttpServerHost::HttpServerHost(const common::net::hostAndPort& httpHost,
                                       const common::net::hostAndPort &innerHost,
                                       const common::net::hostAndPort &webSocketHost)
            : httpHandler_(NULL), httpServer_(NULL), http_thread_(),
              innerHandler_(NULL), innerServer_(NULL), inner_thread_(),
              websocketHandler_(NULL), websocketServer_(NULL), websocket_thread_(),
              connections_(), rstorage_()
        {
            HttpServerInfo hinf(PROJECT_NAME_SERVER_TITLE, PROJECT_DOMAIN);

            httpHandler_ = new HttpInnerServerHandlerHost(hinf, this);
            httpServer_ = new Http2Server(httpHost, httpHandler_);
            httpServer_->setName("proxy_http_server");
            http_thread_ = THREAD_MANAGER()->createThread(&exec_http_server, httpServer_);

            innerHandler_ = new InnerServerHandlerHost(this);
            innerServer_ = new InnerTcpServer(innerHost, innerHandler_);
            innerServer_->setName("inner_server");
            inner_thread_ = THREAD_MANAGER()->createThread(&exec_inner_server, innerServer_);

            websocketHandler_ = new WebSocketServerHandlerHost(hinf, this);
            websocketServer_ = new WebSocketServerHost(webSocketHost, websocketHandler_);
            websocketServer_->setName("websocket_server");
            websocket_thread_ = THREAD_MANAGER()->createThread(&exec_websocket_server, websocketServer_);
        }

        InnerServerHandlerHost * HttpServerHost::innerHandler() const
        {
            return innerHandler_;
        }

        bool HttpServerHost::unRegisterInnerConnectionByHost(TcpClient* connection)
        {
            InnerTcpClient * iconnection = dynamic_cast<InnerTcpClient *>(connection);
            if(!iconnection){
                return false;
            }

            UserAuthInfo hinf = iconnection->serverHostInfo();
            if(!hinf.isValid()){
                return false;
            }

            const std::string host = hinf.host_.host_;
            connections_.erase(host);
            return true;
        }

        bool HttpServerHost::registerInnerConnectionByUser(const UserAuthInfo& user, TcpClient* connection)
        {
            CHECK(user.isValid());
            InnerTcpClient * iconnection = dynamic_cast<InnerTcpClient *>(connection);
            if(!iconnection){
                return false;
            }

            iconnection->setServerHostInfo(user);

            const std::string host = user.host_.host_;
            connections_[host] = iconnection;
            return true;
        }

        bool HttpServerHost::findUser(const UserAuthInfo& user) const
        {
            return rstorage_.findUser(user);
        }

        InnerTcpClient * const HttpServerHost::findInnerConnectionByHost(const std::string& host) const
        {
            inner_connections_type::const_iterator hs = connections_.find(host);
            if(hs == connections_.end()){
                return NULL;
            }

            return (*hs).second;
        }

        void HttpServerHost::setStorageConfig(const redis_sub_configuration_t &config)
        {
            rstorage_.setConfig(config);
            innerHandler_->setStorageConfig(config);
        }

        HttpServerHost::~HttpServerHost()
        {
            delete httpServer_;
            delete innerServer_;
            delete websocketServer_;

            delete websocketHandler_;
            delete innerHandler_;
            delete httpHandler_;
        }

        int HttpServerHost::exec()
        {
            http_thread_->start();
            inner_thread_->start();
            websocket_thread_->start();

            int res = http_thread_->joinAndGet();
            res |= inner_thread_->joinAndGet();
            res |= websocket_thread_->joinAndGet();
            return res;
        }

        void HttpServerHost::stop()
        {
            httpServer_->stop();
            innerServer_->stop();
            websocketServer_->stop();
        }
    }
}
