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

            virtual int start(); //connect

        protected:
            NetworkController * controller_;
            virtual void handleEvent(NetworkEvent* event);

        private:
            class NetworkListener;
            NetworkListener* networkListener_;
        };
    }
}
