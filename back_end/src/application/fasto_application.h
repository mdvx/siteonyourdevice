#pragma once

#include "common/smart_ptr.h"

namespace fasto
{
    class IFastoApplicationImpl
    {
    public:
        IFastoApplicationImpl(int argc, char *argv[]);
        virtual int preExec() = 0; //EXIT_FAILURE, EXIT_SUCCESS
        virtual int exec() = 0; //EXIT_FAILURE, EXIT_SUCCESS
        virtual int postExec() = 0; //EXIT_FAILURE, EXIT_SUCCESS

        virtual void exit(int result) = 0;
        virtual ~IFastoApplicationImpl();
    };

    class FastoApplication
    {
    public:
        FastoApplication(int argc, char *argv[]);
        ~FastoApplication();

        std::string appPath() const;
        std::string appDir() const;
        int argc() const;
        char **argv() const;

        static FastoApplication *instance();

        int exec(); //EXIT_FAILURE, EXIT_SUCCESS
        static void exit(int result);

    private:
        static FastoApplication * self_;

        int argc_;
        char **argv_;

        const common::scoped_ptr<IFastoApplicationImpl> impl_;
    };
}

#define fApp fasto::FastoApplication::instance()
