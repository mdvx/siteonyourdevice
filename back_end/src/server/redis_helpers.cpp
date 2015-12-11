#include "server/redis_helpers.h"

#include <hiredis/hiredis.h>

#include "common/third-party/json-c/json-c/json.h"

#include "common/utils.h"
#include "common/logger.h"

#define GET_ALL_USERS_REDIS_REQUEST "HGETALL users"
#define GET_USER_1S "HGET users %s"

namespace fasto
{
    namespace fastoremote
    {
        namespace
        {
            redisContext * redis_connect(const redis_configuration_t& config)
            {
                common::net::hostAndPort redisHost = config.redisHost_;
                const std::string unixPath = config.redisUnixSock_;

                redisContext * redis = NULL;
                if(unixPath.empty()){
                    redis = redisConnect(redisHost.host_.c_str(), redisHost.port_);
                }
                else{
                    redis = redisConnectUnix(unixPath.c_str());
                }

                return redis;
            }

            typedef std::vector<std::string> hosts_names_t;

            bool parse_user_json(const std::string& login, const char* userJson, UserAuthInfo& info, hosts_names_t& hosts)
            {
                if(!userJson){
                    return false;
                }

                json_object * obj = json_tokener_parse(userJson);
                if(!obj){
                    return false;
                }

                /*json_object* jname = NULL;
                json_object_object_get_ex(obj, "name", &jname);
                if(jname){
                    info.name_ = json_object_get_string(jname);
                }*/

                json_object* jpass = NULL;
                json_object_object_get_ex(obj, "password", &jpass);
                if(jpass){
                    info.password_ = json_object_get_string(jpass);
                }

                json_object* jhosts = NULL;
                json_object_object_get_ex(obj, "hosts", &jhosts);
                if(jhosts){
                    int arraylen = json_object_array_length(jhosts);
                    for (int i = 0; i < arraylen; i++) {
                        json_object* jhost = json_object_array_get_idx(jhosts, i);
                        const char* hoststr = json_object_get_string(jhost);
                        hosts.push_back(hoststr);
                    }
                }

                info.login_ = login;

                json_object_put(obj);
                return true;
            }
        }

        RedisStorage::RedisStorage()
            : config_()
        {

        }

        void RedisStorage::setConfig(const redis_configuration_t& config)
        {
            config_ = config;
        }

        bool RedisStorage::findUser(const UserAuthInfo& user) const
        {
            if(!user.isValid()){
                return false;
            }

            redisContext * redis = redis_connect(config_);
            if(!redis){
                return false;
            }

            const std::string login = user.login_;
            redisReply* reply = (redisReply*)redisCommand(redis, GET_USER_1S, login.c_str());
            if(!reply){
                redisFree(redis);
                return false;
            }

            const char* userJson = reply->str;
            UserAuthInfo info;
            hosts_names_t hosts;
            if(parse_user_json(login, userJson, info, hosts)){
                for(int j = 0; j < hosts.size(); ++j){
                    if(hosts[j] == user.host_.host_ && user.password_ == info.password_){
                        freeReplyObject(reply);
                        redisFree(redis);
                        return true;
                    }
                }
            }

            freeReplyObject(reply);
            redisFree(redis);
            return false;
        }

        RedisSub::RedisSub(RedisSubHandler *handler)
            : stop_(false), handler_(handler)
        {

        }

        void RedisSub::setConfig(const redis_sub_configuration_t &config)
        {
            config_ = config;
        }

        void RedisSub::listen()
        {
            redisContext * redis_sub = redis_connect(config_);
            if(!redis_sub){
                return;
            }

            const std::string channel = config_.channel_in_;

            redisReply* reply = (redisReply*)redisCommand(redis_sub, "SUBSCRIBE %s", channel.c_str());
            if(!reply){
                redisFree(redis_sub);
                return;
            }

            while(!stop_){
                redisReply * lreply = NULL;
                if(redisGetReply(redis_sub, (void**)&lreply) != REDIS_OK){
                    DEBUG_MSG_FORMAT<128>(common::logging::L_WARNING, "REDIS PUB/SUB GET REPLY ERROR: %s", redis_sub->errstr);
                    break;
                }

                bool is_error_reply =  lreply->type != REDIS_REPLY_ARRAY || lreply->elements != 3 ||
                                lreply->element[1]->type != REDIS_REPLY_STRING ||
                                lreply->element[2]->type != REDIS_REPLY_STRING;

                char * chn = lreply->element[1]->str;
                size_t chn_len = lreply->element[1]->len;
                char * msg = lreply->element[2]->str;
                size_t msg_len = lreply->element[2]->len;

                if(handler_){
                    handler_->handleMessage(chn, chn_len, msg, msg_len);
                }

                freeReplyObject(lreply);
            }

            freeReplyObject(reply);
            redisFree(redis_sub);
        }

        void RedisSub::stop()
        {
            stop_ = true;
        }

        bool RedisSub::publish_clients_state(const std::string& msg)
        {
            const char * channel = common::utils::c_strornull(config_.channel_clients_state_);
            size_t chn_len = config_.channel_clients_state_.length();
            return publish(channel, chn_len, msg.c_str(), msg.length());
        }

        bool RedisSub::publish_command_out(const char* msg, size_t msg_len)
        {
            const char * channel = common::utils::c_strornull(config_.channel_out_);
            size_t chn_len = config_.channel_out_.length();
            return publish(channel, chn_len, msg, msg_len);
        }

        bool RedisSub::publish(const char* chn, size_t chn_len, const char* msg, size_t msg_len)
        {
            if(!chn || chn_len == 0){
                return false;
            }

            if(!msg || msg_len == 0){
                return false;
            }

            redisContext * redis_sub = redis_connect(config_);
            if(!redis_sub){
                return false;
            }

            redisReply* reply = (redisReply*)redisCommand(redis_sub, "PUBLISH %s %s", chn, msg);
            if(!reply){
                redisFree(redis_sub);
                return false;
            }

            freeReplyObject(reply);
            redisFree(redis_sub);
            return true;
        }
    }
}
