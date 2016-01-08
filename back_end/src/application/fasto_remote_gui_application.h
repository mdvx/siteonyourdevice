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

#include "fasto_remote_application.h"

#include "network/network_event_handler.h"
#include "http_config.h"

#define CONNECT_LABEL "Connect"
#define DISCONNECT_LABEL "Disconnect"
#define DOMAIN_LABEL "Domain:"
#define PORT_LABEL "Port:"
#define LOGIN_LABEL "Login:"
#define PASSWORD_LABEL "Password:"
#define CONTENT_PATH_LABEL "Content path:"
#define BROWSE_LABEL "..."
#define PRIVATE_SITE_LABEL "Private site:"
#define EXTERNAL_SITE_LABEL "External host:"

#if defined(PROJECT_BUILD_TYPE_VERSION) && defined(PROJECT_BUILD_RELEASE)
    #define ABOUT_TITLE "<h3>" PROJECT_NAME_TITLE " " PROJECT_VERSION "<br/>Revision:" PROJECT_GIT_VERSION "</h3>"
#else
    #define ABOUT_TITLE "<h3>" PROJECT_NAME_TITLE " " PROJECT_VERSION " " PROJECT_BUILD_TYPE_VERSION STRINGIZE(PROJECT_VERSION_TWEAK) "<br/>Revision:" PROJECT_GIT_VERSION "</h3>"
#endif

#define ABOUT_DIALOG_MSG \
    ABOUT_TITLE \
    PROJECT_BREF_DESCRIPTION\
    "<br/>"\
    "<br/>"\
    "Visit " PROJECT_NAME_TITLE " website: <a href=\"http://" PROJECT_DOMAIN "\">" PROJECT_DOMAIN "</a> <br/>"\
    "<br/>"\
    "<a href=\"https://" PROJECT_GITHUB_FORK "\">Fork</a> project or <a href=\"https://" PROJECT_GITHUB_ISSUES "\">submit</a> issues/proposals on GitHub.  <br/>"\
    "<br/>"\
    "Copyright 2014-2016 <a href=\"http://" PROJECT_COMPANYNAME_DOMAIN "\">" PROJECT_COMPANYNAME "</a>. All rights reserved.<br/>"\
    "<br/>"\
    "The program is provided AS IS with NO WARRANTY OF ANY KIND, "\
    "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "\
    "PARTICULAR PURPOSE.<br/>"

namespace fasto {
namespace siteonyourdevice {
namespace application {

class GuiNetworkEventHandler
        : public network::NetworkEventHandler {
 public:
    enum {
      height = 240,
      width = 320,
      login_max_length = 32,
      domain_max_length = 32,
      port_max_length = 6
  };

  explicit GuiNetworkEventHandler(network::NetworkController *controller);
  ~GuiNetworkEventHandler();

  void start() final;

 protected:
  void onConnectClicked(const HttpConfig &config);  // with config
  void onDisconnectClicked();

 private:
  virtual void showImpl() = 0;
};

class FastoRemoteGuiApplication
        : public FastoRemoteApplication {
 public:
  FastoRemoteGuiApplication(int argc, char *argv[]);
  ~FastoRemoteGuiApplication();
};

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
