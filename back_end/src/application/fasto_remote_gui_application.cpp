#include "application/fasto_remote_gui_application.h"

#include "network/network_controller.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        GuiNetworkEventHandler::GuiNetworkEventHandler(NetworkController *controller)
            : NetworkEventHandler(controller)
        {

        }

        GuiNetworkEventHandler::~GuiNetworkEventHandler()
        {

        }

        void GuiNetworkEventHandler::start()
        {
            showImpl();
        }

        void GuiNetworkEventHandler::onConnectClicked(const HttpConfig& config)
        {
            controller_->setConfig(config);
            controller_->connect();
        }

        void GuiNetworkEventHandler::onDisconnectClicked()
        {
            controller_->disConnect();
        }

        FastoRemoteGuiApplication::FastoRemoteGuiApplication(int argc, char *argv[])
            : FastoRemoteApplication(argc, argv)
        {

        }

        FastoRemoteGuiApplication::~FastoRemoteGuiApplication()
        {

        }
    }
}
