#pragma once

#include "common/net/types.h"
#include "common/url.h"

#define SERVER_SETTINGS_SECTION_LABEL "http_server"

//
#define LOGIN_SETTING_LABEL "login"
#define PASSWORD_SETTING_LABEL "password"
//
#define SERVER_TYPE_SETTING_LABEL "server_type"
#define PRIVATE_SITE_SETTING_LABEL "private_site"
// local
#define CONTENT_PATH_SETTING_LABEL "content_path"
#define LOCAL_HOST_SETTING_LABEL "local_host"
// external
#define EXTERNAL_HOST_SETTING_LABEL "external_host"

#define HANDLERS_URLS_SECTION_LABEL "http_handlers_utls"

#define SERVER_SOCKETS_SECTION_LABEL "http_server_sockets"

namespace fasto
{
    namespace siteonyourdevice
    {
        enum http_server_type
        {
            FASTO_SERVER = 0,
            EXTERNAL_SERVER = 1
        };

        struct HttpConfig
        {
            std::string content_path_;
            common::net::hostAndPort local_host_;
            std::string login_;
            std::string password_;
            bool is_private_site_;

            common::net::hostAndPort external_host_;
            http_server_type server_type_;

            typedef std::pair<std::string, std::string>  handlers_urls_t;
            std::vector<handlers_urls_t> handlers_urls_;

            typedef std::pair<std::string, common::uri::Uri> server_sockets_urls_t;
            std::vector<server_sockets_urls_t> server_sockets_urls_;
        };
    }
}
