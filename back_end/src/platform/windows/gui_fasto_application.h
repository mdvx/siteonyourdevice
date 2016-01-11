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

#include "application/fasto_remote_gui_application.h"

namespace fasto {
namespace siteonyourdevice {
namespace application {

class Win32MainWindow
  : public GuiNetworkEventHandler
{
 public:
  Win32MainWindow(network::NetworkController *controller);
  ~Win32MainWindow();

 protected:
  virtual void handleEvent(network::NetworkEvent* event);
  void onConnectClicked();

 private:
  virtual void showImpl();
  void setExternalChekboxState(LRESULT status);

  static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

  LRESULT onCreate();
  LRESULT onDestroy();

  BOOL showPopupMenu(POINT *curpos, int wDefaultItem);

  HWND hwnd_;
  HWND hwndDomainStatic_;
  HWND hwndDomainTextBox_;

  HWND hwndPortStatic_;
  HWND hwndPortTextBox_;

  HWND hwndLoginStatic_;
  HWND hwndLoginTextBox_;

  HWND hwndPasswordStatic_;
  HWND hwndPasswordTextBox_;

  HWND hwndContentPathStatic_;
  HWND hwndContentPathTextBox_;
  HWND hwndBrowseButton_;

  HWND hwndExternalServerCheckbox_;
  HWND hwndExternalHostTextBox_;
  HWND hwndExternalPortTextBox_;

  HWND hwndIsPrivateSiteCheckbox_;

  HWND hwndConnectButton_;
  BOOL isMessageBoxShown_;
};

class WinGuiFastoRemoteApplication
  : public FastoRemoteGuiApplication
{
 public:
  WinGuiFastoRemoteApplication(int argc, char *argv[]);
  ~WinGuiFastoRemoteApplication();
  virtual int exec();
};

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
