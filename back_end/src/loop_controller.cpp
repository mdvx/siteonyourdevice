#include "loop_controller.h"

#include "tcp/tcp_server.h"

#include "common/thread/thread_manager.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        ILoopController::ILoopController()
            : loop_(NULL), handler_(NULL)
        {

        }

        int ILoopController::exec()
        {
            CHECK(!handler_);
            CHECK(!loop_);

            ITcpLoopObserver *handler = createHandler();
            if(!handler){
                return EXIT_FAILURE;
            }

            loop_ = createServer(handler);
            if(!loop_){
                delete handler;
                return EXIT_FAILURE;
            }

            return loop_->exec();
        }

        void ILoopController::start()
        {
            started();
        }

        void ILoopController::stop()
        {
            if(loop_){
                loop_->stop();
            }

            stoped();
        }

        ILoopController::~ILoopController()
        {
            delete loop_;
            delete handler_;
        }

        ILoopThreadController::ILoopThreadController()
            : ILoopController(), loop_thread_()
        {
            loop_thread_ = THREAD_MANAGER()->createThread(&ILoopController::exec, this);
        }

        ILoopThreadController::~ILoopThreadController()
        {

        }

        int ILoopThreadController::join()
        {
            return loop_thread_->joinAndGet();
        }

        void ILoopThreadController::started()
        {
            loop_thread_->start();
        }

        void ILoopThreadController::stoped()
        {
            join();
        }
    }
}
