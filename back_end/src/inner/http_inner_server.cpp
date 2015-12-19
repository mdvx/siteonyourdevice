#include "inner/http_inner_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        RelayClient::RelayClient(ITcpLoop *server, const common::net::socket_info& info)
            : Http2Client(server, info)
        {

        }

        const char* RelayClient::className() const
        {
            return "RelayClient";
        }

        RelayClientEx::RelayClientEx(ITcpLoop* server, const common::net::socket_info& info, const common::net::hostAndPort& externalHost)
            : RelayClient(server, info), external_host_(externalHost), eclient_(NULL)
        {

        }

        common::net::hostAndPort RelayClientEx::externalHost() const
        {
            return external_host_;
        }

        ProxyRelayClient *RelayClientEx::eclient() const
        {
            return eclient_;
        }

        void RelayClientEx::setEclient(ProxyRelayClient *client)
        {
            eclient_ = client;
        }

        const char* RelayClientEx::className() const
        {
            return "RelayClientEx";
        }

        ProxyRelayClient::ProxyRelayClient(ITcpLoop *server, const common::net::socket_info& info, RelayClientEx *relay)
            : TcpClient(server, info), relay_(relay)
        {

        }

        RelayClientEx *ProxyRelayClient::relay() const
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

        Http2InnerServer::Http2InnerServer(ITcpLoopObserver * observer, const configuration_t& config)
            : Http2Server(common::net::hostAndPort(config.domain_, config.port_), observer), config_(config)
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
