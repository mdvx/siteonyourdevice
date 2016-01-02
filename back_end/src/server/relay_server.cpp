#include "server/relay_server.h"

#include <sys/poll.h>

#include "common/thread/thread_manager.h"
#include "common/net/net.h"
#include "common/logger.h"

#include "server/inner/inner_tcp_client.h"
#include "server/server_config.h"

#define BUF_SIZE 4096
#define MAX_RELAY_FD 3

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            IRelayServer::IRelayServer(inner::InnerTcpClient *parent, client_t client)
                : ServerSocketTcp(g_relay_server_host), stop_(false), client_fd_(client), relayThread_(), parent_(parent)
            {
                relayThread_ = THREAD_MANAGER()->createThread(&IRelayServer::exec, this);
            }

            IRelayServer::client_t IRelayServer::client() const
            {
                return client_fd_;
            }

            void IRelayServer::setClient(client_t client)
            {
                client_fd_ = client;
            }

            void IRelayServer::start()
            {
                relayThread_->start();
            }

            IRelayServer::~IRelayServer()
            {
                stop_ = true;
                relayThread_->joinAndGet();
            }

            int IRelayServer::exec()
            {                
                common::Error err = bind();
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                    return EXIT_FAILURE;
                }

                err = listen(5);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                    return EXIT_FAILURE;
                }

                ssize_t nwrite = 0;
                const std::string host_str = common::convertToString(host());
                const cmd_request_t createConnection = createSocketCmd(host());
                err = parent_->write(createConnection, nwrite); //inner command write
                if(err && err->isError()){;
                    DEBUG_MSG_ERROR(err);
                    return EXIT_FAILURE;
                }

                const common::net::socket_descr_type server_fd = info_.fd();
                common::net::socket_descr_type device_fd = INVALID_DESCRIPTOR;

                while (!stop_) {
                    struct pollfd events[MAX_RELAY_FD] = {0};
                    int fds = 1;
                    //prepare fd
                    events[0].fd = server_fd;
                    events[0].events = POLLIN | POLLPRI;

                    if(client_fd_ != INVALID_DESCRIPTOR){
                        events[fds].fd = client_fd_;
                        events[fds].events = POLLIN | POLLPRI;
                        fds++;
                    }

                    if(device_fd != INVALID_DESCRIPTOR){
                        events[fds].fd = device_fd;
                        events[fds].events = POLLIN | POLLPRI ;
                        fds++;
                    }

                    if(client_fd_ != INVALID_DESCRIPTOR && device_fd != INVALID_DESCRIPTOR){
                        for(size_t i = 0; i < requests_.size(); ++i){
                            common::buffer_type request = requests_[i];
                            common::Error err = common::net::write_to_socket(device_fd, (const char*)request.data(), request.size(), nwrite);
                            if(err && err->isError()){
                                DEBUG_MSG_ERROR(err);
                            }
                        }
                        requests_.clear();
                    }

                    int rc = poll(events, fds, 1000);
                    for (size_t i = 0; i < fds && !stop_; ++i){
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
                                    err = accept(client_info);
                                    if(err && err->isError()){
                                        DEBUG_MSG_ERROR(err);
                                        break;
                                    }
                                    else{
                                        new_sd = client_info.fd();
                                        device_fd = new_sd;
                                    }
                                }
                            }

                            if(cevents & POLLPRI){
                                NOTREACHED();
                            }
                        }
                        else if(fd == device_fd){ //device response
                            if (cevents & POLLIN){ //read
                                char buff[BUF_SIZE] = {0};
                                ssize_t nread = 0;
                                common::Error err = common::net::read_from_socket(fd, buff, BUF_SIZE, nread);
                                if((err && err->isError()) || nread == 0){
                                    common::net::close(device_fd);
                                    device_fd = INVALID_DESCRIPTOR;
                                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "relay[%s] device client closed.", host_str);
                                }
                                else{
                                    ssize_t nwrite = 0;
                                    err = common::net::write_to_socket(client_fd_, buff, nread, nwrite);
                                    if(err && err->isError()){
                                        DEBUG_MSG_ERROR(err);
                                    }
                                }
                            }

                            if(cevents & POLLPRI){
                                NOTREACHED();
                            }
                        }
                        else if(fd == client_fd_){ // client request
                            if (cevents & POLLIN){ //read
                                char buff[BUF_SIZE] = {0};
                                ssize_t nread = 0;
                                common::Error err = common::net::read_from_socket(fd, buff, BUF_SIZE, nread);
                                if((err && err->isError()) || nread == 0){
                                    common::net::close(client_fd_);
                                    client_fd_ = INVALID_DESCRIPTOR;
                                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "relay[%s] client closed.", host_str);
                                }
                                else{
                                    ssize_t nwrite = 0;
                                    err = common::net::write_to_socket(device_fd, buff, nread, nwrite);
                                    if(err && err->isError()){
                                        DEBUG_MSG_ERROR(err);
                                    }
                                }
                            }

                            if(cevents & POLLPRI){
                                NOTREACHED();
                            }
                        }
                    }
                }

                if(client_fd_ != INVALID_DESCRIPTOR){
                    common::net::close(client_fd_);
                    client_fd_ = INVALID_DESCRIPTOR;
                }

                if(device_fd != INVALID_DESCRIPTOR){
                    common::net::close(device_fd);
                    device_fd = INVALID_DESCRIPTOR;
                }

                close();

                return EXIT_SUCCESS;
            }

            void IRelayServer::addRequest(const common::buffer_type& request)
            {
                requests_.push_back(request);
            }
        }
    }
}
