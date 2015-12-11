#include "fasto_remote_gui_application.h"

namespace fasto
{
    namespace fastoremote
    {
        GuiNetworkEventHandler::GuiNetworkEventHandler(NetworkController *controller)
            : NetworkEventHandler(controller)
        {

        }

        GuiNetworkEventHandler::~GuiNetworkEventHandler()
        {

        }

        int GuiNetworkEventHandler::start()
        {
            return showImpl();
        }

        void GuiNetworkEventHandler::onConnectClicked(const configuration_t& config)
        {
            controller_->setConfig(config);
            common::Error err = controller_->connect();
            if(err && err->isError()){
                DNOTREACHED();
            }
        }

        void GuiNetworkEventHandler::onDisconnectClicked()
        {
            common::Error err = controller_->disConnect();
            if(err && err->isError()){
                DNOTREACHED();
            }
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
