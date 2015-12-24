#pragma once

#include "http/callbacks/http_callbacks.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        enum HSCTypes
        {
            system_shutdown,
            system_logout,
            system_reboot
        };

        const std::string HSystemCallbackTypes[] = { "shutdown", "logout", "reboot" };

        class HttpSystemCallback
                : public HttpCallbackUrl
        {
        public:
            HttpSystemCallback();
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info);
        };

        class HttpSystemShutdownCallback
                : public HttpCallbackUrl
        {
        public:
            HttpSystemShutdownCallback(HSCTypes type);
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info);

        private:
            const HSCTypes type_;
        };

        common::shared_ptr<IHttpCallback> createSystemHttpCallback(const std::string& name);
    }
}

namespace common
{
    std::string convertToString(fasto::siteonyourdevice::HSCTypes t);
}
