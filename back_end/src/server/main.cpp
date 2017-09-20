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

#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "inih/ini.h"

#include <common/convert2string.h>
#include <common/logger.h>
#include <common/utils.h>
#include <common/file_system/file_system.h>

#include "http_server_host.h"
#include "server_config.h"

#define MAXLINE 80
#define SBUF_SIZE 256
#define BUF_SIZE 4096

/*
  [http_server]
  redis_server=localhost:6479
  redis_unix_path=/var/run/redis/redis.sock
  redis_channel_in_name=COMMANDS_IN
  redis_channel_out_name=COMMANDS_OUT
  redis_channel_clients_state_name=CLIENTS_STATE
*/

namespace {

class SysLog : public std::basic_streambuf<char, std::char_traits<char> > {
 public:
  explicit SysLog(std::string ident, int facility) : buffer_() { openlog(ident.c_str(), LOG_PID, facility); }

  ~SysLog() { closelog(); }

 protected:
  int sync() override {
    if (!buffer_.empty()) {
      const char* buff = buffer_.c_str();
      syslog(LOG_DEBUG, buff);
      buffer_.erase();
    }
    return 0;
  }

  int overflow(int c) override {
    if (c != EOF) {
      buffer_ += static_cast<char>(c);
    } else {
      sync();
    }
    return c;
  }

 private:
  std::string buffer_;
};

const common::net::HostAndPort redis_default_host = common::net::HostAndPort::CreateLocalHost(6379);
sig_atomic_t is_stop = 0;
int total_clients = 0;
const char* config_path = CONFIG_FILE_PATH;

struct configuration_t {
  fasto::siteonyourdevice::server::redis_sub_configuration_t redis_config_;
};

configuration_t config;

int ini_handler_fasto(void* user, const char* section, const char* name, const char* value) {
  configuration_t* pconfig = reinterpret_cast<configuration_t*>(user);

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH("http_server", "redis_server")) {
    common::ConvertFromString(value, &pconfig->redis_config_.redis_host);
    return 1;
  } else if (MATCH("http_server", "redis_unix_path")) {
    pconfig->redis_config_.redis_unix_socket = value;
    return 1;
  } else if (MATCH("http_server", "redis_channel_in_name")) {
    pconfig->redis_config_.channel_in = value;
    return 1;
  } else if (MATCH("http_server", "redis_channel_out_name")) {
    pconfig->redis_config_.channel_out = value;
    return 1;
  } else if (MATCH("http_server", "redis_channel_clients_state_name")) {
    pconfig->redis_config_.channel_clients_state = value;
    return 1;
  } else {
    return 0; /* unknown section/name, error */
  }
}

}  // namespace

fasto::siteonyourdevice::server::HttpServerHost* server = NULL;

void signal_handler(int sig);
void sync_config();
void app_logger_hadler(common::logging::LOG_LEVEL level, const std::string& message);

int main(int argc, char* argv[]) {
  int opt;
  bool daemon_mode = false;
  while ((opt = getopt(argc, argv, "cd:")) != -1) {
    switch (opt) {
      case 'c':
        config_path = argv[optind];
        break;
      case 'd':
        daemon_mode = 1;
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-c] config path [-d] run as daemon\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (daemon_mode) {
    common::create_as_daemon();
    for (int i = 0; i < argc; i++) {
      if (strcmp(argv[i], "-c") == 0) {
        config_path = argv[++i];
      }
    }
  }
  signal(SIGHUP, signal_handler);
  signal(SIGQUIT, signal_handler);

#if defined(NDEBUG)
  common::logging::LOG_LEVEL level = common::logging::LOG_LEVEL_INFO;
#else
  common::logging::LOG_LEVEL level = common::logging::LOG_LEVEL_DEBUG;
#endif
#if defined(LOG_TO_FILE)
  std::string log_path = common::file_system::prepare_path("~/" PROJECT_NAME_LOWERCASE ".log");
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, level);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, level);
#endif
  SysLog* sys_log = nullptr;
  if (daemon_mode) {
    /* Open the log file */
    sys_log = new SysLog(PROJECT_NAME_SERVER, LOG_DAEMON);
    std::clog.rdbuf(sys_log);
    common::logging::SET_LOGER_STREAM(&std::clog);
  }

  server = new fasto::siteonyourdevice::server::HttpServerHost(fasto::siteonyourdevice::server::g_http_host,
                                                               fasto::siteonyourdevice::server::g_inner_host,
                                                               fasto::siteonyourdevice::server::g_websocket_host);

  sync_config();

  common::ErrnoError err = common::file_system::change_directory(HOST_PATH);
  int return_code = EXIT_FAILURE;
  if (err) {
    goto exit;
  }

  return_code = server->exec();

exit:
  delete server;
  delete sys_log;
  return return_code;
}

void signal_handler(int sig) {
  if (sig == SIGHUP) {
    sync_config();
  } else if (sig == SIGQUIT) {
    is_stop = 1;
    server->stop();
  }
}

void sync_config() {
  /*{
    "name": "Alex",
    "password": "1234",
    "hosts": [
      "fastoredis.com:8080",
      "fastonosql.com:8080",
      "fastogt.com:8080"
    ]
  }*/

  // HSET users alex json

  config.redis_config_.redis_host = redis_default_host;
  config.redis_config_.channel_in = CHANNEL_COMMANDS_IN_NAME;
  config.redis_config_.channel_out = CHANNEL_COMMANDS_OUT_NAME;
  config.redis_config_.channel_clients_state = CHANNEL_CLIENTS_STATE_NAME;
  // try to parse settings file
  if (ini_parse(config_path, ini_handler_fasto, &config) < 0) {
    INFO_LOG() << "Can't load config " << config_path << ", use default settings.";
  }

  server->setStorageConfig(config.redis_config_);
}

void app_logger_hadler(common::logging::LOG_LEVEL level, const std::string& message) {
  syslog(level, "[%s] %s", common::logging::log_level_to_text(level), common::normalize_for_printf(message));
}
