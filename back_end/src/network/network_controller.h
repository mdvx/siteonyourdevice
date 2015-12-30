#pragma once

#include "common/error.h"
#include "common/multi_threading/types.h"

#include "globals.h"
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
        class ILoopThreadController;

        class NetworkController
        {
        public:
            NetworkController(int argc, char *argv[]);
            ~NetworkController();

            int exec() SYNC_CALL();
            void exit(int result);

            void connect() ASYNC_CALL(InnerClientConnectedEvent);
            void disConnect() ASYNC_CALL(InnerClientDisconnectedEvent);

            HttpConfig config() const;
            void setConfig(const HttpConfig& config);

        private:
            void readConfig();
            void saveConfig();

            common::multi_threading::mutex_t server_mutex_;
            ILoopThreadController * server_;

            std::string config_path_;
            HttpConfig config_;

            common::thread::EventThread<NetworkEventTypes>* const thread_; //event thread handle
        };
    }
}
