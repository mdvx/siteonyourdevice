#pragma once

#include "application/fasto_application.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class NetworkEventHandler;
        class NetworkController;

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
            NetworkController * controller_;
            NetworkEventHandler * network_handler_;
        };
    }
}
