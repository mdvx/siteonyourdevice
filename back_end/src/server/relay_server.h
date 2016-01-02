#pragma once

#include "commands/commands.h"

#include "tcp/tcp_server.h"

#include "loop_controller.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            class RelayHandler
                    : public tcp::ITcpLoopObserver
            {
            public:
                typedef tcp::TcpClient* client_t;

                RelayHandler();

                virtual void preLooped(tcp::ITcpLoop* server);
                virtual void accepted(tcp::TcpClient* client);
                virtual void moved(tcp::TcpClient* client);
                virtual void closed(tcp::TcpClient* client);
                virtual void timerEmited(tcp::ITcpLoop* server, timer_id_type id);
                virtual void dataReceived(tcp::TcpClient* client);
                virtual void dataReadyToWrite(tcp::TcpClient* client);
                virtual void postLooped(tcp::ITcpLoop* server);

            protected:                
                client_t client_primary_;
                client_t client_secondary_;
            };
        }
    }
}
