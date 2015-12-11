#include "server/http_server_host.h"

#include <unistd.h>

#include "common/string_util.h"
#include "common/sprintf.h"
#include "common/file_system.h"
#include "common/thread/thread_manager.h"
#include "common/net/net.h"
#include "common/logger.h"

#define BUF_SIZE 4096

namespace fasto
{
    namespace fastoremote
    {
        HttpInnerServerHandlerHost::HttpInnerServerHandlerHost(HttpServerHandlerHost *parent)
            : Http2ServerHandler(NULL), parent_(parent)
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
                hclient->send_error(http::HP_1_1, result.first, NULL, error_text.c_str(), false);
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
                hclient->send_error(protocol, http::HS_NOT_FOUND, NULL, "Not registered host.", false);
                hclient->close();
                delete hclient;
                return;
            }

            hclient->setName(hpath);
            http::http_request chrequest = hrequest;
            chrequest.path_.setPath(fpath);

            common::buffer_type res = common::convertToBytes(chrequest);

            innerConnection->addClient(parent_->innerHandler(), hclient, res);
            TcpServer *server = hclient->server();
            server->unregisterClient(hclient);
        }

        HttpInnerServerHandlerHost::~HttpInnerServerHandlerHost()
        {

        }

        HttpServerHandlerHost::HttpServerHandlerHost()
            : connections_()
        {
            innerHandler_ = new InnerServerHandlerHost(this);
            httpHandler_ = new HttpInnerServerHandlerHost(this);
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

        HttpServerHost::HttpServerHost(const common::net::hostAndPort& httpHost, const common::net::hostAndPort &innerHost,
                                       const HttpServerInfo &info, HttpServerHandlerHost * handler)
            : httpServer_(NULL), innerServer_(NULL)
        {
            httpServer_ = new Http2Server(httpHost, handler->httpHandler());
            httpServer_->setHttpServerInfo(info);

            const std::string contentPath = info.contentPath_;
            bool res = common::file_system::change_directory(contentPath);
            DCHECK(res);

            httpThread_ = THREAD_MANAGER()->createThread(&Http2Server::exec, httpServer_);
            httpServer_->setName("proxy_http_server");

            innerServer_ = new InnerTcpServer(innerHost, handler->innerHandler());
            innerThread_ = THREAD_MANAGER()->createThread(&InnerTcpServer::exec, innerServer_);
            innerServer_->setName("inner_server");
        }

        HttpServerHost::~HttpServerHost()
        {
            delete httpServer_;
            delete innerServer_;
        }

        common::Error HttpServerHost::bind()
        {
            common::Error err = httpServer_->bind();
            if(err && err->isError()){
                return err;
            }

            return innerServer_->bind();
        }

        common::Error HttpServerHost::listen(int backlog)
        {
            common::Error err = httpServer_->listen(backlog);
            if(err && err->isError()){
                return err;
            }

            return innerServer_->listen(backlog);
        }

        int HttpServerHost::exec()
        {
            httpThread_->start();
            innerThread_->start();

            int res = httpThread_->joinAndGet();
            res = innerThread_->joinAndGet();
            return res;
        }

        void HttpServerHost::stop()
        {
            httpServer_->stop();
            innerServer_->stop();
        }
    }
}
