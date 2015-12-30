#pragma once

#include "http/http_server_handler.h"

#include "inner/inner_server_command_seq_parser.h"

#include "http_config.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class RelayClient;
        class RelayClientEx;
        class ProxyRelayClient;

        class Http2InnerServerHandler
                : public InnerServerCommandSeqParser, public Http2ServerHandler
        {
        public:
            enum
            {
                ping_timeout_server = 30 //sec
            };

            Http2InnerServerHandler(const HttpServerInfo &info, const common::net::hostAndPort& innerHost, const HttpConfig& config);
            ~Http2InnerServerHandler();

            virtual void preLooped(ITcpLoop* server);
            virtual void accepted(TcpClient* client);
            virtual void closed(TcpClient* client);
            virtual void dataReceived(TcpClient* client);
            virtual void dataReadyToWrite(TcpClient* client);
            virtual void postLooped(ITcpLoop* server);
            virtual void timerEmited(ITcpLoop* server, timer_id_type id);

            UserAuthInfo authInfo() const;
        private:
            common::Error innerConnect(ITcpLoop *server);
            common::Error innerDisConnect(ITcpLoop *server);

            virtual void handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
            virtual void handleInnerResponceCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
            virtual void handleInnerApproveCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);

            void innerDataReceived(InnerClient *iclient);
            void relayDataReceived(RelayClient *rclient);
            void relayExDataReceived(RelayClientEx *rclient);
            void proxyDataReceived(ProxyRelayClient * prclient);

            InnerClient* innerConnection_;
            timer_id_type ping_server_id_timer_;

            const common::net::hostAndPort innerHost_;
            const HttpConfig config_;
        };
    }
}
