/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "server/redis_helpers.h"

#include <hiredis/hiredis.h>

#include <string>
#include <vector>

#include <common/logger.h>
#include <common/utils.h>

#include <json-c/json.h>

#define GET_ALL_USERS_REDIS_REQUEST "HGETALL users"
#define GET_USER_1S "HGET users %s"

namespace fasto {
namespace siteonyourdevice {
namespace server {
namespace {

redisContext* redis_connect(const redis_configuration_t& config) {
  common::net::HostAndPort redisHost = config.redis_host;
  const std::string unixPath = config.redis_unix_socket;

  redisContext* redis = NULL;
  if (unixPath.empty()) {
    std::string host_str = redisHost.GetHost();
    redis = redisConnect(host_str.c_str(), redisHost.GetPort());
  } else {
    redis = redisConnectUnix(unixPath.c_str());
  }

  return redis;
}

typedef std::vector<std::string> hosts_names_t;

bool parse_user_json(const std::string& login, const char* userJson, UserAuthInfo* out_info, hosts_names_t* out_hosts) {
  if (!userJson || !out_info || !out_hosts) {
    return false;
  }

  json_object* obj = json_tokener_parse(userJson);
  if (!obj) {
    return false;
  }

  /*json_object* jname = NULL;
  json_object_object_get_ex(obj, "name", &jname);
  if(jname){
    info.name_ = json_object_get_string(jname);
  }*/

  json_object* jpass = NULL;
  json_object_object_get_ex(obj, "password", &jpass);
  if (jpass) {
    out_info->password = json_object_get_string(jpass);
  }

  json_object* jhosts = NULL;
  json_object_object_get_ex(obj, "hosts", &jhosts);
  if (jhosts) {
    int arraylen = json_object_array_length(jhosts);
    for (int i = 0; i < arraylen; i++) {
      json_object* jhost = json_object_array_get_idx(jhosts, i);
      const char* hoststr = json_object_get_string(jhost);
      out_hosts->push_back(hoststr);
    }
  }

  out_info->login = login;
  json_object_put(obj);
  return true;
}

}  // namespace

RedisStorage::RedisStorage() : config_() {}

void RedisStorage::setConfig(const redis_configuration_t& config) {
  config_ = config;
}

bool RedisStorage::findUser(const UserAuthInfo& user) const {
  if (!user.isValid()) {
    return false;
  }

  redisContext* redis = redis_connect(config_);
  if (!redis) {
    return false;
  }

  std::string login = user.login;
  const char* login_str = login.c_str();
  redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(redis, GET_USER_1S, login_str));
  if (!reply) {
    redisFree(redis);
    return false;
  }

  const char* userJson = reply->str;
  UserAuthInfo info;
  hosts_names_t hosts;
  if (parse_user_json(login, userJson, &info, &hosts)) {
    for (int j = 0; j < hosts.size(); ++j) {
      if (hosts[j] == user.host.GetHost() && user.password == info.password) {
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

RedisSub::RedisSub(RedisSubHandler* handler) : stop_(false), handler_(handler) {}

void RedisSub::setConfig(const redis_sub_configuration_t& config) {
  config_ = config;
}

void RedisSub::listen() {
  redisContext* redis_sub = redis_connect(config_);
  if (!redis_sub) {
    return;
  }

  const char* channel_str = config_.channel_in.c_str();

  void* reply = redisCommand(redis_sub, "SUBSCRIBE %s", channel_str);
  if (!reply) {
    redisFree(redis_sub);
    return;
  }

  while (!stop_) {
    redisReply* lreply = NULL;
    void** plreply = reinterpret_cast<void**>(&lreply);
    if (redisGetReply(redis_sub, plreply) != REDIS_OK) {
      WARNING_LOG() << "REDIS PUB/SUB GET REPLY ERROR: " << redis_sub->errstr;
      break;
    }

    bool is_error_reply = lreply->type != REDIS_REPLY_ARRAY || lreply->elements != 3 ||
                          lreply->element[1]->type != REDIS_REPLY_STRING ||
                          lreply->element[2]->type != REDIS_REPLY_STRING;

    char* chn = lreply->element[1]->str;
    size_t chn_len = lreply->element[1]->len;
    char* msg = lreply->element[2]->str;
    size_t msg_len = lreply->element[2]->len;

    if (handler_) {
      handler_->handleMessage(chn, chn_len, msg, msg_len);
    }

    freeReplyObject(lreply);
  }

  freeReplyObject(reply);
  redisFree(redis_sub);
}

void RedisSub::stop() {
  stop_ = true;
}

bool RedisSub::publish_clients_state(const std::string& msg) {
  const char* channel = config_.channel_clients_state.empty() ? NULL : config_.channel_clients_state.c_str();
  size_t chn_len = config_.channel_clients_state.length();
  return publish(channel, chn_len, msg.c_str(), msg.length());
}

bool RedisSub::publish_command_out(const char* msg, size_t msg_len) {
  const char* channel = config_.channel_out.empty() ? NULL : config_.channel_out.c_str();
  size_t chn_len = config_.channel_out.length();
  return publish(channel, chn_len, msg, msg_len);
}

bool RedisSub::publish(const char* chn, size_t chn_len, const char* msg, size_t msg_len) {
  if (!chn || chn_len == 0) {
    return false;
  }

  if (!msg || msg_len == 0) {
    return false;
  }

  redisContext* redis_sub = redis_connect(config_);
  if (!redis_sub) {
    return false;
  }

  void* rreply = redisCommand(redis_sub, "PUBLISH %s %s", chn, msg);
  if (!rreply) {
    redisFree(redis_sub);
    return false;
  }

  freeReplyObject(rreply);
  redisFree(redis_sub);
  return true;
}

}  // namespace server
}  // namespace siteonyourdevice
}  // namespace fasto
