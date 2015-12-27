#include "websocket/websocket_server_handler.h"

#include <arpa/inet.h>

#include "common/http/http.h"
#include "common/string_util.h"
#include "common/sprintf.h"
#include "common/sha1.h"
#include "common/utils.h"
#include "common/logger.h"

#include "websocket/websocket_client.h"

#define WEBSOCK_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define BUF_LEN 0xFFFF

namespace
{
    enum wsFrameType { // errors starting from 0xF0
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

    size_t getPayloadLength(const uint8_t *inputFrame, size_t inputLength, uint8_t *payloadFieldExtraBytes, enum wsFrameType *frameType)
    {
        size_t payloadLength = inputFrame[1] & 0x7F;
        *payloadFieldExtraBytes = 0;
        if ((payloadLength == 0x7E && inputLength < 4) || (payloadLength == 0x7F && inputLength < 10)) {
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
        }
        else if (payloadLength == 0x7F) {
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

    wsFrameType wsParseInputFrame(const uint8_t *inputFrame, size_t inputLength, uint8_t **dataPtr, size_t *dataLength)
    {
        DCHECK(inputFrame && inputLength);
        if (inputLength < 2)
            return WS_INCOMPLETE_FRAME;
        if ((inputFrame[0] & 0x70) != 0x0) // checks extensions off
            return WS_ERROR_FRAME;
        if ((inputFrame[0] & 0x80) != 0x80) // we haven't continuation frames support
            return WS_ERROR_FRAME; // so, fin flag must be set
        if ((inputFrame[1] & 0x80) != 0x80) // checks masking bit
            return WS_ERROR_FRAME;
        uint8_t opcode = inputFrame[0] & 0x0F;
        if (opcode == WS_TEXT_FRAME ||
            opcode == WS_BINARY_FRAME ||
            opcode == WS_CLOSING_FRAME ||
            opcode == WS_PING_FRAME ||
            opcode == WS_PONG_FRAME
        ){
            wsFrameType frameType = (wsFrameType)opcode;
            uint8_t payloadFieldExtraBytes = 0;
            size_t payloadLength = getPayloadLength(inputFrame, inputLength, &payloadFieldExtraBytes, &frameType);
            if (payloadLength > 0) {
                if (payloadLength + 6 + payloadFieldExtraBytes > inputLength) // 4-maskingKey, 2-header
                    return WS_INCOMPLETE_FRAME;

                uint8_t *maskingKey = (uint8_t *)(&inputFrame[2 + payloadFieldExtraBytes]);
                DCHECK(payloadLength == inputLength - 6 - payloadFieldExtraBytes);
                *dataPtr = (uint8_t *)(&inputFrame[2 + payloadFieldExtraBytes + 4]);
                *dataLength = payloadLength;
                size_t i;
                for (i = 0; i < *dataLength; i++) {
                    (*dataPtr)[i] = (*dataPtr)[i] ^ maskingKey[i%4];
                }
            }
            return frameType;
        }
        return WS_ERROR_FRAME;
    }

    void wsMakeFrame(const uint8_t *data, size_t dataLength, uint8_t *outFrame, size_t *outLength, wsFrameType frameType)
    {
        DCHECK(outFrame && *outLength);
        DCHECK(frameType < 0x10);
        if (dataLength > 0)
            DCHECK(data);

        outFrame[0] = 0x80 | frameType;
        if (dataLength <= 125) {
            outFrame[1] = dataLength;
            *outLength = 2;
        }
        else if (dataLength <= 0xFFFF) {
            outFrame[1] = 126;
            uint16_t payloadLength16b = htons(dataLength);
            memcpy(&outFrame[2], &payloadLength16b, 2);
            *outLength = 4;
        }
        else {
            DCHECK(dataLength <= 0xFFFF);
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
}

namespace fasto
{
    namespace siteonyourdevice
    {
        WebSocketServerHandler::WebSocketServerHandler(const HttpServerInfo &info)
            : Http2ServerHandler(info, NULL)
        {

        }

        void WebSocketServerHandler::processReceived(HttpClient *hclient, const char* request, size_t req_len)
        {
            uint8_t *data = NULL;
            size_t dataSize = 0;
            wsFrameType frame_type = wsParseInputFrame((const uint8_t *)request, req_len, &data, &dataSize);

            if(frame_type == WS_ERROR_FRAME){
                Http2ServerHandler::processReceived(hclient, request, req_len);
            }
            else if(frame_type == WS_INCOMPLETE_FRAME && req_len == BUF_LEN){
                size_t frameSize = BUF_LEN;
                uint8_t odata[BUF_LEN] = {0};
                wsMakeFrame(NULL, 0, odata, &frameSize, WS_CLOSING_FRAME);

                ssize_t nwrite = 0;
                common::Error err = hclient->write(odata, frameSize, nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
                hclient->close();
                delete hclient;
            }
            else if(frame_type == WS_CLOSING_FRAME){
                hclient->close();
                delete hclient;
            }
            else if(frame_type == WS_TEXT_FRAME){
                size_t frameSize = BUF_LEN;
                uint8_t odata[BUF_LEN] = {0};
                wsMakeFrame(data, dataSize, odata, &frameSize, WS_TEXT_FRAME);

                ssize_t nwrite = 0;
                common::Error err = hclient->write(odata, frameSize, nwrite);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                }
            }
            else{
                DNOTREACHED();
            }
        }

        void WebSocketServerHandler::handleRequest(HttpClient *hclient, const common::http::http_request& request, bool notClose)
        {
            using namespace common::http;
            /*
            HTTP/1.1 101 Switching Protocols
            Upgrade: websocket
            Connection: Upgrade
            Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
            */

            const http_protocols protocol = request.protocol();
            if(request.method_ != http_method::HM_GET){
                hclient->send_error(protocol, HS_BAD_REQUEST, NULL, "Bad Request", notClose, info());
                return;
            }

            http_request::header_t connectionField = request.findHeaderByKey("Connection", false);
            const std::string lconnectionField = common::StringToLowerASCII(connectionField.value_);
            bool isUpgrade = lconnectionField.find_first_of("upgrade") != std::string::npos;
            if(!isUpgrade){
                hclient->send_error(protocol, HS_BAD_REQUEST, NULL, "Bad Request", notClose, info());
                return;
            }

            http_request::header_t upgradeField = request.findHeaderByKey("Upgrade", false);
            bool isWebSocket = EqualsASCII(upgradeField.value_, "websocket", false);
            if(!isWebSocket){
                hclient->send_error(protocol, HS_BAD_REQUEST, NULL, "Bad Request", notClose, info());
                return;
            }

            http_request::header_t keyField = request.findHeaderByKey("Sec-WebSocket-Key", false);
            if(!keyField.isValid()){
                hclient->send_error(protocol, HS_BAD_REQUEST, NULL, "Bad Request", notClose, info());
                return;
            }

            http_request::header_t webVersionField = request.findHeaderByKey("Sec-WebSocket-Version", false);
            if(!webVersionField.isValid()){
                hclient->send_error(protocol, HS_BAD_REQUEST, NULL, "Bad Request", notClose, info());
                return;
            }

            const std::string sec_key = keyField.value_ + WEBSOCK_GUID;

            sha1nfo s;

            sha1_init(&s);
            sha1_write(&s, sec_key.c_str(), sec_key.length());
            uint8_t* sha_bin_ptr = sha1_result(&s);

            const common::buffer_type bin_sha1 = MAKE_BUFFER_TYPE_SIZE(sha_bin_ptr, HASH_LENGTH);
            common::buffer_type enc_accept = common::utils::base64::encode64(bin_sha1);
            const std::string header_up = common::MemSPrintf("Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s",
                                                             common::convertToString(enc_accept));
            hclient->send_headers(protocol, HS_SWITCH_PROTOCOL, header_up.c_str(), NULL, NULL, NULL, notClose, info());
        }
    }
}
