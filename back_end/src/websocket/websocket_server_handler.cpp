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

#include "websocket/websocket_server_handler.h"

#include <string>

#ifdef OS_FREEBSD
#include <arpa/inet.h>
#endif

#include "common/http/http.h"
#include "common/string_util.h"
#include "common/sprintf.h"
#include "common/sha1.h"
#include "common/utils.h"
#include "common/logger.h"

#include "websocket/websocket_client.h"

#define WEBSOCK_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define BUF_LEN 0xFFFF

namespace {
enum wsFrameType {  // errors starting from 0xF0
    WS_EMPTY_FRAME = 0xF0,
    WS_ERROR_FRAME = 0xF1,
    WS_INCOMPLETE_FRAME = 0xF2,
    WS_TEXT_FRAME = 0x01,
    WS_BINARY_FRAME = 0x02,
    WS_PING_FRAME = 0x09,
    WS_PONG_FRAME = 0x0A,
    WS_OPENING_FRAME = 0xF3,
    WS_CLOSING_FRAME = 0x08
};

size_t getPayloadLength(const uint8_t *inputFrame,
                        size_t inputLength, uint8_t *payloadFieldExtraBytes,
                        enum wsFrameType *frameType) {
    size_t payloadLength = inputFrame[1] & 0x7F;
    *payloadFieldExtraBytes = 0;
    if ((payloadLength == 0x7E && inputLength < 4)
            || (payloadLength == 0x7F && inputLength < 10)) {
        *frameType = WS_INCOMPLETE_FRAME;
        return 0;
    }
    if (payloadLength == 0x7F && (inputFrame[3] & 0x80) != 0x0) {
        *frameType = WS_ERROR_FRAME;
        return 0;
    }

    if (payloadLength == 0x7E) {
        uint16_t payloadLength16b = 0;
        *payloadFieldExtraBytes = 2;
        memcpy(&payloadLength16b, &inputFrame[2], *payloadFieldExtraBytes);
        payloadLength = ntohs(payloadLength16b);
    } else if (payloadLength == 0x7F) {
        *frameType = WS_ERROR_FRAME;
        return 0;
        /* // implementation for 64bit systems
        uint64_t payloadLength64b = 0;
        *payloadFieldExtraBytes = 8;
        memcpy(&payloadLength64b, &inputFrame[2], *payloadFieldExtraBytes);
        if (payloadLength64b > SIZE_MAX) {
        *frameType = WS_ERROR_FRAME;
        return 0;
        }
        payloadLength = (size_t)ntohll(payloadLength64b);
        */
    }
    return payloadLength;
}

wsFrameType wsParseInputFrame(const uint8_t *inputFrame,
                              size_t inputLength, uint8_t **dataPtr,
                              size_t *dataLength) {
    DCHECK(inputFrame && inputLength);
    if (inputLength < 2)
        return WS_INCOMPLETE_FRAME;
    if ((inputFrame[0] & 0x70) != 0x0)  // checks extensions off
        return WS_ERROR_FRAME;
    if ((inputFrame[0] & 0x80) != 0x80)  // we haven't continuation frames
        return WS_ERROR_FRAME;  // so, fin flag must be set
    if ((inputFrame[1] & 0x80) != 0x80)  // checks masking bit
        return WS_ERROR_FRAME;
    uint8_t opcode = inputFrame[0] & 0x0F;
    if (opcode == WS_TEXT_FRAME ||
        opcode == WS_BINARY_FRAME ||
        opcode == WS_CLOSING_FRAME ||
        opcode == WS_PING_FRAME ||
        opcode == WS_PONG_FRAME) {
        wsFrameType frameType = (wsFrameType)opcode;
        uint8_t payloadFieldExtraBytes = 0;
        size_t payloadLength = getPayloadLength(inputFrame,
                                                inputLength,
                                                &payloadFieldExtraBytes,
                                                &frameType);
        if (payloadLength > 0) {
            if (payloadLength + 6 + payloadFieldExtraBytes > inputLength)  // 4-maskingKey, 2-header
                return WS_INCOMPLETE_FRAME;

            uint8_t *maskingKey = const_cast<uint8_t *>(&inputFrame[2 + payloadFieldExtraBytes]);
            DCHECK(payloadLength == inputLength - 6 - payloadFieldExtraBytes);
            *dataPtr = const_cast<uint8_t *>(&inputFrame[2 + payloadFieldExtraBytes + 4]);
            *dataLength = payloadLength;
            for (size_t i = 0; i < *dataLength; i++) {
                (*dataPtr)[i] = (*dataPtr)[i] ^ maskingKey[i%4];
            }
        }
        return frameType;
    }
    return WS_ERROR_FRAME;
}

void wsMakeFrame(const uint8_t *data,
                 size_t dataLength, uint8_t *outFrame,
                 size_t *outLength, wsFrameType frameType) {
    DCHECK(outFrame && *outLength);
    DCHECK_LT(frameType, 0x10);
    if (dataLength > 0) {
        DCHECK(data);
    }

    outFrame[0] = 0x80 | frameType;
    if (dataLength <= 125) {
        outFrame[1] = dataLength;
        *outLength = 2;
    } else if (dataLength <= 0xFFFF) {
        outFrame[1] = 126;
        uint16_t payloadLength16b = htons(dataLength);
        memcpy(&outFrame[2], &payloadLength16b, 2);
        *outLength = 4;
    } else {
        DCHECK_LE(dataLength, 0xFFFF);
        /* implementation for 64bit systems
        outFrame[1] = 127;
        dataLength = htonll(dataLength);
        memcpy(&outFrame[2], &dataLength, 8);
        *outLength = 10;
        */
    }

    memcpy(&outFrame[*outLength], data, dataLength);
    *outLength+= dataLength;
}
}  // namespace

