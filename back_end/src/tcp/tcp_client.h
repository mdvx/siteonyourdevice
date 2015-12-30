#pragma once

#include "common/patterns/crtp_pattern.h"
#include "common/net/socket_tcp.h"

#include "event_loop.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        class ITcpLoop;

        class TcpClient
                : common::IMetaClassInfo
        {
        public:
            typedef int flags_type;
            friend class ITcpLoop;
            TcpClient(ITcpLoop* server, const common::net::socket_info& info, flags_type flags = EV_READ);
            virtual ~TcpClient();

            ITcpLoop* server() const;

            common::net::socket_info info() const;
            int fd() const;

            virtual common::Error write(const char *data, uint16_t size, ssize_t &nwrite) WARN_UNUSED_RESULT;
            virtual common::Error write(const uint8_t *data, uint16_t size, ssize_t &nwrite) WARN_UNUSED_RESULT;
            virtual common::Error read(char* outData, uint16_t maxSize, ssize_t &nread) WARN_UNUSED_RESULT;

            void close();

            void setName(const std::string& name);
            std::string name() const;

            flags_type flags() const;
            void setFlags(flags_type flags);

            common::id_counter<TcpClient>::type_t id() const;
            virtual const char *className() const;
            std::string formatedName() const;

        private:
            ITcpLoop* server_;
            ev_io * read_write_io_;
            int flags_;

            common::net::SocketHolder sock_;
            std::string name_;
            const common::id_counter<TcpClient> id_;
        };
    }
}
