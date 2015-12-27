#pragma once

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
        class ILoopThreadController;

        class NetworkController
        {
        public:
            NetworkController(int argc, char *argv[]);
            ~NetworkController();

            int exec();
            void exit(int result);

            common::Error connect() WARN_UNUSED_RESULT;
            common::Error disConnect() WARN_UNUSED_RESULT;

            HttpConfig config() const;
            void setConfig(const HttpConfig& config);

        private:
            void readConfig();
            void saveConfig();

            ILoopThreadController * server_;

            std::string config_path_;
            HttpConfig config_;

            common::thread::EventThread<NetworkEventTypes>* const thread_; //event thread handle
        };

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
    }
}
