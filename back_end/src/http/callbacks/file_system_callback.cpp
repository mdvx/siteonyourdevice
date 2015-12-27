#include "http/callbacks/file_system_callback.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "common/string_util.h"
#include "common/http/http.h"
#include "common/logger.h"

#include "http/http_client.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        HttpFileSystemCallback::HttpFileSystemCallback()
            : HttpCallbackUrl(file_system)
        {

        }

        bool HttpFileSystemCallback::handleRequest(HttpClient* hclient, const char* extra_header, const common::http::http_request& request, const HttpServerInfo& info)
        {
            std::string requeststr = common::convertToString(request);
            DEBUG_MSG_FORMAT<1024>(common::logging::L_INFO, "handleRequest:\n%s", requeststr);
            using namespace common::http;
            //keep alive
            http_request::header_t connectionField = request.findHeaderByKey("Connection",false);
            bool isKeepAlive = EqualsASCII(connectionField.value_, "Keep-Alive", false);
            const http_protocols protocol = request.protocol();

            if(request.method_ == http_method::HM_GET || request.method_ == http_method::HM_HEAD){
                common::uri::Upath path = request.path_;
                if(!path.isValid() || path.isRoot()){
                    path = common::uri::Upath("index.html");
                }

                const std::string file_path = path.path();
                int open_flags = O_RDONLY;
    #ifdef COMPILER_MINGW
                open_flags |= O_BINARY;
    #endif
                struct stat sb;
                if (stat(file_path.c_str(), &sb ) < 0){
                    common::Error err = hclient->send_error(protocol, HS_NOT_FOUND, extra_header, "File not found.", isKeepAlive, info);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                    return true;
                }

                if (S_ISDIR(sb.st_mode)){
                    common::Error err = hclient->send_error(protocol, HS_BAD_REQUEST, extra_header, "Bad filename.", isKeepAlive, info);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                    return true;
                }

                int file = open(file_path.c_str(), open_flags);
                if(file == INVALID_DESCRIPTOR) {  /* open the file for reading */
                    common::Error err = hclient->send_error(protocol, HS_FORBIDDEN, NULL, "File is protected.", isKeepAlive, info);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                    return true;
                }

                const std::string mime = path.mime();
                common::Error err = hclient->send_headers(protocol, HS_OK, NULL, mime.c_str(), &sb.st_size, &sb.st_mtime, isKeepAlive, info);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                    ::close(file);
                    return true;
                }

                if(request.method_ == http_method::HM_GET){
                    common::Error err = hclient->send_file_by_fd(protocol, file, sb.st_size);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }
                ::close(file);
                return true;
            }
            else{
                http_request::header_t contentTypeField = request.findHeaderByKey("Content-Type", false);
                if(!contentTypeField.isValid()){
                    common::Error err = hclient->send_error(protocol, HS_NOT_ALLOWED, NULL, "Unsupported request.", isKeepAlive, info);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                    return true;
                }

                const std::string contentTypeValue = contentTypeField.value_;
                if(contentTypeValue == "application/json"){
                    return true;
                }
                else if(contentTypeValue == "application/x-www-form-urlencoded"){
                    return true;
                }
                else if(contentTypeValue == "multipart/form-data"){
                    return true;
                }
                else{
                    common::Error err = hclient->send_error(protocol, HS_NOT_ALLOWED, NULL, "Unsupported content type.", isKeepAlive, info);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                    return true;
                }
            }
        }

        common::shared_ptr<IHttpCallback> createFileSystemHttpCallback(const std::string& name)
        {
            return common::shared_ptr<IHttpCallback>(new HttpFileSystemCallback);
        }
    }
}
