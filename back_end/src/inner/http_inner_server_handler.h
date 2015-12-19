#pragma once

#include "http/http_server_handler.h"

#include "inner/inner_server_command_seq_parser.h"

#include "http_config.h"
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

            virtual void preLooped(ITcpLoop* server);
            virtual void accepted(TcpClient* client);
            virtual void closed(TcpClient* client);
            virtual void dataReceived(TcpClient* client);
            virtual void postLooped(ITcpLoop* server);

            UserAuthInfo authInfo() const;

            common::Error innerConnect(ITcpLoop *server);
            common::Error innerDisConnect(ITcpLoop *server);

            void setConfig(const configuration_t& config);

        private:
            virtual void handleInnerRequestCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[]);
            virtual void handleInnerResponceCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[]);
            virtual void handleInnerApproveCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[]);

            InnerClient* innerConnection_;

            const common::net::hostAndPort host_;
            configuration_t config_;
        };
    }
}
