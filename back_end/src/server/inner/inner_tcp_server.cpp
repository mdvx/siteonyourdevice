#include "server/inner/inner_tcp_server.h"

#include "common/net/net.h"
#include "common/logger.h"
#include "common/md5.h"
#include "common/thread/thread_manager.h"

#include "server/http_server_host.h"
#include "server/server_commands.h"

#include "server/inner/inner_tcp_client.h"

extern "C" {
#include "sds.h"
}

namespace fasto
{
    namespace siteonyourdevice
    {
        class InnerServerHandlerHost::InnerSubHandler
                : public RedisSubHandler
        {
        public:
            InnerSubHandler(InnerServerHandlerHost* parent)
                : parent_(parent)
            {

            }

            void processSubscribed(cmd_seq_type request_id, int argc, char *argv[])
            {
                sds join = sdsempty();
                join = sdscatfmt(join, "%s ", request_id.c_str());
                for (int i = 0; i < argc; ++i) {
                    join = sdscat(join, argv[i]);
                    if (i != argc - 1) {
                        join = sdscatlen(join, " ", 1);
                    }
                }

                publish_command_out(join, sdslen(join));
                sdsfree(join);
            }

            void publish_command_out(const std::string& msg)
            {
                publish_command_out(msg.c_str(), msg.length());
            }

            void publish_command_out(const char* msg, size_t msg_len)
            {
                bool res = parent_->sub_commands_in_->publish_command_out(msg, msg_len);
                if(!res){
                    const std::string err_str = common::MemSPrintf("publish_command_out with args: msg = %s, msg_len = " PRIu64 " failed!", msg, msg_len);
                    DEBUG_MSG(common::logging::L_ERR, err_str);
                }
            }

            virtual void handleMessage(char* channel, size_t channel_len, char* msg, size_t msg_len)
            {
                // [std::string]site [hex_string]seq [std::string]command args ...
                // [hex_string]seq OK/FAIL [std::string]command args ..
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_INFO, "InnerSubHandler channel: %s, %s", channel, msg);

                char *space = strchr(msg, ' ');
                if (!space) {
                    const std::string resp = common::MemSPrintf("UNKNOWN COMMAND: %s", msg);
                    DEBUG_MSG(common::logging::L_WARNING, resp);
                    publish_command_out(resp);
                    return;
                }

                *space = 0;

                char buff[MAX_COMMAND_SIZE] = {0};
                int len = sprintf(buff, STRINGIZE(REQUEST_COMMAND) " %s" END_OF_COMMAND, space + 1); //only REQUEST_COMMAND

                char *star_seq = NULL;
                cmd_id_type seq = strtoul(buff, &star_seq, 10);
                if (*star_seq != ' ') {
                    const std::string resp = common::MemSPrintf("PROBLEM EXTRACTING SEQUENCE: %s", space + 1);
                    DEBUG_MSG(common::logging::L_WARNING, resp);
                    publish_command_out(resp);
                    return;
                }

                const char* id_ptr = strchr(star_seq + 1, ' ');
                if (!id_ptr) {
                    const std::string resp = common::MemSPrintf("PROBLEM EXTRACTING ID: %s", space + 1);
                    DEBUG_MSG(common::logging::L_WARNING, resp);
                    publish_command_out(resp);
                    return;
                }

                size_t len_seq = id_ptr - (star_seq + 1);
                cmd_seq_type id = std::string(star_seq + 1, len_seq);
                const char *cmd = id_ptr;

                InnerTcpClient* fclient = parent_->parent_->findInnerConnectionByHost(msg);
                if(!fclient){
                    int argc;
                    sds *argv = sdssplitargs(cmd, &argc);
                    char* command = argv[0];

                    const std::string resp = common::MemSPrintf(SERVER_COMMANDS_OUT_FAIL_2US(CAUSE_NOT_CONNECTED), id, command);
                    publish_command_out(resp);
                    sdsfreesplitres(argv, argc);
                    return;
                }

                ssize_t nwrite = 0;
                common::Error err = fclient->write(buff, len, nwrite);
                if(err && err->isError()){
                    int argc;
                    sds *argv = sdssplitargs(cmd, &argc);
                    char* command = argv[0];

                    const std::string resp = common::MemSPrintf(SERVER_COMMANDS_OUT_FAIL_2US(CAUSE_NOT_HANDLED), id, command);
                    publish_command_out(resp);
                    sdsfreesplitres(argv, argc);
                    return;
                }

                RequestCallback::callback_t cb = std::bind(&InnerSubHandler::processSubscribed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
                RequestCallback rc(id, cb);
                parent_->subscribeRequest(rc);
            }

            InnerServerHandlerHost * parent_;
        };

