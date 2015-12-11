#include "http/callbacks/system_callback.h"

#include "common/string_util.h"
#include "common/http/http.h"

#include "http/http_client.h"

namespace fasto
{
    namespace fastoremote
    {
        HttpSystemCallback::HttpSystemCallback()
            : IHttpCallback(system)
        {

        }

        bool HttpSystemCallback::handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request)
        {
            using namespace common::http;
            common::uri::Upath path = request.path_;
            if(!path.isValid()){
                return false;
            }

            const std::string ppath =  path.path();
            if(ppath.empty()){
                return false;
            }

            return false;
        }
    }
}
