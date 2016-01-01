#pragma once

#include "application/fasto_application.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace network
        {
            class NetworkEventHandler;
            class NetworkController;
        }

        namespace application
        {
            class FastoRemoteApplication
                    : public IFastoApplicationImpl
            {
            public:
                FastoRemoteApplication(int argc, char *argv[]);
                ~FastoRemoteApplication();

                virtual int exec();
                virtual void exit(int result);

            protected:
                virtual int preExec();
                virtual int postExec();

            protected:
                network::NetworkController * controller_;
                network::NetworkEventHandler * network_handler_;
            };
        }
    }
}
