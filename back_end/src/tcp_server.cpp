#include "tcp_server.h"

#include <inttypes.h>

#include "common/logger.h"

#include "tcp_client.h"

namespace
{
#ifdef OS_WIN
    struct WinsockInit
    {
            WinsockInit()
            {
                WSADATA d;
                if ( WSAStartup(MAKEWORD(2,2), &d) != 0 ) {
                    _exit(1);
                }
            }
            ~WinsockInit(){ WSACleanup(); }
    } winsock_init;
#endif
    struct SigIgnInit
    {
        SigIgnInit()
        {
        #if defined(COMPILER_MINGW)
            signal(13, SIG_IGN);
        #elif defined(COMPILER_MSVC)
        #else
            signal(SIGPIPE, SIG_IGN);
        #endif
        }
    } sig_init;
}

namespace fasto
{
    namespace siteonyourdevice
    {
        // server
        TcpServer::TcpServer(const common::net::hostAndPort& host, TcpServerObserver* observer)
            : sock_(host), total_clients_(0), observer_(observer), loop_(new LibEvLoop),
              accept_io_((struct ev_io*)calloc(1, sizeof(struct ev_io))), id_()
        {
            loop_->setObserver(this);
            accept_io_->data = this;
        }

        TcpServer::~TcpServer()
        {
            delete loop_;

            free(accept_io_);
            accept_io_ = NULL;
        }

        void TcpServer::preLooped(LibEvLoop* loop)
        {
            int fd = sock_.fd();
            ev_io_init(accept_io_, accept_cb, fd, EV_READ);
            loop->start_io(accept_io_);
            if(observer_){
                observer_->preLooped(this);
            }
        }

        void TcpServer::stoped(LibEvLoop* loop)
        {
            loop->stop_io(accept_io_);
        }

        void TcpServer::postLooped(LibEvLoop* loop)
        {
            if(observer_){
                observer_->postLooped(this);
            }
        }

        common::ErrnoError TcpServer::bind()
        {
            return sock_.bind();
        }

        common::ErrnoError TcpServer::listen(int backlog)
        {
            return sock_.listen(backlog);
        }

        int TcpServer::exec()
        {
            return loop_->exec();
        }

        void TcpServer::stop()
        {
            close();
            loop_->stop();
        }

        common::net::hostAndPort TcpServer::host() const
        {
            return sock_.host();
        }

        void TcpServer::registerClient(const common::net::socket_info &info)
        {
            TcpClient* client = createClient(info);
            registerClient(client);
        }

        void TcpServer::unregisterClient(TcpClient * client)
        {
            CHECK(client->server() == this);
            loop_->stop_io(client->read_io_);

            if(observer_){
                observer_->moved(client);
            }

            client->server_ = NULL;
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully unregister client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), id(), --total_clients_);
        }

        void TcpServer::registerClient(TcpClient * client)
        {
            CHECK(client->server() == this);
            // Initialize and start watcher to read client requests
            ev_io_init(client->read_io_, read_cb, client->fd(), EV_READ);
            loop_->start_io(client->read_io_);

            if(observer_){
                observer_->accepted(client);
            }
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully connected with client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), id(), ++total_clients_);
        }

        TcpClient * TcpServer::createClient(const common::net::socket_info& info)
        {
            return new TcpClient(this, info);
        }

        void TcpServer::closeClient(TcpClient *client)
        {
            CHECK(client->server() == this);
            loop_->stop_io(client->read_io_);

            if(observer_){
                observer_->closed(client);
            }
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully disconnected client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), --total_clients_);
        }

        void TcpServer::execInServerThread(function_type func)
        {
            loop_->execInLoopThread(func);
        }

        void TcpServer::setName(const std::string& name)
        {
            name_ = name;
        }

        std::string TcpServer::name() const
        {
            return name_;
        }

        common::id_counter<TcpServer>::type_t TcpServer::id() const
        {
            return id_.id();
        }

        const char* TcpServer::className() const
        {
            return "TcpServer";
        }

        std::string TcpServer::formatedName() const
        {
            return common::MemSPrintf("[%s][%s(%" PRIu32 ")]", name(), className(), id());
        }

        void TcpServer::close()
        {
            sock_.close();
        }

        common::ErrnoError TcpServer::accept(common::net::socket_info& info)
        {
            return sock_.accept(info);
        }

        void TcpServer::read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
        {
            TcpClient* pclient = reinterpret_cast<TcpClient *>(watcher->data);
            TcpServer* pserver = pclient->server();
            LibEvLoop* evloop = reinterpret_cast<LibEvLoop *>(ev_userdata(loop));
            CHECK(pserver && pserver->loop_ == evloop);

            if(EV_ERROR & revents){
                return;
            }

            if(pserver->observer_){
                pserver->observer_->dataReceived(pclient);
            }
        }

        void TcpServer::accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
        {
            TcpServer* pserver = reinterpret_cast<TcpServer *>(watcher->data);
            LibEvLoop* evloop = reinterpret_cast<LibEvLoop *>(ev_userdata(loop));
            CHECK(pserver && pserver->loop_ == evloop);

            if(EV_ERROR & revents){
                return;
            }

            CHECK(watcher->fd == pserver->sock_.fd());

            common::net::socket_info sinfo;
            common::ErrnoError er = pserver->accept(sinfo);

            if (er && er->isError()){
                return ;
            }

            TcpClient* pclient = pserver->createClient(sinfo);
            pserver->registerClient(pclient);
        }

        TcpServerObserver::~TcpServerObserver()
        {

        }
    }
}
