#pragma once

#include "commands/commands.h"

//publish COMMANDS_IN 'host 0 1 ping' 0 => request
//publish COMMANDS_OUT '1 [OK|FAIL] ping args...'

#define SERVER_WHO_ARE_YOU_COMMAND_REQ GENERATE_FMT(SERVER_WHO_ARE_YOU_COMMAND, "")
#define SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1S GENEATATE_FAIL_FMT(SERVER_WHO_ARE_YOU_COMMAND, "%s")
#define SERVER_WHO_ARE_YOU_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_WHO_ARE_YOU_COMMAND, "")

#define SERVER_PLEASE_CONNECT_COMMAND_REQ_1S GENERATE_FMT(SERVER_PLEASE_CONNECT_RELAY_COMMAND, "%s")
#define SERVER_PLEASE_CONNECT_COMMAND_APPROVE_FAIL_1S GENEATATE_FAIL_FMT(SERVER_PLEASE_CONNECT_RELAY_COMMAND, "%s")
#define SERVER_PLEASE_CONNECT_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_PLEASE_CONNECT_RELAY_COMMAND, "")

#define SERVER_PLEASE_DISCONNECT_COMMAND_REQ GENERATE_FMT(SERVER_PLEASE_DISCONNECT_COMMAND, "")
#define SERVER_PLEASE_DISCONNECT_COMMAND_APPROVE_FAIL_1S GENERATE_FMT(SERVER_PLEASE_DISCONNECT_COMMAND, "%s")
#define SERVER_PLEASE_DISCONNECT_COMMAND_APPROVE_SUCCESS GENERATE_FMT(SERVER_PLEASE_DISCONNECT_COMMAND, "")

#define SERVER_PLEASE_SYSTEM_INFO_COMMAND_REQ GENERATE_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "")
#define SERVER_PLEASE_SYSTEM_INFO_COMMAND_APPROVE_FAIL_1S GENERATE_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "%s")
#define SERVER_PLEASE_SYSTEM_INFO_COMMAND_APPROVE_SUCCESS GENERATE_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "")

//for publish only
// id cmd
#define SERVER_COMMANDS_OUT_FAIL_2US(CAUSE) "%" PRIu64 " " FAIL_COMMAND " %s " CAUSE

#define SERVER_NOTIFY_CLIENT_CONNECTED_1S "%s connected"
#define SERVER_NOTIFY_CLIENT_DISCONNECTED_1S "%s disconnected"

namespace fasto
{
    namespace fastoremote
    {
    }
}
