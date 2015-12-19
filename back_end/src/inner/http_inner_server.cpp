#include "inner/http_inner_server.h"

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        ProxyInnerServer::ProxyInnerServer(ITcpLoopObserver* observer)
            : ITcpLoop(observer)
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
