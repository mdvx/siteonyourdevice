#include "tcp_server.h"

#include <inttypes.h>
#include <ev.h>

#include "common/logger.h"
#include "common/thread/platform_thread.h"

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

#define ev_io_init_fasto(ev, cb, fd, events) ev_io_init(&(ev->w_io_), cb, fd, events)
#define ev_io_start_fasto(loop, ev) ev_io_start(loop, &(ev->w_io_))
#define ev_io_stop_fasto(loop, ev) ev_io_stop(loop, &(ev->w_io_))

#define ev_async_init_fasto(ev, cb) ev_async_init(&(ev->async_), cb)
#define ev_async_start_fasto(loop, ev) ev_async_start(loop, &(ev->async_))
#define ev_async_stop_fasto(loop, ev) ev_async_stop(loop, &(ev->async_))

namespace fasto
{
    namespace siteonyourdevice
    {
        struct fasto_cs_sync
        {
            ev_io w_io_;
            void* server_;
            void* client_;
        };

        struct fasto_s_sync
        {
            ev_io w_io_;
            void* server_;
        };

        struct fasto_s_async
        {
            ev_async async_;
            void* server_;
        };

        struct fasto_c_async_cb
        {
            ev_async async_;
            TcpServer::function_type func_;
        };

        struct fasto_cs_async
        {
            ev_async async_;
            void* client_;
            void* server_;
        };

        struct TcpServer::event_impl
        {
            event_impl(TcpServer* server)
                : loop_(ev_loop_new(0)), pserver_(server)
            {

            }

            ~event_impl()
            {
                ev_loop_destroy(loop_);
            }

            void closeClient(TcpClient* client)
            {
                CHECK(exec_id_ == common::thread::PlatformThread::currentId());
                ev_io_stop_fasto(loop_, client->read_io_);

                if(pserver_->observer_){
                    pserver_->observer_->closed(client);
                }
            }

            void unRegisterClient(TcpClient* client)
            {
                CHECK(exec_id_ == common::thread::PlatformThread::currentId());
                ev_io_stop_fasto(loop_, client->read_io_);

                if(pserver_->observer_){
                    pserver_->observer_->moved(client);
                }
            }

            void registerClient(TcpClient* client)
            {
                CHECK(exec_id_ == common::thread::PlatformThread::currentId());
                // Initialize and start watcher to read client requests
                ev_io_init_fasto(client->read_io_, read_cb, client->fd(), EV_READ);
                ev_io_start_fasto(loop_, client->read_io_);

                if(pserver_->observer_){
                    pserver_->observer_->accepted(client);
                }
            }

            void execInServerThread(TcpServer::function_type async_cb)
            {
                if(exec_id_ == common::thread::PlatformThread::currentId()){
                    async_cb();
                }
                else{
                    fasto_c_async_cb* cb = (struct fasto_c_async_cb*)calloc(1, sizeof(struct fasto_c_async_cb));
                    cb->func_ = async_cb;

                    ev_async_init_fasto(cb, async_exec_cb);
                    ev_async_start_fasto(loop_, cb);
                    ev_async_send(loop_, &cb->async_);
                }
            }

            struct ev_loop * const loop_;
            TcpServer* const pserver_;
            common::thread::platform_threadid_type exec_id_;

            int exec()
            {
                exec_id_ = common::thread::PlatformThread::currentId();

                fasto_s_async* pw_stop = pserver_->async_stop_;
                ev_async_start_fasto(loop_, pw_stop);

                fasto_s_sync* pw_accept = pserver_->accept_io_;
                ev_io_init_fasto(pw_accept, accept_cb, pserver_->sock_.fd(), EV_READ);
                ev_io_start_fasto(loop_, pw_accept);
                if(pserver_->observer_){
                    pserver_->observer_->preLooped(pserver_);
                }
                ev_loop(loop_, 0);
                if(pserver_->observer_){
                    pserver_->observer_->postLooped(pserver_);
                }
                return EXIT_SUCCESS;
            }

            void stop()
            {
                pserver_->close();
                ev_async_send(loop_, &pserver_->async_stop_->async_);
            }

            static void stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
            {
                fasto_s_async* ioserver = reinterpret_cast<fasto_s_async *>(watcher);
                TcpServer* pserver = reinterpret_cast<TcpServer *>(ioserver->server_);

                ev_async_stop_fasto(loop, pserver->async_stop_);
                ev_io_stop_fasto(loop, pserver->accept_io_);
                ev_unloop(loop, EVUNLOOP_ONE);
            }

        private:
            static void async_exec_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
            {
                ev_async_stop(loop, watcher);
                fasto_c_async_cb* ioclient = reinterpret_cast<fasto_c_async_cb *>(watcher);
                ioclient->func_();
                free(ioclient);
            }

            static void read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
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

            static void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
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
        };

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
            : sock_(host), total_clients_(0), observer_(observer), impl_(new TcpServer::event_impl(this)),
              accept_io_((struct fasto_s_sync*)calloc(1, sizeof(struct fasto_s_sync))),
              async_stop_((struct fasto_s_async*)calloc(1, sizeof(struct fasto_s_async))), id_()
        {
            accept_io_->server_ = this;

            async_stop_->server_ = this;
            ev_async_init_fasto(async_stop_, event_impl::stop_cb);
        }

        TcpServer::~TcpServer()
        {
            close();

            delete impl_;

            free(async_stop_);
            async_stop_ = NULL;

            free(accept_io_);
            accept_io_ = NULL;
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
            return impl_->exec();
        }

        void TcpServer::stop()
        {
            impl_->stop();
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
            impl_->unRegisterClient(client);
            client->server_ = NULL;
            client->read_io_->server_ = NULL;
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully unregister client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), id(), --total_clients_);
        }

        void TcpServer::registerClient(TcpClient * client)
        {
            DCHECK(client->server() == this);
            impl_->registerClient(client);
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
            impl_->closeClient(client);
            DEBUG_MSG_FORMAT<256>(common::logging::L_INFO, "Successfully disconnected client[%s], from server[%s], %d client(s) connected.",
                                  client->formatedName(), formatedName(), --total_clients_);
        }

        void TcpServer::execInServerThread(function_type func)
        {
            impl_->execInServerThread(func);
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

        TcpServerObserver::~TcpServerObserver()
        {

        }
    }
}
