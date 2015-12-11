#pragma once

#include <vector>
#include <string>

namespace fasto
{
    namespace fastoremote
    {
        typedef struct
        {
            uint16_t port_;
            std::string content_path_;
            std::string domain_;
            std::string login_;
            std::string password_;
            bool is_private_site_;

            typedef std::pair<std::string, std::string>  handlers_urls_t;
            std::vector<handlers_urls_t> handlers_urls_;
        } configuration_t;

    }
}
