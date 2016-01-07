#include "platform/linux/gui_fasto_application.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "network/network_controller.h"

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
        namespace application
        {
            class GtkMainWindow::EventHandler
            {
            public:
                EventHandler(GtkMainWindow* parent)
                    : parent_(parent)
                {

                }

                static void click_external_domain_change_callback(GtkToggleButton *button, gpointer callback_data)
                {
                    EventHandler* handler = static_cast<EventHandler*>(callback_data);
                    GtkMainWindow * parent = handler->parent_;

                    parent->setExternalChekboxState(gtk_toggle_button_get_active(button));
                }

                static void click_browse_content_callback(GtkButton *button, gpointer callback_data)
                {
                    EventHandler* handler = static_cast<EventHandler*>(callback_data);
                    GtkMainWindow * parent = handler->parent_;

                    GtkWidget * chose_content_path  = gtk_file_chooser_dialog_new("Select content folder", GTK_WINDOW(parent->window_),
                                                                                            GTK_FILE_CHOOSER_ACTION_OPEN,
                                                                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                                                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                                                            NULL);
                    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(chose_content_path), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

                    if (gtk_dialog_run (GTK_DIALOG (chose_content_path)) == GTK_RESPONSE_ACCEPT){
                        char *filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (chose_content_path));
                        gtk_entry_set_text(GTK_ENTRY(parent->content_path_text_), filepath);
                        g_free(filepath);
                    }
                    gtk_widget_destroy(chose_content_path);
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

            GtkMainWindow::GtkMainWindow(network::NetworkController *controller)
                : GuiNetworkEventHandler(controller), connect_button_(NULL),
                  domain_text_(NULL), port_text_(NULL), login_text_(NULL), password_text_(NULL), content_path_text_(NULL),
                  browse_content_button_(NULL), external_host_(NULL), external_port_(NULL)
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
                browse_content_button_ = gtk_button_new_with_label(BROWSE_LABEL);
                gtk_box_pack_start(GTK_BOX(cbox), browse_content_button_, FALSE, FALSE, 0);
                g_signal_connect(G_OBJECT(browse_content_button_), "clicked", G_CALLBACK(EventHandler::click_browse_content_callback), evhandler_);

                gtk_container_add(GTK_CONTAINER(vbox), cbox);
                //

                GtkWidget *exbox = gtk_hbox_new(TRUE, 1);
                is_external_domain_ = gtk_check_button_new_with_label(EXTERNAL_SITE_LABEL);
                gtk_box_pack_start(GTK_BOX(exbox), is_external_domain_, FALSE, FALSE, 0);
                g_signal_connect(G_OBJECT(is_external_domain_), "clicked",  G_CALLBACK(EventHandler::click_external_domain_change_callback), evhandler_);

                external_host_ = gtk_entry_new();
                gtk_box_pack_start(GTK_BOX(exbox), external_host_, FALSE, FALSE, 0);
                external_port_ = gtk_entry_new();
                gtk_box_pack_start(GTK_BOX(exbox), external_port_, FALSE, FALSE, 0);
                gtk_entry_set_max_length(GTK_ENTRY(external_port_), port_max_length);
                gtk_container_add(GTK_CONTAINER(vbox), exbox);

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

            void GtkMainWindow::handleEvent(network::NetworkEvent* event)
            {
                if(event->eventType() == network::InnerClientConnectedEvent::EventType){
                    network::InnerClientConnectedEvent * ev = static_cast<network::InnerClientConnectedEvent*>(event);
                    gtk_button_set_label(GTK_BUTTON(connect_button_), DISCONNECT_LABEL);
                    g_signal_handler_disconnect(connect_button_, connect_signal_id);
                    connect_signal_id = g_signal_connect(G_OBJECT(connect_button_), "clicked", G_CALLBACK(EventHandler::click_disconnect_callback), evhandler_);
                }
                else if(event->eventType() == network::InnerClientDisconnectedEvent::EventType){
                    network::InnerClientDisconnectedEvent * ev = static_cast<network::InnerClientDisconnectedEvent*>(event);
                    g_signal_handler_disconnect(connect_button_, connect_signal_id);
                    connect_signal_id = g_signal_connect(G_OBJECT(connect_button_), "clicked", G_CALLBACK(EventHandler::click_connect_callback), evhandler_);
                    gtk_button_set_label(GTK_BUTTON(connect_button_), CONNECT_LABEL);
                }
                else{

                }

                GuiNetworkEventHandler::handleEvent(event);
            }

            void GtkMainWindow::showImpl()
            {
                HttpConfig cur_config = controller_->config();

                const std::string loc_host = cur_config.local_host.host_;
                gtk_entry_set_text(GTK_ENTRY(domain_text_), loc_host.c_str());
                const std::string loc_portstr = common::convertToString(cur_config.local_host.port_);
                gtk_entry_set_text(GTK_ENTRY(port_text_), loc_portstr.c_str());
                gtk_entry_set_text(GTK_ENTRY(login_text_), cur_config.login.c_str());
                gtk_entry_set_text(GTK_ENTRY(password_text_), cur_config.password.c_str());
                gtk_entry_set_text(GTK_ENTRY(content_path_text_), cur_config.content_path.c_str());
                gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(is_private_site_), cur_config.is_private_site);
                gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(is_external_domain_), cur_config.server_type ==  EXTERNAL_SERVER);

                const std::string ex_host = cur_config.external_host.host_;
                gtk_entry_set_text(GTK_ENTRY(external_host_), ex_host.c_str());
                const std::string exportstr = common::convertToString(cur_config.external_host.port_);
                gtk_entry_set_text(GTK_ENTRY(external_port_), exportstr.c_str());

                gtk_widget_show_all(window_);
            }

            void GtkMainWindow::onConnectClicked()
            {
                HttpConfig old_config = controller_->config();

                const std::string loc_host = gtk_entry_get_text(GTK_ENTRY(domain_text_));
                const uint16_t loc_port = common::convertFromString<uint16_t>(gtk_entry_get_text(GTK_ENTRY(port_text_)));
                old_config.local_host = common::net::hostAndPort(loc_host, loc_port);

                old_config.login = gtk_entry_get_text(GTK_ENTRY(login_text_));
                old_config.password = gtk_entry_get_text(GTK_ENTRY(password_text_));
                old_config.content_path = gtk_entry_get_text(GTK_ENTRY(content_path_text_));
                old_config.is_private_site = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(is_private_site_));
                old_config.server_type = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(is_external_domain_)) ? EXTERNAL_SERVER : FASTO_SERVER;

                const std::string exhoststr = gtk_entry_get_text(GTK_ENTRY(external_host_));
                const uint16_t ex_port = common::convertFromString<uint16_t>(gtk_entry_get_text(GTK_ENTRY(external_port_)));
                old_config.external_host = common::net::hostAndPort(exhoststr, ex_port);
                GuiNetworkEventHandler::onConnectClicked(old_config);
            }

            void GtkMainWindow::setExternalChekboxState(bool is_active)
            {
                gtk_widget_set_sensitive(browse_content_button_, !is_active);
                gtk_widget_set_sensitive(content_path_text_, !is_active);
                gtk_widget_set_sensitive(port_text_, !is_active);

                gtk_widget_set_sensitive(external_host_, is_active);
                gtk_widget_set_sensitive(external_port_, is_active);
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
}
