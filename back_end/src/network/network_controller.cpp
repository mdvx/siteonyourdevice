/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "network_controller.h"

#include <string>

#include "inih/ini.h"

#include "common/file_system.h"
#include "common/logger.h"
#include "common/utils.h"
#include "common/thread/event_bus.h"

#include "inner/http_inner_server.h"
#include "inner/http_inner_server_handler.h"

#include "loop_controller.h"

#include "application/fasto_application.h"

#include "server/server_config.h"

#include "network/network_events.h"

namespace {

typedef common::multi_threading::unique_lock<common::multi_threading::mutex_t> lock_t;

int ini_handler_fasto(void* user, const char* section, const char* name, const char* value) {
  fasto::siteonyourdevice::HttpConfig* pconfig = reinterpret_cast<fasto::siteonyourdevice::HttpConfig*>(user);

  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH(SERVER_SETTINGS_SECTION_LABEL, CONTENT_PATH_SETTING_LABEL)) {
    const std::string contentPath = value;
    pconfig->content_path = common::file_system::stable_dir_path(contentPath);
    return 1;
  } else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, LOCAL_HOST_SETTING_LABEL)) {
    pconfig->local_host = common::convertFromString<common::net::hostAndPort>(value);
    return 1;
  } else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, LOGIN_SETTING_LABEL)) {
    pconfig->login = value;
    return 1;
  } else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, PASSWORD_SETTING_LABEL)) {
    pconfig->password = value;
    return 1;
  } else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, PRIVATE_SITE_SETTING_LABEL)) {
    pconfig->is_private_site = atoi(value);
    return 1;
  } else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, EXTERNAL_HOST_SETTING_LABEL)) {
    pconfig->external_host = common::convertFromString<common::net::hostAndPort>(value);
    return 1;
  } else if (MATCH(SERVER_SETTINGS_SECTION_LABEL, SERVER_TYPE_SETTING_LABEL)) {
    pconfig->server_type = (fasto::siteonyourdevice::http_server_type)atoi(value);
    return 1;
  } else if (strcmp(section, HANDLERS_URLS_SECTION_LABEL) == 0) {
    pconfig->handlers_urls.push_back(std::make_pair(name, value));
    return 1;
  } else if (strcmp(section, SERVER_SOCKETS_SECTION_LABEL) == 0) {
    common::uri::Uri uri = common::uri::Uri(value);
    pconfig->server_sockets_urls.push_back(std::make_pair(name, uri));
    return 1;
  } else {
    return 0;  /* unknown section/name, error */
  }
}

class HttpAuthObserver
  : public fasto::siteonyourdevice::http::IHttpAuthObserver {
 public:
  explicit HttpAuthObserver(fasto::siteonyourdevice::inner::Http2InnerServerHandler* servh)
    : servh_(servh) {
  }

  virtual bool userCanAuth(const std::string& user, const std::string& password) {
    if (user.empty()) {
        return false;
    }

    if (password.empty()) {
        return false;
    }

    const fasto::siteonyourdevice::UserAuthInfo& ainf = servh_->authInfo();
    if (!ainf.isValid()) {
       return false;
    }

    return ainf.login == user && ainf.password == password;
  }

 private:
  fasto::siteonyourdevice::inner::Http2InnerServerHandler* const servh_;
};

class ServerControllerBase
  : public fasto::siteonyourdevice::ILoopThreadController {
 public:
  ServerControllerBase(const fasto::siteonyourdevice::HttpConfig& config,
                       const fasto::siteonyourdevice::UserAuthInfo& ainfo)
    : config_(config), ainfo_(ainfo), authChecker_(NULL) {
  }

  ~ServerControllerBase() {
    delete authChecker_;
  }

 protected:
  const fasto::siteonyourdevice::HttpConfig config_;
  const fasto::siteonyourdevice::UserAuthInfo ainfo_;

 private:
  fasto::siteonyourdevice::tcp::ITcpLoopObserver * createHandler() {
    fasto::siteonyourdevice::HttpServerInfo hs(PROJECT_NAME_TITLE, PROJECT_DOMAIN);
    fasto::siteonyourdevice::inner::Http2InnerServerHandler* handler = new fasto::siteonyourdevice::inner::Http2InnerServerHandler(hs, g_inner_host, config_);
    authChecker_ = new HttpAuthObserver(handler);
    handler->setAuthChecker(authChecker_);

    // handler prepare
    for (size_t i = 0; i < config_.handlers_urls.size(); ++i) {
      fasto::siteonyourdevice::HttpConfig::handlers_urls_t handurl = config_.handlers_urls[i];
      const std::string httpcallbackstr = handurl.second;
      std::string httpcallback_ns = handurl.second;
      std::string httpcallback_name;
      std::string::size_type ns_del = httpcallbackstr.find_first_of("::");
      if (ns_del != std::string::npos) {
        httpcallback_ns = httpcallbackstr.substr(0, ns_del);
        httpcallback_name = httpcallbackstr.substr(ns_del + 2);
      }

      common::shared_ptr<fasto::siteonyourdevice::IHttpCallback> hhandler = fasto::siteonyourdevice::IHttpCallback::createHttpCallback(httpcallback_ns, httpcallback_name);
      if (hhandler) {
        handler->registerHttpCallback(handurl.first, hhandler);
      }
    }

    for (size_t i = 0; i < config_.server_sockets_urls.size(); ++i) {
      fasto::siteonyourdevice::HttpConfig::server_sockets_urls_t sock_url = config_.server_sockets_urls[i];
      const common::uri::Uri url = sock_url.second;
      handler->registerSocketUrl(url);
    }

    // handler prepare
    return handler;
  }

  HttpAuthObserver* authChecker_;
};

