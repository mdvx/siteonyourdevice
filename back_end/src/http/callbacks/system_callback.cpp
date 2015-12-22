#include "http/callbacks/system_callback.h"

#include "common/system/system.h"
#include "common/string_util.h"

#include "http/http_client.h"

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

        DNOTREACHED();
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
            using namespace common::http;
            //keep alive
            http_request::header_t connectionField = request.findHeaderByKey("Connection",false);
            bool isKeepAlive = EqualsASCII(connectionField.value_, "Keep-Alive", false);
            const http_protocols protocol = request.protocol();

            common::Error err = common::system::systemShutdown(common::system::SHUTDOWN);
            if(err && err->isError()){
                hclient->send_error(protocol, HS_NOT_ALLOWED, NULL, "Shutdown failed.", isKeepAlive, info);
                return true;
            }

            hclient->send_ok(protocol, NULL, "Your device shutdowned!", isKeepAlive, info);
            return true;
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
