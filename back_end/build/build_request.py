#!/usr/bin/env python
import pika
import uuid
import json

HOST = 'localhost'

class BuildRpcClient(object):
    def __init__(self):
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(
                host=HOST))

        self.channel = self.connection.channel()

        result = self.channel.queue_declare(exclusive=True)
        self.callback_queue = result.method.queue

        self.channel.basic_consume(self.on_response, no_ack=True,
                                   queue=self.callback_queue)

    def on_response(self, ch, method, props, body):
        if self.corr_id == props.correlation_id:
            self.response = body

    def call(self, platform, arch, branding_variables, package_type):
        self.response = None
        self.corr_id = str(uuid.uuid4())
        data = {
            'branding_variables': branding_variables,
            'platform': platform,
            'arch': arch,
            'package_type' : package_type
        }
        self.channel.basic_publish(exchange='',
                                   routing_key='rpc_queue',
                                   properties=pika.BasicProperties(
                                         reply_to = self.callback_queue,
                                         correlation_id = self.corr_id,
                                         ),
                                   body=json.dumps(data))
        while self.response is None:
            self.connection.process_data_events()
        return self.response

build_rpc = BuildRpcClient()

response = build_rpc.call('linux', 64, '-DPROJECT_NAME=Fasto', 'DEB')
print(" [.] Got %r" % response)
