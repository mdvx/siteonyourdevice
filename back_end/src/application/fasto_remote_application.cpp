#include "application/fasto_remote_application.h"

#include <stdlib.h>

#include "network/network_controller.h"

#if defined(BUILD_CONSOLE)
    #include "fasto_remote_application.h"
#else
    #if defined(OS_WIN)
        #include "platform/windows/gui_fasto_application.h"
    #elif defined(OS_MACOSX)
        #include "platform/macosx/gui_fasto_application.h"
    #elif defined(OS_ANDROID)
        #include "fasto_remote_application.h"
    #else
        #include "platform/linux/gui_fasto_application.h"
    #endif
#endif

namespace fasto
{
    namespace siteonyourdevice
    {
        NetworkEventHandler* createHandlerImpl(NetworkController * controler)
        {
            using namespace siteonyourdevice;
            #if defined(BUILD_CONSOLE)
                return new NetworkEventHandler(controler);
            #else
            #if defined(OS_WIN)
                return new Win32MainWindow(controler);
            #elif defined(OS_MACOSX)
                return new MacMainWindow(controler);
            #elif defined(OS_ANDROID)
                return new NetworkEventHandler(controler);
            #else
                return new GtkMainWindow(controler);
            #endif
            #endif
        }

        FastoRemoteApplication::FastoRemoteApplication(int argc, char *argv[])
            : IFastoApplicationImpl(argc, argv), controller_(NULL), network_handler_(NULL)
        {
        }

        FastoRemoteApplication::~FastoRemoteApplication()
        {

        }

        int FastoRemoteApplication::preExec()
        {
            controller_ = new NetworkController(fApp->argc(), fApp->argv());
            network_handler_ = createHandlerImpl(controller_);
            network_handler_->start();
            return EXIT_SUCCESS;
        }

        int FastoRemoteApplication::exec()
        {
            return controller_->exec();
        }

        int FastoRemoteApplication::postExec()
        {
            delete network_handler_;
            delete controller_;
            return EXIT_SUCCESS;
        }

        void FastoRemoteApplication::exit(int result)
        {
            controller_->exit(result);
        }
    }
}
