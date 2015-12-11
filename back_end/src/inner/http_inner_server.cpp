#include "inner/http_inner_server.h"

#include "inner/http_inner_server_handler.h"

namespace fasto
{
    namespace fastoremote
    {
        RelayClient::RelayClient(TcpServer* server, const common::net::socket_info& info)
            : Http2Client(server, info)
        {

        }

        const char* RelayClient::className() const
        {
            return "RelayClient";
        }

        Http2InnerServer::Http2InnerServer(const common::net::hostAndPort& host, Http2InnerServerHandler * handler, const configuration_t& config)
            : Http2Server(host, handler), handler_(handler), config_(config)
        {

        }

        common::Error Http2InnerServer::innerConnect()
        {
            return handler_->innerConnect(this);
        }

        common::Error Http2InnerServer::innerDisConnect()
        {
            return handler_->innerDisConnect(this);
        }

        TcpClient * Http2InnerServer::createClient(const common::net::socket_info &info)
        {
            Http2Client *cl = new Http2Client(this, info);
            cl->setIsAuthenticated(!config_.is_private_site_);
            return cl;
        }

        RelayClient* Http2InnerServer::createRelayClient(const common::net::socket_info &info)
        {
            RelayClient *cl = new RelayClient(this, info);
            cl->setIsAuthenticated(!config_.is_private_site_);
            return cl;
        }
    }
}
