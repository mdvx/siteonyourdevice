#pragma once

#include "tcp/tcp_client.h"

#include "commands/commands.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            class InnerClient
                    : public tcp::TcpClient
            {
            public:
                InnerClient(tcp::ITcpLoop *server, const common::net::socket_info& info);
                const char* className() const;

                common::Error write(const cmd_request_t& request, ssize_t* nwrite) WARN_UNUSED_RESULT;
                common::Error write(const cmd_responce_t& responce, ssize_t* nwrite) WARN_UNUSED_RESULT;
                common::Error write(const cmd_approve_t& approve, ssize_t* nwrite) WARN_UNUSED_RESULT;

            private:
                using tcp::TcpClient::write;
            };
        }
    }
}