        InnerServerHandlerHost::InnerServerHandlerHost(HttpServerHost *parent)
            : parent_(parent), sub_commands_in_(NULL), handler_(NULL), ping_client_id_timer_(INVALID_TIMER_ID)
        {
            handler_ = new InnerSubHandler(this);
            sub_commands_in_ = new RedisSub(handler_);
            redis_subscribe_command_in_thread_ = THREAD_MANAGER()->createThread(&RedisSub::listen, sub_commands_in_);
        }

        InnerServerHandlerHost::~InnerServerHandlerHost()
        {
            sub_commands_in_->stop();
            redis_subscribe_command_in_thread_->join();
            delete sub_commands_in_;
            delete handler_;
        }

        void InnerServerHandlerHost::preLooped(ITcpLoop* server)
        {
            ping_client_id_timer_ = server->createTimer(ping_timeout_clients, ping_timeout_clients);
        }

        void InnerServerHandlerHost::moved(TcpClient* client)
        {

        }

        void InnerServerHandlerHost::postLooped(ITcpLoop *server)
        {

        }

        void InnerServerHandlerHost::timerEmited(ITcpLoop* server, timer_id_type id)
        {
            if(ping_client_id_timer_ == id){
                std::vector<TcpClient *> online_clients = server->clients();
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_INFO, "Ping inner client(%" PRIuS ") connected.", online_clients.size());
                
                for(size_t i = 0; i < online_clients.size(); ++i){
                    TcpClient* client = online_clients[i];
                    const std::string ping_request = make_request(PING_COMMAND_REQ);
                    ssize_t nwrite = 0;
                    common::Error err = client->write(ping_request.c_str(), ping_request.size(), nwrite);
                    if(err && err->isError()){
                        client->close();
                        delete client;
                    }
                }
            }
        }

        void InnerServerHandlerHost::accepted(TcpClient* client)
        {
            ssize_t nwrite = 0;
            const std::string whoareyou = make_request(SERVER_WHO_ARE_YOU_COMMAND_REQ);
            client->write(whoareyou.c_str(), whoareyou.size(), nwrite);
        }

