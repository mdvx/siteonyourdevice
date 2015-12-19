#pragma once

#include "http/callbacks/http_callbacks.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class HttpSystemCallback
                : public IHttpCallback
        {
        public:
            HttpSystemCallback();
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info);
        };
    }
}
