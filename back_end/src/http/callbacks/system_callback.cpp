#include "http/callbacks/system_callback.h"

namespace common
{
    std::string convertToString(fasto::siteonyourdevice::HSCTypes t)
    {
        return fasto::siteonyourdevice::HSystemCallbackTypes[t];
    }

    template<>
    fasto::siteonyourdevice::HSCTypes convertFromString(const std::string& text)
    {
        for (uint32_t i = 0; i < SIZEOFMASS(fasto::siteonyourdevice::HSystemCallbackTypes); ++i){
            if (text == fasto::siteonyourdevice::HSystemCallbackTypes[i]){
                return static_cast<fasto::siteonyourdevice::HSCTypes>(i);
            }
        }

        NOTREACHED();
        return fasto::siteonyourdevice::system_shutdown;
    }
}

namespace fasto
{
    namespace siteonyourdevice
    {
        HttpSystemCallback::HttpSystemCallback()
            : IHttpCallback(system)
        {

        }

        bool HttpSystemCallback::handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info)
        {
            return false;
        }

        HttpSystemShutdownCallback::HttpSystemShutdownCallback()
            : IHttpCallback(system)
        {

        }

        bool HttpSystemShutdownCallback::handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info)
        {
            return false;
        }

        common::shared_ptr<IHttpCallback> createSystemHttpCallback(const std::string& name)
        {
            HSCTypes sys_type = common::convertFromString<HSCTypes>(name);
            if(sys_type == system_shutdown){
                return common::shared_ptr<IHttpCallback>(new HttpSystemShutdownCallback);
            }
            else{
                DNOTREACHED();
                return common::shared_ptr<IHttpCallback>();
            }
        }
    }
}
