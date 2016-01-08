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

#pragma once

#include "tcp/tcp_server.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace http
        {
            class HttpServer
                    : public tcp::TcpServer
            {
            public:
                HttpServer(const common::net::hostAndPort& host, tcp::ITcpLoopObserver *observer);
                ~HttpServer();

                virtual const char* className() const;

            protected:
                virtual tcp::TcpClient * createClient(const common::net::socket_info &info);
            };

            class Http2Server
                    : public HttpServer
            {
            public:
                Http2Server(const common::net::hostAndPort& host, tcp::ITcpLoopObserver *observer);

                virtual const char* className() const;

            protected:
                virtual tcp::TcpClient * createClient(const common::net::socket_info &info);
            };
        }
    }
}
