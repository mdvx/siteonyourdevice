#pragma once

#include "network_controller.h"
#include "fasto_application.h"

namespace fasto
{
    namespace siteonyourdevice
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
            NetworkController * controller_;
            NetworkEventHandler * network_handler_;
        };
    }
}
