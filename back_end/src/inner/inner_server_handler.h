#pragma once

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

#include "http_config.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace inner
        {
            class InnerServerHandler
                    : public InnerServerCommandSeqParser
            {
            public:
                InnerServerHandler(const HttpConfig& config);

                UserAuthInfo authInfo() const;

            protected:
                const HttpConfig config_;

            private:
                virtual void handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
                virtual void handleInnerResponceCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
                virtual void handleInnerApproveCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]);
            };
        }
    }
}
