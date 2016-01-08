#include "inner/inner_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            InnerClient::InnerClient(tcp::ITcpLoop *server, const common::net::socket_info& info)
                : TcpClient(server, info)
            {

            }

            const char* InnerClient::className() const
            {
                return "InnerClient";
            }

            common::Error InnerClient::write(const cmd_request_t& request, ssize_t* nwrite)
            {
                return TcpClient::write(request.data(), request.size(), nwrite);
            }

            common::Error InnerClient::write(const cmd_responce_t& responce, ssize_t* nwrite)
            {
                return TcpClient::write(responce.data(), responce.size(), nwrite);
            }

            common::Error InnerClient::write(const cmd_approve_t& approve, ssize_t* nwrite)
            {
                return TcpClient::write(approve.data(), approve.size(), nwrite);
            }
        }
    }
}
