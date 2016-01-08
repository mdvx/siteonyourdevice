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

#include "inner/http_inner_server_handler.h"

#include <string>

#include "common/thread/event_bus.h"
#include "common/logger.h"
#include "common/net/net.h"

#include "network/network_events.h"

#include "inner/inner_client.h"
#include "inner/inner_relay_client.h"

#define GB (1024*1024*1024)
#define BUF_SIZE 4096

namespace fasto {
namespace siteonyourdevice {
namespace inner {

Http2InnerServerHandler::Http2InnerServerHandler(const HttpServerInfo& info,
                                                 const common::net::hostAndPort& innerHost,
                                                 const HttpConfig &config)
    : InnerServerHandler(config), Http2ServerHandler(info, NULL),
      innerConnection_(NULL), ping_server_id_timer_(INVALID_TIMER_ID), innerHost_(innerHost) {
}

Http2InnerServerHandler::~Http2InnerServerHandler() {
    delete innerConnection_;
    innerConnection_ = NULL;
}

void Http2InnerServerHandler::preLooped(tcp::ITcpLoop* server) {
    ping_server_id_timer_ = server->createTimer(ping_timeout_server, ping_timeout_server);
    CHECK(!innerConnection_);

    common::net::socket_info client_info;
    common::ErrnoError err = common::net::connect(innerHost_, common::net::ST_SOCK_STREAM,
                                                  0, client_info);
    if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
        auto ex_event = make_exception_event(new network::InnerClientConnectedEvent(this, authInfo()), err);
        EVENT_BUS()->postEvent(ex_event);
        return;
    }

    InnerClient* connection = new InnerClient(server, client_info);
    innerConnection_ = connection;
    server->registerClient(connection);

    Http2ServerHandler::preLooped(server);
}

void Http2InnerServerHandler::accepted(tcp::TcpClient* client) {
}

void Http2InnerServerHandler::closed(tcp::TcpClient* client) {
    if (client == innerConnection_) {
        EVENT_BUS()->postEvent(new network::InnerClientDisconnectedEvent(this, authInfo()));
        innerConnection_ = NULL;
        return;
    }

    ProxyRelayClient * prclient = dynamic_cast<ProxyRelayClient*>(client);  // proxyrelay connection
    if (prclient) {
        RelayClientEx * rclient = prclient->relay();
        rclient->setEclient(NULL);
    }
}

void Http2InnerServerHandler::postLooped(tcp::ITcpLoop* server) {
    if (innerConnection_) {
        InnerClient* connection = innerConnection_;
        connection->close();
        delete connection;
    }

    Http2ServerHandler::postLooped(server);
}

void Http2InnerServerHandler::timerEmited(tcp::ITcpLoop* server, timer_id_type id) {
    if (id == ping_server_id_timer_ && innerConnection_) {
        const cmd_request_t ping_request = make_request(PING_COMMAND_REQ);
        ssize_t nwrite = 0;
        InnerClient* client = innerConnection_;
        common::Error err = client->write(ping_request, &nwrite);
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
            client->close();
            delete client;
        }
    }
}

void Http2InnerServerHandler::innerDataReceived(InnerClient* iclient) {
    char buff[MAX_COMMAND_SIZE] = {0};
    ssize_t nread = 0;
    common::Error err = iclient->read(buff, MAX_COMMAND_SIZE, &nread);
    if ((err && err->isError()) || nread == 0) {
        DEBUG_MSG_ERROR(err);
        iclient->close();
        delete iclient;
        return;
    }

    handleInnerDataReceived(iclient, buff, nread);
}

void Http2InnerServerHandler::relayDataReceived(inner::RelayClient* rclient) {
    char buff[BUF_SIZE] = {0};
    ssize_t nread = 0;
    common::Error err = rclient->read(buff, BUF_SIZE, &nread);
    if ((err && err->isError()) || nread == 0) {
        rclient->close();
        delete rclient;
        return;
    }

    Http2ServerHandler::processReceived(rclient, buff, nread);
}

void Http2InnerServerHandler::relayExDataReceived(inner::RelayClientEx* rclient) {
    char buff[BUF_SIZE] = {0};
    ssize_t nread = 0;
    common::Error err = rclient->read(buff, BUF_SIZE, &nread);
    if ((err && err->isError()) || nread == 0) {
        rclient->close();
        delete rclient;
        return;
    }

    const common::net::hostAndPort externalHost = rclient->externalHost();

    common::net::socket_info client_info;
    common::http::http_protocols protocol = common::http::HP_1_1;
    if (common::http2::is_preface_data(buff, nread)) {
        protocol = common::http::HP_2_0;
    } else if (common::http2::is_frame_header_data(buff, nread)) {
        protocol = common::http::HP_2_0;
    }

    if (externalHost.isValid()) {
        ProxyRelayClient* eclient = rclient->eclient();
        if (!eclient) {
            common::Error err = common::net::connect(externalHost, common::net::ST_SOCK_STREAM, 0, client_info);
            if (err && err->isError()) {
                DEBUG_MSG_ERROR(err);
                const std::string error_text = err->description();
                err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL,
                                          error_text.c_str(), false, info());
                if (err && err->isError()) {
                    DEBUG_MSG_ERROR(err);
                }
                return;
            }

            tcp::ITcpLoop* server = rclient->server();
            eclient = new ProxyRelayClient(server, client_info, rclient);
            server->registerClient(eclient);
            rclient->setEclient(eclient);
        }

        CHECK(eclient);

        ssize_t nwrite = 0;
        err = eclient->write(buff, nread, &nwrite);
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
            const std::string error_text = err->description();
            err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL,
                                      error_text.c_str(), false, info());
            if (err && err->isError()) {
                DEBUG_MSG_ERROR(err);
            }
            return;
        }
    } else {
        err = rclient->send_error(protocol, common::http::HS_INTERNAL_ERROR, NULL,
                                  "Invalid external host!", false, info());
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
    }
}

void Http2InnerServerHandler::proxyDataReceived(ProxyRelayClient* prclient) {
    char buff[BUF_SIZE] = {0};
    ssize_t nread = 0;
    common::Error err = prclient->read(buff, BUF_SIZE, &nread);
    if ((err && err->isError()) || nread == 0) {
        prclient->close();
        delete prclient;
        return;
    }

    RelayClient * rclient = prclient->relay();
    ssize_t nwrite = 0;
    err = rclient->TcpClient::write(buff, nread, &nwrite);
    if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
    }
    return;
}

void Http2InnerServerHandler::dataReceived(tcp::TcpClient* client) {
    if (client == innerConnection_) {
        innerDataReceived(innerConnection_);
        return;
    }

    RelayClientEx * rexclient = dynamic_cast<RelayClientEx*>(client);  // relay external connection
    if (rexclient) {
        relayExDataReceived(rexclient);
        return;
    }

    RelayClient * rclient = dynamic_cast<RelayClient*>(client);  // relay connection
    if (rclient) {
        relayDataReceived(rclient);
        return;
    }

    ProxyRelayClient * prclient = dynamic_cast<ProxyRelayClient*>(client);  // proxyrelay connection
    if (prclient) {
        proxyDataReceived(prclient);
        return;
    }

    Http2ServerHandler::dataReceived(client);  // direct connection
}

void Http2InnerServerHandler::dataReadyToWrite(tcp::TcpClient* client) {
}

}  // namespace inner
}  // namespace siteonyourdevice
}  // namespace fasto
