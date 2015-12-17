#include "tcp_server.h"

#include <inttypes.h>

#include "common/logger.h"

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
        // client
        TcpClient::TcpClient(TcpServer *server, const common::net::socket_info &info)
            : server_(server), read_io_((struct fasto_cs_sync*)calloc(1, sizeof(struct fasto_cs_sync))),
              sock_(info), name_(), id_()
        {
            read_io_->client_ = this;
            read_io_->server_ = server;
        }

        common::net::socket_info TcpClient::info() const
        {
            return sock_.info();
        }

        int TcpClient::fd() const
        {
            return sock_.fd();
        }

        common::ErrnoError TcpClient::write(const char* data, uint16_t size, ssize_t &nwrite)
        {
            return sock_.write(data, size, nwrite);
        }

        common::ErrnoError TcpClient::read(char* outData, uint16_t maxSize, ssize_t &nread)
        {
            return sock_.read(outData, maxSize, nread);
        }

        TcpClient::~TcpClient()
        {
            free(read_io_);
            read_io_ = NULL;
        }

        TcpServer* TcpClient::server() const
        {
            return server_;
        }

        void TcpClient::close()
        {
            if(server_){
                server_->closeClient(this);
            }
            sock_.close();
        }

        void TcpClient::setName(const std::string& name)
        {
            name_ = name;
        }

        std::string TcpClient::name() const
        {
            return name_;
        }

        common::id_counter<TcpClient>::type_t TcpClient::id() const
        {
            return id_.id();
        }

        const char* TcpClient::className() const
        {
            return "TcpClient";
        }

        std::string TcpClient::formatedName() const
        {
            return common::MemSPrintf("[%s][%s(%" PRIu32 ")]", name(), className(), id());
        }

        // server
        TcpServer::TcpServer(const common::net::hostAndPort& host, TcpServerObserver* observer)
            : sock_(host), total_clients_(0), observer_(observer), impl_(),
              accept_io_((struct fasto_s_sync*)calloc(1, sizeof(struct fasto_s_sync))), id_()
        {
            impl_.setObserver(this);
            accept_io_->server_ = this;
        }

        TcpServer::~TcpServer()
        {
            free(accept_io_);
            accept_io_ = NULL;
        }

        void TcpServer::preLooped(EvLoop* loop)
        {
            int fd = sock_.fd();
            ev_io_init_fasto(accept_io_, accept_cb, fd, EV_READ);
            loop->start_io(accept_io_);
            if(observer_){
                observer_->preLooped(this);
            }
        }

        void TcpServer::stoped(EvLoop* loop)
        {
            loop->stop_io(accept_io_);
        }

        void TcpServer::postLooped(EvLoop* loop)
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
            return impl_.exec();
        }

        void TcpServer::stop()
        {
            close();
            impl_.stop();
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
            DCHECK(client->server() == this);
            impl_.stop_io(client->read_io_);

            if(observer_){
                observer_->moved(client);
            }

            client->server_ = NULL;
            client->read_io_->server_ = NULL;
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully unregister client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), id(), --total_clients_);
        }

        void TcpServer::registerClient(TcpClient * client)
        {
            DCHECK(client->server() == this);
            // Initialize and start watcher to read client requests
            ev_io_init_fasto(client->read_io_, read_cb, client->fd(), EV_READ);
            impl_.start_io(client->read_io_);

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
            DCHECK(client->server() == this);
            impl_.stop_io(client->read_io_);

            if(observer_){
                observer_->closed(client);
            }
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully disconnected client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), --total_clients_);
        }

        void TcpServer::execInServerThread(function_type func)
        {
            impl_.execInLoopThread(func);
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
            fasto_cs_sync* ioserver = reinterpret_cast<fasto_cs_sync *>(watcher);

            TcpClient* pclient = reinterpret_cast<TcpClient *>(ioserver->client_);
            TcpServer* pserver = reinterpret_cast<TcpServer *>(ioserver->server_);

            if(EV_ERROR & revents){
                return;
            }

            if(pserver->observer_){
                pserver->observer_->dataReceived(pclient);
            }
        }

        void TcpServer::accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
        {
            fasto_s_sync* ioserver = reinterpret_cast<fasto_s_sync *>(watcher);
            TcpServer* pserver = reinterpret_cast<TcpServer *>(ioserver->server_);
            DCHECK(pserver);

            if(EV_ERROR & revents){
                return;
            }

            DCHECK(watcher->fd == pserver->sock_.fd());

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
