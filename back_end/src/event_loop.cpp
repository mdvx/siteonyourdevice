#include "event_loop.h"

#include <stdlib.h>

#define ev_async_cb_init_fasto(ev, cb) ev_async_init(&(ev->async_), cb)
#define ev_async_cb_start_fasto(loop, ev) ev_async_start(loop, &(ev->async_))
#define ev_async_cb_stop_fasto(loop, ev) ev_async_stop(loop, &(ev->async_))
#define ev_async_cb_send_fasto(loop, ev) ev_async_send(loop, &(ev->async_))

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace
        {
            struct fasto_async_cb
            {
                ev_async async_;
                async_loop_exec_function_type func_;
            };

            void async_exec_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
            {
                ev_async_stop(loop, watcher);
                fasto_async_cb* ioclient = reinterpret_cast<fasto_async_cb *>(watcher);
                ioclient->func_();
                free(ioclient);
            }
        }

        EvLoopObserver::~EvLoopObserver()
        {

        }

        LibEvLoop::LibEvLoop()
            : loop_(ev_loop_new(0)), observer_(NULL), exec_id_(),
              async_stop_((struct ev_async*)calloc(1, sizeof(struct ev_async)))
        {
            ev_async_init(async_stop_, stop_cb);
            ev_set_userdata(loop_, this);
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

        void LibEvLoop::start_io(ev_io *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_start(loop_, io);
        }

        void LibEvLoop::stop_io(ev_io *io)
        {
            CHECK(exec_id_ == common::thread::PlatformThread::currentId());
            ev_io_stop(loop_, io);
        }

        void LibEvLoop::execInLoopThread(async_loop_exec_function_type async_cb)
        {
            if(exec_id_ == common::thread::PlatformThread::currentId()){
                async_cb();
            }
            else{
                fasto_async_cb* cb = (struct fasto_async_cb*)calloc(1, sizeof(struct fasto_async_cb));
                cb->func_ = async_cb;

                ev_async_cb_init_fasto(cb, async_exec_cb);
                ev_async_cb_start_fasto(loop_, cb);
                ev_async_cb_send_fasto(loop_, cb);
            }
        }

        int LibEvLoop::exec()
        {
            exec_id_ = common::thread::PlatformThread::currentId();

            ev_async_start(loop_, async_stop_);
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
            ev_async_send(loop_, async_stop_);
        }


        void LibEvLoop::stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents)
        {
            LibEvLoop* evloop = reinterpret_cast<LibEvLoop *>(ev_userdata(loop));
            ev_async_stop(loop, evloop->async_stop_);
            if(evloop->observer_){
                evloop->observer_->stoped(evloop);
            }
            ev_unloop(loop, EVUNLOOP_ONE);
        }
    }
}
