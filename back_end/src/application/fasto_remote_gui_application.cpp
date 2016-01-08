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

#include "application/fasto_remote_gui_application.h"

#include "network/network_controller.h"

namespace fasto {
namespace siteonyourdevice {
namespace application {

GuiNetworkEventHandler::GuiNetworkEventHandler(network::NetworkController *controller)
    : NetworkEventHandler(controller) {
}

GuiNetworkEventHandler::~GuiNetworkEventHandler() {
}

void GuiNetworkEventHandler::start() {
    showImpl();
}

void GuiNetworkEventHandler::onConnectClicked(const HttpConfig& config) {
    controller_->setConfig(config);
    controller_->connect();
}

void GuiNetworkEventHandler::onDisconnectClicked() {
    controller_->disConnect();
}

FastoRemoteGuiApplication::FastoRemoteGuiApplication(int argc, char *argv[])
    : FastoRemoteApplication(argc, argv) {
}

FastoRemoteGuiApplication::~FastoRemoteGuiApplication() {
}

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
