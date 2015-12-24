#include "server/http_server_host.h"

#include <unistd.h>

#include "common/thread/thread_manager.h"
#include "common/logger.h"

#include "http/http_client.h"

#include "loop_controller.h"

#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    {
        HttpInnerServerHandlerHost::HttpInnerServerHandlerHost(const HttpServerInfo &info, HttpServerHandlerHost *parent)
            : Http2ServerHandler(info, NULL), parent_(parent)
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

            innerConnection->addClient(parent_->innerHandler(), hclient, res);
            ITcpLoop *server = hclient->server();
            server->unregisterClient(hclient);
        }

        HttpInnerServerHandlerHost::~HttpInnerServerHandlerHost()
        {

        }

        HttpServerHandlerHost::HttpServerHandlerHost(const HttpServerInfo &info)
            : connections_()
        {
            innerHandler_ = new InnerServerHandlerHost(this);
            httpHandler_ = new HttpInnerServerHandlerHost(info, this);
        }

        void HttpServerHandlerHost::setStorageConfig(const redis_sub_configuration_t& config)
        {
            rstorage_.setConfig(config);
            innerHandler_->setStorageConfig(config);
        }

        bool HttpServerHandlerHost::unRegisterInnerConnectionByHost(TcpClient* connection)
        {
            InnerTcpClient * iconnection = dynamic_cast<InnerTcpClient *>(connection);
            if(!iconnection){
                return false;
            }

            HostInfo hinf = iconnection->serverHostInfo();
            if(!hinf.isValid()){
                return false;
            }

            const std::string host = hinf.host_.host_;
            connections_.erase(host);
            return true;
        }

        bool HttpServerHandlerHost::registerInnerConnectionByUser(const UserAuthInfo& user, TcpClient* connection)
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

        bool HttpServerHandlerHost::findUser(const UserAuthInfo& user) const
        {
            return rstorage_.findUser(user);
        }

        InnerTcpClient * const HttpServerHandlerHost::findInnerConnectionByHost(const std::string& host) const
        {
            inner_connections_type::const_iterator hs = connections_.find(host);
            if(hs == connections_.end()){
                return NULL;
            }

            return (*hs).second;
        }

        InnerServerHandlerHost* HttpServerHandlerHost::innerHandler() const
        {
            return innerHandler_;
        }

        HttpInnerServerHandlerHost* HttpServerHandlerHost::httpHandler() const
        {
            return httpHandler_;
        }

        HttpServerHandlerHost::~HttpServerHandlerHost()
        {
            delete innerHandler_;
            delete httpHandler_;
        }

        namespace
        {
            class HttpServerController
                    : public ILoopThreadController
            {
                const common::net::hostAndPort host_;
                HttpInnerServerHandlerHost * handler_;

            public:
                HttpServerController(const common::net::hostAndPort& httpHost, HttpInnerServerHandlerHost * handler)
                    : host_(httpHost), handler_(handler)
                {

                }

            private:
                virtual ITcpLoopObserver * createHandler()
                {
                    return handler_;
                }

                virtual ITcpLoop * createServer(ITcpLoopObserver * handler)
                {
                    Http2Server * server = new Http2Server(host_, handler);
                    server->setName("proxy_http_server");

                    common::Error err = server->bind();
                    if(err && err->isError()){
                        delete server;
                        return NULL;
                    }

                    err = server->listen(5);
                    if(err && err->isError()){
                        delete server;
                        return NULL;
                    }

                    return server;
                }
            };

            class InnerServerController
                    : public ILoopThreadController
            {
                const common::net::hostAndPort host_;
                InnerServerHandlerHost * handler_;

            public:
                InnerServerController(const common::net::hostAndPort& innerHost, InnerServerHandlerHost * handler)
                    : host_(innerHost), handler_(handler)
                {

                }

            private:
                virtual ITcpLoopObserver * createHandler()
                {
                    return handler_;
                }

                virtual ITcpLoop * createServer(ITcpLoopObserver * handler)
                {
                    InnerTcpServer * server = new InnerTcpServer(host_, handler);
                    server->setName("inner_server");

                    common::Error err = server->bind();
                    if(err && err->isError()){
                        delete server;
                        return NULL;
                    }

                    err = server->listen(5);
                    if(err && err->isError()){
                        delete server;
                        return NULL;
                    }

                    return server;
                }
            };
        }

        HttpServerHost::HttpServerHost(const common::net::hostAndPort& httpHost, const common::net::hostAndPort &innerHost, HttpServerHandlerHost * handler)
            : httpServer_(NULL), innerServer_(NULL)
        {
            httpServer_ = new HttpServerController(httpHost, handler->httpHandler());
            innerServer_ = new InnerServerController(innerHost, handler->innerHandler());
        }

        HttpServerHost::~HttpServerHost()
        {
            delete httpServer_;
            delete innerServer_;
        }

        int HttpServerHost::exec()
        {
            httpServer_->start();
            innerServer_->start();

            int res = httpServer_->join();
            res |= innerServer_->join();
            return res;
        }

        void HttpServerHost::stop()
        {
            httpServer_->stop();
            innerServer_->stop();
        }
    }
}
