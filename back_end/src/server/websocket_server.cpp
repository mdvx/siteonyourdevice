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

            std::string request(buff, nread);
            common::http::http_request hrequest;
            std::pair<common::http::http_status, common::Error> result = parse_http_request(request, hrequest);

            if(result.second && result.second->isError()){
                const std::string error_text = result.second->description();
                hclient->send_error(common::http::HP_1_1, result.first, NULL, error_text.c_str(), false, info());
                hclient->close();
                delete hclient;
                return;
            }

            processWebsocketRequest(hclient, hrequest);
        }

        void WebSocketServerHandlerHost::processWebsocketRequest(HttpClient *hclient, const common::http::http_request& hrequest)
        {
            const common::http::http_protocols protocol = hrequest.protocol();
            common::uri::Upath path = hrequest.path_;
            std::string hpath = path.hpath();
            std::string fpath = path.fpath();
            const common::net::hostAndPort host = common::convertFromString<common::net::hostAndPort>(hpath);
            std::string hpath_without_port = host.host_;

            InnerTcpClient * innerConnection = parent_->findInnerConnectionByHost(hpath_without_port);
            if(!innerConnection){
                DEBUG_MSG_FORMAT<1024>(common::logging::L_WARNING, "WebSocketServerHandlerHost not found host %s, request str:\n%s",
                                       hpath, common::convertToString(hrequest));
                hclient->send_error(protocol, common::http::HS_NOT_FOUND, NULL, "Not registered host.", false, info());
                hclient->close();
                delete hclient;
                return;
            }

            hclient->setName(common::convertToString(host));
            common::http::http_request chrequest = hrequest;
            chrequest.path_.setPath(fpath);

            common::buffer_type res = common::convertToBytes(chrequest);

            innerConnection->addWebsocketRelayClient(parent_->innerHandler(), hclient, res, common::net::hostAndPort("localhost", host.port_));
            ITcpLoop *server = hclient->server();
            server->unregisterClient(hclient);
        }
    }
}
