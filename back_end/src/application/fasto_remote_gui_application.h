#pragma once

#include "fasto_remote_application.h"

#define CONNECT_LABEL "Connect"
#define DISCONNECT_LABEL "Disconnect"
#define DOMAIN_LABEL "Domain:"
#define PORT_LABEL "Port:"
#define LOGIN_LABEL "Login:"
#define PASSWORD_LABEL "Password:"
#define CONTENT_PATH_LABEL "Content path:"
#define BROWSE_LABEL "..."
#define PRIVATE_SITE_LABEL "Private site:"

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
    "Copyright 2014-2015 <a href=\"http://" PROJECT_COMPANYNAME_DOMAIN "\">" PROJECT_COMPANYNAME "</a>. All rights reserved.<br/>"\
    "<br/>"\
    "The program is provided AS IS with NO WARRANTY OF ANY KIND, "\
    "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "\
    "PARTICULAR PURPOSE.<br/>"

namespace fasto
{
    namespace siteonyourdevice
    {
        class GuiNetworkEventHandler
                : public NetworkEventHandler
        {
        public:
            enum
            {
                height = 240,
                width = 320,
                login_max_length = 32,
                domain_max_length = 32,
                port_max_length = 6
            };

            GuiNetworkEventHandler(NetworkController *controller);
            ~GuiNetworkEventHandler();

            virtual int start() final;

        protected:
            void onConnectClicked(const configuration_t &config); //with config
            void onDisconnectClicked();

        private:
            virtual int showImpl() = 0;
        };

        class FastoRemoteGuiApplication
                : public FastoRemoteApplication
        {
        public:
            FastoRemoteGuiApplication(int argc, char *argv[]);
            ~FastoRemoteGuiApplication();
        };
    }
}
