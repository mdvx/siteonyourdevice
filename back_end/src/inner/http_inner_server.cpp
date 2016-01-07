#include "inner/http_inner_server.h"

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            ProxyInnerServer::ProxyInnerServer(tcp::ITcpLoopObserver* observer)
                : ITcpLoop(observer)
            {

            }

            const char* ProxyInnerServer::className() const
            {
                return "ProxyInnerServer";
            }

            tcp::TcpClient * ProxyInnerServer::createClient(const common::net::socket_info& info)
            {
                return new tcp::TcpClient(this, info);
            }

            Http2InnerServer::Http2InnerServer(tcp::ITcpLoopObserver * observer, const HttpConfig& config)
                : Http2Server(config.local_host, observer), config_(config)
            {

            }

            const char* Http2InnerServer::className() const
            {
                return "Http2InnerServer";
            }

            tcp::TcpClient * Http2InnerServer::createClient(const common::net::socket_info &info)
            {
                http::Http2Client *cl = new http::Http2Client(this, info);
                cl->setIsAuthenticated(!config_.is_private_site);
                return cl;
            }
        }
    }
}
