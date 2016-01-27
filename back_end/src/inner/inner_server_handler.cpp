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

#include "inner/inner_server_handler.h"

#include <string>

#include "common/third-party/json-c/json-c/json.h"

#include "common/error.h"
#include "common/logger.h"
#include "common/net/net.h"
#include "common/system_info/system_info.h"
#include "common/thread/event_bus.h"

#include "network/network_events.h"

#include "tcp/tcp_server.h"

#include "inner/inner_client.h"
#include "inner/inner_relay_client.h"

#include "client_commands.h"

namespace fasto {
namespace siteonyourdevice {
namespace inner {

InnerServerHandler::InnerServerHandler(const common::net::hostAndPort& innerHost,
                                           const HttpConfig& config) :
  config_(config), inner_connection_(nullptr),
  ping_server_id_timer_(INVALID_TIMER_ID), innerHost_(innerHost) {
}

InnerServerHandler::~InnerServerHandler(){
  delete inner_connection_;
  inner_connection_ = nullptr;
}

void InnerServerHandler::preLooped(tcp::ITcpLoop* server){
  ping_server_id_timer_ = server->createTimer(ping_timeout_server, ping_timeout_server);
  CHECK(!inner_connection_);

  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(innerHost_, common::net::ST_SOCK_STREAM,
                                                  0, &client_info);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    auto ex_event = make_exception_event(new network::InnerClientConnectedEvent(this, authInfo()), err);
    EVENT_BUS()->postEvent(ex_event);
    return;
  }

