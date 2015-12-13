#pragma once

#include "http/http_server_handler.h"

#include "commands/commands.h"

namespace fasto
{
    namespace fastoremote
    {
        typedef uint64_t cmd_id_type;

        class RequestCallback
        {
        public:
            typedef std::function<void(cmd_id_type request_id, int argc, char *argv[])> callback_t;
            RequestCallback(cmd_id_type request_id, callback_t cb);
            cmd_id_type request_id() const;
            void execute(int argc, char *argv[]);

        private:
            cmd_id_type request_id_;
            callback_t cb_;
        };

        class InnerClient
                : public TcpClient
        {
        public:
            InnerClient(TcpServer *server, const common::net::socket_info& info);
            const char* className() const;
        };

        class InnerServerCommandSeqParser
        {
        public:
            InnerServerCommandSeqParser();
            virtual ~InnerServerCommandSeqParser();

            template<typename... Args>
            std::string make_request(const char* cmd_fmt, Args... args)
            {
                char buff[MAX_COMMAND_SIZE] = {0};
                int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, REQUEST_COMMAND, next_id(), args...);
                CHECK(res != -1);
                return buff;
            }

            void subscribe_request(const RequestCallback& req);

        protected:
            void handleInnerDataReceived(InnerClient *connection, char *buff, uint32_t buff_len);

            template<typename... Args>
            std::string make_responce(cmd_id_type id, const char* cmd_fmt, Args... args)
            {
                char buff[MAX_COMMAND_SIZE] = {0};
                int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, RESPONCE_COMMAND, id, args...);
                CHECK(res != -1);
                return buff;;
            }

            template<typename... Args>
            std::string make_approve_responce(cmd_id_type id, const char* cmd_fmt, Args... args)
            {
                char buff[MAX_COMMAND_SIZE] = {0};
                int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, APPROVE_COMMAND, id, args...);
                CHECK(res != -1);
                return buff;;
            }

        private:
            void processRequest(cmd_id_type request_id, int argc, char *argv[]);

            cmd_id_type next_id();
            virtual void handleInnerRequestCommand(InnerClient *connection, uint64_t id, int argc, char *argv[]) = 0; //called when argv not NULL and argc > 0 , only responce
            virtual void handleInnerResponceCommand(InnerClient *connection, uint64_t id, int argc, char *argv[]) = 0; //called when argv not NULL and argc > 0, only approve responce
            virtual void handleInnerApproveCommand(InnerClient *connection, uint64_t id, int argc, char *argv[]) = 0; //called when argv not NULL and argc > 0

            common::atomic_ullong_t id_;
            std::vector<RequestCallback> subscribed_requests_;
        };
    }
}
