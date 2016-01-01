#include "platform/windows/gui_fasto_application.h"

#include <windows.h>
#include <Shlobj.h>

#include "network/network_events.h"
#include "network/network_controller.h"

#define ID_ABOUT       2000
#define ID_EXIT        2001

#define ID_DOMAIN_STATIC 2002
#define ID_DOMAIN_TEXTBOX 2003
#define ID_PORT_STATIC 2004
#define ID_PORT_TEXTBOX 2005
#define ID_LOGIN_STATIC 2006
#define ID_LOGIN_TEXTBOX 2007
#define ID_CONTENT_PATH_STATIC 2008
#define ID_CONTENT_PATH_TEXTBOX 2009
#define ID_PASSWORD_STATIC 2010
#define ID_PASSWORD_TEXTBOX 2011
#define ID_BROWSE_BUTTON 2012
#define ID_PRIVATE_SITE_CHECKBOX 2013
#define ID_EXTERNAL_SITE_CHECKBOX 2014
#define ID_EXTERNAL_SITE_HOST_TEXTBOX 2015
#define ID_EXTERNAL_SITE_PORT_TEXTBOX 2016
#define ID_CONNECT_BUTTON 2017

#define IDR_STATE_OFFLINE 2018
#define IDR_STATE_ONLINE 2019

#define ID_APPICON 101

namespace
{
    void RemoveTrayIcon(HWND hWnd, UINT uID)
    {
        NOTIFYICONDATA  nid;
        nid.hWnd = hWnd;
        nid.uID  = uID;
        Shell_NotifyIcon(NIM_DELETE, &nid);
    }

