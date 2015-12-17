#include "inner/http_inner_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        RelayClient::RelayClient(TcpServer* server, const common::net::socket_info& info)
            : Http2Client(server, info)
        {

        }

        const char* RelayClient::className() const
        {
            return "RelayClient";
        }

        Http2InnerServer::Http2InnerServer(const common::net::hostAndPort& host, TcpServerObserver * observer, const configuration_t& config)
            : Http2Server(host, observer), config_(config)
        {

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
