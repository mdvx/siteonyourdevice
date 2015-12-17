#pragma once

#include "application/fasto_remote_gui_application.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class Win32MainWindow
                : public GuiNetworkEventHandler
        {
        public:
            Win32MainWindow(NetworkController *controller);
            ~Win32MainWindow();

        protected:
            virtual void handleEvent(NetworkEvent* event);
            void onConnectClicked();

        private:
            virtual int showImpl();

            static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
            virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

            LRESULT OnCreate();
            LRESULT OnDestroy();

            BOOL ShowPopupMenu(POINT *curpos, int wDefaultItem);

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
    }
}
