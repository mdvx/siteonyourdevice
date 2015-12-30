#pragma once

#include "common/http/http.h"

#include "tcp/tcp_server.h"
#include "http/callbacks/http_callbacks.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class Http2Client;
        class ILoopController;

        class IHttpAuthObserver
        {
        public:
            IHttpAuthObserver();
            virtual bool userCanAuth(const std::string& user, const std::string& password) = 0;
            virtual ~IHttpAuthObserver();
        };

        class HttpServerHandler
                : public ITcpLoopObserver
        {
        public:
            typedef common::shared_ptr<IHttpCallback> http_callback_t;
            typedef std::map<std::string, http_callback_t> http_callbacks_t;

            typedef std::pair<common::uri::Uri, ILoopController*> socket_url_t;
            typedef std::vector<socket_url_t> sockets_url_t;

            HttpServerHandler(const HttpServerInfo& info, IHttpAuthObserver * observer);
            virtual void preLooped(ITcpLoop* server);
            virtual void accepted(TcpClient* client);
            virtual void moved(TcpClient* client);
            virtual void closed(TcpClient* client);
            virtual void dataReceived(TcpClient* client);
            virtual void dataReadyToWrite(TcpClient* client);
            virtual void postLooped(ITcpLoop* server);
            virtual void timerEmited(ITcpLoop* server, timer_id_type id);

            virtual ~HttpServerHandler();

            void registerHttpCallback(const std::string& url, http_callback_t callback);
            void registerSocketUrl(const common::uri::Uri& url);

            void setAuthChecker(IHttpAuthObserver * authChecker);

            const HttpServerInfo& info() const;

        protected:
            virtual void processReceived(HttpClient *hclient, const char* request, size_t req_len);
            virtual void handleRequest(HttpClient *hclient, const common::http::http_request& hrequest, bool notClose);

        private:
            bool tryToHandleAsRegisteredCallback(HttpClient* hclient, const std::string& uri, const common::http::http_request& request);
            bool tryAuthenticateIfNeeded(HttpClient* hclient, const char* extra_header, const common::http::http_request& request);


            http_callbacks_t httpCallbacks_;
            const common::shared_ptr<IHttpCallback> fshandler_;
            IHttpAuthObserver * authChecker_;

            sockets_url_t sockets_urls_;

            const HttpServerInfo info_;
        };

        class Http2ServerHandler
                : public HttpServerHandler
        {
        public:
            Http2ServerHandler(const HttpServerInfo &info, IHttpAuthObserver * observer);

        protected:
            virtual void processReceived(HttpClient *hclient, const char* request, size_t req_len);

        private:
            void handleHttp2Request(Http2Client* h2client, const char* request, uint32_t req_len);
        };
    }
}
