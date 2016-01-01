#include "websocket/websocket_server.h"

#include "websocket/websocket_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace websocket
        {
            WebSocketServer::WebSocketServer(const common::net::hostAndPort& host, tcp::ITcpLoopObserver* observer)
                : TcpServer(host, observer)
            {

            }

            const char* WebSocketServer::className() const
            {
                return "WebSocketServer";
            }

            tcp::TcpClient * WebSocketServer::createClient(const common::net::socket_info& info)
            {
                return new WebSocketClient(this, info);
            }
        }
    }
}
