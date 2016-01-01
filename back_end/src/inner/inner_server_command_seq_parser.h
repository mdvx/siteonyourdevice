#pragma once

#include "tcp/tcp_client.h"

#include "commands/commands.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class RequestCallback
        {
        public:
            typedef std::function<void(cmd_seq_type request_id, int argc, char *argv[])> callback_t;
            RequestCallback(cmd_seq_type request_id, callback_t cb);
            cmd_seq_type request_id() const;
            void execute(int argc, char *argv[]);

        private:
            cmd_seq_type request_id_;
            callback_t cb_;
        };

        class InnerClient
                : public TcpClient
        {
        public:
            InnerClient(ITcpLoop *server, const common::net::socket_info& info);
            const char* className() const;

            common::Error write(const cmd_request_t& request, ssize_t& nwrite) WARN_UNUSED_RESULT;
            common::Error write(const cmd_responce_t& responce, ssize_t& nwrite) WARN_UNUSED_RESULT;
            common::Error write(const cmd_approve_t& approve, ssize_t& nwrite) WARN_UNUSED_RESULT;
        };

        class InnerServerCommandSeqParser
        {
        public:
            InnerServerCommandSeqParser();
            virtual ~InnerServerCommandSeqParser();

            template<typename... Args>
            cmd_request_t make_request(const char* cmd_fmt, Args... args)
            {
                char buff[MAX_COMMAND_SIZE] = {0};
                cmd_seq_type id = next_id();
                int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, REQUEST_COMMAND, id, args...);
                CHECK(res != -1);
                return cmd_request_t(id, buff);
            }

            void subscribeRequest(const RequestCallback& req);

        protected:
            void handleInnerDataReceived(InnerClient *connection, char *buff, uint32_t buff_len);

            template<typename... Args>
            cmd_responce_t make_responce(cmd_seq_type id, const char* cmd_fmt, Args... args)
            {
                char buff[MAX_COMMAND_SIZE] = {0};
                int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, RESPONCE_COMMAND, id, args...);
                CHECK(res != -1);
                return cmd_responce_t(id, buff);
            }

            template<typename... Args>
            cmd_approve_t make_approve_responce(cmd_seq_type id, const char* cmd_fmt, Args... args)
            {
                char buff[MAX_COMMAND_SIZE] = {0};
                int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, APPROVE_COMMAND, id, args...);
                CHECK(res != -1);
                return cmd_approve_t(id, buff);
            }

        private:
            void processRequest(cmd_seq_type request_id, int argc, char *argv[]);

            cmd_seq_type next_id();
            virtual void handleInnerRequestCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]) = 0; //called when argv not NULL and argc > 0 , only responce
            virtual void handleInnerResponceCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]) = 0; //called when argv not NULL and argc > 0, only approve responce
            virtual void handleInnerApproveCommand(InnerClient *connection, cmd_seq_type id, int argc, char *argv[]) = 0; //called when argv not NULL and argc > 0

            common::atomic<uintmax_t> id_;
            std::vector<RequestCallback> subscribed_requests_;
        };
    }
}
