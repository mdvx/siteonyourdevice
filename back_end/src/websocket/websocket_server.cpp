#include "websocket/websocket_server.h"

#include "websocket/websocket_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        WebSocketServer::WebSocketServer(const common::net::hostAndPort& host, ITcpLoopObserver* observer)
            : TcpServer(host, observer)
        {

        }

        const char* WebSocketServer::className() const
        {
            return "WebSocketServer";
        }

        TcpClient * WebSocketServer::createClient(const common::net::socket_info& info)
        {
            return new WebSocketClient(this, info);
        }
    }
}
