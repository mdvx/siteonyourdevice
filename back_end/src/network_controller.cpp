#include "network_controller.h"

#include "inih/ini.h"

#include "common/logger.h"
#include "common/file_system.h"
#include "common/thread/thread_manager.h"
#include "common/net/net.h"
#include "common/thread/event_bus.h"
#include "common/utils.h"

#include "fasto_application.h"

#include "server/server_config.h"

#include "inner/http_inner_server.h"
#include "inner/http_inner_server_handler.h"

/*
    [http_server]
    domain=fatoredis.com
    port=8080
    content_path=content
    login=atopilski
    password=1234
    private_site=0
    [http_handlers_utls]
*/

namespace fasto
{
    namespace fastoremote
    {
        namespace
        {
            int ini_handler(void* user, const char* section, const char* name, const char* value)
            {
                configuration_t* pconfig = (configuration_t*)user;

                #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
                if (MATCH("http_server", "port")) {
                    pconfig->port_ = atoi(value);
                    return 1;
                }
                else if (MATCH("http_server", "content_path")) {
                    const std::string contentPath = value;
                    pconfig->content_path_ = common::file_system::stable_dir_path(contentPath);
                    return 1;
                }
                else if (MATCH("http_server", "domain")) {
                    pconfig->domain_ = value;
                    return 1;
                }
                else if (MATCH("http_server", "login")) {
                    pconfig->login_ = value;
                    return 1;
                }
                else if (MATCH("http_server", "password")) {
                    pconfig->password_ = value;
                    return 1;
                }
                else if(MATCH("http_server", "private_site")){
                    pconfig->is_private_site_ = atoi(value);
                    return 1;
                }
                else if(strcmp(section, "http_handlers_utls") == 0){
                    pconfig->handlers_urls_.push_back(std::make_pair(name, value));
                    return 1;
                }
                else {
                    return 0;  /* unknown section/name, error */
                }
            }
        }

        void NetworkEventHandler::handleEvent(NetworkEvent *event)
        {
            if(event->eventType() == InnerClientConnectedEvent::EventType){
                //InnerClientConnectedEvent * ev = static_cast<InnerClientConnectedEvent*>(event);
            }
            else if(event->eventType() == InnerClientAutorizedEvent::EventType){
                //InnerClientAutorizedEvent * ev = static_cast<InnerClientAutorizedEvent*>(event);
            }
            else if(event->eventType() == InnerClientDisconnectedEvent::EventType){
                //InnerClientDisconnectedEvent * ev = static_cast<InnerClientDisconnectedEvent*>(event);
            }
            else{

            }
        }

        class NetworkEventHandler::NetworkListener
                : public common::IListener<NetworkEventTypes>
        {
            NetworkEventHandler * const app_;
        public:
            NetworkListener(NetworkEventHandler * app)
                : common::IListener<NetworkEventTypes>(), app_(app)
            {
                EVENT_BUS()->subscribe<InnerClientConnectedEvent>(this);
                EVENT_BUS()->subscribe<InnerClientDisconnectedEvent>(this);
                EVENT_BUS()->subscribe<InnerClientAutorizedEvent>(this);
            }

            ~NetworkListener()
            {
                EVENT_BUS()->unsubscribe<InnerClientAutorizedEvent>(this);
                EVENT_BUS()->unsubscribe<InnerClientDisconnectedEvent>(this);
                EVENT_BUS()->unsubscribe<InnerClientConnectedEvent>(this);
            }

            virtual void handleEvent(event_t* event)
            {
                app_->handleEvent(event);
            }
        };

        NetworkEventHandler::NetworkEventHandler(NetworkController *controller)
            : controller_(controller)
        {
            networkListener_ = new NetworkListener(this);
        }

        NetworkEventHandler::~NetworkEventHandler()
        {
            delete networkListener_;
        }

        int NetworkEventHandler::start()
        {
            common::Error err = controller_->connect();
            if(err && err->isError()){
            
            }
            return EXIT_SUCCESS;        
        }

        class NetworkController::HttpAuthObserver
                : public IHttpAuthObserver
        {
        public:
            HttpAuthObserver(Http2InnerServerHandler* servh)
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
            Http2InnerServerHandler* const servh_;
        };

