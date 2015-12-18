#include "inner/http_inner_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        RelayClient::RelayClient(ITcpLoop *server, const common::net::socket_info& info, const common::net::hostAndPort& externalHost)
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

        ProxyRelayClient::ProxyRelayClient(ITcpLoop *server, const common::net::socket_info& info, RelayClient * relay)
            : TcpClient(server, info), relay_(relay)
        {

        }

        RelayClient * ProxyRelayClient::relay() const
        {
            return relay_;
        }

        ProxyInnerServer::ProxyInnerServer(ITcpLoopObserver* observer, const configuration_t &config)
            : ITcpLoop(observer), config_(config)
        {

        }

        const char* ProxyInnerServer::className() const
        {
            return "ProxyInnerServer";
        }

        Http2InnerServer::Http2InnerServer(const common::net::hostAndPort& host, ITcpLoopObserver * observer, const configuration_t& config)
            : Http2Server(host, observer), config_(config)
        {

        }

        const char* Http2InnerServer::className() const
        {
            return "Http2InnerServer";
        }

        TcpClient * Http2InnerServer::createClient(const common::net::socket_info &info)
        {
            Http2Client *cl = new Http2Client(this, info);
            cl->setIsAuthenticated(!config_.is_private_site_);
            return cl;
        }
    }
}
