#include "inner/inner_server_command_seq_parser.h"

#include "common/logger.h"

extern "C" {
#include "sds.h"
}

#define GB (1024*1024*1024)
#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    {
        RequestCallback::RequestCallback(cmd_seq_type request_id, callback_t cb)
            : request_id_(request_id), cb_(cb)
        {

        }

        cmd_seq_type RequestCallback::request_id() const
        {
            return request_id_;
        }

        void RequestCallback::execute(int argc, char *argv[])
        {
            if(!cb_){
                return;
            }

            return cb_(request_id_, argc, argv);
        }

        InnerClient::InnerClient(ITcpLoop *server, const common::net::socket_info& info)
            : TcpClient(server, info)
        {

        }

        const char* InnerClient::className() const
        {
            return "InnerClient";
        }

        common::Error InnerClient::write(const cmd_request_t& request, ssize_t &nwrite)
        {
            return TcpClient::write(request.data(), request.size(), nwrite);
        }

        common::Error InnerClient::write(const cmd_responce_t& responce, ssize_t &nwrite)
        {
            return TcpClient::write(responce.data(), responce.size(), nwrite);
        }

        common::Error InnerClient::write(const cmd_approve_t& approve, ssize_t &nwrite)
        {
            return TcpClient::write(approve.data(), approve.size(), nwrite);
        }

        InnerServerCommandSeqParser::InnerServerCommandSeqParser()
            : id_()
        {
        }

        InnerServerCommandSeqParser::~InnerServerCommandSeqParser()
        {

        }

        cmd_seq_type InnerServerCommandSeqParser::next_id()
        {
            size_t next_id = id_++;
            char bytes[sizeof(size_t)];
            betoh_memcpy(&bytes, &next_id, sizeof(bytes));
            std::string hex = common::HexEncode(&bytes, sizeof(bytes), true);
            return hex;
        }

        namespace
        {
            bool exec_reqest(RequestCallback req, cmd_seq_type request_id, int argc, char *argv[])
            {
                if(request_id == req.request_id()){
                    req.execute(argc, argv);
                    return true;
                }

                return false;
            }
        }

        void InnerServerCommandSeqParser::processRequest(cmd_seq_type request_id, int argc, char *argv[])
        {
            subscribed_requests_.erase(std::remove_if(subscribed_requests_.begin(), subscribed_requests_.end(),
                                                      std::bind(&exec_reqest, std::placeholders::_1, request_id, argc, argv)),
                                                      subscribed_requests_.end());
        }

        void InnerServerCommandSeqParser::subscribeRequest(const RequestCallback& req)
        {
            subscribed_requests_.push_back(req);
        }

        void InnerServerCommandSeqParser::handleInnerDataReceived(InnerClient *connection, char *buff, uint32_t buff_len)
        {
            ssize_t nwrite = 0;
            char *end = strstr(buff, END_OF_COMMAND);
            if (!end) {
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "UNKNOWN SEQUENCE: %s", buff);
                const cmd_responce_t resp = make_responce(next_id(), STATE_COMMAND_RESP_FAIL_1S, buff);
                common::Error err = connection->write(resp, nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                connection->close();
                delete connection;
                return;
            }

            *end = 0;

            char *star_seq = NULL;
            cmd_id_type seq = strtoul(buff, &star_seq, 10);
            if (*star_seq != ' ') {
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "PROBLEM EXTRACTING SEQUENCE: %s", buff);
                const cmd_responce_t resp = make_responce(next_id(), STATE_COMMAND_RESP_FAIL_1S, buff);
                common::Error err = connection->write(resp, nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                connection->close();
                delete connection;
                return;
            }

            const char* id_ptr = strchr(star_seq + 1, ' ');
            if (!id_ptr) {
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "PROBLEM EXTRACTING ID: %s", buff);
                const cmd_responce_t resp = make_responce(next_id(), STATE_COMMAND_RESP_FAIL_1S, buff);
                common::Error err = connection->write(resp, nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                connection->close();
                delete connection;
                return;
            }

            size_t len_seq = id_ptr - (star_seq + 1);
            cmd_seq_type id = std::string(star_seq + 1, len_seq);
            const char *cmd = id_ptr;

            int argc;
            sds *argv = sdssplitargs(cmd, &argc);
            processRequest(id, argc, argv);
            if (argv == NULL) {
                DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_WARNING, "PROBLEM PARSING INNER COMMAND: %s", buff);
                const cmd_responce_t resp = make_responce(id, STATE_COMMAND_RESP_FAIL_1S, buff);
                common::Error err = connection->write(resp, nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                connection->close();
                delete connection;
                return;
            }

            DEBUG_MSG_FORMAT<MAX_COMMAND_SIZE * 2>(common::logging::L_INFO, "HANDLE INNER COMMAND client[%s] seq:% " CID_FMT ", id:%s, cmd: %s",
                                                   connection->formatedName(), seq, id, cmd);
            if(seq == REQUEST_COMMAND){
                handleInnerRequestCommand(connection, id, argc, argv);
            }
            else if(seq == RESPONCE_COMMAND){
                handleInnerResponceCommand(connection, id, argc, argv);
            }
            else if(seq == APPROVE_COMMAND){
                handleInnerApproveCommand(connection, id, argc, argv);
            }
            else{
                NOTREACHED();
            }
            sdsfreesplitres(argv, argc);
        }
    }
}
