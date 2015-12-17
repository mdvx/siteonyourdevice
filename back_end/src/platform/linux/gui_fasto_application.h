#pragma once

#include <gtk/gtkstyle.h>

#include "application/fasto_remote_gui_application.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class GtkMainWindow
                : public GuiNetworkEventHandler
        {
        public:
            GtkMainWindow(NetworkController* controller);
            ~GtkMainWindow();

        protected:
            void handleEvent(NetworkEvent* event);
            void onConnectClicked();

        private:
            virtual int showImpl();

            GtkWidget * window_;
            GtkWidget * connect_button_;
            GtkWidget * domain_text_;
            GtkWidget * port_text_;
            GtkWidget * login_text_;
            GtkWidget * password_text_;
            GtkWidget * content_path_text_;
            GtkWidget * is_private_site_;

            class EventHandler;
            EventHandler* evhandler_;

        private:
            uint32_t connect_signal_id;
        };

        class GtkGuiFastoRemoteApplication
                : public FastoRemoteGuiApplication
        {
        public:
            GtkGuiFastoRemoteApplication(int argc, char *argv[]);
            ~GtkGuiFastoRemoteApplication();
            virtual int exec();
        };
    }
}
