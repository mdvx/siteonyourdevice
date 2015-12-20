#pragma once

#include <ev.h>

#include <functional>

#include "common/thread/platform_thread.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        typedef std::function<void()> async_loop_exec_function_type;

        class EvLoopObserver
        {
        public:
            virtual ~EvLoopObserver();

            virtual void preLooped(class LibEvLoop* loop) = 0;
            virtual void stoped(class LibEvLoop* loop) = 0;
            virtual void postLooped(class LibEvLoop* loop) = 0;
        };

        class LibEvLoop
        {
        public:
            LibEvLoop();
            ~LibEvLoop();

            void setObserver(EvLoopObserver* observer);

            void start_io(ev_io *io);
            void stop_io(ev_io *io);

            void execInLoopThread(async_loop_exec_function_type async_cb);

            int exec();
            void stop();

        private:
            static void stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents);

            struct ev_loop * const loop_;
            EvLoopObserver * observer_;
            common::thread::platform_threadid_type exec_id_;
            ev_async * async_stop_;
        };
    }
}
