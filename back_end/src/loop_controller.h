#pragma once

#include "common/thread/thread.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace tcp
        {
            class ITcpLoop;
            class ITcpLoopObserver;
        }

        class ILoopController
        {
        public:
            ILoopController();
            virtual ~ILoopController();

            void start();
            int exec();
            void stop();

        protected:
            tcp::ITcpLoop * loop_;
            tcp::ITcpLoopObserver * handler_;

        private:
            virtual tcp::ITcpLoopObserver * createHandler() = 0;
            virtual tcp::ITcpLoop * createServer(tcp::ITcpLoopObserver * handler) = 0;
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

            virtual tcp::ITcpLoopObserver * createHandler() = 0;
            virtual tcp::ITcpLoop * createServer(tcp::ITcpLoopObserver * handler) = 0;

            virtual void started();
            virtual void stoped();

            std::shared_ptr<common::thread::Thread<int> > loop_thread_;
        };
    }
}
