#include "platform/macosx/gui_fasto_application.h"

#include "AppDelegate.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class MacMainWindow::impl
        {
            AppDelegate* delegate_;
        public:
            impl()
                : delegate_(NULL)
            {

            }

            void setDelegate(AppDelegate* delegate)
            {
                delegate_ = delegate;
            }

            ~impl()
            {
            }

            AppDelegate* delegate() const
            {
                return delegate_;
            }
        };

        MacMainWindow::MacMainWindow(NetworkController *controler)
            : GuiNetworkEventHandler(controler), impl_(new impl)
        {

        }

        MacMainWindow::~MacMainWindow()
        {
            delete impl_;
        }

        void MacMainWindow::onExit()
        {
            fApp->exit(EXIT_SUCCESS);
        }

        int MacMainWindow::showImpl()
        {
            return EXIT_SUCCESS;
        }

        void MacMainWindow::handleEvent(NetworkEvent* event)
        {
            [impl_->delegate() handleNetworkEvent: event];
            GuiNetworkEventHandler::handleEvent(event);
        }

        MacOSXGuiFastoRemoteApplication::MacOSXGuiFastoRemoteApplication(int argc, char *argv[])
            : FastoRemoteGuiApplication(argc, argv)
        {

        }

        MacOSXGuiFastoRemoteApplication::~MacOSXGuiFastoRemoteApplication()
        {

        }

        void MacOSXGuiFastoRemoteApplication::exit(int result)
        {
            FastoRemoteGuiApplication::exit(result);
            FastoRemoteGuiApplication::exec();
            FastoRemoteGuiApplication::postExec();
        }

        int MacOSXGuiFastoRemoteApplication::exec()
        {
            // make sure the application singleton has been instantiated
            NSApplication * application = [NSApplication sharedApplication];

            MacMainWindow * macw = dynamic_cast<MacMainWindow *>(network_handler_);
            CHECK(macw);

            // instantiate our application delegate
            id applicationDelegate = [[AppDelegate alloc] initWithCxxWindow: macw cxxController : controller_];
            macw->impl_->setDelegate(applicationDelegate);

            // assign our delegate to the NSApplication
            [application setDelegate:applicationDelegate];

            // call the run method of our application
            [application run];

            FastoRemoteGuiApplication::exit(EXIT_SUCCESS);
            return FastoRemoteGuiApplication::exec();
        }
    }
}
