#pragma once

#include "common/sprintf.h"

#define END_OF_COMMAND "\r\n"

#define CAUSE_INVALID_ARGS "invalid_args"
#define CAUSE_INVALID_USER "invalid_user"
#define CAUSE_UNREGISTERED_USER "unregistered_user"
#define CAUSE_DOUBLE_CONNECTION_HOST "double_connection_host"
#define CAUSE_CONNECT_FAILED "connect_failed"
#define CAUSE_AUTH_FAILED "auth_failed"
#define CAUSE_PARSE_COMMAND_FAILED "parse_command_failed"
#define CAUSE_INVALID_SEQ "invalid_sequence"
#define CAUSE_NOT_CONNECTED "not_connected"
#define CAUSE_NOT_HANDLED "not_handled"

#define FAIL_COMMAND "fail"
#define SUCCESS_COMMAND "ok"

#define MAX_COMMAND_SIZE 256
#define IS_EQUAL_COMMAND(BUF, CMD) BUF && memcmp(BUF, CMD, sizeof(CMD) - 1) == 0

#define CID_FMT PRIu8

#define GENERATE_FMT(CMD, CMD_FMT) "%" CID_FMT " %s " CMD " " CMD_FMT END_OF_COMMAND
#define GENEATATE_SUCCESS_FMT(CMD, CMD_FMT) "%" CID_FMT " %s " SUCCESS_COMMAND " " CMD " " CMD_FMT END_OF_COMMAND
#define GENEATATE_FAIL_FMT(CMD, CMD_FMT) "%" CID_FMT " %s " FAIL_COMMAND " " CMD " " CMD_FMT END_OF_COMMAND

#define REQUEST_COMMAND 0u
#define RESPONCE_COMMAND 1u
#define APPROVE_COMMAND 2u

#define PING_COMMAND "ping"
#define PING_COMMAND_RESP_SUCCESS GENEATATE_SUCCESS_FMT(PING_COMMAND, "pong")
#define PING_COMMAND_APPROVE_FAIL_1S GENEATATE_SUCCESS_FMT(PING_COMMAND, "%s")
#define PING_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(PING_COMMAND, "")

#define STATE_COMMAND "state_command"
#define STATE_COMMAND_RESP_FAIL_1S GENEATATE_FAIL_FMT(STATE_COMMAND, "%s")
#define STATE_COMMAND_RESP_SUCCESS GENEATATE_SUCCESS_FMT(STATE_COMMAND, "")

#define SERVER_WHO_ARE_YOU_COMMAND "who_are_you"
#define SERVER_PLEASE_CONNECT_RELAY_COMMAND "plz_connect_relay"
#define SERVER_PLEASE_DISCONNECT_COMMAND "plz_disconnect"
#define SERVER_PLEASE_SYSTEM_INFO_COMMAND "plz_system_info"

//request
//[size_t](0) [hex_string]seq [std::string]command args ...

//responce
//[size_t](1) [hex_string]seq [OK|FAIL] [std::string]command args ...

//approve
//[size_t](2) [hex_string]seq [OK|FAIL] [std::string]command args ...

namespace fasto
{
    namespace siteonyourdevice
    {
        typedef std::string cmd_seq_type;
        typedef uint8_t cmd_id_type;
    }
}
