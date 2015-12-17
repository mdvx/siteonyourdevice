#include "event_loop.h"

#include <inttypes.h>

#include "common/logger.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        EvLoop::EvLoop()
            : observer_(NULL), loop_(ev_loop_new(0)), exec_id_(),
              async_stop_((struct fasto_s_async*)calloc(1, sizeof(struct fasto_s_async)))
        {
            ev_async_init_fasto(async_stop_, stop_cb);
            async_stop_->server_ = this;
        }

        void EvLoop::setObserver(EvLoopObserver* observer)
        {
            observer_ = observer;
        }

        void EvLoop::start_io(fasto_cs_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_start_fasto(loop_, io);
        }

        void EvLoop::stop_io(fasto_cs_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_stop_fasto(loop_, io);
        }

        void EvLoop::start_io(fasto_s_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_start_fasto(loop_, io);
        }

        void EvLoop::stop_io(fasto_s_sync *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_stop_fasto(loop_, io);
        }

        void EvLoop::send_async_io(fasto_s_async *io)
        {
            ev_async_send(loop_, &io->async_);
        }

        void EvLoop::execInLoopThread(function_type async_cb)
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

        int EvLoop::exec()
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

        void EvLoop::stop()
        {
            ev_async_send(loop_, &async_stop_->async_);
        }

        EvLoop::~EvLoop()
        {
            ev_loop_destroy(loop_);

            free(async_stop_);
            async_stop_ = NULL;
        }
        void EvLoop::stopImpl()
        {
            ev_async_stop_fasto(loop_, async_stop_);
            if(observer_){
                observer_->stoped(this);
            }
            ev_unloop(loop_, EVUNLOOP_ONE);
        }

        void EvLoop::stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
        {
            fasto_s_async* iostop = reinterpret_cast<fasto_s_async *>(watcher);
            EvLoop* evloop = reinterpret_cast<EvLoop *>(iostop->server_);
            evloop->stopImpl();
        }

        void EvLoop::async_exec_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
        {
            ev_async_stop(loop, watcher);
            fasto_c_async_cb* ioclient = reinterpret_cast<fasto_c_async_cb *>(watcher);
            ioclient->func_();
            free(ioclient);
        }
    }
}
