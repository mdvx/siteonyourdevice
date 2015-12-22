#pragma once

#include "http/callbacks/http_callbacks.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        enum HSCTypes
        {
            system_shutdown,
            system_reboot
        };

        const std::string HSystemCallbackTypes[] = { "shutdown", "reboot" };

        class HttpSystemCallback
                : public IHttpCallback
        {
        public:
            HttpSystemCallback();
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info);
        };

        class HttpSystemShutdownCallback
                : public IHttpCallback
        {
        public:
            HttpSystemShutdownCallback();
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info);
        };

        common::shared_ptr<IHttpCallback> createSystemHttpCallback(const std::string& name);
    }
}

namespace common
{
    std::string convertToString(fasto::siteonyourdevice::HSCTypes t);
}
