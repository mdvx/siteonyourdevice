#pragma once

#include <ev.h>

#include <functional>

#include "common/thread/platform_thread.h"

#define ev_io_init_fasto(ev, cb, fd, events) ev_io_init(&(ev->w_io_), cb, fd, events)
#define ev_io_start_fasto(loop, ev) ev_io_start(loop, &(ev->w_io_))
#define ev_io_stop_fasto(loop, ev) ev_io_stop(loop, &(ev->w_io_))

#define ev_async_init_fasto(ev, cb) ev_async_init(&(ev->async_), cb)
#define ev_async_start_fasto(loop, ev) ev_async_start(loop, &(ev->async_))
#define ev_async_stop_fasto(loop, ev) ev_async_stop(loop, &(ev->async_))
#define ev_async_send_fasto(loop, ev) ev_async_send(loop, &(ev->async_))

namespace fasto
{
    namespace siteonyourdevice
    {
        struct fasto_c_sync
        {
            ev_io w_io_;
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

        typedef std::function<void()> function_type;

        class EvLoopObserver
        {
        public:
            virtual ~EvLoopObserver();

            virtual void preLooped(struct LibEvLoop* loop) = 0;
            virtual void stoped(struct LibEvLoop* loop) = 0;
            virtual void postLooped(struct LibEvLoop* loop) = 0;
        };

        class LibEvLoop
        {
        public:
            LibEvLoop();
            ~LibEvLoop();

            void setObserver(EvLoopObserver* observer);

            void start_io(fasto_c_sync *io);
            void stop_io(fasto_c_sync *io);
            void start_io(fasto_s_sync *io);
            void stop_io(fasto_s_sync *io);

            void execInLoopThread(function_type async_cb);

            int exec();
            void stop();

        private:
            void stopImpl();

            static void stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents);

            struct ev_loop * const loop_;
            EvLoopObserver * observer_;
            common::thread::platform_threadid_type exec_id_;
            fasto_s_async * async_stop_;
        };
    }
}
