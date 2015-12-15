#include "platform/linux/gui_fasto_application.h"

#include <stdlib.h>

#include <gtk/gtk.h>

namespace
{
    void trayOnAbout(GtkMenuItem *item, gpointer user_data)
    {
    }

    void trayOnExit(GtkMenuItem *item, gpointer user_data)
    {
        gtk_main_quit();
    }

    void trayIconOnClick(GtkStatusIcon *trayIcon, gpointer window)
    {
        gtk_widget_show(GTK_WIDGET(window));
        gtk_window_deiconify(GTK_WINDOW(window));
    }

    void trayIconOnMenu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
    {
        gtk_menu_popup(GTK_MENU(user_data), NULL, NULL, gtk_status_icon_position_menu, status_icon, button, activate_time);
    }


    void destroy(GtkWidget *window, gpointer user_data)
    {
        gtk_main_quit();
    }

    gboolean delete_event (GtkWidget *window, GdkEvent *event, gpointer data)
    {
        return FALSE;
    }

    gboolean window_state_event (GtkWidget *widget, GdkEventWindowState *event, gpointer trayIcon)
    {
        if(event->changed_mask == GDK_WINDOW_STATE_ICONIFIED &&
                (event->new_window_state == GDK_WINDOW_STATE_ICONIFIED || event->new_window_state == (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED))){
            gtk_widget_hide (GTK_WIDGET(widget));
            gtk_status_icon_set_visible(GTK_STATUS_ICON(trayIcon), TRUE);
        }
        else if(event->changed_mask == GDK_WINDOW_STATE_WITHDRAWN &&
                (event->new_window_state == GDK_WINDOW_STATE_ICONIFIED || event->new_window_state == (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED))){
            gtk_status_icon_set_visible(GTK_STATUS_ICON(trayIcon), FALSE);
        }
        return TRUE;
    }
}

namespace fasto
{
    namespace siteonyourdevice
    {
        class GtkMainWindow::EventHandler
        {
        public:
            EventHandler(GtkMainWindow* parent)
                : parent_(parent)
            {

            }

            static void click_connect_callback(GtkButton *button, gpointer callback_data)
            {
                EventHandler* handler = static_cast<EventHandler*>(callback_data);
                handler->parent_->onConnectClicked();
            }

            static void click_disconnect_callback(GtkButton *button, gpointer callback_data)
            {
                EventHandler* handler = static_cast<EventHandler*>(callback_data);
                handler->parent_->onDisconnectClicked();
            }

        private:
            GtkMainWindow * parent_;
        };

