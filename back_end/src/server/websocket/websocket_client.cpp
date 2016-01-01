#include "server/websocket/websocket_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            WebSocketClientHost::WebSocketClientHost(tcp::ITcpLoop* server, const common::net::socket_info& info)
                : Http2Client(server, info)
            {

            }

            const char* WebSocketClientHost::className() const
            {
                return "WebSocketClientHost";
            }
        }
    }
}
