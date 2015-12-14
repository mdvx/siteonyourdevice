#include "infos.h"

#include "common/sprintf.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        UserAuthInfo::UserAuthInfo()
            : login_(), password_(), host_()
        {

        }

        UserAuthInfo::UserAuthInfo(const std::string& login, const std::string& password, const common::net::hostAndPort& host)
            : login_(login), password_(password), host_(host)
        {

        }

        bool UserAuthInfo::isValid() const
        {
            return !login_.empty() && host_.isValid();
        }

        HttpServerInfo::HttpServerInfo()
            : serverName_(), serverUrl_(), contentPath_()
        {

        }

        HttpServerInfo::HttpServerInfo(const std::string& serverName, const std::string& serverUrl, const std::string& contentPath)
            : serverName_(serverName), serverUrl_(serverUrl), contentPath_(contentPath)
        {

        }
    }
}

namespace common
{
    std::string convertToString(const fasto::siteonyourdevice::UserAuthInfo& uinfo)
    {
        std::string uinfoStr = common::MemSPrintf("%s:%s:%s", uinfo.login_, uinfo.password_, convertToString(uinfo.host_));
        return uinfoStr;
    }

    template<>
    fasto::siteonyourdevice::UserAuthInfo convertFromString(const std::string& uinfoStr)
    {
        size_t up = uinfoStr.find_first_of(':');
        if(up == std::string::npos){
            return fasto::siteonyourdevice::UserAuthInfo();
        }

        size_t ph = uinfoStr.find_first_of(':', up + 1);
        if(ph == std::string::npos){
            return fasto::siteonyourdevice::UserAuthInfo();
        }

        fasto::siteonyourdevice::UserAuthInfo uinfo;
        uinfo.login_ = uinfoStr.substr(0, up);
        uinfo.password_ = uinfoStr.substr(up + 1, ph - up - 1);
        uinfo.host_ = convertFromString<common::net::hostAndPort>(uinfoStr.substr(ph + 1));

        return uinfo;
    }
}