        NetworkController::NetworkController(int argc, char *argv[])
            : handler_(NULL), server_(NULL),
              config_(), thread_(EVENT_BUS()->createEventThread<NetworkEventTypes>()),
              authChecker_(NULL)
        {
            Http2InnerServerHandler* servh = new Http2InnerServerHandler(g_inner_host);
            handler_ = servh;
            authChecker_ = new HttpAuthObserver(servh);
            handler_->setAuthChecker(authChecker_);

            bool daemon_mode = false;

            std::string config_path = CONFIG_FILE_NAME;

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
            delete authChecker_;

            delete server_;
            delete handler_;

            EVENT_BUS()->destroyEventThread(thread_);
            EVENT_BUS()->stop();

            saveConfig();
        }

        int NetworkController::exec()
        {
            if(http_thread_){    //if connect dosen't clicked
                return http_thread_->joinAndGet();
            }

            return EXIT_SUCCESS;
        }

        void NetworkController::exit(int result)
        {
            if(!server_){    //if connect dosen't clicked
                return;
            }

            server_->stop();
        }

        common::Error NetworkController::connect()
        {
            if(server_){
                return server_->innerConnect();
            }

            configuration_t config = config_;

            const common::net::hostAndPort hs(config.domain_, config.port_);
            const UserAuthInfo auth(config.login_, config.password_, hs);
            handler_->setAuthInfo(auth);
            handler_->clearHttpCallback();

            for(int i = 0; i < config.handlers_urls_.size(); ++i){
                configuration_t::handlers_urls_t handurl = config.handlers_urls_[i];
                common::shared_ptr<IHttpCallback> handler = IHttpCallback::createHttpCallback(handurl.second);
                if(handler){
                    handler_->registerHttpCallback(handurl.first, handler);
                }
            }

            server_ = new Http2InnerServer(hs, handler_, config_);
            server_->setHttpServerInfo(HttpServerInfo(PROJECT_NAME_TITLE, PROJECT_DOMAIN, config.content_path_));
            const std::string contentPath = config.content_path_;
            bool res = common::file_system::change_directory(contentPath);
            DCHECK(res);
            server_->setName("local_http_server");

            common::ErrnoError err = server_->bind();
            if(err && err->isError()){
                delete server_;
                server_ = NULL;
                return err;
            }

            err = server_->listen(5);
            if(err && err->isError()){
                delete server_;
                server_ = NULL;
                return err;
            }

            http_thread_ = THREAD_MANAGER()->createThread(&Http2Server::exec, server_);
            http_thread_->start();

            return common::Error();
        }

        common::Error NetworkController::disConnect()
        {
            if(!server_){    //if connect dosen't clicked
                return common::Error();
            }

            common::Error err = server_->innerDisConnect();
            if(err && err->isError()){
                return err;
            }

            const std::string appdir = fApp->appDir();
            bool res = common::file_system::change_directory(appdir);
            DCHECK(res);

            return common::Error();
        }

        configuration_t NetworkController::config() const
        {
            return config_;
        }

        void NetworkController::setConfig(const configuration_t& config)
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

            configSave.write("[http_server]\n");
            configSave.writeFormated("domain=%s\n", config_.domain_);
            configSave.writeFormated("port=%u\n", config_.port_);
            configSave.writeFormated("login=%s\n", config_.login_);
            configSave.writeFormated("password=%s\n", config_.password_);
            configSave.writeFormated("content_path=%s\n", config_.content_path_);
            configSave.writeFormated("private_site=%u\n", config_.is_private_site_);
            configSave.write("[http_handlers_utls]\n");
            configSave.close();
        }

        void NetworkController::readConfig()
        {
            const char* path = config_path_.c_str();

            configuration_t config;
            //default settings
            config.port_ = USER_SPECIFIC_DEFAULT_PORT;
            config.domain_ = USER_SPECIFIC_DEFAULT_DOMAIN;
            config.content_path_ = fApp->appDir();
            config.login_ = USER_SPECIFIC_DEFAULT_LOGIN;
            config.password_ = USER_SPECIFIC_DEFAULT_PASSWORD;
            config.is_private_site_ = USER_SPECIFIC_DEFAULT_PRIVATE_SITE;

            //try to parse settings file
            if (ini_parse(path, ini_handler, &config) < 0) {
                DEBUG_MSG_FORMAT<128>(common::logging::L_INFO, "Can't load config '%s', use default settings.", path);
            }

            config_ = config;
        }
    }
}