#pragma once

#include "fasto_remote_gui_application.h"

namespace fasto
{
    namespace fastoremote
    {
        class MacOSXGuiFastoRemoteApplication
                : public FastoRemoteGuiApplication
        {
        public:
            MacOSXGuiFastoRemoteApplication(int argc, char *argv[]);
            ~MacOSXGuiFastoRemoteApplication();
            virtual int exec();
        };

        class MacMainWindow
                : public GuiNetworkEventHandler
        {
            friend class MacOSXGuiFastoRemoteApplication;
        public:
            MacMainWindow(NetworkController * controler);
            ~MacMainWindow();

            using GuiNetworkEventHandler::onConnectClicked; //with config
            using GuiNetworkEventHandler::onDisconnectClicked;

        protected:
            virtual void handleEvent(NetworkEvent* event);

        private:
            virtual int showImpl();
            class impl;
            impl* const impl_;
        };
    }
}
