#include "inner/inner_relay_client.h"

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
    }
}
