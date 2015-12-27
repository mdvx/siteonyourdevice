#pragma once

#include "common/net/types.h"
#include "common/url.h"

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
            uint16_t port_;
            std::string content_path_;
            std::string domain_;
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
