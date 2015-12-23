#include "http/http_server_handler.h"

#include "http/http_client.h"

#include "server/server_config.h"

#include "common/utils.h"
#include "common/string_util.h"
#include "common/logger.h"

#define BUF_SIZE 4096
#define AUTH_BASIC_METHOD "Basic"

namespace fasto
{
    namespace siteonyourdevice
    {
        IHttpAuthObserver::IHttpAuthObserver()
        {

        }

        IHttpAuthObserver::~IHttpAuthObserver()
        {

        }

        void HttpServerHandler::preLooped(ITcpLoop *server)
        {

        }

        void HttpServerHandler::accepted(TcpClient* client)
        {

        }

        void HttpServerHandler::moved(TcpClient* client)
        {

        }

        void HttpServerHandler::closed(TcpClient* client)
        {

        }

        void HttpServerHandler::dataReceived(TcpClient* client)
        {
            char buff[BUF_SIZE] = {0};
            ssize_t nread = 0;
            common::Error err = client->read(buff, BUF_SIZE, nread);
            if((err && err->isError()) || nread == 0){
                client->close();
                delete client;
                return;
            }

            HttpClient* hclient = dynamic_cast<HttpClient*>(client);
            CHECK(hclient);
            processReceived(hclient, buff, nread);
        }

        void HttpServerHandler::postLooped(ITcpLoop *server)
        {

        }

        void HttpServerHandler::registerHttpCallback(const std::string& url, http_callback_t callback)
        {
            httpCallbacks_[url] = callback;
        }

        void HttpServerHandler::unRegisterHttpCallback(const std::string& url)
        {
            http_callbacks_t::const_iterator it = httpCallbacks_.find(url);
            httpCallbacks_.erase(it, httpCallbacks_.end());
        }

        void HttpServerHandler::clearHttpCallback()
        {
            httpCallbacks_.clear();
        }

        void HttpServerHandler::setAuthChecker(IHttpAuthObserver *observer)
        {
            authChecker_ = observer;
        }

        const HttpServerInfo& HttpServerHandler::info() const
        {
            return info_;
        }

        bool HttpServerHandler::tryToHandleAsRegisteredCallback(HttpClient* hclient, const std::string& uri, const common::http::http_request& request)
        {
            http_callbacks_t::const_iterator it = httpCallbacks_.find(uri);
            if(it == httpCallbacks_.end()){
                return false;
            }

            http_callback_t callback = (*it).second;
            if(!callback){
                NOTREACHED();
                return false;
            }

            return callback->handleRequest(hclient, NULL, request, info());
        }

        bool HttpServerHandler::tryAuthenticateIfNeeded(HttpClient* hclient, const char* extra_header, const common::http::http_request& request)
        {
            if(!authChecker_){
                return false;
            }

            if(hclient->isAuthenticated()){
                return false;
            }

            using namespace common;

            http::http_request::header_t authField = request.findHeaderByKey("Authorization", false);
            if(authField.isValid()){
                std::string auth = authField.value_;
                size_t delem_method = auth.find_first_of(' ');
                if(delem_method != std::string::npos){
                    const std::string method = auth.substr(0, delem_method);
                    if(method == AUTH_BASIC_METHOD){
                        const std::string enc_auth = common::utils::base64::decode64(auth.substr(delem_method + 1));

                        size_t delem = enc_auth.find_first_of(':');
                        if(delem != std::string::npos){
                            const std::string name = enc_auth.substr(0, delem);
                            const std::string password = enc_auth.substr(delem + 1);
                            if(authChecker_->userCanAuth(name, password)){
                                hclient->setIsAuthenticated(true);
                                return false;
                            }
                        }
                    }
                }

                common::Error err = hclient->send_error(http::HP_1_1, http::HS_UNAUTHORIZED, "WWW-Authenticate: " AUTH_BASIC_METHOD " realm=User or password incorrect, try again", NULL, true, info());
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                return true;
            }

            common::Error err = hclient->send_error(http::HP_1_1, http::HS_UNAUTHORIZED, "WWW-Authenticate: " AUTH_BASIC_METHOD " realm=Private page please authenticate", NULL, true, info());
            if(err && err->isError()){
                DEBUG_MSG_ERROR(err);
            }

            return true;
        }

