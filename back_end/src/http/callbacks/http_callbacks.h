#pragma once

#include "common/http/http.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpClient;

        enum HCTypes
        {
            system,
            file_system //handle all requests
        };

        const std::string HCallbackTypes[] = { "system", "file_system" };

        class IHttpCallback
        {
        public:
            ~IHttpCallback();
            HCTypes type() const;
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request) = 0;

            static common::shared_ptr<IHttpCallback> createHttpCallback(HCTypes type);
            static common::shared_ptr<IHttpCallback> createHttpCallback(const std::string& name);

        protected:
            IHttpCallback(HCTypes type);

        private:
            const HCTypes type_;
        };
    }
}

namespace common
{
    std::string convertToString(fasto::siteonyourdevice::HCTypes t);
}