class LocalHttpServerController
        : public ServerControllerBase {
 public:
  LocalHttpServerController(const fasto::siteonyourdevice::HttpConfig& config,
                            const fasto::siteonyourdevice::UserAuthInfo& ainfo)
    : ServerControllerBase(config, ainfo) {
    common::Error err = common::file_system::change_directory(config.content_path);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }
  }

  ~LocalHttpServerController() {
    std::string appdir = fApp->appDir();
    common::Error err = common::file_system::change_directory(appdir);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }
  }

 private:
  fasto::siteonyourdevice::tcp::ITcpLoop * createServer(fasto::siteonyourdevice::tcp::ITcpLoopObserver * handler) {
    fasto::siteonyourdevice::inner::Http2InnerServer* serv = new fasto::siteonyourdevice::inner::Http2InnerServer(handler, config_);
    serv->setName("local_http_server");

    common::Error err = serv->bind();
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
      auto ex_event = make_exception_event(new fasto::siteonyourdevice::network::InnerClientConnectedEvent(this, ainfo_), err);
      EVENT_BUS()->postEvent(ex_event);
      delete serv;
      return NULL;
    }

    err = serv->listen(5);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
      auto ex_event = make_exception_event(new fasto::siteonyourdevice::network::InnerClientConnectedEvent(this, ainfo_), err);
      EVENT_BUS()->postEvent(ex_event);
      delete serv;
      return NULL;
    }

    return serv;
  }
};

class ExternalHttpServerController
  : public ServerControllerBase {
 public:
  ExternalHttpServerController(const fasto::siteonyourdevice::HttpConfig& config,
                               const fasto::siteonyourdevice::UserAuthInfo& ainfo)
    : ServerControllerBase(config, ainfo) {
  }

 private:
  fasto::siteonyourdevice::tcp::ITcpLoop * createServer(fasto::siteonyourdevice::tcp::ITcpLoopObserver * handler) {
    fasto::siteonyourdevice::inner::ProxyInnerServer* serv = new fasto::siteonyourdevice::inner::ProxyInnerServer(handler);
    serv->setName("proxy_http_server");
    return serv;
  }
};
}  // namespace

namespace fasto {
namespace siteonyourdevice {
namespace network {

NetworkController::NetworkController(int argc, char *argv[])
  : server_mutex_(), server_(NULL), config_(),
    thread_(EVENT_BUS()->createEventThread<NetworkEventTypes>()) {
  bool daemon_mode = false;
#ifdef OS_MACOSX
  std::string config_path = PROJECT_NAME ".app/Contents/Resources/" CONFIG_FILE_NAME;
#else
  std::string config_path = CONFIG_FILE_NAME;
#endif
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-c") == 0) {
      config_path = argv[++i];
    } else if (strcmp(argv[i], "-d") == 0) {
      daemon_mode = true;
    }
  }