        GtkMainWindow::GtkMainWindow(NetworkController *controller)
            : GuiNetworkEventHandler(controller), connect_button_(NULL),
              domain_text_(NULL), port_text_(NULL), login_text_(NULL), password_text_(NULL), content_path_text_(NULL)
        {

            evhandler_ = new EventHandler(this);

            window_ = gtk_window_new (GTK_WINDOW_TOPLEVEL);
            gtk_window_set_icon_from_file(GTK_WINDOW(window_), "/usr/share/icons/" PROJECT_NAME_LOWERCASE ".png", NULL);
            gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
            gtk_window_set_title (GTK_WINDOW(window_), PROJECT_NAME_TITLE);
            gtk_window_set_default_size(GTK_WINDOW(window_), height, width);
            gtk_container_set_border_width(GTK_CONTAINER(window_), 10);

            GtkWidget *vbox = gtk_vbox_new(TRUE, 1);

            // domain and port
            GtkWidget *dpbox = gtk_hbox_new(TRUE, 1);

            GtkWidget *dhdbox = gtk_hbox_new(TRUE, 1);
            GtkWidget *domain_label  = gtk_label_new(DOMAIN_LABEL);
            gtk_box_pack_start(GTK_BOX(dhdbox), domain_label, FALSE, FALSE, 0);
            domain_text_ = gtk_entry_new();
            gtk_entry_set_max_length(GTK_ENTRY(domain_text_), domain_max_length);
            gtk_box_pack_start(GTK_BOX(dhdbox), domain_text_, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(dpbox), dhdbox);

            GtkWidget *pdbox = gtk_hbox_new(TRUE, 1);
            GtkWidget *port_label = gtk_label_new(PORT_LABEL);
            gtk_box_pack_start(GTK_BOX(pdbox), port_label, FALSE, FALSE, 0);
            port_text_ = gtk_entry_new();
            gtk_entry_set_max_length(GTK_ENTRY(port_text_), port_max_length);
            gtk_box_pack_start(GTK_BOX(pdbox), port_text_, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(dpbox), pdbox);

            gtk_container_add(GTK_CONTAINER(vbox), dpbox);
            //

            // login and password
            GtkWidget *lpbox = gtk_hbox_new(TRUE, 1);

            GtkWidget *lhdbox = gtk_hbox_new(TRUE, 1);
            GtkWidget *login_label  = gtk_label_new(LOGIN_LABEL);
            gtk_box_pack_start(GTK_BOX(lhdbox), login_label, FALSE, FALSE, 0);
            login_text_ = gtk_entry_new();
            gtk_entry_set_max_length(GTK_ENTRY(login_text_), login_max_length);
            gtk_box_pack_start(GTK_BOX(lhdbox), login_text_, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(lpbox), lhdbox);

            GtkWidget *pasdbox = gtk_hbox_new(TRUE, 1);
            GtkWidget *password_label = gtk_label_new(PASSWORD_LABEL);
            gtk_box_pack_start(GTK_BOX(pasdbox), password_label, FALSE, FALSE, 0);
            password_text_ = gtk_entry_new();
            gtk_entry_set_visibility (GTK_ENTRY(password_text_), FALSE);
            gtk_entry_set_invisible_char(GTK_ENTRY(password_text_), 9679);
            gtk_box_pack_start(GTK_BOX(pasdbox), password_text_, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(lpbox), pasdbox);

            gtk_container_add(GTK_CONTAINER(vbox), lpbox);
            //

            // content path
            GtkWidget *cbox = gtk_hbox_new(TRUE, 1);
            GtkWidget *cpbox = gtk_hbox_new(TRUE, 1);
            GtkWidget *content_path_label  = gtk_label_new(CONTENT_PATH_LABEL);
            gtk_box_pack_start(GTK_BOX(cpbox), content_path_label, FALSE, FALSE, 0);
            content_path_text_ = gtk_entry_new();
            gtk_box_pack_start(GTK_BOX(cpbox), content_path_text_, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(cbox), cpbox);

            gtk_container_add(GTK_CONTAINER(vbox), cbox);
            //

            is_private_site_ = gtk_check_button_new_with_label(PRIVATE_SITE_LABEL);
            gtk_box_pack_start(GTK_BOX(vbox), is_private_site_, FALSE, FALSE, 0);

            GtkWidget * menu = gtk_menu_new();
            GtkWidget * menuAbout = gtk_image_menu_item_new_with_mnemonic("About");
            GtkWidget * menuExit = gtk_image_menu_item_new_with_mnemonic("Exit");
            g_signal_connect(G_OBJECT(menuAbout), "activate", G_CALLBACK(trayOnAbout), NULL);
            g_signal_connect(G_OBJECT(menuExit), "activate", G_CALLBACK(trayOnExit), NULL);
            gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuAbout);
            gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuExit);

            GtkStatusIcon *trayIcon = gtk_status_icon_new_from_file("/usr/share/icons/" PROJECT_NAME_LOWERCASE ".png");
            g_signal_connect(G_OBJECT(trayIcon), "activate", G_CALLBACK(trayIconOnClick), window_);
            g_signal_connect(G_OBJECT(trayIcon), "popup-menu", G_CALLBACK(trayIconOnMenu), menu);
            gtk_status_icon_set_tooltip(trayIcon, PROJECT_NAME_TITLE);
            gtk_status_icon_set_visible(trayIcon, FALSE); //set icon initially invisible

            gtk_widget_show_all(menu);

            connect_button_ = gtk_button_new_with_label(CONNECT_LABEL);
            gtk_box_pack_start(GTK_BOX(vbox), connect_button_, FALSE, FALSE, 0);
            connect_signal_id = g_signal_connect(G_OBJECT(connect_button_), "clicked", G_CALLBACK(EventHandler::click_connect_callback), evhandler_);

