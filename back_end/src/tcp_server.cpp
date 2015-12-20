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
        ITcpLoop::ITcpLoop(ITcpLoopObserver* observer)
            : loop_(new LibEvLoop), observer_(observer), total_clients_(0), id_()
        {
            loop_->setObserver(this);
        }

        ITcpLoop::~ITcpLoop()
        {
            delete loop_;
        }

        int ITcpLoop::exec()
        {
            return loop_->exec();
        }

        void ITcpLoop::stop()
        {
            loop_->stop();
        }

        void ITcpLoop::registerClient(const common::net::socket_info &info)
        {
            TcpClient* client = createClient(info);
            registerClient(client);
        }

        void ITcpLoop::unregisterClient(TcpClient * client)
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

        void ITcpLoop::registerClient(TcpClient * client)
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

        void ITcpLoop::closeClient(TcpClient *client)
        {
            CHECK(client->server() == this);
            loop_->stop_io(client->read_io_);

            if(observer_){
                observer_->closed(client);
            }
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully disconnected client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), --total_clients_);
        }

        common::id_counter<ITcpLoop>::type_t ITcpLoop::id() const
        {
            return id_.id();
        }

        void ITcpLoop::execInLoopThread(async_loop_exec_function_type func)
        {
            loop_->execInLoopThread(func);
        }

        void ITcpLoop::setName(const std::string& name)
        {
            name_ = name;
        }

        std::string ITcpLoop::name() const
        {
            return name_;
        }

        std::string ITcpLoop::formatedName() const
        {
            return common::MemSPrintf("[%s][%s(%" PRIu32 ")]", name(), className(), id());
        }

        TcpClient * ITcpLoop::createClient(const common::net::socket_info& info)
        {
            return new TcpClient(this, info);
        }

        void ITcpLoop::read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
        {
            TcpClient* pclient = reinterpret_cast<TcpClient *>(watcher->data);
            ITcpLoop* pserver = pclient->server();
            LibEvLoop* evloop = reinterpret_cast<LibEvLoop *>(ev_userdata(loop));
            CHECK(pserver && pserver->loop_ == evloop);

            if(EV_ERROR & revents){
                return;
            }

            if(pserver->observer_){
                pserver->observer_->dataReceived(pclient);
            }
        }

        void ITcpLoop::preLooped(LibEvLoop* loop)
        {
            if(observer_){
                observer_->preLooped(this);
            }
        }

        void ITcpLoop::stoped(LibEvLoop* loop)
        {

        }

        void ITcpLoop::postLooped(LibEvLoop* loop)
        {
            if(observer_){
                observer_->postLooped(this);
            }
        }

        // server
        TcpServer::TcpServer(const common::net::hostAndPort& host, ITcpLoopObserver* observer)
            : ITcpLoop(observer), sock_(host), accept_io_((struct ev_io*)calloc(1, sizeof(struct ev_io)))
        {
            accept_io_->data = this;
        }

        TcpServer::~TcpServer()
        {
            free(accept_io_);
            accept_io_ = NULL;
        }

        void TcpServer::preLooped(LibEvLoop* loop)
        {
            int fd = sock_.fd();
            ev_io_init(accept_io_, accept_cb, fd, EV_READ);
            loop->start_io(accept_io_);
            ITcpLoop::preLooped(loop);
        }

        void TcpServer::stoped(LibEvLoop* loop)
        {
            loop->stop_io(accept_io_);
            ITcpLoop::stoped(loop);
        }

        common::Error TcpServer::bind()
        {
            return sock_.bind();
        }

        common::Error TcpServer::listen(int backlog)
        {
            return sock_.listen(backlog);
        }

        void TcpServer::stop()
        {
            sock_.close();
            ITcpLoop::stop();
        }

        const char* TcpServer::className() const
        {
            return "TcpServer";
        }

        common::net::hostAndPort TcpServer::host() const
        {
            return sock_.host();
        }

        common::Error TcpServer::accept(common::net::socket_info& info)
        {
            return sock_.accept(info);
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
            common::Error er = pserver->accept(sinfo);

            if (er && er->isError()){
                return ;
            }

            TcpClient* pclient = pserver->createClient(sinfo);
            pserver->registerClient(pclient);
        }

        ITcpLoopObserver::~ITcpLoopObserver()
        {

        }
    }
}
