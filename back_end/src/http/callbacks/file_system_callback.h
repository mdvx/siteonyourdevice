#pragma once

#include "http/callbacks/http_callbacks.h"

namespace fasto
{
    namespace fastoremote
    {
        class HttpFileSystemCallback
                : public IHttpCallback
        {
        public:
            HttpFileSystemCallback();
            virtual bool handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request);
        };
    }
}
