#include "network_controller.h"

#include "inih/ini.h"

#include "common/file_system.h"
#include "common/logger.h"
#include "common/utils.h"
#include "common/thread/event_bus.h"

#include "loop_controller.h"

#include "application/fasto_application.h"

#include "server/server_config.h"

#include "inner/http_inner_server.h"
#include "inner/http_inner_server_handler.h"

#include "network/network_events.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace
        {
            int ini_handler_fasto(void* user, const char* section, const char* name, const char* value)
            {
                HttpConfig* pconfig = (HttpConfig*)user;

                #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
                if (MATCH(SERVER_SETTINGS_SECTION_LABEL, CONTENT_PATH_SETTING_LABEL)) {
                    const std::string contentPath = value;
                    pconfig->content_path_ = common::file_system::stable_dir_path(contentPath);
                    return 1;
                }
                else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, LOCAL_HOST_SETTING_LABEL)) {
                    pconfig->local_host_ = common::convertFromString<common::net::hostAndPort>(value);
                    return 1;
                }
                else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, LOGIN_SETTING_LABEL)) {
                    pconfig->login_ = value;
                    return 1;
                }
                else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, PASSWORD_SETTING_LABEL)) {
                    pconfig->password_ = value;
                    return 1;
                }
                else if(MATCH(SERVER_SETTINGS_SECTION_LABEL, PRIVATE_SITE_SETTING_LABEL)){
                    pconfig->is_private_site_ = atoi(value);
                    return 1;
                }
                else if(MATCH(SERVER_SETTINGS_SECTION_LABEL, EXTERNAL_HOST_SETTING_LABEL)){
                    pconfig->external_host_ = common::convertFromString<common::net::hostAndPort>(value);
                    return 1;
                }
                else if(MATCH(SERVER_SETTINGS_SECTION_LABEL, SERVER_TYPE_SETTING_LABEL)){
                    pconfig->server_type_ = (http_server_type)atoi(value);
                    return 1;
                }
                else if(strcmp(section, HANDLERS_URLS_SECTION_LABEL) == 0){
                    pconfig->handlers_urls_.push_back(std::make_pair(name, value));
                    return 1;
                }
                else if(strcmp(section, SERVER_SOCKETS_SECTION_LABEL) == 0){
                    common::uri::Uri uri = common::uri::Uri(value);
                    pconfig->server_sockets_urls_.push_back(std::make_pair(name, uri));
                    return 1;
                }
                else {
                    return 0;  /* unknown section/name, error */
                }
            }
        }

        class HttpAuthObserver
                : public http::IHttpAuthObserver
        {
        public:
            HttpAuthObserver(inner::Http2InnerServerHandler* servh)
                : servh_(servh)
            {

            }

            virtual bool userCanAuth(const std::string& user, const std::string& password)
            {
                if(user.empty()){
                    return false;
                }

                if(password.empty()){
                    return false;
                }

                const UserAuthInfo& ainf = servh_->authInfo();
                if(!ainf.isValid()){
                    return false;
                }

                return ainf.login_ == user && ainf.password_ == password;
            }

        private:
            inner::Http2InnerServerHandler* const servh_;
        };

        namespace
        {
            class ServerControllerBase
                : public ILoopThreadController
            {
            public:
                ServerControllerBase(const HttpConfig& config, const UserAuthInfo& ainfo)
                    : config_(config), authChecker_(NULL), ainfo_(ainfo)
                {

                }

                ~ServerControllerBase()
                {
                    delete authChecker_;
                }

            protected:
                const HttpConfig config_;
                const UserAuthInfo ainfo_;

            private:
                tcp::ITcpLoopObserver * createHandler()
                {
                    HttpServerInfo hs(PROJECT_NAME_TITLE, PROJECT_DOMAIN);
                    inner::Http2InnerServerHandler* handler = new inner::Http2InnerServerHandler(hs, g_inner_host, config_);
                    authChecker_ = new HttpAuthObserver(handler);
                    handler->setAuthChecker(authChecker_);

                    // handler prepare
                    for(size_t i = 0; i < config_.handlers_urls_.size(); ++i){
                        HttpConfig::handlers_urls_t handurl = config_.handlers_urls_[i];
                        const std::string httpcallbackstr = handurl.second;
                        std::string httpcallback_ns = handurl.second;
                        std::string httpcallback_name;
                        std::string::size_type ns_del = httpcallbackstr.find_first_of("::");
                        if(ns_del != std::string::npos){
                            httpcallback_ns = httpcallbackstr.substr(0, ns_del);
                            httpcallback_name = httpcallbackstr.substr(ns_del + 2);
                        }

                        common::shared_ptr<IHttpCallback> hhandler = IHttpCallback::createHttpCallback(httpcallback_ns, httpcallback_name);
                        if(hhandler){
                            handler->registerHttpCallback(handurl.first, hhandler);
                        }
                    }

                    for(size_t i = 0; i < config_.server_sockets_urls_.size(); ++i){
                        HttpConfig::server_sockets_urls_t sock_url = config_.server_sockets_urls_[i];
                        const common::uri::Uri url = sock_url.second;
                        handler->registerSocketUrl(url);
                    }

                    // handler prepare
                    return handler;
                }

                HttpAuthObserver* authChecker_;
            };

            class LocalHttpServerController
                    : public ServerControllerBase
            {
            public:
                LocalHttpServerController(const HttpConfig& config, const UserAuthInfo& ainfo)
                    : ServerControllerBase(config, ainfo)
                {
                    const std::string contentPath = config.content_path_;
                    common::Error err = common::file_system::change_directory(contentPath);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }

                ~LocalHttpServerController()
                {
                    const std::string appdir = fApp->appDir();
                    common::Error err = common::file_system::change_directory(appdir);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }

           private:
                tcp::ITcpLoop * createServer(tcp::ITcpLoopObserver * handler)
                {
                    inner::Http2InnerServer* serv = new inner::Http2InnerServer(handler, config_);
                    serv->setName("local_http_server");

                    common::Error err = serv->bind();
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                        EVENT_BUS()->postEvent(make_exception_event(new network::InnerClientConnectedEvent(this, ainfo_), err));
                        delete serv;
                        return NULL;
                    }

                    err = serv->listen(5);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                        EVENT_BUS()->postEvent(make_exception_event(new network::InnerClientConnectedEvent(this, ainfo_), err));
                        delete serv;
                        return NULL;
                    }

                    return serv;
                }
            };

            class ExternalHttpServerController
                    : public ServerControllerBase
            {
            public:
                ExternalHttpServerController(const HttpConfig& config, const UserAuthInfo& ainfo)
                    : ServerControllerBase(config, ainfo)
                {

                }

           private:
                tcp::ITcpLoop * createServer(tcp::ITcpLoopObserver * handler)
                {
                    inner::ProxyInnerServer* serv = new inner::ProxyInnerServer(handler);
                    serv->setName("proxy_http_server");
                    return serv;
                }
            };
        }

        namespace network
        {
            NetworkController::NetworkController(int argc, char *argv[])
                : server_mutex_(), server_(NULL), config_(), thread_(EVENT_BUS()->createEventThread<NetworkEventTypes>())
            {
                bool daemon_mode = false;
#ifdef OS_MACOSX
                std::string config_path = PROJECT_NAME ".app/Contents/Resources/" CONFIG_FILE_NAME;
#else
                std::string config_path = CONFIG_FILE_NAME;
#endif
                for (int i = 0; i < argc; i++) {
                    if (strcmp(argv[i], "-c") == 0) {
                        config_path = argv[++i];
                    }
                    else if(strcmp(argv[i], "-d") == 0){
                        daemon_mode = true;
                    }
                }

                config_path_ = config_path;
#if defined(BUILD_CONSOLE) && defined(OS_POSIX)
                if(daemon_mode){
                    common::create_as_daemon();
                }
#endif
                readConfig();
            }

            NetworkController::~NetworkController()
            {
                EVENT_BUS()->destroyEventThread(thread_);
                EVENT_BUS()->stop();

                delete server_;

                saveConfig();
            }

            int NetworkController::exec()
            {
                using namespace common::multi_threading;
                unique_lock<mutex_t> lock(server_mutex_);

                if(server_){    //if connect dosen't clicked
                    return server_->join();
                }

                return EXIT_SUCCESS;
            }

            void NetworkController::exit(int result)
            {
                using namespace common::multi_threading;
                unique_lock<mutex_t> lock(server_mutex_);

                if(!server_){    //if connect dosen't clicked
                    return;
                }

                server_->stop();
            }

            void NetworkController::connect()
            {
                using namespace common::multi_threading;
                unique_lock<mutex_t> lock(server_mutex_);

                if(server_){    //if connected
                    common::Error err = common::make_error_value("Failed, double connection!", common::Value::E_ERROR, common::logging::L_ERR);
                    EVENT_BUS()->postEvent(make_exception_event(new InnerClientConnectedEvent(this, authInfo()), err));
                    return;
                }

                const http_server_type server_type = config_.server_type_;
                const common::net::hostAndPort externalHost = config_.external_host_;
                if(server_type == FASTO_SERVER){
                    server_ = new LocalHttpServerController(config_, authInfo());
                    server_->start();
                }
                else if(server_type == EXTERNAL_SERVER && externalHost.isValid()) {
                    server_ = new ExternalHttpServerController(config_, authInfo());
                    server_->start();
                }
                else{
                    common::Error err = common::make_error_value("Invalid https server settings!", common::Value::E_ERROR, common::logging::L_ERR);
                    EVENT_BUS()->postEvent(make_exception_event(new InnerClientConnectedEvent(this, authInfo()), err));
                }
            }

            void NetworkController::disConnect()
            {
                using namespace common::multi_threading;
                unique_lock<mutex_t> lock(server_mutex_);

                if(!server_){    //if connect dosen't clicked
                    common::Error err = common::make_error_value("Failed, not connected!", common::Value::E_ERROR, common::logging::L_ERR);
                    EVENT_BUS()->postEvent(make_exception_event(new InnerClientDisconnectedEvent(this, authInfo()), err));
                    return;
                }

                server_->stop();
                delete server_;
                server_ = NULL;
            }

            UserAuthInfo NetworkController::authInfo() const
            {
                return UserAuthInfo(config_.login_, config_.password_, config_.local_host_);
            }

            HttpConfig NetworkController::config() const
            {
                return config_;
            }

            void NetworkController::setConfig(const HttpConfig& config)
            {
                config_ = config;
            }

            void NetworkController::saveConfig()
            {
                common::file_system::Path configPath(config_path_);
                common::file_system::File configSave(configPath);
                if(!configSave.open("w")){
                    return;
                }

                configSave.write("[" SERVER_SETTINGS_SECTION_LABEL "]\n");
                configSave.writeFormated(LOCAL_HOST_SETTING_LABEL "=%s\n", common::convertToString(config_.local_host_));
                configSave.writeFormated(LOGIN_SETTING_LABEL "=%s\n", config_.login_);
                configSave.writeFormated(PASSWORD_SETTING_LABEL "=%s\n", config_.password_);
                configSave.writeFormated(CONTENT_PATH_SETTING_LABEL "=%s\n", config_.content_path_);
                configSave.writeFormated(PRIVATE_SITE_SETTING_LABEL "=%u\n", config_.is_private_site_);
                configSave.writeFormated(EXTERNAL_HOST_SETTING_LABEL "=%s\n", common::convertToString(config_.external_host_));
                configSave.writeFormated(SERVER_TYPE_SETTING_LABEL "=%u\n", config_.server_type_);
                configSave.write("[" HANDLERS_URLS_SECTION_LABEL "]\n");
                for(int i = 0; i < config_.handlers_urls_.size(); ++i){
                    HttpConfig::handlers_urls_t handurl = config_.handlers_urls_[i];
                    configSave.writeFormated("%s=%s\n", handurl.first, handurl.second);
                }
                configSave.write("[" SERVER_SOCKETS_SECTION_LABEL "]\n");
                for(int i = 0; i < config_.server_sockets_urls_.size(); ++i){
                    HttpConfig::server_sockets_urls_t sock_url = config_.server_sockets_urls_[i];
                    const std::string url = sock_url.second.get_url();
                    configSave.writeFormated("%s=%s\n", sock_url.first, url);
                }
                configSave.close();
            }

            void NetworkController::readConfig()
            {
#ifdef OS_MACOSX
                const std::string spath = fApp->appDir() + config_path_;
                const char* path = spath.c_str();
#else
                const char* path = config_path_.c_str();
#endif

                HttpConfig config;
                //default settings
                config.local_host_ = common::net::hostAndPort(USER_SPECIFIC_DEFAULT_DOMAIN, USER_SPECIFIC_DEFAULT_PORT);
                config.content_path_ = fApp->appDir();
                config.login_ = USER_SPECIFIC_DEFAULT_LOGIN;
                config.password_ = USER_SPECIFIC_DEFAULT_PASSWORD;
                config.is_private_site_ = USER_SPECIFIC_DEFAULT_PRIVATE_SITE;
                config.external_host_ = common::net::hostAndPort("localhost", 80);
                config.server_type_ = FASTO_SERVER;

                //try to parse settings file
                if (ini_parse(path, ini_handler_fasto, &config) < 0) {
                    DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Can't load config '%s', use default settings.", path);
                }

                config_ = config;
            }
        }
    }
}