            gtk_container_add(GTK_CONTAINER(window_), vbox);

            g_signal_connect(G_OBJECT(window_), "destroy", G_CALLBACK(destroy), NULL);
            g_signal_connect(G_OBJECT(window_), "delete_event", G_CALLBACK(delete_event), trayIcon);
            g_signal_connect(G_OBJECT(window_), "window-state-event", G_CALLBACK(window_state_event), trayIcon);
        }

        GtkMainWindow::~GtkMainWindow()
        {
            delete evhandler_;
        }

        void GtkMainWindow::handleEvent(NetworkEvent* event)
        {
            if(event->eventType() == InnerClientConnectedEvent::EventType){
                InnerClientConnectedEvent * ev = static_cast<InnerClientConnectedEvent*>(event);
            }
            else if(event->eventType() == InnerClientAutorizedEvent::EventType){
                InnerClientAutorizedEvent * ev = static_cast<InnerClientAutorizedEvent*>(event);
                gtk_button_set_label(GTK_BUTTON(connect_button_), DISCONNECT_LABEL);
                g_signal_handler_disconnect(connect_button_, connect_signal_id);
                connect_signal_id = g_signal_connect(G_OBJECT(connect_button_), "clicked", G_CALLBACK(EventHandler::click_disconnect_callback), evhandler_);
            }
            else if(event->eventType() == InnerClientDisconnectedEvent::EventType){
                InnerClientDisconnectedEvent * ev = static_cast<InnerClientDisconnectedEvent*>(event);
                g_signal_handler_disconnect(connect_button_, connect_signal_id);
                connect_signal_id = g_signal_connect(G_OBJECT(connect_button_), "clicked", G_CALLBACK(EventHandler::click_connect_callback), evhandler_);
                gtk_button_set_label(GTK_BUTTON(connect_button_), CONNECT_LABEL);
            }
            else{

            }
        }

        int GtkMainWindow::showImpl()
        {
            configuration_t cur_config = controller_->config();

            gtk_entry_set_text(GTK_ENTRY(domain_text_), cur_config.domain_.c_str());
            const std::string portstr = common::convertToString(cur_config.port_);
            gtk_entry_set_text(GTK_ENTRY(port_text_), portstr.c_str());
            gtk_entry_set_text(GTK_ENTRY(login_text_), cur_config.login_.c_str());
            gtk_entry_set_text(GTK_ENTRY(password_text_), cur_config.password_.c_str());
            gtk_entry_set_text(GTK_ENTRY(content_path_text_), cur_config.content_path_.c_str());
            gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(is_private_site_), cur_config.is_private_site_);
            gtk_widget_show_all(window_);
            return EXIT_SUCCESS;
        }

        void GtkMainWindow::onConnectClicked()
        {
            configuration_t old_config = controller_->config();
            
            old_config.domain_ = gtk_entry_get_text(GTK_ENTRY(domain_text_));
            const char *portstr = gtk_entry_get_text(GTK_ENTRY(port_text_));
            old_config.port_ = common::convertFromString<uint16_t>(portstr);
            old_config.login_ = gtk_entry_get_text(GTK_ENTRY(login_text_));
            old_config.password_ = gtk_entry_get_text(GTK_ENTRY(password_text_));
            old_config.content_path_ = gtk_entry_get_text(GTK_ENTRY(content_path_text_));
            old_config.is_private_site_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(is_private_site_));
            GuiNetworkEventHandler::onConnectClicked(old_config);
        }

        GtkGuiFastoRemoteApplication::GtkGuiFastoRemoteApplication(int argc, char *argv[])
            : FastoRemoteGuiApplication(argc, argv)
        {
            gtk_init(&argc, &argv);
        }

        GtkGuiFastoRemoteApplication::~GtkGuiFastoRemoteApplication()
        {

        }

        int GtkGuiFastoRemoteApplication::exec()
        {
            gtk_main();
            FastoRemoteGuiApplication::exit(EXIT_SUCCESS);
            return FastoRemoteGuiApplication::exec();
        }
    }
}
