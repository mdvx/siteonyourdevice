#pragma once

#include "application/fasto_remote_gui_application.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace application
        {
            class MacOSXGuiFastoRemoteApplication
                    : public FastoRemoteGuiApplication
            {
            public:
                MacOSXGuiFastoRemoteApplication(int argc, char *argv[]);
                ~MacOSXGuiFastoRemoteApplication();

                virtual int exec();
                virtual void exit(int result);
            };

            class MacMainWindow
                    : public GuiNetworkEventHandler
            {
                friend class MacOSXGuiFastoRemoteApplication;
            public:
                MacMainWindow(network::NetworkController * controler);
                ~MacMainWindow();

                using GuiNetworkEventHandler::onConnectClicked; //with config
                using GuiNetworkEventHandler::onDisconnectClicked;
                void onExit();

            protected:
                virtual void handleEvent(network::NetworkEvent* event);

            private:
                virtual void showImpl();
                class impl;
                impl* const impl_;
            };
        }
    }
}