  InnerClient* connection = new InnerClient(server, client_info);
  inner_connection_ = connection;
  server->registerClient(connection);
}

void InnerServerHandler::accepted(tcp::TcpClient* client){

}

void InnerServerHandler::moved(tcp::TcpClient* client){

}

void InnerServerHandler::closed(tcp::TcpClient* client){
  if (client == inner_connection_) {
    EVENT_BUS()->postEvent(new network::InnerClientDisconnectedEvent(this, authInfo()));
    inner_connection_ = nullptr;
    return;
  }
}

void InnerServerHandler::dataReceived(tcp::TcpClient* client){
  char buff[MAX_COMMAND_SIZE] = {0};
  ssize_t nread = 0;
  common::Error err = client->read(buff, MAX_COMMAND_SIZE, &nread);
  if ((err && err->isError()) || nread == 0) {
    DEBUG_MSG_ERROR(err);
    client->close();
    delete client;
    return;
  }

  handleInnerDataReceived(dynamic_cast<InnerClient*>(client), buff, nread);
}

void InnerServerHandler::dataReadyToWrite(tcp::TcpClient* client){

}

void InnerServerHandler::postLooped(tcp::ITcpLoop* server){
  if (inner_connection_) {
    InnerClient* connection = inner_connection_;
    connection->close();
    delete connection;
  }
}

void InnerServerHandler::timerEmited(tcp::ITcpLoop* server, timer_id_type id){
  if (id == ping_server_id_timer_ && inner_connection_) {
    const cmd_request_t ping_request = make_request(PING_COMMAND_REQ);
    ssize_t nwrite = 0;
    InnerClient* client = inner_connection_;
    common::Error err = client->write(ping_request, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
      client->close();
      delete client;
    }
  }
}

UserAuthInfo InnerServerHandler::authInfo() const {
  return UserAuthInfo(config_.login, config_.password, config_.local_host);
}

void InnerServerHandler::setConfig(const HttpConfig& config){
  config_ = config;
}

void InnerServerHandler::handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id,
                                                   int argc, char *argv[]) {
  ssize_t nwrite = 0;
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, PING_COMMAND)) {
    const cmd_responce_t pong = make_responce(id, PING_COMMAND_RESP_SUCCESS);
    common::Error err = connection->write(pong, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }
  } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {
    std::string authStr = common::convertToString(authInfo());
    cmd_responce_t iAm = make_responce(id, CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1S,
                                           authStr);
    common::Error err = connection->write(iAm, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_HTTP_COMMAND)) {
    if (argc > 1) {
      const char* hostandport = argv[1];
      if (hostandport) {
        common::net::hostAndPort host = common::convertFromString<common::net::hostAndPort>(hostandport);
        common::net::socket_info rinfo;
        common::Error err = common::net::connect(host, common::net::ST_SOCK_STREAM,
                                                 NULL, &rinfo);
        if (err && err->isError()) {
          cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S,
                                              CAUSE_CONNECT_FAILED);
          err = connection->write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }

        auto find_by_name = [](tcp::ITcpLoop * loop) -> bool {
          return loop->name() == "local_http_server" ||
              loop->name() == "proxy_http_server";  // hardcode
        };

        tcp::ITcpLoop* server = tcp::ITcpLoop::findExistLoopByPredicate(find_by_name);
        if(!server){
            cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S,
                                                CAUSE_CONNECT_FAILED);
            err = connection->write(resp, &nwrite);
            if (err && err->isError()) {
              DEBUG_MSG_ERROR(err);
            }
            return;
        }

        cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_SUCCSESS_1S,
                                                    hostandport);
        err = connection->write(resp, &nwrite);
        if (err && err->isError()) {
          DEBUG_MSG_ERROR(err);
          return;
        }

        RelayClient *relayConnection = NULL;
        if (config_.server_type == EXTERNAL_SERVER) {
          relayConnection = new RelayClientEx(server, rinfo, config_.external_host);
        } else {
          relayConnection = new RelayClient(server, rinfo);
        }
        relayConnection->setIsAuthenticated(!config_.is_private_site);

        auto cb = [server, relayConnection]() {
          server->registerClient(relayConnection);
        };
        server->execInLoopThread(cb);
      } else {
        NOTREACHED();
      }
    } else {
      cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S,
                                                CAUSE_INVALID_ARGS);
      common::Error err = connection->write(resp, &nwrite);
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
      }
    }
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND)) {
    if (argc > 2) {
      const char* hostandport = argv[1];
      const char* hostandport_src = argv[2];
      if (hostandport && hostandport_src) {
        common::net::hostAndPort host_src = common::convertFromString<common::net::hostAndPort>(hostandport_src);
        if (!host_src.isValid()) {
          cmd_responce_t resp = make_responce(id,
                                              CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S,
                                              CAUSE_INVALID_ARGS);
          common::Error err = connection->write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }

        common::net::hostAndPort host = common::convertFromString<common::net::hostAndPort>(hostandport);
        common::net::socket_info rinfo;
        common::Error err = common::net::connect(host,
                                                         common::net::ST_SOCK_STREAM, NULL, &rinfo);
        if (err && err->isError()) {
          cmd_responce_t resp = make_responce(id,
                                              CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S,
                                              CAUSE_CONNECT_FAILED);
          err = connection->write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
            return;
        }

        tcp::ITcpLoop * server = nullptr;

        if (config_.server_type == EXTERNAL_SERVER) {
          auto find_by_name = [](tcp::ITcpLoop * loop) -> bool {
            return loop->name() == "proxy_http_server";  // hardcode
          };

          server = tcp::TcpServer::findExistLoopByPredicate(find_by_name);
        } else {
          server = tcp::TcpServer::findExistServerByHost(host_src);
        }

        if (!server) {
          cmd_responce_t resp = make_responce(id,
                                              CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S,
                                              CAUSE_INVALID_ARGS);
          common::Error err = connection->write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }

        RelayClient* relay_connection = nullptr;
        if (config_.server_type == EXTERNAL_SERVER) {
          relay_connection = new RelayClientEx(server, rinfo, host_src);
        } else {
          relay_connection = new RelayClient(server, rinfo);
        }
        auto cb = [server, relay_connection]() {
          server->registerClient(relay_connection);
        };
        server->execInLoopThread(cb);

        cmd_responce_t resp = make_responce(id,
                                            CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_SUCCSESS_1S,
                                            hostandport);
        err = connection->write(resp, &nwrite);
        if (err && err->isError()) {
          DEBUG_MSG_ERROR(err);
          return;
        }
      } else {
        NOTREACHED();
      }
    } else {
      cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S,
                                          CAUSE_INVALID_ARGS);
      common::Error err = connection->write(resp, &nwrite);
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
      }
    }
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_SYSTEM_INFO_COMMAND)) {
    common::system_info::CpuInfo c1 = common::system_info::currentCpuInfo();
    std::string brand = c1.brandName();

    int64_t ram_total = common::system_info::amountOfPhysicalMemory();
    int64_t ram_free = common::system_info::amountOfAvailablePhysicalMemory();

    std::string os_name = common::system_info::operatingSystemName();
    std::string os_version = common::system_info::operatingSystemVersion();
    std::string os_arch = common::system_info::operatingSystemArchitecture();

    std::string os = common::MemSPrintf("%s %s(%s)", os_name, os_version, os_arch);

    json_object * info_json = json_object_new_object();
    json_object_object_add(info_json, "os", json_object_new_string(os.c_str()));
    json_object_object_add(info_json, "cpu", json_object_new_string(brand.c_str()));
    json_object_object_add(info_json, "ram_total", json_object_new_int64(ram_total));
    json_object_object_add(info_json, "ram_free", json_object_new_int64(ram_free));

    const char *info_json_string = json_object_get_string(info_json);
    cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_SUCCSESS_1J,
                                        info_json_string);
    common::Error err = connection->write(resp, &nwrite);
    if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
    }
    json_object_put(info_json);
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONFIG_COMMAND)) {
    json_object * config_json = json_object_new_object();
    const std::string local_host_str = common::convertToString(config_.local_host);
    json_object_object_add(config_json, LOCAL_HOST_SETTING_LABEL,
                           json_object_new_string(local_host_str.c_str()));
    json_object_object_add(config_json, CONTENT_PATH_SETTING_LABEL,
                           json_object_new_string(config_.content_path.c_str()));
    json_object_object_add(config_json, PRIVATE_SITE_SETTING_LABEL,
                           json_object_new_boolean(config_.is_private_site));
    const std::string external_host_str = common::convertToString(config_.external_host);
    json_object_object_add(config_json, EXTERNAL_HOST_SETTING_LABEL,
                           json_object_new_string(external_host_str.c_str()));
    json_object_object_add(config_json, SERVER_TYPE_SETTING_LABEL,
                           json_object_new_int(config_.server_type));

    json_object* jhttp_urls = json_object_new_array();
    for (size_t i = 0; i < config_.handlers_urls.size(); ++i) {
        HttpConfig::handlers_urls_t url = config_.handlers_urls[i];
        json_object* jhttp_url = json_object_new_object();
        json_object_object_add(jhttp_url, "url", json_object_new_string(url.first.c_str()));
        json_object_object_add(jhttp_url, "handler",
                               json_object_new_string(url.second.c_str()));
        json_object_array_add(jhttp_urls, jhttp_url);
    }
    json_object_object_add(config_json, HANDLERS_URLS_SECTION_LABEL, jhttp_urls);

    json_object* jsockets_urls = json_object_new_array();
    for (size_t i = 0; i < config_.server_sockets_urls.size(); ++i) {
        HttpConfig::server_sockets_urls_t url = config_.server_sockets_urls[i];
        json_object* jsocket_url = json_object_new_object();
        json_object_object_add(jsocket_url, "type", json_object_new_string(url.first.c_str()));
        const std::string surl = url.second.get_url();
        json_object_object_add(jsocket_url, "path", json_object_new_string(surl.c_str()));
        json_object_array_add(jsockets_urls, jsocket_url);
    }
    json_object_object_add(config_json, SERVER_SOCKETS_SECTION_LABEL, jsockets_urls);

    const char *config_json_string = json_object_get_string(config_json);
    cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONFIG_COMMAND_RESP_SUCCSESS_1J,
                                        config_json_string);
    common::Error err = connection->write(resp, &nwrite);
    if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
    }

    json_object_put(config_json);
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_SET_CONFIG_COMMAND)) {
    if (argc > 1) {
       const char* config_json_str = argv[1];
       json_object * config_json = json_tokener_parse(config_json_str);
       if (!config_json) {
           cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_FAIL_1S,
                                               CAUSE_INVALID_ARGS);
           common::Error err = connection->write(resp, &nwrite);
           if (err && err->isError()) {
             DEBUG_MSG_ERROR(err);
           }
           return;
       }

       HttpConfig new_config;
       new_config.login = config_.login;
       new_config.password = config_.password;

       json_object* jlocal_host = NULL;
       json_object_object_get_ex(config_json, LOCAL_HOST_SETTING_LABEL, &jlocal_host);
       if (jlocal_host) {
         const char * lhost_str = json_object_get_string(jlocal_host);
         new_config.local_host = common::convertFromString<common::net::hostAndPort>(lhost_str);
       }

       json_object* jexternal_host = NULL;
       json_object_object_get_ex(config_json, EXTERNAL_HOST_SETTING_LABEL, &jexternal_host);
       if (jexternal_host) {
         const char * ehost_str = json_object_get_string(jexternal_host);
         new_config.external_host = common::convertFromString<common::net::hostAndPort>(ehost_str);
       }

       json_object* jcontent_path = NULL;
       json_object_object_get_ex(config_json, CONTENT_PATH_SETTING_LABEL, &jcontent_path);
       if (jcontent_path) {
         new_config.content_path = json_object_get_string(jcontent_path);
       }

       json_object* jprivate_site = NULL;
       json_object_object_get_ex(config_json, PRIVATE_SITE_SETTING_LABEL, &jprivate_site);
       if (jprivate_site) {
         new_config.is_private_site = json_object_get_boolean(jprivate_site);
       }

       json_object* jserver_type = NULL;
       json_object_object_get_ex(config_json, SERVER_TYPE_SETTING_LABEL, &jserver_type);
       if (jserver_type) {
         new_config.server_type = static_cast<http_server_type>(json_object_get_int(jserver_type));
       }

       json_object* jhandler_urls = NULL;
       json_object_object_get_ex(config_json, HANDLERS_URLS_SECTION_LABEL, &jhandler_urls);
       if (jhandler_urls) {
          int urls_count = json_object_array_length(jhandler_urls);
          for(int i = 0; i < urls_count; ++i){
            json_object *jurlh = json_object_array_get_idx(jhandler_urls, i);

            json_object* jurl = NULL;
            json_object_object_get_ex(jurlh, "url", &jurl);

            json_object* jhandler = NULL;
            json_object_object_get_ex(jurlh, "handler", &jhandler);
            if(jurl && jhandler){
              std::string url_str = json_object_get_string(jurl);
              std::string handler_str = json_object_get_string(jhandler);
              new_config.handlers_urls.push_back(std::make_pair(url_str, handler_str));
            }
          }
       }

       json_object* jsockets_section = NULL;
       json_object_object_get_ex(config_json, SERVER_SOCKETS_SECTION_LABEL, &jsockets_section);
       if (jsockets_section) {
          int sockets_count = json_object_array_length(jsockets_section);
          for(int i = 0; i < sockets_count; ++i){
            json_object *jtsocket = json_object_array_get_idx(jsockets_section, i);

            json_object* jtype = NULL;
            json_object_object_get_ex(jtsocket, "type", &jtype);

            json_object* jpath = NULL;
            json_object_object_get_ex(jtsocket, "path", &jpath);
            if(jtype && jpath){
              std::string type_str = json_object_get_string(jtype);
              std::string path_str = json_object_get_string(jpath);
              new_config.server_sockets_urls.push_back(std::make_pair(type_str,
                                                                      common::uri::Uri(path_str)));
            }
          }
       }

       if(new_config.local_host.host != config_.local_host.host){
           cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_FAIL_1S,
                                               CAUSE_INVALID_ARGS);
           common::Error err = connection->write(resp, &nwrite);
           if (err && err->isError()) {
             DEBUG_MSG_ERROR(err);
           }
           json_object_put(config_json);
           return;
       }

       cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_SUCCSESS);
       common::Error err = connection->write(resp, &nwrite);
       if (err && err->isError()) {
         DEBUG_MSG_ERROR(err);
       }
       json_object_put(config_json);

       EVENT_BUS()->postEvent(new network::ConfigChangedEvent(this, new_config));
    } else {
      cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_FAIL_1S,
                                          CAUSE_INVALID_ARGS);
      common::Error err = connection->write(resp, &nwrite);
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
      }
    }
  } else {
      DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING,
                                         "UNKNOWN REQUEST COMMAND: %s", command);
  }
}

