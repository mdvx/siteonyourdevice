#include "http/http_client.h"

#include <inttypes.h>
#include <errno.h>

#include "common/sprintf.h"
#include "common/net/net.h"
#include "common/logger.h"
#include "common/file_system.h"

#include "http_server.h"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

namespace fasto
{
    namespace siteonyourdevice
    {
        HttpClient::HttpClient(TcpServer *server, const common::net::socket_info& info)
            : TcpClient(server, info), isAuth_(false)
        {

        }

        const char* HttpClient::className() const
        {
            return "HttpClient";
        }

        void HttpClient::setIsAuthenticated(bool auth)
        {
            isAuth_ = auth;
        }

        bool HttpClient::isAuthenticated() const
        {
            return isAuth_;
        }

        common::ErrnoError HttpClient::send_error(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* text, bool is_keep_alive)
        {
            CHECK(protocol <= common::http::HP_1_1);
            const HttpServer* serv = dynamic_cast<const HttpServer*>(server());
            CHECK(serv);
            const std::string title = common::convertToString(status);
            const HttpServerInfo info = serv->info();

            char err_data[1024] = {0};
            off_t err_len = common::SPrintf(err_data, "\
                    <!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n\
                    <html>\n\
                    <head>\n\
                    <meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n\
                    <title>%d %s</title>\n\
                    </head>\n\
                    <body bgcolor=\"#cc9999\">\n\
                    <h4>%d %s</h4>\n%s\n"
                    "<hr>\n\
                    <address><a href=\"%s\">%s</a></address>\n\
                    </body>\n\
                    </html>\n", status, title, status, title, text, info.serverUrl_, info.serverName_);
            send_headers(protocol, status, extra_header, "text/html", &err_len, NULL, is_keep_alive);

            ssize_t nwrite = 0;
            common::ErrnoError err = write(err_data, err_len, nwrite);
            DCHECK(!err);
            return err;
        }

        common::ErrnoError HttpClient::send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size)
        {
            CHECK(protocol <= common::http::HP_1_1);
            return common::net::send_file_to_fd(fd(), fdesc, 0, size);
        }