        void HttpServerHandler::processReceived(HttpClient *hclient, const char* request, uint32_t req_len)
        {
            using namespace common;

            http::http_request hrequest;
            std::pair<http::http_status, common::Error> result = http::parse_http_request(std::string(request, req_len), hrequest);

            if(result.second && result.second->isError()){
                const std::string error_text = result.second->description();
                DEBUG_MSG_ERROR(result.second);
                common::Error err = hclient->send_error(http::HP_1_1, result.first, NULL, error_text.c_str(), false, info());
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                hclient->close();
                delete hclient;
                return;
            }

            //keep alive
            http::http_request::header_t connectionField = hrequest.findHeaderByKey("Connection", false);
            bool isKeepAlive = EqualsASCII(connectionField.value_, "Keep-Alive", false);

            http::http_request::header_t hostField = hrequest.findHeaderByKey("Host", false);
            bool isProxy = EqualsASCII(hostField.value_, HTTP_PROXY_HOST_NAME, false);

            handleRequest(hclient, hrequest, isKeepAlive | isProxy);
        }

        void HttpServerHandler::handleRequest(HttpClient *hclient, const common::http::http_request& hrequest, bool notClose)
        {
            common::uri::Upath path = hrequest.path_;

            if(tryAuthenticateIfNeeded(hclient, NULL, hrequest)){
                goto cleanup;
            }

            if(tryToHandleAsRegisteredCallback(hclient, path.path(), hrequest)){
                goto cleanup;
            }

            if(fshandler_->handleRequest(hclient, NULL, hrequest, info())){
                goto cleanup;
            }

            NOTREACHED();

        cleanup:
            if(!notClose){
                hclient->close();
                delete hclient;
            }
        }

        HttpServerHandler::HttpServerHandler(const HttpServerInfo& info, IHttpAuthObserver *observer)
            : fshandler_(IHttpCallback::createHttpCallback(file_system)), authChecker_(observer), info_(info)
        {
            CHECK(fshandler_);
        }

        HttpServerHandler::~HttpServerHandler()
        {
        }

        Http2ServerHandler::Http2ServerHandler(const HttpServerInfo& info, IHttpAuthObserver * observer)
            : HttpServerHandler(info, observer)
        {

        }

        void Http2ServerHandler::processReceived(HttpClient *hclient, const char* request, uint32_t req_len)
        {
            using namespace common;            
            if(http2::is_preface_data(request, req_len)){
                Http2Client* h2client = dynamic_cast<Http2Client*>(hclient);
                CHECK(h2client);

                const char* settings_frame = request + PREFACE_STARTS_LEN;
                uint32_t set_lengh = req_len - PREFACE_STARTS_LEN;
                handleHttp2Request(h2client, settings_frame, set_lengh);
            }
            else if(http2::is_frame_header_data(request, req_len)){
                Http2Client* h2client = dynamic_cast<Http2Client*>(hclient);
                CHECK(h2client);

                handleHttp2Request(h2client, request, req_len);
            }
            else{
                HttpServerHandler::processReceived(hclient, request, req_len);
            }
        }

        void Http2ServerHandler::handleHttp2Request(Http2Client* h2client, const char* request, uint32_t req_len)
        {
            const std::string hexstr = common::HexEncode(request, req_len, false);

            using namespace common;

            http2::frames_t frames = http2::parse_frames(request, req_len);
            DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "frame_header_data hex: %s", hexstr);
            h2client->processFrames(frames);

            http2::frames_t headers_frames = http2::find_frames_by_type(frames, http2::HTTP2_HEADERS);
            for(int i = 0; i < headers_frames.size(); ++i){
                common::http2::frame_headers* head = (common::http2::frame_headers*)(&headers_frames[i]);
                common::http::http_request request;
                std::pair<common::http::http_status, common::Error> result = common::http2::parse_http_request(*head, request);
                if(result.second && result.second->isError()){
                    const std::string error_text = result.second->description();
                    DEBUG_MSG_ERROR(result.second);
                    common::Error err = h2client->send_error(http::HP_2_0, result.first, NULL, error_text.c_str(), false, info());
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                    h2client->close();
                    delete h2client;
                    return;
                }

                handleRequest(h2client, request, true);
            }
        }
    }
}
