#include "inner/http_inner_server_handler.h"

#include "common/third-party/json-c/json-c/json.h"

#include "common/thread/event_bus.h"
#include "common/logger.h"
#include "common/net/net.h"
#include "common/system_info/system_info.h"

#include "network/network_events.h"
#include "client_commands.h"

#include "inner/inner_relay_client.h"
#include "inner/inner_client.h"

#define GB (1024*1024*1024)
#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    { 
        namespace inner
        {
            Http2InnerServerHandler::Http2InnerServerHandler(const HttpServerInfo& info, const common::net::hostAndPort &innerHost, const HttpConfig &config)
                : Http2ServerHandler(info, NULL), innerConnection_(NULL), ping_server_id_timer_(INVALID_TIMER_ID), innerHost_(innerHost), config_(config)
            {

            }

            Http2InnerServerHandler::~Http2InnerServerHandler()
            {
                delete innerConnection_;
                innerConnection_ = NULL;
            }

            void Http2InnerServerHandler::preLooped(tcp::ITcpLoop *server)
            {
                ping_server_id_timer_ = server->createTimer(ping_timeout_server, ping_timeout_server);
                CHECK(!innerConnection_);

                common::net::socket_info client_info;
                common::ErrnoError err = common::net::connect(innerHost_, common::net::ST_SOCK_STREAM, 0, client_info);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                    EVENT_BUS()->postEvent(make_exception_event(new network::InnerClientConnectedEvent(this, authInfo()), err));
                    return;
                }

                InnerClient* connection = new InnerClient(server, client_info);
                innerConnection_ = connection;
                server->registerClient(connection);

                Http2ServerHandler::preLooped(server);
            }

            void Http2InnerServerHandler::accepted(tcp::TcpClient* client)
            {
            }

            void Http2InnerServerHandler::closed(tcp::TcpClient* client)
            {
                if(client == innerConnection_){
                    EVENT_BUS()->postEvent(new network::InnerClientDisconnectedEvent(this, authInfo()));
                    innerConnection_ = NULL;
                    return;
                }

                ProxyRelayClient * prclient = dynamic_cast<ProxyRelayClient*>(client); //proxyrelay connection
                if(prclient){
                    RelayClientEx * rclient = prclient->relay();
                    rclient->setEclient(NULL);
                }
            }

            void Http2InnerServerHandler::postLooped(tcp::ITcpLoop *server)
            {
                if(innerConnection_){
                    InnerClient* connection = innerConnection_;
                    connection->close();
                    delete connection;
                }

                Http2ServerHandler::postLooped(server);
            }

            void Http2InnerServerHandler::timerEmited(tcp::ITcpLoop* server, timer_id_type id)
            {
                if(id == ping_server_id_timer_ && innerConnection_){
                    const cmd_request_t ping_request = make_request(PING_COMMAND_REQ);
                    ssize_t nwrite = 0;
                    InnerClient* client = innerConnection_;
                    common::Error err = client->write(ping_request, nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                        client->close();
                        delete client;
                    }
                }
            }

            void Http2InnerServerHandler::handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
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

            void Http2InnerServerHandler::handleInnerResponceCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
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

            void Http2InnerServerHandler::handleInnerApproveCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
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

            UserAuthInfo Http2InnerServerHandler::authInfo() const
            {
                return UserAuthInfo(config_.login_, config_.password_, config_.local_host_);
            }

            void Http2InnerServerHandler::innerDataReceived(InnerClient* iclient)
            {
                char buff[MAX_COMMAND_SIZE] = {0};
                ssize_t nread = 0;
                common::Error err = iclient->read(buff, MAX_COMMAND_SIZE, nread);
                if((err && err->isError()) || nread == 0){
                    DEBUG_MSG_ERROR(err);
                    iclient->close();
                    delete iclient;
                    return;
                }

                handleInnerDataReceived(iclient, buff, nread);
            }

            void Http2InnerServerHandler::relayDataReceived(inner::RelayClient * rclient)
            {
                char buff[BUF_SIZE] = {0};
                ssize_t nread = 0;
                common::Error err = rclient->read(buff, BUF_SIZE, nread);
                if((err && err->isError()) || nread == 0){
                    rclient->close();
                    delete rclient;
                    return;
                }

                Http2ServerHandler::processReceived(rclient, buff, nread);
            }

            void Http2InnerServerHandler::relayExDataReceived(inner::RelayClientEx *rclient)
            {
                char buff[BUF_SIZE] = {0};
                ssize_t nread = 0;
                common::Error err = rclient->read(buff, BUF_SIZE, nread);
                if((err && err->isError()) || nread == 0){
                    rclient->close();
                    delete rclient;
                    return;
                }

                const common::net::hostAndPort externalHost = rclient->externalHost();

                common::net::socket_info client_info;
                common::http::http_protocols protocol = common::http::HP_1_1;
                if(common::http2::is_preface_data(buff, nread)){
                    protocol = common::http::HP_2_0;
                }
                else if(common::http2::is_frame_header_data(buff, nread)){
                    protocol = common::http::HP_2_0;
                }

                if(externalHost.isValid()){
                    ProxyRelayClient* eclient = rclient->eclient();
                    if(!eclient){
                        common::Error err = common::net::connect(externalHost, common::net::ST_SOCK_STREAM, 0, client_info);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            const std::string error_text = err->description();
                            err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL, error_text.c_str(), false, info());
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                            return;
                        }

                        tcp::ITcpLoop* server = rclient->server();
                        eclient = new ProxyRelayClient(server, client_info, rclient);
                        server->registerClient(eclient);
                        rclient->setEclient(eclient);
                    }

                    CHECK(eclient);

                    ssize_t nwrite = 0;
                    err = eclient->write(buff, nread, nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                        const std::string error_text = err->description();
                        err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL, error_text.c_str(), false, info());
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                        }
                        return;
                    }
                }
                else{
                    err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL, "Invalid external host!", false, info());
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }
            }

            void Http2InnerServerHandler::proxyDataReceived(ProxyRelayClient * prclient)
            {
                char buff[BUF_SIZE] = {0};
                ssize_t nread = 0;
                common::Error err = prclient->read(buff, BUF_SIZE, nread);
                if((err && err->isError()) || nread == 0){
                    prclient->close();
                    delete prclient;
                    return;
                }

                RelayClient * rclient = prclient->relay();
                ssize_t nwrite = 0;
                err = rclient->TcpClient::write(buff, nread, nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                return;
            }

            void Http2InnerServerHandler::dataReceived(tcp::TcpClient* client)
            {
                if(client == innerConnection_){
                    innerDataReceived(innerConnection_);
                    return;
                }

                RelayClientEx * rexclient = dynamic_cast<RelayClientEx*>(client); //relay external connection
                if(rexclient){
                    relayExDataReceived(rexclient);
                    return;
                }

                RelayClient * rclient = dynamic_cast<RelayClient*>(client); //relay connection
                if(rclient){
                    relayDataReceived(rclient);
                    return;
                }

                ProxyRelayClient * prclient = dynamic_cast<ProxyRelayClient*>(client); //proxyrelay connection
                if(prclient){
                    proxyDataReceived(prclient);
                    return;
                }

                Http2ServerHandler::dataReceived(client); //direct connection
            }

            void Http2InnerServerHandler::dataReadyToWrite(tcp::TcpClient* client)
            {

            }
        }
    }
}