        common::ErrnoError HttpClient::send_headers(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* mime_type, off_t* length, time_t* mod, bool is_keep_alive)
        {
            CHECK(protocol <= common::http::HP_1_1);
            const HttpServer* serv = dynamic_cast<const HttpServer*>(server());
            CHECK(serv);
            const std::string title = common::convertToString(status);
            const HttpServerInfo info = serv->info();

            time_t now = time(NULL);
            char timebuf[100];
            strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &now ));

            char header_data[1024] = {0};
            int cur_pos = common::SPrintf(header_data, HTTP_2_0_PROTOCOL_NAME " %d %s\r\nServer: %s\r\nDate: %s\r\n", status, title, info.serverName_, timebuf);

            if (extra_header){
                int exlen = common::SPrintf(header_data + cur_pos, "%s\r\n", extra_header);
                cur_pos += exlen;
            }
            if (mime_type){
                int mim_t = common::SPrintf(header_data + cur_pos, "Content-Type: %s\r\n", mime_type);
                cur_pos += mim_t;
            }
            if (length){
                int len = common::SPrintf(header_data + cur_pos, "Content-Length: %" PRId32 "\r\n", *length);
                cur_pos += len;
            }
            if (mod){
                strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(mod));
                int mlen = common::SPrintf(header_data + cur_pos, "Last-Modified: %s\r\n", timebuf);
                cur_pos += mlen;
            }

            if(!is_keep_alive){
                #define CONNECTION_CLOSE "Connection: close\r\n\r\n"
                const int last_len = sizeof(CONNECTION_CLOSE) - 1;
                memcpy(header_data + cur_pos, CONNECTION_CLOSE, last_len);
                cur_pos += last_len;
            }
            else{
                #define CONNECTION_KEEP_ALIVE "Keep-Alive: timeout=15, max=100\r\n\r\n"
                const int last_len = sizeof(CONNECTION_KEEP_ALIVE) - 1;
                memcpy(header_data + cur_pos, CONNECTION_KEEP_ALIVE, last_len);
                cur_pos += last_len;
            }

            DCHECK(strlen(header_data) == cur_pos);
            ssize_t nwrite = 0;
            common::ErrnoError err = write(header_data, cur_pos, nwrite);
            DCHECK(!err);
            return err;
        }

        Http2Client::Http2Client(TcpServer *server, const common::net::socket_info& info)
            : HttpClient(server, info), streams_()
        {

        }

        const char* Http2Client::className() const
        {
            return "Http2Client";
        }

        bool Http2Client::is_http2() const
        {
            StreamSPtr main_stream = findStreamByStreamID(0);
            return main_stream.get();
        }

        common::ErrnoError Http2Client::send_error(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* text, bool is_keep_alive)
        {
            using namespace common;
            if(is_http2() && protocol == http::HP_2_0){
                const HttpServer* serv = dynamic_cast<const HttpServer*>(server());
                CHECK(serv);
                const std::string title = common::convertToString(status);
                const HttpServerInfo info = serv->info();

                char err_data[1024] = {0};
                off_t err_len = common::SPrintf(err_data, "\
                        <!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n\
                        <html>\n\
                        <head>\n\
                        <meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n\
                        <title>%d %s</title>\n\
                        </head>\n\
                        <body bgcolor=\"#cc9999\">\n\
                        <h4>%d %s</h4>\n%s\n"
                        "<hr>\n\
                        <address><a href=\"%s\">%s</a></address>\n\
                        </body>\n\
                        </html>\n", status, title, status, title, text, info.serverUrl_, info.serverName_);
                common::ErrnoError err = send_headers(protocol, status, extra_header, "text/html", &err_len, NULL, is_keep_alive);
                if(err && err->isError()){
                    return err;
                }

                StreamSPtr header_stream = findStreamByType(http2::HTTP2_HEADERS);
                if(!header_stream){
                    return DEBUG_MSG_PERROR("findStreamByType", EAGAIN);
                }

                http2::frame_hdr hdr = http2::frame_data::create_frame_header(http2::HTTP2_FLAG_END_STREAM, header_stream->sid(), err_len);
                http2::frame_data fdata(hdr, err_data);
                return header_stream->sendFrame(fdata);
            }

            return HttpClient::send_error(protocol, status, extra_header, text, is_keep_alive);
        }

        namespace
        {
            struct send_data_helper
            {
                StreamSPtr header_stream;
                uint32_t all_size;
            };

            common::ErrnoError send_data_frame(const char* buff, uint32_t buff_len, void *user_data, uint32_t *processed)
            {
                using namespace common;
                send_data_helper *helper = (send_data_helper *)user_data;
                StreamSPtr header_stream = helper->header_stream;

                uint8_t flags = 0;
                if(helper->all_size - buff_len == 0){
                    flags = http2::HTTP2_FLAG_END_STREAM;
                }

                http2::frame_hdr hdr = http2::frame_data::create_frame_header(flags, header_stream->sid(), buff_len);
                http2::frame_data fdata(hdr, buff);
                common::ErrnoError err = header_stream->sendFrame(fdata);
                if(err && err->isError()){
                    return err;
                }

                *processed = buff_len;
                helper->all_size -= buff_len;

                return common::ErrnoError();
            }
        }

        common::ErrnoError Http2Client::send_file_by_fd(common::http::http_protocols protocol, int fdesc, off_t size)
        {
            using namespace common;

            if(is_http2() && protocol == common::http::HP_2_0){
                StreamSPtr header_stream = findStreamByType(http2::HTTP2_HEADERS);
                if(!header_stream){
                    return DEBUG_MSG_PERROR("findStreamByType", EAGAIN);
                }

                send_data_helper help;
                help.header_stream = header_stream;
                help.all_size = size;

                common::ErrnoError err = common::file_system::read_file_cb(fdesc, 0, size, &send_data_frame, &help);
                if(err && err->isError()){
                    DEBUG_MSG_ERROR(err);
                    return err;
                }

                return common::ErrnoError();
            }

            return HttpClient::send_file_by_fd(protocol, fdesc, size);
        }

        common::ErrnoError Http2Client::send_headers(common::http::http_protocols protocol, common::http::http_status status, const char* extra_header, const char* mime_type, off_t* length, time_t* mod, bool is_keep_alive)
        {
            using namespace common;

            if(is_http2() && protocol == http::HP_2_0){
                const HttpServer* serv = dynamic_cast<const HttpServer*>(server());
                CHECK(serv);
                const HttpServerInfo info = serv->info();

                StreamSPtr header_stream = findStreamByType(http2::HTTP2_HEADERS);
                if(!header_stream){
                    return DEBUG_MSG_PERROR("findStreamByType", EAGAIN);
                }

                http2::http2_nvs_t nvs;

                http2::http2_nv nvstatus;
                nvstatus.name = MAKE_BUFFER_TYPE(":status");
                nvstatus.value = common::convertToBytes((uint32_t)status);
                nvs.push_back(nvstatus);

                char timebuf[100];
                time_t now = time(NULL);
                strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
                http2::http2_nv nvdate;
                nvdate.name = MAKE_BUFFER_TYPE("date");
                nvdate.value = common::convertToBytes(timebuf);
                nvs.push_back(nvdate);

                http2::http2_nv nvserver;
                nvserver.name = MAKE_BUFFER_TYPE("server");
                nvserver.value = common::convertToBytes(info.serverName_);
                nvs.push_back(nvserver);

                /*http2::http2_nv nvenc;
                nvenc.name = MAKE_BUFFER_TYPE("content-encoding");
                nvenc.value = MAKE_BUFFER_TYPE("deflate");
                nvs.push_back(nvenc);*/

                if(mime_type){
                    http2::http2_nv nvmime;
                    nvmime.name = MAKE_BUFFER_TYPE("content-type");
                    nvmime.value = common::convertToBytes(mime_type);
                    nvs.push_back(nvmime);
                }
                if(length){
                    http2::http2_nv nvlen;
                    nvlen.name = MAKE_BUFFER_TYPE("content-length");
                    nvlen.value = common::convertToBytes(*length);
                    nvs.push_back(nvlen);
                }

                if(mod){
                    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(mod));
                    http2::http2_nv nvmod;
                    nvmod.name = MAKE_BUFFER_TYPE("last-modified");
                    nvmod.value = common::convertToBytes(timebuf);
                    nvs.push_back(nvmod);
                }

                http2::http2_deflater hd;
                buffer_type buff;
                hd.http2_deflate_hd_bufs(buff, nvs);

                common::http2::frame_hdr hdr = common::http2::frame_headers::create_frame_header(http2::HTTP2_FLAG_END_HEADERS, header_stream->sid(), buff.size());
                common::http2::frame_headers fhdr(hdr, buff);

                return header_stream->sendFrame(fhdr);
            }

            return HttpClient::send_headers(protocol, status, extra_header, mime_type, length, mod, is_keep_alive);
        }

        StreamSPtr Http2Client::findStreamByStreamID(uint32_t stream_id) const
        {
            for (int i = 0; i < streams_.size(); ++i) {
                StreamSPtr stream = streams_[i];
                if(stream->sid() == stream_id){
                    return stream;
                }
            }

            return StreamSPtr();
        }

        StreamSPtr Http2Client::findStreamByType(common::http2::frame_type type) const
        {
            for (int i = 0; i < streams_.size(); ++i) {
                StreamSPtr stream = streams_[i];
                if(stream->type() == type){
                    return stream;
                }
            }

            return StreamSPtr();
        }

        bool Http2Client::isSettingNegotiated() const
        {
            StreamSPtr settings = findStreamByStreamID(0);
            if(!settings){
                return false;
            }

            HTTP2SettingsStream* rsettings = dynamic_cast<HTTP2SettingsStream*>(settings.get());
            CHECK(rsettings);
            return rsettings->isNegotiated();
        }

        void Http2Client::processFrames(const common::http2::frames_t& frames)
        {
            common::net::socket_info inf = info();
            for(int i = 0; i < frames.size(); ++i){
                common::http2::frame_base frame = frames[i];
                StreamSPtr stream = findStreamByStreamID(frame.stream_id());
                if(!stream){
                    IStream* nstream = IStream::createStream(inf, frame);
                    stream = StreamSPtr(nstream);
                    streams_.push_back(stream);
                }

                stream->processFrame(frame);
            }
        }
    }
}
