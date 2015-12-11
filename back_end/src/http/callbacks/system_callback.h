#pragma once

#include "http/callbacks/http_callbacks.h"

namespace fasto
{
    namespace fastoremote
    {
        class HttpSystemCallback
                : public IHttpCallback
        {
        public:
            HttpSystemCallback();
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request);
        };
    }
}
