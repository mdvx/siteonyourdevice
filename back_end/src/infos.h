#pragma once

#include "common/net/types.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        struct UserAuthInfo
        {
            UserAuthInfo();
            UserAuthInfo(const std::string& login, const std::string& password, const common::net::hostAndPort& host);

            bool isValid() const;

            std::string login_;
            std::string password_;
            common::net::hostAndPort host_;
        };

        inline bool operator ==(const UserAuthInfo& lhs,const UserAuthInfo& rhs)
        {
            return lhs.login_ == rhs.login_ && lhs.password_ == rhs.password_ && lhs.host_ == rhs.host_;
        }

        inline bool operator!=(const UserAuthInfo& x, const UserAuthInfo& y)
        {
            return !(x == y);
        }

        struct HttpServerInfo
        {
            HttpServerInfo();
            HttpServerInfo(const std::string& serverName, const std::string& serverUrl, const std::string& contentPath);

            std::string serverName_;
            std::string serverUrl_;
            std::string contentPath_;
        };
    }
}


namespace common
{
    std::string convertToString(const fasto::siteonyourdevice::UserAuthInfo& uinfo);
}
