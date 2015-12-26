#include "server/relay_server.h"

#include <sys/poll.h>

#include "common/thread/thread_manager.h"
#include "common/net/net.h"
#include "common/logger.h"

#include "server/inner/inner_tcp_client.h"
#include "server/server_config.h"

#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    {
        IRelayServer::IRelayServer(InnerTcpClient *parent, client_t client)
            : ServerSocketTcp(g_relay_server_host), stop_(false), client_(client), relayThread_(), parent_(parent)
        {
            relayThread_ = THREAD_MANAGER()->createThread(&IRelayServer::exec, this);
        }

        IRelayServer::client_t IRelayServer::client() const
        {
            return client_;
        }

        void IRelayServer::setClient(client_t client)
        {
            client_ = client;
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
            static const int max_poll_ev = 3;
            common::Error err = bind();
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
            const std::string createConnection = createSocketCmd(host());
            err = parent_->write(createConnection.c_str(), createConnection.size(), nwrite); //inner command write
            if(err && err->isError()){;
                NOTREACHED();
                return EXIT_FAILURE;
            }

            const common::net::socket_descr_type server_fd = info_.fd();
            common::net::socket_descr_type client_fd = INVALID_DESCRIPTOR;

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
                        if(err && err->isError()){
                            DEBUG_MSG_ERROR(err);
                        }
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
                        if(fd == client_fd){ //device response
                            if (cevents & POLLIN){ //read
                                char buff[BUF_SIZE] = {0};
                                ssize_t nread = 0;
                                common::Error err = common::net::read_from_socket(fd, buff, BUF_SIZE, nread);
                                if((err && err->isError()) || nread == 0){
                                    common::net::close(client_fd);
                                    client_fd = INVALID_DESCRIPTOR;
                                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "(relay) device client closed!");
                                }
                                else{
                                    ssize_t nwrite = 0;
                                    err = common::net::write_to_socket(rm_client_fd, buff, nread, nwrite);
                                    if(err && err->isError()){
                                        DEBUG_MSG_ERROR(err);
                                    }
                                }
                            }

                            if(cevents & POLLPRI){
                                NOTREACHED();
                            }
                        }
                        else if(fd == rm_client_fd){ // client request
                            if (cevents & POLLIN){ //read
                                char buff[BUF_SIZE] = {0};
                                ssize_t nread = 0;
                                common::Error err = common::net::read_from_socket(fd, buff, BUF_SIZE, nread);
                                if((err && err->isError()) || nread == 0){
                                    rclient->close();
                                    client_.reset();
                                    DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "(relay) client closed!");
                                }
                                else{
                                    ssize_t nwrite = 0;
                                    err = common::net::write_to_socket(client_fd, buff, nread, nwrite);
                                    if(err && err->isError()){
                                        DEBUG_MSG_ERROR(err);
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

            DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "(relay) exit loop!");
            if(client_){
                client_->close();
                client_.reset();
            }
            
            if(client_fd != INVALID_DESCRIPTOR){
                common::net::close(client_fd);
                client_fd = INVALID_DESCRIPTOR;
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