namespace fasto {
namespace siteonyourdevice {
namespace websocket {

WebSocketServerHandler::WebSocketServerHandler(const HttpServerInfo &info)
    : Http2ServerHandler(info, NULL) {
}

void WebSocketServerHandler::processReceived(http::HttpClient *hclient,
                                             const char* request, size_t req_len) {
    uint8_t *data = NULL;
    size_t dataSize = 0;
    wsFrameType frame_type = wsParseInputFrame((const uint8_t *)request, req_len, &data, &dataSize);

    if (frame_type == WS_ERROR_FRAME) {
        Http2ServerHandler::processReceived(hclient, request, req_len);
    } else if (frame_type == WS_INCOMPLETE_FRAME && req_len == BUF_LEN) {
        size_t frameSize = BUF_LEN;
        uint8_t odata[BUF_LEN] = {0};
        wsMakeFrame(NULL, 0, odata, &frameSize, WS_CLOSING_FRAME);

        ssize_t nwrite = 0;
        common::Error err = hclient->write((const char*)odata, frameSize, &nwrite);
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
        hclient->close();
        delete hclient;
    } else if (frame_type == WS_CLOSING_FRAME) {
        hclient->close();
        delete hclient;
    } else if (frame_type == WS_TEXT_FRAME) {
        size_t frameSize = BUF_LEN;
        uint8_t odata[BUF_LEN] = {0};
        wsMakeFrame(data, dataSize, odata, &frameSize, WS_TEXT_FRAME);

        ssize_t nwrite = 0;
        common::Error err = hclient->write((const char*)odata, frameSize, &nwrite);
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
    } else if (frame_type == WS_BINARY_FRAME) {
        size_t frameSize = BUF_LEN;
        uint8_t odata[BUF_LEN] = {0};
        wsMakeFrame(data, dataSize, odata, &frameSize, WS_BINARY_FRAME);

        ssize_t nwrite = 0;
        common::Error err = hclient->write((const char*)odata, frameSize, &nwrite);
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
    } else {
        DNOTREACHED();
    }
}

void WebSocketServerHandler::handleRequest(http::HttpClient *hclient,
                                           const common::http::http_request& request,
                                           bool notClose) {
    /*
    HTTP/1.1 101 Switching Protocols
    Upgrade: websocket
    Connection: Upgrade
    Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
    */

    const common::http::http_protocols protocol = request.protocol();
    if (request.method_ != common::http::http_method::HM_GET) {
        common::Error err = hclient->send_error(protocol, common::http::HS_BAD_REQUEST, NULL,
                                                "Bad Request", notClose, info());
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
        return;
    }

    common::http::header_t connectionField = request.findHeaderByKey("Connection", false);
    const std::string lconnectionField = common::StringToLowerASCII(connectionField.value_);
    bool isUpgrade = lconnectionField.find_first_of("upgrade") != std::string::npos;
    if (!isUpgrade) {
        common::Error err = hclient->send_error(protocol, common::http::HS_BAD_REQUEST,
                                                NULL, "Bad Request",
                                                notClose, info());
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
        return;
    }

    common::http::header_t upgradeField = request.findHeaderByKey("Upgrade", false);
    bool isWebSocket = EqualsASCII(upgradeField.value_, "websocket", false);
    if (!isWebSocket) {
        common::Error err = hclient->send_error(protocol, common::http::HS_BAD_REQUEST,
                                                NULL, "Bad Request",
                                                notClose, info());
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
        return;
    }

    common::http::header_t key_field = request.findHeaderByKey("Sec-WebSocket-Key", false);
    if (!key_field.isValid()) {
        common::Error err = hclient->send_error(protocol, common::http::HS_BAD_REQUEST,
                                                NULL, "Bad Request",
                                                notClose, info());
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
        return;
    }

    common::http::header_t web_vers_field = request.findHeaderByKey("Sec-WebSocket-Version", false);
    if (!web_vers_field.isValid()) {
        common::Error err = hclient->send_error(protocol, common::http::HS_BAD_REQUEST,
                                                NULL, "Bad Request",
                                                notClose, info());
        if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
        }
        return;
    }

    const std::string sec_key = key_field.value_ + WEBSOCK_GUID;

    sha1nfo s;

    sha1_init(&s);
    sha1_write(&s, sec_key.c_str(), sec_key.length());
    uint8_t* sha_bin_ptr = sha1_result(&s);

    common::buffer_type bin_sha1 = MAKE_BUFFER_TYPE_SIZE(sha_bin_ptr, HASH_LENGTH);
    common::buffer_type enc_accept = common::utils::base64::encode64(bin_sha1);
    std::string header_up = common::MemSPrintf("Upgrade: websocket\r\n"
                                               "Connection: Upgrade\r\n"
                                               "Sec-WebSocket-Accept: %s",
                                                common::convertToString(enc_accept));
    common::Error err = hclient->send_headers(protocol, common::http::HS_SWITCH_PROTOCOL,
                                              header_up.c_str(), NULL,
                                              NULL, NULL,
                                              notClose, info());
    if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
    }
}

}  // namespace websocket
}  // namespace siteonyourdevice
}  // namespace fasto