        void InnerServerHandlerHost::closed(TcpClient* client)
        {
            bool isOk = parent_->unRegisterInnerConnectionByHost(client);
            if(isOk){
                InnerTcpClient * iconnection = dynamic_cast<InnerTcpClient *>(client);
                if(iconnection){
                    UserAuthInfo hinf = iconnection->serverHostInfo();
                    const std::string hoststr = hinf.host_.host_;
                    const std::string connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_DISCONNECTED_1S, hoststr);
                    bool res = sub_commands_in_->publish_clients_state(connected_resp);
                    if(!res){
                        const std::string err_str = common::MemSPrintf("publish_clients_state with args: connected_resp = %s failed!", connected_resp);
                        DEBUG_MSG(common::logging::L_ERR, err_str);
                    }
                }
            }
        }

        void InnerServerHandlerHost::dataReceived(TcpClient* client)
        {
            char buff[MAX_COMMAND_SIZE] = {0};
            ssize_t nread = 0;
            common::Error err = client->read(buff, MAX_COMMAND_SIZE, nread);
            if((err && err->isError()) || nread == 0){
                client->close();
                delete client;
                return;
            }

            InnerTcpClient* iclient = dynamic_cast<InnerTcpClient*>(client);
            CHECK(iclient);

            handleInnerDataReceived(iclient, buff, nread);
        }

        void InnerServerHandlerHost::setStorageConfig(const redis_sub_configuration_t& config)
        {
            sub_commands_in_->setConfig(config);
            redis_subscribe_command_in_thread_->start();
        }

        void InnerServerHandlerHost::handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
        {
            char* command = argv[0];

            if(IS_EQUAL_COMMAND(command, PING_COMMAND)){
                const std::string pong = make_responce(id, PING_COMMAND_RESP_SUCCESS);
                ssize_t nwrite = 0;
                connection->write(pong.c_str(), pong.size(), nwrite);
            }
            else{
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN COMMAND: %s", command);
            }
        }

        void InnerServerHandlerHost::handleInnerResponceCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
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
                            connection->write(resp.c_str(), resp.size(), nwrite);
                            goto fail;
                        }

                        const std::string resp =  make_approve_responce(id, PING_COMMAND_APPROVE_SUCCESS);
                        common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                        if(err && err->isError()){
                            goto fail;
                        }
                    }
                    else{
                        const std::string resp = make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
                        connection->write(resp.c_str(), resp.size(), nwrite);
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)){
                    if(argc > 2){
                        const char* uauthstr = argv[2];
                        if(!uauthstr){
                            const std::string resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
                            connection->write(resp.c_str(), resp.size(), nwrite);
                            goto fail;
                        }

                        UserAuthInfo uauth = common::convertFromString<UserAuthInfo>(uauthstr);
                        if(!uauth.isValid()){
                            const std::string resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_USER);
                            connection->write(resp.c_str(), resp.size(), nwrite);
                            goto fail;
                        }

                        MD5_CTX ctx;
                        MD5_Init(&ctx);
                        const char * passwordstr = uauth.password_.c_str();
                        unsigned long len = uauth.password_.size();
                        MD5_Update(&ctx, passwordstr, len);
                        unsigned char digest[16];
                        MD5_Final(digest, &ctx);
                        uauth.password_ = common::HexEncode(digest, sizeof(digest), true);

                        bool isOk = parent_->findUser(uauth);
                        if(!isOk){
                            const std::string resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_UNREGISTERED_USER);
                            connection->write(resp.c_str(), resp.size(), nwrite);
                            goto fail;
                        }

                        const std::string hoststr = uauth.host_.host_;
                        InnerTcpClient* fclient = parent_->findInnerConnectionByHost(hoststr);
                        if(fclient){
                            const std::string resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_DOUBLE_CONNECTION_HOST);
                            connection->write(resp.c_str(), resp.size(), nwrite);
                            goto fail;
                        }

                        const std::string resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_SUCCESS);
                        common::Error err = connection->write(resp.c_str(), resp.size(), nwrite);
                        if(err && err->isError()){
                            const std::string resp2 = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, err->description());
                            connection->write(resp2.c_str(), resp2.size(), nwrite);
                            goto fail;
                        }

                        isOk = parent_->registerInnerConnectionByUser(uauth, connection);
                        if(isOk){
                            const std::string connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_CONNECTED_1S, hoststr);
                            bool res = sub_commands_in_->publish_clients_state(connected_resp);
                            if(!res){
                                const std::string err_str = common::MemSPrintf("publish_clients_state with args: connected_resp = %s failed!", connected_resp);
                                DEBUG_MSG(common::logging::L_ERR, err_str);
                            }
                        }
                    }
                    else{
                        const std::string resp = make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
                        connection->write(resp.c_str(), resp.size(), nwrite);
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_HTTP_COMMAND)){

                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_DISCONNECT_HTTP_COMMAND)){
                    connection->close();
                    delete connection;
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND)){

                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_DISCONNECT_WEBSOCKET_COMMAND)){
                    connection->close();
                    delete connection;
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_SYSTEM_INFO_COMMAND)){

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

            return;

        fail:
            connection->close();
            delete connection;
        }

        void InnerServerHandlerHost::handleInnerApproveCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[])
        {
            char* command = argv[0];

            if(IS_EQUAL_COMMAND(command, SUCCESS_COMMAND)){
                if(argc > 1){
                    const char* okrespcommand = argv[1];
                    if(IS_EQUAL_COMMAND(okrespcommand, PING_COMMAND)){
                    }
                    else if(IS_EQUAL_COMMAND(okrespcommand, SERVER_WHO_ARE_YOU_COMMAND)){
                    }
                }
            }
            else if(IS_EQUAL_COMMAND(command, FAIL_COMMAND)){

            }
            else{
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN COMMAND: %s", command);
            }
        }

        InnerTcpServer::InnerTcpServer(const common::net::hostAndPort& host, ITcpLoopObserver *observer)
            : TcpServer(host, observer)
        {

        }

        const char* InnerTcpServer::className() const
        {
            return "InnerTcpServer";
        }

        TcpClient * InnerTcpServer::createClient(const common::net::socket_info &info)
        {
            return new InnerTcpClient(this, info);
        }
    }
}
