#pragma once

#include "common/thread/thread.h"
#include "common/error.h"

#include "network_events.h"

#include "http_config.h"

namespace common
{
    namespace thread
    {
        template<typename type_t>
        class EventThread;
    }
}

namespace fasto
{
    namespace siteonyourdevice
    {
        class Http2InnerServerHandler;
        class ITcpLoop;

        class NetworkController;

        class NetworkEventHandler
        {
        public:
            NetworkEventHandler(NetworkController *controller);
            virtual ~NetworkEventHandler();

            virtual int start(); //connect

        protected:
            NetworkController * controller_;
            virtual void handleEvent(NetworkEvent* event);

        private:
            class NetworkListener;
            NetworkListener* networkListener_;
        };

        class NetworkController
        {
        public:
            NetworkController(int argc, char *argv[]);
            ~NetworkController();

            int exec();
            void exit(int result);

            common::Error connect() WARN_UNUSED_RESULT;
            common::Error disConnect() WARN_UNUSED_RESULT;

            configuration_t config() const;
            void setConfig(const configuration_t& config);

        private:
            void readConfig();
            void saveConfig();

            Http2InnerServerHandler * handler_;
            ITcpLoop * server_;
            std::shared_ptr<common::thread::Thread<int> > http_thread_; //http thread handle

            std::string config_path_;
            configuration_t config_;

            common::thread::EventThread<NetworkEventTypes>* const thread_; //event thread handle

            class HttpAuthObserver;
            HttpAuthObserver* authChecker_;
        };
    }
}
