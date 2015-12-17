#include "inner/http_inner_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        RelayClient::RelayClient(TcpServer* server, const common::net::socket_info& info, const common::net::hostAndPort& externalHost)
            : Http2Client(server, info), external_host_(externalHost), eclient_(NULL)
        {

        }

        common::net::hostAndPort RelayClient::externalHost() const
        {
            return external_host_;
        }

        ProxyRelayClient *RelayClient::eclient() const
        {
            return eclient_;
        }

        void RelayClient::setEclient(ProxyRelayClient *client)
        {
            eclient_ = client;
        }

        const char* RelayClient::className() const
        {
            return "RelayClient";
        }

        ProxyRelayClient::ProxyRelayClient(TcpServer* server, const common::net::socket_info& info, RelayClient * relay)
            : TcpClient(server, info), relay_(relay)
        {

        }

        RelayClient * ProxyRelayClient::relay() const
        {
            return relay_;
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
            RelayClient *cl = new RelayClient(this, info, config_.external_host_);
            cl->setIsAuthenticated(!config_.is_private_site_);
            return cl;
        }
    }
}
