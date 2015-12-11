#include "fasto_application.h"

#include <stdlib.h>

#include "common/file_system.h"

#if defined(BUILD_CONSOLE)
    #include "fasto_remote_application.h"
#else
    #if defined(OS_WIN)
        #include "platform/windows/gui_fasto_application.h"
    #elif defined(OS_MACOSX)
        #include "platform/macosx/gui_fasto_application.h"
    #elif defined(OS_ANDROID)
        #include "fasto_remote_application.h"
    #else
        #include "platform/linux/gui_fasto_application.h"
    #endif
#endif

namespace fasto
{
    namespace
    {
        IFastoApplicationImpl* createImpl(int argc, char *argv[])
        {
#if defined(BUILD_CONSOLE)
            return new fastoremote::FastoRemoteApplication(argc, argv);
#else
    #if defined(OS_WIN)
            return new fastoremote::WinGuiFastoRemoteApplication(argc, argv);
    #elif defined(OS_MACOSX)
            return new fastoremote::MacOSXGuiFastoRemoteApplication(argc, argv);
    #elif defined(OS_ANDROID)
            return new fastoremote::FastoRemoteApplication(argc, argv);
    #else
            return new fastoremote::GtkGuiFastoRemoteApplication(argc, argv);
    #endif
#endif
        }
    }

    IFastoApplicationImpl::IFastoApplicationImpl(int argc, char *argv[])
    {
    }

    IFastoApplicationImpl::~IFastoApplicationImpl()
    {

    }

    FastoApplication* FastoApplication::self_ = NULL;

    FastoApplication::FastoApplication(int argc, char *argv[])
        : argc_(argc), argv_(argv), impl_(createImpl(argc, argv))
    {
        CHECK(!self_);
        if(!self_){
            self_ = this;
        }
    }

    int FastoApplication::argc() const
    {
        return argc_;
    }

    char **FastoApplication::argv() const
    {
        return argv_;
    }

    FastoApplication::~FastoApplication()
    {
        self_ = NULL;
    }

    FastoApplication *FastoApplication::instance()
    {
        return self_;
    }

    std::string FastoApplication::appPath() const
    {
        return argv_[0];
    }

    std::string FastoApplication::appDir() const
    {
    #ifdef OS_MACOSX
        const std::string appP = common::file_system::pwd();
    #else
        const std::string appP = appPath();
    #endif
        return common::file_system::get_dir_path(appP);
    }

    int FastoApplication::exec()
    {
        int res = impl_->preExec();
        if(res == EXIT_FAILURE){
            return EXIT_FAILURE;
        }

        res = impl_->exec();
        if(res == EXIT_FAILURE){
            return EXIT_FAILURE;
        }
        return impl_->postExec();
    }

    void FastoApplication::exit(int result)
    {
        if(!self_){
            return;
        }

        self_->impl_->exit(result);
    }
}