  config_path_ = config_path;
#if defined(BUILD_CONSOLE) && defined(OS_POSIX)
  if (daemon_mode) {
    common::create_as_daemon();
  }
#endif
  readConfig();
}

NetworkController::~NetworkController() {
  EVENT_BUS()->destroyEventThread(thread_);
  EVENT_BUS()->stop();

  delete server_;

  saveConfig();
}

int NetworkController::exec() {
  lock_t lock(server_mutex_);

  if (server_) {  // if connect dosen't clicked
      return server_->join();
  }

  return EXIT_SUCCESS;
}

void NetworkController::exit(int result) {
  lock_t lock(server_mutex_);

  if (!server_) {  // if connect dosen't clicked
      return;
  }

  server_->stop();
}

void NetworkController::connect() {
  lock_t lock(server_mutex_);

  if (server_) {  // if connected
    common::Error err = common::make_error_value("Failed, double connection!",
                                                 common::Value::E_ERROR, common::logging::L_ERR);
    auto ex_event = make_exception_event(new InnerClientConnectedEvent(this, authInfo()), err);
    EVENT_BUS()->postEvent(ex_event);
    return;
  }

  const http_server_type server_type = config_.server_type;
  const common::net::hostAndPort externalHost = config_.external_host;
  if (server_type == FASTO_SERVER) {
     server_ = new LocalHttpServerController(config_, authInfo());
     server_->start();
  } else if (server_type == EXTERNAL_SERVER && externalHost.isValid()) {
     server_ = new ExternalHttpServerController(config_, authInfo());
     server_->start();
  } else {
     common::Error err = common::make_error_value("Invalid https server settings!",
                                                     common::Value::E_ERROR, common::logging::L_ERR);
      auto ex_event = make_exception_event(new InnerClientConnectedEvent(this, authInfo()), err);
      EVENT_BUS()->postEvent(ex_event);
  }
}

void NetworkController::disConnect() {
  lock_t lock(server_mutex_);

  if (!server_) {  // if connect dosen't clicked
    common::Error err = common::make_error_value("Failed, not connected!",
                                                 common::Value::E_ERROR,
                                                 common::logging::L_ERR);
    auto ex_event = make_exception_event(new InnerClientDisconnectedEvent(this, authInfo()), err);
      EVENT_BUS()->postEvent(ex_event);
      return;
  }

  server_->stop();
  delete server_;
  server_ = NULL;
}

UserAuthInfo NetworkController::authInfo() const {
  return UserAuthInfo(config_.login, config_.password, config_.local_host);
}

HttpConfig NetworkController::config() const {
  return config_;
}

void NetworkController::setConfig(const HttpConfig& config) {
  config_ = config;
}

void NetworkController::saveConfig() {
  common::file_system::Path configPath(config_path_);
  common::file_system::File configSave(configPath);
  if (!configSave.open("w")) {
      return;
  }

  configSave.write("[" SERVER_SETTINGS_SECTION_LABEL "]\n");
  configSave.writeFormated(LOCAL_HOST_SETTING_LABEL "=%s\n",
                           common::convertToString(config_.local_host));
  configSave.writeFormated(LOGIN_SETTING_LABEL "=%s\n", config_.login);
  configSave.writeFormated(PASSWORD_SETTING_LABEL "=%s\n", config_.password);
  configSave.writeFormated(CONTENT_PATH_SETTING_LABEL "=%s\n", config_.content_path);
  configSave.writeFormated(PRIVATE_SITE_SETTING_LABEL "=%u\n", config_.is_private_site);
  configSave.writeFormated(EXTERNAL_HOST_SETTING_LABEL "=%s\n",
                           common::convertToString(config_.external_host));
  configSave.writeFormated(SERVER_TYPE_SETTING_LABEL "=%u\n", config_.server_type);
  configSave.write("[" HANDLERS_URLS_SECTION_LABEL "]\n");
  for (size_t i = 0; i < config_.handlers_urls.size(); ++i) {
    HttpConfig::handlers_urls_t handurl = config_.handlers_urls[i];
    configSave.writeFormated("%s=%s\n", handurl.first, handurl.second);
  }
  configSave.write("[" SERVER_SOCKETS_SECTION_LABEL "]\n");
  for (size_t i = 0; i < config_.server_sockets_urls.size(); ++i) {
    HttpConfig::server_sockets_urls_t sock_url = config_.server_sockets_urls[i];
    const std::string url = sock_url.second.get_url();
    configSave.writeFormated("%s=%s\n", sock_url.first, url);
  }
  configSave.close();
}

void NetworkController::readConfig() {
#ifdef OS_MACOSX
  const std::string spath = fApp->appDir() + config_path_;
  const char* path = spath.c_str();
#else
  const char* path = config_path_.c_str();
#endif

  HttpConfig config;
  // default settings
  config.local_host = common::net::hostAndPort(USER_SPECIFIC_DEFAULT_DOMAIN,
                                               USER_SPECIFIC_DEFAULT_PORT);
  config.content_path = fApp->appDir();
  config.login = USER_SPECIFIC_DEFAULT_LOGIN;
  config.password = USER_SPECIFIC_DEFAULT_PASSWORD;
  config.is_private_site = USER_SPECIFIC_DEFAULT_PRIVATE_SITE;
  config.external_host = common::net::hostAndPort("localhost", 80);
  config.server_type = FASTO_SERVER;

  // try to parse settings file
  if (ini_parse(path, ini_handler_fasto, &config) < 0) {
    DEBUG_MSG_FORMAT<256>(common::logging::L_INFO,
                          "Can't load config '%s', use default settings.", path);
  }

  config_ = config;
}

}  // namespace network
}  // namespace siteonyourdevice
}  // namespace fasto
