#pragma once

#include "common/http/http.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace http
        {
            class HttpClient;
        }

        enum HCTypes
        {
            system,
            file_system //handle all requests
        };

        const std::string HCallbackTypes[] = { "system", "file_system" };

        class IHttpCallback
        {
        public:
            IHttpCallback();
            virtual ~IHttpCallback();
            virtual bool handleRequest(http::HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info) = 0;

            static common::shared_ptr<IHttpCallback> createHttpCallback(HCTypes type);
            static common::shared_ptr<IHttpCallback> createHttpCallback(const std::string& ns_name, const std::string& name);
        };

        class HttpCallbackUrl
                : public IHttpCallback
        {
        public:
            HttpCallbackUrl(HCTypes type);
            HCTypes type() const;

        private:
            const HCTypes type_;
        };
    }
}

namespace common
{
    std::string convertToString(fasto::siteonyourdevice::HCTypes t);
}
