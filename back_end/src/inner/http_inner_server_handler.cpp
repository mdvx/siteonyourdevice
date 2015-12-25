#include "inner/http_inner_server_handler.h"

#include "common/third-party/json-c/json-c/json.h"

#include "common/thread/event_bus.h"
#include "common/logger.h"
#include "common/net/net.h"
#include "common/system_info/system_info.h"

#include "network_events.h"
#include "client_commands.h"

#include "inner/inner_relay_client.h"

#define GB (1024*1024*1024)
#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    { 
        Http2InnerServerHandler::Http2InnerServerHandler(const HttpServerInfo& info, const common::net::hostAndPort &innerHost)
            : Http2ServerHandler(info, NULL), innerConnection_(NULL), innerHost_(innerHost), config_()
        {

        }

        Http2InnerServerHandler::~Http2InnerServerHandler()
        {
            delete innerConnection_;
            innerConnection_ = NULL;
        }

        void Http2InnerServerHandler::preLooped(ITcpLoop *server)
        {
            innerConnect(server);
            Http2ServerHandler::preLooped(server);
        }

        void Http2InnerServerHandler::accepted(TcpClient* client)
        {
            if(client == innerConnection_){
                EVENT_BUS()->postEvent(new InnerClientConnectedEvent(this, authInfo()));
            }
        }

        void Http2InnerServerHandler::closed(TcpClient* client)
        {
            if(client == innerConnection_){
                EVENT_BUS()->postEvent(new InnerClientDisconnectedEvent(this));
                innerConnection_ = NULL;
                return;
            }

            ProxyRelayClient * prclient = dynamic_cast<ProxyRelayClient*>(client); //proxyrelay connection
            if(prclient){
                RelayClientEx * rclient = prclient->relay();
                rclient->setEclient(NULL);
            }
        }

        void Http2InnerServerHandler::postLooped(ITcpLoop *server)
        {
            innerDisConnect(server);
            Http2ServerHandler::postLooped(server);
        }

        void Http2InnerServerHandler::handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
        {
            ssize_t nwrite = 0;
            char* command = argv[0];

            if(IS_EQUAL_COMMAND(command, PING_COMMAND)){
                const std::string pong = make_responce(id, PING_COMMAND_RESP_SUCCESS);
                common::Error err = connection->write(pong.c_str(), pong.size(), nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
            }
            else if(IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)){
                const std::string authStr = common::convertToString(authInfo());
                const std::string iAm = make_responce(id, CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1S, authStr);
                common::Error err = connection->write(iAm.c_str(), iAm.size(), nwrite);
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
                            const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S, CAUSE_CONNECT_FAILED);
                            err = connection->write(resp.c_str(), resp.size(), nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                            return;
                        }

                        const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_SUCCSESS_1S, hostandport);
                        err = connection->write(resp.c_str(), resp.size(), nwrite);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            return;
                        }

                        ITcpLoop* server = connection->server();
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
                }
                else{
                    const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
                    common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }
            }
            else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_DISCONNECT_HTTP_COMMAND)){
                const std::string ok_disconnect = make_responce(id, CLIENT_PLEASE_DISCONNECT_HTTP_COMMAND_RESP_SUCCSESS);
                common::Error err = connection->write(ok_disconnect.c_str(), ok_disconnect.size(), nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                connection->close();
                delete connection;
            }
            else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND)){
                if(argc > 1){
                    const char* hostandport = argv[1];
                    if(hostandport){
                        common::net::hostAndPort host = common::convertFromString<common::net::hostAndPort>(hostandport);
                        common::net::socket_info rinfo;
                        common::Error err = common::net::connect(host, common::net::ST_SOCK_STREAM, NULL, rinfo);
                        if(err && err->isError()){
                            const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S, CAUSE_CONNECT_FAILED);
                            err = connection->write(resp.c_str(), resp.size(), nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                            return;
                        }

                        const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_SUCCSESS_1S, hostandport);
                        err = connection->write(resp.c_str(), resp.size(), nwrite);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            return;
                        }

                        RelayClient *relayConnection = NULL;
                        if(config_.server_type_ == EXTERNAL_SERVER){
                            ITcpLoop* server = connection->server();
                            CHECK(server);
                            relayConnection = new RelayClientEx(server, rinfo, config_.external_host_);
                            server->registerClient(relayConnection);
                        }
                        else{
                            DNOTREACHED();
                            //relayConnection = new RelayClient(server, rinfo);
                            //server->registerClient(relayConnection);
                        }
                    }
                }
                else{
                    const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
                    common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }
            }
            else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_DISCONNECT_WEBSOCKET_COMMAND)){
                const std::string ok_disconnect = make_responce(id, CLIENT_PLEASE_DISCONNECT_WEBSOCKET_COMMAND_RESP_SUCCSESS);
                common::Error err = connection->write(ok_disconnect.c_str(), ok_disconnect.size(), nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                connection->close();
                delete connection;
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
                const std::string resp = make_responce(id, CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_SUCCSESS_1J, info_json_string);
                common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                json_object_put(info_json);
            }
            else{
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN REQUEST COMMAND: %s", command);
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
                            const std::string resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
                            common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                            return;
                        }

                        const std::string resp = make_approve_responce(id, PING_COMMAND_APPROVE_SUCCESS);
                        common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            return;
                        }
                    }
                    else{
                        const std::string resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
                        common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                        }
                    }
                }
                else{
                    DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN RESPONCE COMMAND: %s", command);
                }
            }
            else if(IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1){

            }
            else{
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN STATE COMMAND: %s", state_command);
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
                        EVENT_BUS()->postEvent(new InnerClientAutorizedEvent(this));
                    }
                }
            }
            else if(IS_EQUAL_COMMAND(command, FAIL_COMMAND)){

            }
            else{
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN COMMAND: %s", command);
            }
        }

        UserAuthInfo Http2InnerServerHandler::authInfo() const
        {
            const common::net::hostAndPort hs(config_.domain_, config_.port_);
            return UserAuthInfo(config_.login_, config_.password_, hs);
        }

        common::Error Http2InnerServerHandler::innerConnect(ITcpLoop *server)
        {
            if(innerConnection_){
                return common::Error();
            }

            common::net::socket_info client_info;
            common::ErrnoError err = common::net::connect(innerHost_, common::net::ST_SOCK_STREAM, 0, client_info);
            if(err && err->isError()){
                return err;
            }

            InnerClient* connection = new InnerClient(server, client_info);
            innerConnection_ = connection;

            auto cb = [server, connection]()
            {               
                server->registerClient(connection);
            };

            server->execInLoopThread(cb);
            return common::Error();
        }

        void Http2InnerServerHandler::setConfig(const configuration_t& config)
        {
            config_ = config;
        }

        common::Error Http2InnerServerHandler::innerDisConnect(ITcpLoop *server)
        {
            if(!innerConnection_){
                return common::Error();
            }

            InnerClient* connection = innerConnection_;
            auto cb = [connection]()
            {
                connection->close();
                delete connection;
            };

            server->execInLoopThread(cb);
            return common::Error();
        }

        void Http2InnerServerHandler::innerDataReceived(InnerClient* iclient)
        {
            char buff[MAX_COMMAND_SIZE] = {0};
            ssize_t nread = 0;
            common::Error err = iclient->read(buff, MAX_COMMAND_SIZE, nread);
            if((err && err->isError()) || nread == 0){
                iclient->close();
                delete iclient;
                return;
            }

            handleInnerDataReceived(iclient, buff, nread);
        }

        void Http2InnerServerHandler::relayDataReceived(RelayClient * rclient)
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

        void Http2InnerServerHandler::relayExDataReceived(RelayClientEx *rclient)
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

                    ITcpLoop* server = rclient->server();
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

        void Http2InnerServerHandler::dataReceived(TcpClient* client)
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
    }
}