    void AddTrayIcon(HWND hWnd, UINT uID, UINT uCallbackMsg, UINT uIcon)
    {
        NOTIFYICONDATA  nid;
        nid.hWnd             = hWnd;
        nid.uID              = uID;
        nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = uCallbackMsg;
        nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_APPICON));
        strcpy(nid.szTip, PROJECT_NAME_TITLE);
        Shell_NotifyIcon(NIM_ADD, &nid );
    }
}

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace application
        {
            Win32MainWindow::Win32MainWindow(network::NetworkController *controller)
                : GuiNetworkEventHandler(controller), hwnd_(NULL),
                hwndDomainStatic_(NULL), hwndDomainTextBox_(NULL), hwndPortStatic_(NULL), hwndPortTextBox_(NULL),
                hwndLoginStatic_(NULL), hwndLoginTextBox_(NULL),
                hwndPasswordStatic_(NULL), hwndPasswordTextBox_(NULL),
                hwndContentPathStatic_(NULL), hwndContentPathTextBox_(NULL),
                hwndBrowseButton_(NULL),
                hwndExternalServerCheckbox_(NULL), hwndExternalHostTextBox_(NULL), hwndExternalPortTextBox_(NULL),
                hwndIsPrivateSiteCheckbox_(NULL),
                hwndConnectButton_(NULL), isMessageBoxShown_(false)
            {
                const HINSTANCE hInstance = GetModuleHandle(NULL);

                WNDCLASSEX wclx;
                memset(&wclx, 0, sizeof(wclx));
                wclx.cbSize         = sizeof( wclx );
                wclx.style          = 0;
                wclx.lpfnWndProc    = &wndProc;
                wclx.cbClsExtra     = 0;
                wclx.cbWndExtra     = 0;
                wclx.hInstance      = GetModuleHandle(NULL);
                wclx.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(ID_APPICON));
                wclx.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(ID_APPICON));
                wclx.hCursor        = LoadCursor( NULL, IDC_ARROW );
                wclx.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );
                wclx.lpszMenuName   = NULL;
                wclx.lpszClassName  = PROJECT_NAME_TITLE;
                RegisterClassEx(&wclx);
            }

            Win32MainWindow::~Win32MainWindow()
            {
                const HINSTANCE hInstance = GetModuleHandle(NULL);
                UnregisterClass(PROJECT_NAME_TITLE, hInstance);
            }

            void Win32MainWindow::showImpl()
            {
                const HINSTANCE hInstance = GetModuleHandle(NULL);

                HWND hwnd = CreateWindow(PROJECT_NAME_TITLE, PROJECT_NAME_TITLE, WS_OVERLAPPED | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX, 0, 0,
                                     width, height, NULL, NULL, hInstance, this);
                if (!hwnd) {
                    MessageBox(NULL, "Can't create window!", TEXT("Warning!"), MB_ICONERROR | MB_OK | MB_TOPMOST);
                    return;
                }

                RECT rc;
                GetWindowRect(hwnd, &rc);
                int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right)/2;
                int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom)/2;
                SetWindowPos(hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

                HttpConfig cur_config = controller_->config();

                SetWindowText(hwndDomainTextBox_, cur_config.domain_.c_str());
                const std::string portstr = common::convertToString(cur_config.port_);
                SetWindowText(hwndPortTextBox_, portstr.c_str());
                SetWindowText(hwndLoginTextBox_, cur_config.login_.c_str());
                SetWindowText(hwndPasswordTextBox_, cur_config.password_.c_str());
                SetWindowText(hwndContentPathTextBox_, cur_config.content_path_.c_str());

                setExternalChekboxState(cur_config.server_type_ == EXTERNAL_SERVER ? BST_CHECKED : BST_UNCHECKED);
                SendMessage(hwndIsPrivateSiteCheckbox_, BM_SETCHECK, cur_config.is_private_site_ ? BST_CHECKED : BST_UNCHECKED, 0);
                const std::string ex_host = cur_config.external_host_.host_;
                SetWindowText(hwndExternalHostTextBox_, ex_host.c_str());
                const std::string ex_portstr = common::convertToString(cur_config.external_host_.port_);
                SetWindowText(hwndExternalPortTextBox_, ex_portstr.c_str());
            }

            void Win32MainWindow::setExternalChekboxState(LRESULT status)
            {
                if(status == BST_CHECKED){
                    SendMessage(hwndExternalServerCheckbox_, BM_SETCHECK, BST_CHECKED, 0);
                    EnableWindow(hwndBrowseButton_, FALSE);
                    EnableWindow(hwndContentPathTextBox_, FALSE);
                    EnableWindow(hwndPortTextBox_, FALSE);

                    EnableWindow(hwndExternalHostTextBox_, TRUE);
                    EnableWindow(hwndExternalPortTextBox_, TRUE);
                }
                else if(status == BST_UNCHECKED){
                    SendMessage(hwndExternalServerCheckbox_, BM_SETCHECK, BST_UNCHECKED, 0);
                    EnableWindow(hwndBrowseButton_, TRUE);
                    EnableWindow(hwndContentPathTextBox_, TRUE);
                    EnableWindow(hwndPortTextBox_, TRUE);

                    EnableWindow(hwndExternalHostTextBox_, FALSE);
                    EnableWindow(hwndExternalPortTextBox_, FALSE);
                }
            }

            LRESULT Win32MainWindow::onCreate()
            {
                const HINSTANCE hInstance = GetModuleHandle(NULL);
                const int control_height = 25;
                const int textbox_control_height = 20;
                const int control_padding = 5;
                const int padding = 20;

                const int first_line_x = padding;
                const int first_line_y = padding;

                const int second_line_x = first_line_x;
                const int second_line_y = first_line_y + control_padding + textbox_control_height;

                const int third_line_x = second_line_x;
                const int third_line_y = second_line_y + control_padding + textbox_control_height;

                const int foth_line_x = third_line_x;
                const int foth_line_y = third_line_y + control_padding + textbox_control_height;

                const int fifth_line_x = foth_line_x;
                const int fifth_line_y = foth_line_y + (control_padding + textbox_control_height);

                const int six_line_x = fifth_line_x;
                const int six_line_y = fifth_line_y + control_padding + textbox_control_height;

                const int seven_line_x = six_line_x;
                const int seven_line_y = six_line_y + control_padding + textbox_control_height;

                const int label_width = (width - padding * 2)/4;
                const int text_box_width = (width - padding * 2) * 2 / 5;

                AddTrayIcon(hwnd_, 1, WM_APP, 0);
                // 1st
                hwndDomainStatic_ = CreateWindow(TEXT("STATIC"), TEXT(DOMAIN_LABEL),
                                                  WS_CHILD | WS_VISIBLE,
                                                  first_line_x, first_line_y,
                                                  label_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_DOMAIN_STATIC,
                                                  hInstance, NULL);

                const int first_line_x2 = first_line_x  + label_width + control_padding;
                hwndDomainTextBox_ = CreateWindow(TEXT("EDIT"), TEXT(""),
                                                  WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                                  first_line_x2, first_line_y,
                                                  text_box_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_DOMAIN_TEXTBOX,
                                                  hInstance, NULL);
                SendMessage(hwndDomainTextBox_, EM_SETLIMITTEXT, domain_max_length, 0);

                const int first_line_x3 = first_line_x2 + text_box_width + control_padding * 2;
                const int label_port_width = label_width / 2;
                hwndPortStatic_ = CreateWindow(TEXT("STATIC"), TEXT(PORT_LABEL),
                                                  WS_CHILD | WS_VISIBLE,
                                                  first_line_x3, first_line_y,
                                                  label_port_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_PORT_STATIC,
                                                  hInstance, NULL);

                const int first_line_x4 = first_line_x3 + label_port_width + control_padding;
                hwndPortTextBox_ = CreateWindow(TEXT("EDIT"), TEXT(""),
                                                  WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                                  first_line_x4, first_line_y,
                                                  40, textbox_control_height,
                                                  hwnd_, (HMENU)ID_PORT_TEXTBOX,
                                                  hInstance, NULL);
                SendMessage(hwndPortTextBox_, EM_SETLIMITTEXT, port_max_length, 0);
                //

                // 2nd
                hwndLoginStatic_ = CreateWindow(TEXT("STATIC"), TEXT(LOGIN_LABEL),
                                                  WS_CHILD | WS_VISIBLE,
                                                  second_line_x, second_line_y,
                                                  label_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_LOGIN_STATIC,
                                                  hInstance, NULL);

                const int second_line_x2 = first_line_x  + label_width + control_padding;
                hwndLoginTextBox_ = CreateWindow(TEXT("EDIT"), TEXT(""),
                                                  WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                                  second_line_x2, second_line_y,
                                                  text_box_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_LOGIN_TEXTBOX,
                                                  hInstance, NULL);
                SendMessage(hwndLoginTextBox_, EM_SETLIMITTEXT, login_max_length, 0);
                //

                // 3rd
                hwndPasswordStatic_ = CreateWindow(TEXT("STATIC"), TEXT(PASSWORD_LABEL),
                                                  WS_CHILD | WS_VISIBLE,
                                                  third_line_x, third_line_y,
                                                  label_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_PASSWORD_STATIC,
                                                  hInstance, NULL);

                const int third_line_x2 = first_line_x  + label_width + control_padding;
                hwndPasswordTextBox_ = CreateWindow(TEXT("EDIT"), TEXT(""),
                                                  ES_PASSWORD | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                                  third_line_x2, third_line_y,
                                                  text_box_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_PASSWORD_TEXTBOX,
                                                  hInstance, NULL);
                //

                // 4rd
                const int label_content_width = label_width * 3/2;
                hwndContentPathStatic_ = CreateWindow(TEXT("STATIC"), TEXT(CONTENT_PATH_LABEL),
                                                  WS_CHILD | WS_VISIBLE,
                                                  foth_line_x, foth_line_y,
                                                  label_content_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_CONTENT_PATH_STATIC,
                                                  hInstance, NULL);

                const int foth_line_x2 = first_line_x  + label_content_width + control_padding;
                hwndContentPathTextBox_ = CreateWindow(TEXT("EDIT"), TEXT(""),
                                                  WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                                  foth_line_x2, foth_line_y,
                                                  text_box_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_CONTENT_PATH_TEXTBOX,
                                                  hInstance, NULL);

                hwndBrowseButton_ = CreateWindow(TEXT("BUTTON"), TEXT(BROWSE_LABEL),
                                                  WS_VISIBLE | WS_CHILD | BS_TEXT | WS_TABSTOP,
                                                  foth_line_x2 + text_box_width + control_padding, foth_line_y,
                                                  40, control_height - control_padding,
                                                  hwnd_, (HMENU) ID_BROWSE_BUTTON,
                                                  hInstance, NULL);

                // external
                hwndExternalServerCheckbox_ = CreateWindow(TEXT("BUTTON"), TEXT(EXTERNAL_SITE_LABEL),
                                                  WS_VISIBLE | WS_CHILD | BS_CHECKBOX | WS_TABSTOP,
                                                  fifth_line_x, fifth_line_y,
                                                  label_width + 40, control_height,
                                                  hwnd_, (HMENU) ID_EXTERNAL_SITE_CHECKBOX,
                                                  hInstance, NULL);

                const int fifth_line_x2 = fifth_line_x + label_width + 40 + control_padding;
                hwndExternalHostTextBox_ = CreateWindow(TEXT("EDIT"), TEXT(""),
                                                  WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                                  fifth_line_x2, fifth_line_y,
                                                  text_box_width, textbox_control_height,
                                                  hwnd_, (HMENU)ID_EXTERNAL_SITE_HOST_TEXTBOX,
                                                  hInstance, NULL);

                const int fifth_line_x3 = fifth_line_x2 + text_box_width + control_padding;
                hwndExternalPortTextBox_ = CreateWindow(TEXT("EDIT"), TEXT(""),
                                                        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                                        fifth_line_x3, fifth_line_y,
                                                        40, textbox_control_height,
                                                        hwnd_, (HMENU)ID_EXTERNAL_SITE_PORT_TEXTBOX,
                                                        hInstance, NULL);

                //
                hwndIsPrivateSiteCheckbox_ = CreateWindow(TEXT("BUTTON"), TEXT(PRIVATE_SITE_LABEL),
                                                  WS_VISIBLE | WS_CHILD | BS_CHECKBOX | WS_TABSTOP,
                                                  six_line_x, six_line_y,
                                                  width - padding * 2, control_height,
                                                  hwnd_, (HMENU) ID_PRIVATE_SITE_CHECKBOX,
                                                  hInstance, NULL);

                //

                hwndConnectButton_ = CreateWindow(TEXT("BUTTON"), TEXT(CONNECT_LABEL),
                                                  WS_VISIBLE | WS_CHILD | BS_TEXT | WS_TABSTOP,
                                                  seven_line_x, seven_line_y,
                                                  width - padding * 2, control_height,
                                                  hwnd_, (HMENU) ID_CONNECT_BUTTON,
                                                  hInstance, NULL);
                return 0;
            }

            LRESULT Win32MainWindow::onDestroy()
            {
                RemoveTrayIcon(hwnd_, 1);
                ::DestroyWindow(hwndConnectButton_);

                ::DestroyWindow(hwndIsPrivateSiteCheckbox_);

                ::DestroyWindow(hwndExternalServerCheckbox_);
                ::DestroyWindow(hwndExternalHostTextBox_);
                ::DestroyWindow(hwndExternalServerCheckbox_);

                ::DestroyWindow(hwndContentPathStatic_);
                ::DestroyWindow(hwndContentPathTextBox_);

                ::DestroyWindow(hwndPasswordStatic_);
                ::DestroyWindow(hwndPasswordTextBox_);

                ::DestroyWindow(hwndLoginStatic_);
                ::DestroyWindow(hwndLoginTextBox_);

                ::DestroyWindow(hwndPortStatic_);
                ::DestroyWindow(hwndPortTextBox_);

                ::DestroyWindow(hwndDomainStatic_);
                ::DestroyWindow(hwndDomainTextBox_);
                return 0;
            }

            BOOL Win32MainWindow::showPopupMenu(POINT *curpos, int wDefaultItem)
            {
                HMENU hPop = CreatePopupMenu();
                if (isMessageBoxShown_) {
                    return FALSE;
                }

                InsertMenu(hPop, 0, MF_BYPOSITION | MF_STRING, ID_ABOUT, "About..." );
                InsertMenu(hPop, 1, MF_BYPOSITION | MF_STRING, ID_EXIT , "Exit" );

                SetMenuDefaultItem(hPop, ID_ABOUT, FALSE);
                SetFocus(hwnd_);
                SendMessage(hwnd_, WM_INITMENUPOPUP, (WPARAM)hPop, 0 );

                POINT pt;

                if (!curpos) {
                    GetCursorPos(&pt);
                    curpos = &pt;
                }

                WORD cmd = TrackPopupMenu(hPop, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, curpos->x, curpos->y, 0, hwnd_, NULL );
                SendMessage(hwnd_, WM_COMMAND, cmd, 0);
                DestroyMenu(hPop);
                return 0;
            }

            LRESULT Win32MainWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
            {
                switch (uMsg)
                {
                    case WM_CREATE:
                    {
                        return onCreate();
                    }
                    case WM_DESTROY:
                    {
                        onDestroy();
                        ::PostQuitMessage(0);
                        return 0;
                    }
                    case WM_COMMAND:
                    {
                        if (isMessageBoxShown_) {
                            return 1;
                        }
                        switch (LOWORD(wParam))
                        {
                            case ID_BROWSE_BUTTON:
                            {
                                if(BN_CLICKED == HIWORD(wParam)){
                                    TCHAR szDir[MAX_PATH];
                                    BROWSEINFO bInfo = {0};
                                    bInfo.hwndOwner = hwnd_;
                                    bInfo.pidlRoot = NULL;
                                    bInfo.pszDisplayName = szDir; // Address of a buffer to receive the display name of the folder selected by the user
                                    bInfo.lpszTitle = "Please, select a folder with content of site"; // Title of the dialog
                                    bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
                                    bInfo.lpfn = NULL;
                                    bInfo.lParam = 0;
                                    bInfo.iImage = -1;

                                    LPITEMIDLIST lpItem = SHBrowseForFolder( &bInfo);
                                    if(lpItem){
                                        SHGetPathFromIDList(lpItem, szDir);
                                        SetWindowText(hwndContentPathTextBox_, szDir);
                                    }
                                }
                                return 0;
                            }
                            case ID_PRIVATE_SITE_CHECKBOX:
                            {
                                LRESULT res = SendMessage(hwndIsPrivateSiteCheckbox_, BM_GETCHECK, 0, 0);
                                if(res == BST_CHECKED){
                                    SendMessage(hwndIsPrivateSiteCheckbox_, BM_SETCHECK, BST_UNCHECKED, 0);
                                }
                                else if(res == BST_UNCHECKED){
                                    SendMessage(hwndIsPrivateSiteCheckbox_, BM_SETCHECK, BST_CHECKED, 0);
                                }
                                return 0;
                            }
                            case ID_EXTERNAL_SITE_CHECKBOX:
                            {
                                LRESULT res = SendMessage(hwndExternalServerCheckbox_, BM_GETCHECK, 0, 0);
                                if(res == BST_CHECKED){
                                    setExternalChekboxState(BST_UNCHECKED);
                                }
                                else if(res == BST_UNCHECKED){
                                    setExternalChekboxState(BST_CHECKED);
                                }
                                return 0;
                            }
                            case ID_CONNECT_BUTTON:
                            {
                                if(BN_CLICKED == HIWORD(wParam)){
                                    char lbl[1024] = {0};
                                    GetWindowText(hwndConnectButton_, lbl, sizeof(lbl));
                                    if(strcmp(lbl, CONNECT_LABEL) == 0){
                                        onConnectClicked();
                                    }
                                    else{
                                        onDisconnectClicked();
                                    }
                                }
                                return 0;
                            }
                            case ID_ABOUT:
                            {
                                isMessageBoxShown_ = TRUE;
                                MessageBox(hwnd_, ABOUT_DIALOG_MSG, PROJECT_NAME_TITLE, MB_ICONINFORMATION | MB_OK );
                                isMessageBoxShown_ = FALSE;
                                return 0;
                            }
                            case ID_EXIT:
                            {
                                PostMessage(hwnd_, WM_CLOSE, 0, 0 );
                                return 0;
                            }
                            default:
                            {
                                return 0;
                            }
                        }
                    }
                    case WM_APP:
                    {
                        switch (lParam) {
                            case WM_LBUTTONDBLCLK:
                                ShowWindow(hwnd_, SW_RESTORE);
                                return 0;
                            case WM_RBUTTONUP:
                            case WM_CONTEXTMENU:
                                SetForegroundWindow(hwnd_);
                                showPopupMenu(NULL, -1);
                                PostMessage(hwnd_, WM_APP + 1, 0, 0 );
                                return 0;
                        }
                        return 0;
                    }
                }
                return DefWindowProc(hwnd_, uMsg, wParam, lParam );
            }

            LRESULT CALLBACK Win32MainWindow::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
            {
                Win32MainWindow *pThis = NULL;
                if (uMsg == WM_CREATE){
                    LPCREATESTRUCT pCreate = reinterpret_cast<LPCREATESTRUCT>(lParam);
                    pThis = (Win32MainWindow*)pCreate->lpCreateParams;
                    SetWindowLongPtr(hWnd, GWLP_USERDATA, PtrToUlong(pThis));
                    pThis->hwnd_ = hWnd;
                }
                else{
                    pThis = (Win32MainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                }

                if (pThis){
                    return pThis->handleMessage(uMsg, wParam, lParam);
                }
                else{
                    return DefWindowProc(hWnd, uMsg, wParam, lParam);
                }
            }

            void Win32MainWindow::handleEvent(network::NetworkEvent* event)
            {
                if(event->eventType() == network::InnerClientConnectedEvent::EventType){
                    network::InnerClientConnectedEvent * ev = static_cast<network::InnerClientConnectedEvent*>(event);
                    SendMessage(hwndConnectButton_, WM_SETTEXT, 0, (LPARAM)DISCONNECT_LABEL);
                }
                else if(event->eventType() == network::InnerClientDisconnectedEvent::EventType){
                    network::InnerClientDisconnectedEvent * ev = static_cast<network::InnerClientDisconnectedEvent*>(event);
                    SendMessage(hwndConnectButton_, WM_SETTEXT, 0, (LPARAM)CONNECT_LABEL);
                }
                else{

                }

                GuiNetworkEventHandler::handleEvent(event);
            }

            void Win32MainWindow::onConnectClicked()
            {
                HttpConfig old_config = controller_->config();

                char lbl[1024] = {0};
                GetWindowText(hwndDomainTextBox_, lbl, sizeof(lbl));
                old_config.domain_ = lbl;

                GetWindowText(hwndPortTextBox_, lbl, sizeof(lbl));
                old_config.port_ = common::convertFromString<uint16_t>(lbl);

                GetWindowText(hwndLoginTextBox_, lbl, sizeof(lbl));
                old_config.login_ = lbl;

                GetWindowText(hwndPasswordTextBox_, lbl, sizeof(lbl));
                old_config.password_ = lbl;

                GetWindowText(hwndContentPathTextBox_, lbl, sizeof(lbl));
                old_config.content_path_ = lbl;

                LRESULT res = SendMessage(hwndExternalServerCheckbox_, BM_GETCHECK, 0, 0);
                old_config.server_type_ = res == BST_CHECKED ? EXTERNAL_SERVER : FASTO_SERVER;

                res = SendMessage(hwndIsPrivateSiteCheckbox_, BM_GETCHECK, 0, 0);
                old_config.is_private_site_ = res == BST_CHECKED;

                GetWindowText(hwndExternalHostTextBox_, lbl, sizeof(lbl));
                std::string ex_host = lbl;

                GetWindowText(hwndExternalPortTextBox_, lbl, sizeof(lbl));
                uint16_t ex_port = common::convertFromString<uint16_t>(lbl);

                old_config.external_host_ = common::net::hostAndPort(ex_host, ex_port);

                GuiNetworkEventHandler::onConnectClicked(old_config);
            }

            int WinGuiFastoRemoteApplication::exec()
            {
                HWND hwnd = FindWindow(PROJECT_NAME_TITLE, PROJECT_NAME_TITLE);
                MSG msg;
                while(GetMessage(&msg, NULL, 0, 0)) {
                    if (IsDialogMessage(hwnd, &msg) == 0){
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
                int res = msg.wParam;

                FastoRemoteGuiApplication::exit(res);
                return FastoRemoteGuiApplication::exec();
            }

            WinGuiFastoRemoteApplication::WinGuiFastoRemoteApplication(int argc, char *argv[])
                : FastoRemoteGuiApplication(argc, argv)
            {
            }

            WinGuiFastoRemoteApplication::~WinGuiFastoRemoteApplication()
            {
            }
        }
    }
}
