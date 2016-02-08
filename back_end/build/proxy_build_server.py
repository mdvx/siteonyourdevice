#!/usr/bin/env python
import pika
import threading
import json
import sys
import config
from build_request import ResponceHander

def print_usage():
    print("Usage:\n"
        "[required] argv[1] queue_name\n")

class OutBuildRpcServer(object):
    def __init__(self, rpc_queue_name):
        credentials = pika.PlainCredentials(config.USER_NAME, config.PASSWORD)
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(host = config.SERVER_HOST, credentials = credentials))
        self.channel = self.connection.channel()
        self.channel.queue_declare(queue = rpc_queue_name)
        self.channel.basic_qos(prefetch_count = 1)
        self.channel.basic_consume(self.on_request, queue = rpc_queue_name)
        self.exists_channels = {}

    def add_channel(self, platform, channel):
        self.exists_channels[platform] = channel

    def on_request(self, ch, method, props, body):
        print("Received request %r" % body)
        data = json.loads(body)
        platform = data.get('platform')
        op_id = props.correlation_id
        channel = self.exists_channels.get(platform, None)
        if channel == None:
            ch.basic_publish(exchange = '',
                         routing_key = props.reply_to,
                         properties = pika.BasicProperties(correlation_id = op_id),
                         body = 'Build machine not exist')
            ch.basic_ack(delivery_tag = method.delivery_tag)
            return

        handler = ResponceHander(op_id, channel)
        response = handler.execute(platform, body)

        ch.basic_publish(exchange = '',
                         routing_key = props.reply_to,
                         properties = pika.BasicProperties(correlation_id = op_id),
                         body = response)
        ch.basic_ack(delivery_tag = method.delivery_tag)


    def start(self):
        print("Awaiting in RPC build requests")
        self.channel.start_consuming()

class ThreadedProxyBuildInListener(threading.Thread):
    def __init__(self, queue_name):
        threading.Thread.__init__(self)
        self.server = OutBuildRpcServer(queue_name)

    def run(self):
        self.server.start()

class ProxyBuildRpcServer(object):
    def __init__(self, rpc_queue_name_in, rpc_queue_name_out):
        credentials = pika.PlainCredentials(config.USER_NAME, config.PASSWORD)
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(host = config.SERVER_HOST, credentials = credentials))
        self.channel = self.connection.channel()
        self.channel.queue_declare(queue = rpc_queue_name_out)
        self.channel.basic_consume(self.on_request, queue = rpc_queue_name_out, no_ack=True)

        self.listener_thread = ThreadedProxyBuildInListener(rpc_queue_name_in)

    def start(self):
        print("Awaiting proxy RPC build requests")
        self.listener_thread.start()
        self.channel.start_consuming()

    def on_request(self, ch, method, props, body):
        print("Received build system machine %r" % body)
        self.listener_thread.server.add_channel(body, ch)

if __name__ == "__main__":
    argc = len(sys.argv)

    if argc > 1:
        queue_name = sys.argv[1]
    else:
        print("Queue name not passed!")
        print_usage()
        quit()

    server = ProxyBuildRpcServer(queue_name, config.RPC_BUILD_SERVER_QUEUE)
    server.start()