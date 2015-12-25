#include "server/websocket_server.h"

#include "common/logger.h"

#include "inner/inner_tcp_client.h"

#include "server/http_server_host.h"

#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    {
        WebSocketServerHost::WebSocketServerHost(const common::net::hostAndPort& host, ITcpLoopObserver *observer)
            : Http2Server(host, observer)
        {

        }

        const char* WebSocketServerHost::className() const
        {
            return "WebSocketServerHost";
        }

        TcpClient * WebSocketServerHost::createClient(const common::net::socket_info &info)
        {
            return new WebSocketClientHost(this, info);
        }

        WebSocketClientHost::WebSocketClientHost(ITcpLoop* server, const common::net::socket_info& info)
            : Http2Client(server, info)
        {

        }

        WebSocketServerHandlerHost::WebSocketServerHandlerHost(const HttpServerInfo &info, HttpServerHost * parent)
            : Http2ServerHandler(info, NULL), parent_(parent)
        {

        }

        void WebSocketServerHandlerHost::dataReceived(TcpClient* client)
        {
            char buff[BUF_SIZE] = {0};
            ssize_t nread = 0;
            common::Error err = client->read(buff, BUF_SIZE, nread);
            if((err && err->isError()) || nread == 0){
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

            std::string hpath_without_port = common::convertFromString<common::net::hostAndPort>(hpath).host_;

            InnerTcpClient * innerConnection = parent_->findInnerConnectionByHost(hpath_without_port);
            if(!innerConnection){
                common::http::http_request::header_t refererField = hrequest.findHeaderByKey("Referer", false);
                if(refererField.isValid()){
                    common::uri::Uri refpath(refererField.value_);
                    common::uri::Upath rpath = refpath.path();
                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "hpath: %s, fpath %s", hpath, fpath);
                    hpath = common::convertFromString<common::net::hostAndPort>(rpath.path()).host_;
                    fpath = path.path();
                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "after hpath: %s, fpath %s", hpath, fpath);
                }

                innerConnection = parent_->findInnerConnectionByHost(hpath);
            }

            if(!innerConnection){
                DEBUG_MSG_FORMAT<1024>(common::logging::L_WARNING, "WebSocketServerHandlerHost not found host %s, request str:\n%s", hpath, convertToString(hrequest));
                hclient->send_error(protocol, http::HS_NOT_FOUND, NULL, "Not registered host.", false, info());
                hclient->close();
                delete hclient;
                return;
            }

            hclient->setName(hpath);
            http::http_request chrequest = hrequest;
            chrequest.path_.setPath(fpath);

            common::buffer_type res = common::convertToBytes(chrequest);

            innerConnection->addWebsocketRelayClient(parent_->innerHandler(), hclient, res);
            ITcpLoop *server = hclient->server();
            server->unregisterClient(hclient);
        }
    }
}
