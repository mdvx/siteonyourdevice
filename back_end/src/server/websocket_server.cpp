#include "server/websocket_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        WebSocketServerHost::WebSocketServerHost(const common::net::hostAndPort& host, ITcpLoopObserver *observer)
            : Http2Server(host, observer)
        {

        }

        const char* WebSocketServerHost::className() const
        {
            return "WebSocketServerHost";
        }

        TcpClient * WebSocketServerHost::createClient(const common::net::socket_info &info)
        {
            return new WebSocketClientHost(this, info);
        }

        WebSocketClientHost::WebSocketClientHost(ITcpLoop* server, const common::net::socket_info& info)
            : Http2Client(server, info)
        {

        }
    }
}
