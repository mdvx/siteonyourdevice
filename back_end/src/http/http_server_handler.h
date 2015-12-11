#pragma once

#include "common/http/http.h"

#include "tcp_server.h"
#include "http/callbacks/http_callbacks.h"

namespace fasto
{
    namespace fastoremote
    {
        class Http2Client;

        class IHttpAuthObserver
        {
        public:
            IHttpAuthObserver();
            virtual bool userCanAuth(const std::string& user, const std::string& password) = 0;
            virtual ~IHttpAuthObserver();
        };

        class HttpServerHandler
                : public TcpServerObserver
        {
        public:
            typedef common::shared_ptr<IHttpCallback> http_callback_t;
            typedef std::map<std::string, http_callback_t> http_callbacks_t;

            HttpServerHandler(IHttpAuthObserver * observer);
            virtual void preLooped(TcpServer* server);
            virtual void accepted(fasto::fastoremote::TcpClient* client);
            virtual void moved(TcpClient* client);
            virtual void closed(fasto::fastoremote::TcpClient* client);
            virtual void dataReceived(fasto::fastoremote::TcpClient* client);
            virtual void postLooped(TcpServer* server);
            virtual ~HttpServerHandler();

            void registerHttpCallback(const std::string& url, http_callback_t callback);
            void unRegisterHttpCallback(const std::string& url);
            void clearHttpCallback();

            void setAuthChecker(IHttpAuthObserver *observer);

        protected:
            virtual void processReceived(HttpClient *hclient, const char* request, uint32_t req_len);
            void handleRequest(HttpClient *hclient, const common::http::http_request& hrequest, bool notClose);

        private:
            bool tryToHandleAsRegisteredCallback(HttpClient* hclient, const std::string& uri, const common::http::http_request& request);
            bool tryAuthenticateIfNeeded(HttpClient* hclient, const char* extra_header, const common::http::http_request& request);

            http_callbacks_t httpCallbacks_;
            const common::shared_ptr<IHttpCallback> fshandler_;
            IHttpAuthObserver * authChecker_;
        };

        class Http2ServerHandler
                : public HttpServerHandler
        {
        public:
            Http2ServerHandler(IHttpAuthObserver * observer);

        protected:
            virtual void processReceived(HttpClient *hclient, const char* request, uint32_t req_len);

        private:
            void handleHttp2Request(Http2Client* h2client, const char* request, uint32_t req_len);
        };
    }
}
