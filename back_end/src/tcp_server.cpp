#include "tcp_server.h"

#include <inttypes.h>

#include "common/logger.h"
#include "common/multi_threading/types.h"

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

namespace
{
    common::multi_threading::mutex_t g_exists_servers_mutex_;
    std::vector<fasto::siteonyourdevice::TcpServer*> g_exists_servers_;
}

namespace fasto
{
    namespace siteonyourdevice
    {
        ITcpLoop::ITcpLoop(ITcpLoopObserver* observer)
            : loop_(new LibEvLoop), observer_(observer), clients_(), id_()
        {
            loop_->setObserver(this);
        }

        ITcpLoop::~ITcpLoop()
        {
            delete loop_;
        }

        int ITcpLoop::exec()
        {
            int res = loop_->exec();
            return res;
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
            clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
            DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "Successfully unregister client[%s], from server[%s], %" PRIuS " client(s) connected.",
                                  client->formatedName(), formatedName(), clients_.size());
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
            clients_.push_back(client);
            DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "Successfully connected with client[%s], from server[%s], %" PRIuS " client(s) connected.",
                                  client->formatedName(), formatedName(), clients_.size());
        }

        void ITcpLoop::closeClient(TcpClient *client)
        {
            CHECK(client->server() == this);
            loop_->stop_io(client->read_io_);

            if(observer_){
                observer_->closed(client);
            }
            clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
            DEBUG_MSG_FORMAT<512>(common::logging::L_INFO, "Successfully disconnected client[%s], from server[%s], %" PRIuS " client(s) connected.",
                                  client->formatedName(), formatedName(), clients_.size());
        }

        common::id_counter<ITcpLoop>::type_t ITcpLoop::id() const
        {
            return id_.id();
        }

        void ITcpLoop::execInLoopThread(async_loop_exec_function_type func)
        {
            loop_->execInLoopThread(func);
        }

        bool ITcpLoop::isLoopThread() const
        {
            return loop_->isLoopThread();
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
            return common::MemSPrintf("[%s][%s(%ju)]", name(), className(), id());
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

            const std::vector<TcpClient *> cl = clients_;

            for(size_t i = 0; i < cl.size(); ++i){
                TcpClient * client = cl[i];
                client->close();
                delete client;
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

        TcpServer* TcpServer::findExistServerByHost(const common::net::hostAndPort& host)
        {
            if(!host.isValid()){
                return NULL;
            }

            common::multi_threading::unique_lock<common::multi_threading::mutex_t> loc(g_exists_servers_mutex_);
            for(size_t i = 0; i < g_exists_servers_.size(); ++i){
                TcpServer* server = g_exists_servers_[i];
                if(server && server->host() == host){
                    return server;
                }
            }
            return NULL;
        }

        void TcpServer::preLooped(LibEvLoop* loop)
        {
            int fd = sock_.fd();
            ev_io_init(accept_io_, accept_cb, fd, EV_READ);
            loop->start_io(accept_io_);

            {
                common::multi_threading::unique_lock<common::multi_threading::mutex_t> loc(g_exists_servers_mutex_);
                g_exists_servers_.push_back(this);
            }
            ITcpLoop::preLooped(loop);
        }

        void TcpServer::postLooped(LibEvLoop* loop)
        {
            {
                common::multi_threading::unique_lock<common::multi_threading::mutex_t> loc(g_exists_servers_mutex_);
                g_exists_servers_.erase(std::remove(g_exists_servers_.begin(), g_exists_servers_.end(), this), g_exists_servers_.end());
            }
            ITcpLoop::postLooped(loop);
        }

        void TcpServer::stoped(LibEvLoop* loop)
        {
            common::Error err = sock_.close();
            if(err && err->isError()){
                DEBUG_MSG_ERROR(err);
            }

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
            common::Error err = pserver->accept(sinfo);

            if (err && err->isError()){
                DEBUG_MSG_ERROR(err);
                return;
            }

            TcpClient* pclient = pserver->createClient(sinfo);
            pserver->registerClient(pclient);
        }

        ITcpLoopObserver::~ITcpLoopObserver()
        {

        }
    }
}
