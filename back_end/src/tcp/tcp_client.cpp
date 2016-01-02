#include "tcp/tcp_client.h"

#include <inttypes.h>

#include "common/logger.h"

#include "tcp/tcp_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace tcp
        {
            // client
            TcpClient::TcpClient(ITcpLoop *server, const common::net::socket_info &info, flags_type flags)
                : server_(server), read_write_io_((struct ev_io*)calloc(1, sizeof(struct ev_io))),
                  flags_(flags), sock_(info), name_(), id_()
            {
                read_write_io_->data = this;
            }

            common::net::socket_info TcpClient::info() const
            {
                return sock_.info();
            }

            int TcpClient::fd() const
            {
                return sock_.fd();
            }

            common::Error TcpClient::write(const char* data, uint16_t size, ssize_t &nwrite)
            {
                return sock_.write(data, size, nwrite);
            }

            common::Error TcpClient::read(char* out, uint16_t max_size, ssize_t &nread)
            {
                return sock_.read(out, max_size, nread);
            }

            TcpClient::~TcpClient()
            {
                free(read_write_io_);
                read_write_io_ = NULL;
            }

            ITcpLoop *TcpClient::server() const
            {
                return server_;
            }

            void TcpClient::close()
            {
                if(server_){
                    server_->closeClient(this);
                }

                common::Error err = sock_.close();
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
            }

            void TcpClient::setName(const std::string& name)
            {
                name_ = name;
            }

            std::string TcpClient::name() const
            {
                return name_;
            }

            TcpClient::flags_type TcpClient::flags() const
            {
                return flags_;
            }

            void TcpClient::setFlags(flags_type flags)
            {
                flags_ = flags;
                server_->changeFlags(this);
            }

            common::id_counter<TcpClient>::type_t TcpClient::id() const
            {
                return id_.id();
            }

            const char* TcpClient::className() const
            {
                return "TcpClient";
            }

            std::string TcpClient::formatedName() const
            {
                return common::MemSPrintf("[%s][%s(%" PRIuMAX ")]", name(), className(), id());
            }
        }
    }
}
