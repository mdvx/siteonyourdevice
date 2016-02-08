#!/usr/bin/env python
import pika
import sys
import threading
from build_server import BuildRpcServer
import base

class ThreadedBuildRpcServer(threading.Thread):
    def __init__(self, platform):
        threading.Thread.__init__(self)
        self.server = BuildRpcServer(platform)

    def run(self):
        self.server.start()

# argv[1] build platform

class ProxyConnector(object):
    def __init__(self, platform):
        credentials = pika.PlainCredentials(base.USER_NAME, base.PASSWORD)
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(host = base.REMOTE_HOST, credentials = credentials))
        self.channel = self.connection.channel()
        self.channel.queue_declare(queue = base.RPC_BUILD_SERVER_QUEUE)
        self.channel.basic_publish(exchange = '', routing_key = base.RPC_BUILD_SERVER_QUEUE, body = platform)

        self.server_listener = ThreadedBuildRpcServer(platform)

    def start(self):
        print('Proxy connector started')
        self.server_listener.start()
        return

argc = len(sys.argv)

if argc > 1:
    platform_str = sys.argv[1]
else:
    platform_str = base.get_os()

proxy = ProxyConnector(platform_str)
proxy.start()
