#include "server/inner_tcp_server.h"

#include <sys/poll.h>

#include "common/thread/thread_manager.h"
#include "common/net/net.h"
#include "common/logger.h"
#include "common/md5.h"

#include "server/http_server_host.h"
#include "server_commands.h"

#define BUF_SIZE 4096

extern "C" {
#include "sds.h"
}

namespace fasto
{
    namespace siteonyourdevice
    {
        RelayServer::RelayServer(InnerServerHandlerHost *handler, InnerTcpClient *parent, const common::net::hostAndPort& host, client_t client)
            : ServerSocketTcp(host), stop_(false), client_(client), relayThread_(), parent_(parent), handler_(handler)
        {
            relayThread_ = THREAD_MANAGER()->createThread(&RelayServer::exec, this);
        }

        RelayServer::client_t RelayServer::client() const
        {
            return client_;
        }

        void RelayServer::setClient(client_t client)
        {
            client_ = client;
        }

        void RelayServer::start()
        {
            relayThread_->start();
        }

        RelayServer::~RelayServer()
        {
            stop_ = true;
            relayThread_->joinAndGet();
        }

        common::ErrnoError RelayServer::bindAvailible()
        {
            common::ErrnoError err;
            do{
                host_.port_ = common::net::getAvailiblePort();
                err = bind();
            }
            while(err && err->isError());

            return common::ErrnoError();
        }

        int RelayServer::exec()
        {
            static const int max_poll_ev = 3;
            common::ErrnoError err = bindAvailible();
            if(err && err->isError()){
                NOTREACHED();
                return EXIT_FAILURE;
            }

            err = listen(5);
            if(err && err->isError()){
                NOTREACHED();
                return EXIT_FAILURE;
            }

            ssize_t nwrite = 0;
            const std::string createConnection = handler_->make_request(SERVER_PLEASE_CONNECT_COMMAND_REQ_1S, common::convertToString(host()));
            err = parent_->write(createConnection, nwrite); //inner command write
            if(err && err->isError()){;
                NOTREACHED();
                return EXIT_FAILURE;
            }

            const int server_fd = info_.fd();
            int client_fd = INVALID_DESCRIPTOR;

            while (!stop_) {
                client_t rclient = client_;

                struct pollfd events[max_poll_ev] = {0};
                int fds = 1;
                //prepare fd
                events[0].fd = server_fd;
                events[0].events = POLLIN | POLLPRI;

                int rm_client_fd = rclient ? rclient->fd() : INVALID_DESCRIPTOR;
                if(rm_client_fd != INVALID_DESCRIPTOR){
                    events[fds].fd = rm_client_fd;
                    events[fds].events = POLLIN | POLLPRI;
                    fds++;
                }

                if(client_fd != INVALID_DESCRIPTOR){
                    events[fds].fd = client_fd;
                    events[fds].events = POLLIN | POLLPRI ;
                    fds++;
                }

                if(rclient && client_fd != INVALID_DESCRIPTOR){
                    for(int i = 0; i < requests_.size(); ++i){
                        common::buffer_type request = requests_[i];
                        common::Error err = common::net::write_to_socket(client_fd, request, nwrite);
                        DCHECK(!err);
                    }
                    requests_.clear();
                }

                int rc = poll(events, fds, 1000);
                for (int i = 0; i < fds && !stop_; ++i){
                    int fd = events[i].fd;
                    short int cevents = events[i].revents;
                    if(cevents == 0){
                        continue;
                    }

                    if(fd == server_fd){ //accept
                        if (cevents & POLLIN){ //read
                            int new_sd = INVALID_DESCRIPTOR;
                            while(new_sd == INVALID_DESCRIPTOR){
                                common::net::socket_info client_info;
                                common::Error er = accept(client_info);
                                if(!er){
                                    new_sd = client_info.fd();
                                    client_fd = new_sd;
                                }
                                else{
                                    NOTREACHED();
                                    break;
                                }
                            }                            
                        }

                        if(cevents & POLLPRI){
                            NOTREACHED();
                        }
                    }
                    else {
                        if(fd == client_fd){
                            if (cevents & POLLIN){ //read
                                char buff[BUF_SIZE] = {0};
                                ssize_t nread = 0;
                                common::Error err = common::net::read_from_socket(fd, buff, BUF_SIZE, nread);
                                if((err && err->isError()) || nread == 0){
                                    common::net::close(client_fd);
                                    client_fd = INVALID_DESCRIPTOR;
                                }
                                else{
                                    ssize_t nwrite = 0;
                                    err = common::net::write_to_socket(rm_client_fd, buff, nread, nwrite);
                                    if(err && err->isError()){
                                        //NOTREACHED();
                                    }
                                }
                            }

                            if(cevents & POLLPRI){
                                NOTREACHED();
                            }
                        }
                        else if(fd == rm_client_fd){
                            if (cevents & POLLIN){ //read
                                char buff[BUF_SIZE] = {0};
                                ssize_t nread = 0;
                                common::Error err = common::net::read_from_socket(fd, buff, BUF_SIZE, nread);
                                if((err && err->isError()) || nread == 0){
                                    rclient->close();
                                    client_.reset();
                                }
                                else{
                                    ssize_t nwrite = 0;
                                    err = common::net::write_to_socket(client_fd, buff, nread, nwrite);
                                    if(err && err->isError()){
                                        //NOTREACHED();
                                    }
                                }
                            }

                            if(cevents & POLLPRI){
                                NOTREACHED();
                            }
                        }
                        else{
                            NOTREACHED();
                        }
                    }
                }
            }

            return EXIT_SUCCESS;
        }

        void RelayServer::addRequest(const common::buffer_type& request)
        {
            requests_.push_back(request);
        }

