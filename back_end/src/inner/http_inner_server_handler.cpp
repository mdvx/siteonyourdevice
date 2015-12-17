#include "inner/http_inner_server_handler.h"

#include "common/third-party/json-c/json-c/json.h"

#include "common/thread/event_bus.h"
#include "common/logger.h"
#include "common/net/net.h"
#include "common/system_info/system_info.h"

#include "network_events.h"
#include "client_commands.h"

#include "inner/http_inner_server.h"

#define GB (1024*1024*1024)
#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    { 
        Http2InnerServerHandler::Http2InnerServerHandler(const common::net::hostAndPort &host)
            : Http2ServerHandler(NULL), innerConnection_(NULL), authInfo_(), host_(host)
        {

        }

        Http2InnerServerHandler::~Http2InnerServerHandler()
        {
            delete innerConnection_;
            innerConnection_ = NULL;
        }

        void Http2InnerServerHandler::preLooped(TcpServer* server)
        {
            innerConnect(server);
        }

        void Http2InnerServerHandler::accepted(TcpClient* client)
        {
            if(client == innerConnection_){
                EVENT_BUS()->postEvent(new InnerClientConnectedEvent(this, authInfo_));
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
                RelayClient * rclient = prclient->relay();
                rclient->setEclient(NULL);
            }
        }

        void Http2InnerServerHandler::postLooped(TcpServer* server)
        {
            innerDisConnect(server);
        }

        void Http2InnerServerHandler::handleInnerRequestCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[])
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
                const std::string authStr = common::convertToString(authInfo_);
                const std::string iAm = make_responce(id, CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1S, authStr);
                common::Error err = connection->write(iAm.c_str(), iAm.size(), nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
            }
            else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_RELAY_COMMAND)){
                if(argc > 1){
                    const char* hostandport = argv[1];
                    if(hostandport){
                        common::net::hostAndPort host = common::convertFromString<common::net::hostAndPort>(hostandport);
                        common::net::socket_info rinfo;
                        common::Error err = common::net::connect(host, common::net::ST_SOCK_STREAM, NULL, rinfo);
                        if(err && err->isError()){
                            const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_COMMAND_RESP_FAIL_1S, CAUSE_CONNECT_FAILED);
                            err = connection->write(resp.c_str(), resp.size(), nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                            return;
                        }

                        const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_COMMAND_RESP_SUCCSESS_1S, hostandport);
                        err = connection->write(resp.c_str(), resp.size(), nwrite);
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                            return;
                        }

                        Http2InnerServer* hserver = dynamic_cast<Http2InnerServer*>(connection->server());
                        CHECK(hserver);

                        RelayClient * relayConnection = hserver->createRelayClient(rinfo);
                        hserver->registerClient(relayConnection);
                    }
                }
                else{
                    const std::string resp = make_responce(id, CLIENT_PLEASE_CONNECT_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
                    common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                    if(err && err->isError()){
                        DEBUG_MSG_ERROR(err);
                    }
                }
            }
            else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_DISCONNECT_COMMAND)){
                const std::string ok_disconnect = make_responce(id, CLIENT_PLEASE_DISCONNECT_COMMAND_RESP_SUCCSESS);
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

        void Http2InnerServerHandler::handleInnerResponceCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[])
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

        void Http2InnerServerHandler::handleInnerApproveCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[])
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

        void Http2InnerServerHandler::setAuthInfo(const UserAuthInfo& authInfo)
        {
            authInfo_ = authInfo;
        }

        const UserAuthInfo& Http2InnerServerHandler::authInfo() const
        {
            return authInfo_;
        }

        common::Error Http2InnerServerHandler::innerConnect(TcpServer* server)
        {
            if(innerConnection_){
                return common::Error();
            }

            common::net::socket_info client_info;
            common::ErrnoError err = common::net::connect(host_, common::net::ST_SOCK_STREAM, 0, client_info);
            if(err && err->isError()){
                return err;
            }

            InnerClient* connection = new InnerClient(server, client_info);
            innerConnection_ = connection;

            auto cb = [server, connection]()
            {               
                server->registerClient(connection);
            };

            server->execInServerThread(cb);
            return common::Error();
        }

        common::Error Http2InnerServerHandler::innerDisConnect(TcpServer *server)
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

            server->execInServerThread(cb);
            return common::Error();
        }

        void Http2InnerServerHandler::dataReceived(TcpClient* client)
        {
            if(client == innerConnection_){
                char buff[MAX_COMMAND_SIZE] = {0};
                ssize_t nread = 0;
                common::ErrnoError err = client->read(buff, MAX_COMMAND_SIZE, nread);
                if((err && err->isError()) || nread == 0){
                    client->close();
                    delete client;
                    return;
                }

                handleInnerDataReceived(innerConnection_, buff, nread);
                return;
            }

            RelayClient * rclient = dynamic_cast<RelayClient*>(client); //relay connection
            if(rclient){
                char buff[BUF_SIZE] = {0};
                ssize_t nread = 0;
                common::ErrnoError err = client->read(buff, BUF_SIZE, nread);
                if((err && err->isError()) || nread == 0){
                    rclient->close();
                    delete rclient;
                    return;
                }

                const common::net::hostAndPort externalHost = rclient->externalHost();

                if(externalHost.isValid()){
                    common::net::socket_info client_info;
                    common::http::http_protocols protocol = common::http::HP_1_1;
                    if(common::http2::is_preface_data(buff, nread)){
                        protocol = common::http::HP_2_0;
                    }
                    else if(common::http2::is_frame_header_data(buff, nread)){
                        protocol = common::http::HP_2_0;
                    }

                    ProxyRelayClient* eclient = rclient->eclient();
                    if(!eclient){
                        common::Error err = common::net::connect(externalHost, common::net::ST_SOCK_STREAM, 0, client_info);
                        if(err && err->isError()){
                            const std::string error_text = err->description();
                            rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL, error_text.c_str(), false);
                            return;
                        }

                        TcpServer* server = rclient->server();
                        eclient = new ProxyRelayClient(server, client_info, rclient);
                        server->registerClient(eclient);
                        rclient->setEclient(eclient);
                    }

                    CHECK(eclient);

                    ssize_t nwrite = 0;
                    err = eclient->write(buff, nread, nwrite);
                    if(err && err->isError()){
                        const std::string error_text = err->description();
                        rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL, error_text.c_str(), false);
                        return;
                    }
                }
                else { //reley our web server
                    Http2ServerHandler::processReceived(rclient, buff, nread);
                }
            }
            else{
                ProxyRelayClient * prclient = dynamic_cast<ProxyRelayClient*>(client); //proxyrelay connection
                if(prclient){
                    char buff[BUF_SIZE] = {0};
                    ssize_t nread = 0;
                    common::ErrnoError err = prclient->read(buff, BUF_SIZE, nread);
                    if((err && err->isError()) || nread == 0){
                        prclient->close();
                        delete prclient;
                        return;
                    }

                    RelayClient * rclient = prclient->relay();
                    ssize_t nwrite = 0;
                    err = rclient->TcpClient::write(buff, nread, nwrite);
                    if(err && err->isError()){

                    }
                    return;
                }

                Http2ServerHandler::dataReceived(client); //direct connection
            }
        }
    }
}
