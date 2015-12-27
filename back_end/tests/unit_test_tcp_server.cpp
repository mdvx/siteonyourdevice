#include <gtest/gtest.h>

#include "common/thread/thread_manager.h"
#include "tcp/tcp_server.h"

#define TCP_SERVERS_COUNT 10
#define START_PORT 4567

TEST(TcpServer, start_exec_stop_exit)
{
    using namespace fasto::siteonyourdevice;

    std::shared_ptr<common::thread::Thread<int> > tp[TCP_SERVERS_COUNT];
    TcpServer* servers[TCP_SERVERS_COUNT];

    for (int i = 0; i < SIZEOFMASS(tp); ++i){
        common::net::hostAndPort host("localhost", START_PORT + i);
        servers[i] = new TcpServer(host);
        common::Error err = servers[i]->bind();
        ASSERT_FALSE(err);
        err = servers[i]->listen(5);
        ASSERT_FALSE(err);

        tp[i] = THREAD_MANAGER()->createThread(&ITcpLoop::exec, servers[i]);
        tp[i]->start();
    }

    sleep(1);

    for (int i = 0; i < SIZEOFMASS(tp); ++i){
        servers[i]->stop();
        int res = tp[i]->joinAndGet();
        ASSERT_TRUE(res == EXIT_SUCCESS);
        delete servers[i];
    }
}
