#include "websocket/websocket_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        WebSocketClient::WebSocketClient(ITcpLoop* server, const common::net::socket_info& info)
            : Http2Client(server, info)
        {

        }

        const char* WebSocketClient::className() const
        {
            return "WebSocketClient";
        }
    }
}
