/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <gtk/gtkstyle.h>

#include "application/fasto_remote_gui_application.h"

namespace fasto {
namespace siteonyourdevice {
namespace application {

class GtkMainWindow
        : public GuiNetworkEventHandler {
 public:
  explicit GtkMainWindow(network::NetworkController* controller);
  ~GtkMainWindow();

 protected:
  void handleEvent(network::NetworkEvent* event);
  void onConnectClicked();

  void setExternalChekboxState(bool is_active);

 private:
  virtual void showImpl();

  GtkWidget * window_;
  GtkWidget * connect_button_;

  GtkWidget * domain_text_;
  GtkWidget * port_text_;

  GtkWidget * login_text_;
  GtkWidget * password_text_;
  GtkWidget * content_path_text_;
  GtkWidget * browse_content_button_;

  GtkWidget * is_external_domain_;
  GtkWidget * external_host_;
  GtkWidget * external_port_;

  GtkWidget * is_private_site_;

  class EventHandler;
  EventHandler* evhandler_;

 private:
  uint32_t connect_signal_id;
};

class GtkGuiFastoRemoteApplication
        : public FastoRemoteGuiApplication {
 public:
  GtkGuiFastoRemoteApplication(int argc, char *argv[]);
  ~GtkGuiFastoRemoteApplication();
  virtual int exec();
};

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
