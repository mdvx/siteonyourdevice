#include <gtest/gtest.h>

#include "http/http_streams.h"

TEST(IStream, create)
{
    using namespace fasto::fastoremote;
    common::http2::frame_priority fr(common::http2::frame_priority::create_frame_header(0, 0), NULL);
    common::net::socket_info sinf;
    IStream * stream = IStream::createStream(sinf, fr);
    delete stream;
}
