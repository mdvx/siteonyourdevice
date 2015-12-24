#pragma once

#include "common/thread/thread.h"

#include "common/net/types.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class ITcpLoop;
        class ITcpLoopObserver;

        class ILoopController
        {
        public:
            ILoopController();
            virtual ~ILoopController();

            void start();
            int exec();
            void stop();

        protected:
            ITcpLoop * loop_;
            ITcpLoopObserver * handler_;

        private:
            virtual ITcpLoopObserver * createHandler() = 0;
            virtual ITcpLoop * createServer(ITcpLoopObserver * handler) = 0;
            virtual void started() = 0;
            virtual void stoped() = 0;
        };

        class ILoopThreadController
                : public ILoopController
        {
        public:
            ILoopThreadController();
            virtual ~ILoopThreadController();

            int join();

        private:
            using ILoopController::exec;

            virtual ITcpLoopObserver * createHandler() = 0;
            virtual ITcpLoop * createServer(ITcpLoopObserver * handler) = 0;

            virtual void started();
            virtual void stoped();

            std::shared_ptr<common::thread::Thread<int> > loop_thread_;
        };
    }
}
