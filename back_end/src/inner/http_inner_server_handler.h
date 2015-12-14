#pragma once

#include "http/http_server_handler.h"

#include "inner/inner_server_command_seq_parser.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class Http2InnerServerHandler
                : public InnerServerCommandSeqParser, public Http2ServerHandler
        {
        public:
            Http2InnerServerHandler(const common::net::hostAndPort& host);
            ~Http2InnerServerHandler();

            virtual void preLooped(TcpServer* server);
            virtual void accepted(TcpClient* client);
            virtual void closed(TcpClient* client);
            virtual void dataReceived(TcpClient* client);
            virtual void postLooped(TcpServer* server);

            void setAuthInfo(const UserAuthInfo& authInfo);
            const UserAuthInfo& authInfo() const;

            common::Error innerConnect(TcpServer *server);
            common::Error innerDisConnect(TcpServer *server);

        private:
            virtual void handleInnerRequestCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[]);
            virtual void handleInnerResponceCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[]);
            virtual void handleInnerApproveCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[]);

            InnerClient* innerConnection_;
            UserAuthInfo authInfo_;
            const common::net::hostAndPort host_;
        };
    }
}
