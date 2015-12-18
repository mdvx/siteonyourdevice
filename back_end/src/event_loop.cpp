#include "event_loop.h"

#include <stdlib.h>

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace
        {
            struct fasto_c_async_cb
            {
                ev_async async_;
                function_type func_;
            };

            void async_exec_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
            {
                ev_async_stop(loop, watcher);
                fasto_c_async_cb* ioclient = reinterpret_cast<fasto_c_async_cb *>(watcher);
                ioclient->func_();
                free(ioclient);
            }
        }

        EvLoopObserver::~EvLoopObserver()
        {

        }

        LibEvLoop::LibEvLoop()
            : observer_(NULL), loop_(ev_loop_new(0)), exec_id_(),
              async_stop_((struct fasto_s_async*)calloc(1, sizeof(struct fasto_s_async)))
        {
            ev_async_init_fasto(async_stop_, stop_cb);
            async_stop_->server_ = this;
        }

        LibEvLoop::~LibEvLoop()
        {
            ev_loop_destroy(loop_);

            free(async_stop_);
            async_stop_ = NULL;
        }

        void LibEvLoop::setObserver(EvLoopObserver* observer)
        {
            observer_ = observer;
        }

        void LibEvLoop::start_io(fasto_c_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_start_fasto(loop_, io);
        }

        void LibEvLoop::stop_io(fasto_c_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_stop_fasto(loop_, io);
        }

        void LibEvLoop::start_io(fasto_s_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_start_fasto(loop_, io);
        }

        void LibEvLoop::stop_io(fasto_s_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_stop_fasto(loop_, io);
        }

        void LibEvLoop::execInLoopThread(function_type async_cb)
        {
            if(exec_id_ == common::thread::PlatformThread::currentId()){
                async_cb();
            }
            else{
                fasto_c_async_cb* cb = (struct fasto_c_async_cb*)calloc(1, sizeof(struct fasto_c_async_cb));
                cb->func_ = async_cb;

                ev_async_init_fasto(cb, async_exec_cb);
                ev_async_start_fasto(loop_, cb);
                ev_async_send_fasto(loop_, cb);
            }
        }

        int LibEvLoop::exec()
        {
            exec_id_ = common::thread::PlatformThread::currentId();

            ev_async_start_fasto(loop_, async_stop_);
            if(observer_){
                observer_->preLooped(this);
            }
            ev_loop(loop_, 0);
            if(observer_){
                observer_->postLooped(this);
            }
            return EXIT_SUCCESS;
        }

        void LibEvLoop::stop()
        {
            ev_async_send_fasto(loop_, async_stop_);
        }

        void LibEvLoop::stopImpl()
        {
            ev_async_stop_fasto(loop_, async_stop_);
            if(observer_){
                observer_->stoped(this);
            }
            ev_unloop(loop_, EVUNLOOP_ONE);
        }

        void LibEvLoop::stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
        {
            fasto_s_async* iostop = reinterpret_cast<fasto_s_async *>(watcher);
            LibEvLoop* evloop = reinterpret_cast<LibEvLoop *>(iostop->server_);
            evloop->stopImpl();
        }
    }
}
