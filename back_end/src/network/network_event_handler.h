#pragma once

#include "network/network_events.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class NetworkController;

        class NetworkEventHandler
        {
        public:
            NetworkEventHandler(NetworkController *controller);
            virtual ~NetworkEventHandler();

            virtual void start() ASYNC_CALL(InnerClientConnectedEvent); //connect

        protected:
            NetworkController * controller_;
            virtual void handleEvent(NetworkEvent* event);
            virtual void handleExceptionEvent(NetworkEvent* event, common::Error err);

        private:
            class NetworkListener;
            NetworkListener* networkListener_;
        };
    }
}
