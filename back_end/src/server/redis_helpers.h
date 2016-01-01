#pragma once

#include "common/net/types.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            struct redis_configuration_t
            {
                common::net::hostAndPort redisHost_;
                std::string redisUnixSock_;
            };

            struct redis_sub_configuration_t
                    : public redis_configuration_t
            {
                std::string channel_in_;
                std::string channel_out_;
                std::string channel_clients_state_;
            };

            class RedisStorage
            {
            public:
                RedisStorage();
                void setConfig(const redis_configuration_t& config);

                bool findUser(const UserAuthInfo& user) const WARN_UNUSED_RESULT;

            private:
                redis_configuration_t config_;
            };

            class RedisSubHandler
            {
            public:
                virtual void handleMessage(char* channel, size_t channel_len, char* msg, size_t msg_len) = 0;
            };

            class RedisSub
            {
            public:
                RedisSub(RedisSubHandler * handler);

                void setConfig(const redis_sub_configuration_t& config);
                void listen();
                void stop();

                bool publish_clients_state(const std::string& msg) WARN_UNUSED_RESULT;
                bool publish_command_out(const char* msg, size_t msg_len) WARN_UNUSED_RESULT;
                bool publish(const char* chn, size_t chn_len, const char* msg, size_t msg_len) WARN_UNUSED_RESULT;

            private:
                RedisSubHandler * const handler_;
                redis_sub_configuration_t config_;
                bool stop_;
            };
        }
    }
}