        InnerTcpClient::InnerTcpClient(TcpServer* server, const common::net::socket_info& info)
            : InnerClient(server, info), hinfo_(), relays_()
        {

        }

        const char* InnerTcpClient::InnerTcpClient::className() const
        {
            return "InnerTcpClient";
        }

        void InnerTcpClient::addClient(InnerServerHandlerHost *handler, TcpClient* client, const common::buffer_type &request)
        {
            RelayServer::client_t rrclient(client);

            for(int i = 0; i < relays_.size(); ++i){
                relay_server_t rserver = relays_[i];
                RelayServer::client_t rclient = rserver->client();
                if(!rclient){
                    rserver->addRequest(request);
                    rserver->setClient(rrclient);
                    return;
                }

                if(rclient == rrclient){
                    rserver->addRequest(request);
                    return;
                }
            }

            TcpServer* server = rrclient->server();
            common::net::hostAndPort hs = server->host();
            std::shared_ptr<RelayServer> tmp(new RelayServer(handler, this, hs, rrclient));
            tmp->addRequest(request);
            tmp->start();
            relays_.push_back(tmp);
        }

        InnerTcpClient::~InnerTcpClient()
        {
            relays_.clear();
        }

        void InnerTcpClient::setServerHostInfo(const HostInfo& info)
        {
            hinfo_ = info;
        }

        const HostInfo& InnerTcpClient::serverHostInfo() const
        {
            return hinfo_;
        }

        class InnerServerHandlerHost::InnerSubHandler
                : public RedisSubHandler
        {
        public:
            InnerSubHandler(InnerServerHandlerHost* parent)
                : parent_(parent)
            {

            }

            void processSubscribed(cmd_id_type request_id, int argc, char *argv[])
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
                // [std::string]site [uint64_t]seq [std::string]command args ...
                // [uint64_t]seq OK/FAIL [std::string]command args ..
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
                uint64_t seq = strtoull(buff, &star_seq, 10);
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
                cmd_id_type id = std::string(star_seq + 1, len_seq);
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
                common::ErrnoError err = fclient->write(buff, len, nwrite);
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
                parent_->subscribe_request(rc);
            }

            InnerServerHandlerHost * parent_;
        };

        InnerServerHandlerHost::InnerServerHandlerHost(HttpServerHandlerHost *parent)
            : parent_(parent), sub_commands_in_(NULL), handler_(NULL)
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

        void InnerServerHandlerHost::preLooped(TcpServer* server)
        {

        }

        void InnerServerHandlerHost::moved(TcpClient* client)
        {

        }

        void InnerServerHandlerHost::postLooped(TcpServer* server)
        {

        }

        void InnerServerHandlerHost::accepted(TcpClient* client)
        {
            ssize_t nwrite = 0;
            client->write(make_request(SERVER_WHO_ARE_YOU_COMMAND_REQ), nwrite);
        }

        void InnerServerHandlerHost::closed(TcpClient* client)
        {
            bool isOk = parent_->unRegisterInnerConnectionByHost(client);
            if(isOk){
                InnerTcpClient * iconnection = dynamic_cast<InnerTcpClient *>(client);
                if(iconnection){
                    HostInfo hinf = iconnection->serverHostInfo();
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

        void InnerServerHandlerHost::handleInnerRequestCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[])
        {
            char* command = argv[0];

            if(IS_EQUAL_COMMAND(command, PING_COMMAND)){
                const std::string pong = make_responce(id, PING_COMMAND_RESP_SUCCESS);
                ssize_t nwrite = 0;
                connection->write(pong, nwrite);
            }
            else{
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN COMMAND: %s", command);
            }
        }

        void InnerServerHandlerHost::handleInnerResponceCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[])
        {
            ssize_t nwrite = 0;
            char* state_command = argv[0];

            if(IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1){
                char* command = argv[1];
                if(IS_EQUAL_COMMAND(command, PING_COMMAND)){
                    if(argc > 2){
                        const char* pong = argv[2];
                        if(!pong){
                            connection->write(make_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS), nwrite);
                            goto fail;
                        }

                        common::ErrnoError err = connection->write(make_approve_responce(id, PING_COMMAND_APPROVE_SUCCESS), nwrite);
                        if(err && err->isError()){
                            goto fail;
                        }
                    }
                    else{
                        connection->write(make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS), nwrite);
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)){
                    if(argc > 2){
                        const char* uauthstr = argv[2];
                        if(!uauthstr){
                            connection->write(make_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS), nwrite);
                            goto fail;
                        }

                        UserAuthInfo uauth = common::convertFromString<UserAuthInfo>(uauthstr);
                        if(!uauth.isValid()){
                            connection->write(make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_USER), nwrite);
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
                            connection->write(make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_UNREGISTERED_USER), nwrite);
                            goto fail;
                        }

                        const std::string hoststr = uauth.host_.host_;
                        InnerTcpClient* fclient = parent_->findInnerConnectionByHost(hoststr);
                        if(fclient){
                            connection->write(make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_DOUBLE_CONNECTION_HOST), nwrite);
                            goto fail;
                        }

                        common::ErrnoError err = connection->write(make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_SUCCESS), nwrite);
                        if(err && err->isError()){
                            connection->write(make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, err->description()), nwrite);
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
                        connection->write(make_approve_responce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS), nwrite);
                    }
                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONNECT_RELAY_COMMAND)){

                }
                else if(IS_EQUAL_COMMAND(command, SERVER_PLEASE_DISCONNECT_COMMAND)){
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

        void InnerServerHandlerHost::handleInnerApproveCommand(InnerClient *connection, cmd_id_type id, int argc, char *argv[])
        {

        }

        InnerTcpServer::InnerTcpServer(const common::net::hostAndPort& host, TcpServerObserver* observer)
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