void InnerServerHandler::handleInnerResponceCommand(InnerClient *connection,
                                                    cmd_seq_type id, int argc, char *argv[]) {
  ssize_t nwrite = 0;
  char* state_command = argv[0];

  if (IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1) {
    char* command = argv[1];
    if (IS_EQUAL_COMMAND(command, PING_COMMAND)) {
      if (argc > 2) {
        const char* pong = argv[2];
          if (!pong) {
            cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S,
                                                       CAUSE_INVALID_ARGS);
            common::Error err = connection->write(resp, &nwrite);
            if (err && err->isError()) {
              DEBUG_MSG_ERROR(err);
            }
            return;
          }

          const cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_SUCCESS);
          common::Error err = connection->write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
            return;
          }
      } else {
        cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S,
                                                   CAUSE_INVALID_ARGS);
        common::Error err = connection->write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
        }
      } else {
          DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING,
                                             "UNKNOWN RESPONCE COMMAND: %s", command);
      }
  } else if (IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1) {
  } else {
      DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING,
                                         "UNKNOWN STATE COMMAND: %s", state_command);
  }
}

void InnerServerHandler::handleInnerApproveCommand(InnerClient *connection,
                                                   cmd_seq_type id, int argc, char *argv[]) {
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, SUCCESS_COMMAND)) {
    if (argc > 1) {
      const char* okrespcommand = argv[1];
      if (IS_EQUAL_COMMAND(okrespcommand, PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(okrespcommand, SERVER_WHO_ARE_YOU_COMMAND)) {
        EVENT_BUS()->postEvent(new network::InnerClientConnectedEvent(this, authInfo()));
      }
    }
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
  } else {
    DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING,
                                      "UNKNOWN COMMAND: %s", command);
  }
}

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
