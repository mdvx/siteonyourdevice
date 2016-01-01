#include "http/http_server.h"

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace http
        {
            HttpServer::HttpServer(const common::net::hostAndPort& host, tcp::ITcpLoopObserver* observer)
                : TcpServer(host, observer)
            {

            }

            HttpServer::~HttpServer()
            {

            }

            tcp::TcpClient *HttpServer::createClient(const common::net::socket_info& info)
            {
                return new HttpClient(this, info);
            }

            const char* HttpServer::className() const
            {
                return "HttpServer";
            }

            Http2Server::Http2Server(const common::net::hostAndPort& host, tcp::ITcpLoopObserver *observer)
                : HttpServer(host, observer)
            {

            }

            tcp::TcpClient *Http2Server::createClient(const common::net::socket_info &info)
            {
                return new Http2Client(this, info);
            }

            const char* Http2Server::className() const
            {
                return "Http2Server";
            }
        }
    }
}
