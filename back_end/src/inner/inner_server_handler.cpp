#include "inner/inner_server_handler.h"

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

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            InnerServerHandler::InnerServerHandler(const HttpConfig& config)
                : config_(config)
            {

            }

            UserAuthInfo InnerServerHandler::authInfo() const
            {
                return UserAuthInfo(config_.login_, config_.password_, config_.local_host_);
            }

            void InnerServerHandler::handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
            {
                ssize_t nwrite = 0;
                char* command = argv[0];

                if(IS_EQUAL_COMMAND(command, PING_COMMAND)){
                    const cmd_responce_t pong = make_responce(id, PING_COMMAND_RESP_SUCCESS);
                    common::Error err = connection->write(pong, nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)){
                    const std::string authStr = common::convertToString(authInfo());
                    const cmd_responce_t iAm = make_responce(id, CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1S, authStr);
                    common::Error err = connection->write(iAm, nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_HTTP_COMMAND)){
                    if(argc > 1){
                        const char* hostandport = argv[1];
                        if(hostandport){
                            common::net::hostAndPort host = common::convertFromString<common::net::hostAndPort>(hostandport);
                            common::net::socket_info rinfo;
                            common::Error err = common::net::connect(host, common::net::ST_SOCK_STREAM, NULL, rinfo);
                            if(err && err->isError()){
                                const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S, CAUSE_CONNECT_FAILED);
                                err = connection->write(resp, nwrite);
                                if(err && err->isError()){
                                    DEBUG_MSG_ERROR(err);
                                }
                                return;
                            }

                            const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_SUCCSESS_1S, hostandport);
                            err = connection->write(resp, nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                                return;
                            }

                            tcp::ITcpLoop* server = connection->server();
                            CHECK(server);

                            RelayClient *relayConnection = NULL;
                            if(config_.server_type_ == EXTERNAL_SERVER){
                                relayConnection = new RelayClientEx(server, rinfo, config_.external_host_);
                            }
                            else{
                                relayConnection = new RelayClient(server, rinfo);
                            }
                            relayConnection->setIsAuthenticated(!config_.is_private_site_);
                            server->registerClient(relayConnection);
                        }
                        else{
                            NOTREACHED();
                        }
                    }
                    else{
                        const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
                        common::Error err = connection->write(resp, nwrite);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                        }
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND)){
                    if(argc > 2){
                        const char* hostandport = argv[1];
                        const char* hostandport_src = argv[2];
                        if(hostandport && hostandport_src){
                            common::net::hostAndPort host_src = common::convertFromString<common::net::hostAndPort>(hostandport_src);
                            if(!host_src.isValid()){
                                const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
                                common::Error err = connection->write(resp, nwrite);
                                if(err && err->isError()){
                                    DEBUG_MSG_ERROR(err);
                                }
                                return;
                            }

                            common::net::hostAndPort host = common::convertFromString<common::net::hostAndPort>(hostandport);
                            common::net::socket_info rinfo;
                            common::Error err = common::net::connect(host, common::net::ST_SOCK_STREAM, NULL, rinfo);
                            if(err && err->isError()){
                                const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S, CAUSE_CONNECT_FAILED);
                                err = connection->write(resp, nwrite);
                                if(err && err->isError()){
                                    DEBUG_MSG_ERROR(err);
                                }
                                return;
                            }

                            if(config_.server_type_ == EXTERNAL_SERVER){
                                tcp::ITcpLoop* server = connection->server();
                                CHECK(server);

                                RelayClientEx* relayConnection = new RelayClientEx(server, rinfo, host_src);
                                server->registerClient(relayConnection);
                            }
                            else{
                                tcp::TcpServer * existWebServer = tcp::TcpServer::findExistServerByHost(host_src);
                                if(!existWebServer){
                                    const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
                                    common::Error err = connection->write(resp, nwrite);
                                    if(err && err->isError()){
                                        DEBUG_MSG_ERROR(err);
                                    }
                                    return;
                                }

                                RelayClient* relayConnection = new RelayClient(existWebServer, rinfo);
                                auto cb = [existWebServer, relayConnection]()
                                {
                                    existWebServer->registerClient(relayConnection);
                                };
                                existWebServer->execInLoopThread(cb);
                            }

                            const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_SUCCSESS_1S, hostandport);
                            err = connection->write(resp, nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                                return;
                            }
                        }
                        else{
                            NOTREACHED();
                        }
                    }
                    else{
                        const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
                        common::Error err = connection->write(resp, nwrite);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                        }
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_SYSTEM_INFO_COMMAND)){
                    using namespace common::system_info;

                    CpuInfo c1 = currentCpuInfo();
                    std::string brand = c1.brandName();

                    int64_t ram_total = amountOfPhysicalMemory();
                    int64_t ram_free = amountOfAvailablePhysicalMemory();

                    std::string os_name = operatingSystemName();
                    std::string os_version = operatingSystemVersion();
                    std::string os_arch = operatingSystemArchitecture();

                    std::string os = common::MemSPrintf("%s %s(%s)", os_name, os_version, os_arch);

                    json_object * info_json = json_object_new_object();
                    json_object_object_add(info_json, "os", json_object_new_string(os.c_str()));
                    json_object_object_add(info_json, "cpu", json_object_new_string(brand.c_str()));
                    json_object_object_add(info_json, "ram_total", json_object_new_int64(ram_total));
                    json_object_object_add(info_json, "ram_free", json_object_new_int64(ram_free));

                    const char *info_json_string = json_object_get_string(info_json);
                    const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_SUCCSESS_1J, info_json_string);
                    common::Error err = connection->write(resp, nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                    json_object_put(info_json);
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONFIG_COMMAND)){
                    json_object * config_json = json_object_new_object();
                    const std::string local_host_str = common::convertToString(config_.local_host_);
                    json_object_object_add(config_json, LOCAL_HOST_SETTING_LABEL, json_object_new_string(local_host_str.c_str()));
                    //json_object_object_add(config_json, LOGIN_SETTING_LABEL, json_object_new_string(config_.login_.c_str()));
                    //json_object_object_add(config_json, PASSWORD_SETTING_LABEL, json_object_new_string(config_.password_.c_str()));
                    json_object_object_add(config_json, CONTENT_PATH_SETTING_LABEL, json_object_new_string(config_.content_path_.c_str()));
                    json_object_object_add(config_json, PRIVATE_SITE_SETTING_LABEL, json_object_new_boolean(config_.is_private_site_));
                    const std::string external_host_str = common::convertToString(config_.external_host_);
                    json_object_object_add(config_json, EXTERNAL_HOST_SETTING_LABEL, json_object_new_string(external_host_str.c_str()));
                    json_object_object_add(config_json, SERVER_TYPE_SETTING_LABEL, json_object_new_int(config_.server_type_));

                    json_object* jhttp_urls = json_object_new_array();
                    for(size_t i = 0; i < config_.handlers_urls_.size(); ++i){
                        HttpConfig::handlers_urls_t url = config_.handlers_urls_[i];
                        json_object* jhttp_url = json_object_new_object();
                        json_object_object_add(jhttp_url, "url", json_object_new_string(url.first.c_str()));
                        json_object_object_add(jhttp_url, "handler", json_object_new_string(url.second.c_str()));
                        json_object_array_add(jhttp_urls, jhttp_url);
                    }
                    json_object_object_add(config_json, HANDLERS_URLS_SECTION_LABEL, jhttp_urls);

                    json_object* jsockets_urls = json_object_new_array();
                    for(size_t i = 0; i < config_.server_sockets_urls_.size(); ++i){
                        HttpConfig::server_sockets_urls_t url = config_.server_sockets_urls_[i];
                        json_object* jsocket_url = json_object_new_object();
                        json_object_object_add(jsocket_url, "type", json_object_new_string(url.first.c_str()));
                        const std::string surl = url.second.get_url();
                        json_object_object_add(jsocket_url, "path", json_object_new_string(surl.c_str()));
                        json_object_array_add(jsockets_urls, jsocket_url);
                    }
                    json_object_object_add(config_json, SERVER_SOCKETS_SECTION_LABEL, jsockets_urls);

                    const char *config_json_string = json_object_get_string(config_json);
                    const cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_CONFIG_COMMAND_RESP_SUCCSESS_1J, config_json_string);
                    common::Error err = connection->write(resp, nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }

                    json_object_put(config_json);
                }
                else{
                    DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING, "UNKNOWN REQUEST COMMAND: %s", command);
                }
            }

            void InnerServerHandler::handleInnerResponceCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
            {
                ssize_t nwrite = 0;
                char* state_command = argv[0];

                if(IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1){
                    char* command = argv[1];
                    if(IS_EQUAL_COMMAND(command, PING_COMMAND)){
                        if(argc > 2){
                            const char* pong = argv[2];
                            if(!pong){
                                const cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
                                common::Error err = connection->write(resp, nwrite);
                                if(err && err->isError()){
                                    DEBUG_MSG_ERROR(err);
                                }
                                return;
                            }

                            const cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_SUCCESS);
                            common::Error err = connection->write(resp, nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                                return;
                            }
                        }
                        else{
                            const cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
                            common::Error err = connection->write(resp, nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                        }
                    }
                    else{
                        DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING, "UNKNOWN RESPONCE COMMAND: %s", command);
                    }
                }
                else if(IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1){

                }
                else{
                    DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING, "UNKNOWN STATE COMMAND: %s", state_command);
                }
            }

            void InnerServerHandler::handleInnerApproveCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
            {
                char* command = argv[0];

                if(IS_EQUAL_COMMAND(command, SUCCESS_COMMAND)){
                    if(argc > 1){
                        const char* okrespcommand = argv[1];
                        if(IS_EQUAL_COMMAND(okrespcommand, PING_COMMAND)){
                        }
                        else if(IS_EQUAL_COMMAND(okrespcommand, SERVER_WHO_ARE_YOU_COMMAND)){
                            EVENT_BUS()->postEvent(new network::InnerClientConnectedEvent(this, authInfo()));
                        }
                    }
                }
                else if(IS_EQUAL_COMMAND(command, FAIL_COMMAND)){

                }
                else{
                    DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE>(common::logging::L_WARNING, "UNKNOWN COMMAND: %s", command);
                }
            }
        }
    }
}
