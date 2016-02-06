#!/usr/bin/env python
import pika
import json
import subprocess
import os

class Platform(object):
    def __init__(self, platform, archs, package_types):
        self.platform = platform
        self.archs = archs
        self.package_types = package_types

HOST = 'localhost'
SUPPORTED_PLATFORMS = [Platform('linux', [32, 64], ['DEB', 'RPM', 'TGZ']),
                       Platform('windows', [32, 64], ['NSIS', 'ZIP']),
                       Platform('macosx', [64], ['DragNDrop', 'ZIP']),
                       Platform('freebsd', [64], ['TGZ']) ]

connection = pika.BlockingConnection(pika.ConnectionParameters(
        host=HOST))

channel = connection.channel()

channel.queue_declare(queue='rpc_queue')

def run_command(cmd):
    output = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT)
    for line in iter(output.stdout.readline, b''):
        print(line.rstrip())

def build_package(op_id, platform, arch, branding_variables, package_type):

    platform_or_none = next((x for x in SUPPORTED_PLATFORMS if x.platform == platform), None)

    if platform_or_none == None:
        return 'invalid platform'

    if not arch in platform_or_none.archs:
        return 'invalid arch'
    if not package_type in platform_or_none.package_types:
        return 'invalid package_type'

    print('build started for: {0}, platform: {1}_{2}'.format(op_id, platform, arch))

    pwd = os.getcwd();
    dir_name = 'build_{0}_for_{1}'.format(platform, op_id);
    os.mkdir(dir_name)

    os.chdir(dir_name);

    cmake_args = '-DCMAKE_BUILD_TYPE=RELEASE -DOS_ARCH={0} {1}'.format(arch, branding_variables)
    cmake_line = ['cmake', '../../', '-G', 'Unix Makefiles', cmake_args]
    run_command(cmake_line)

    make_line = ['make', 'install', '-j2']
    run_command(make_line)

    package_line = ['cpack', '-G', package_type]
    run_command(package_line)

    os.chdir(pwd);

    print('build finished for: {0}, platform: {1}_{2}'.format(op_id, platform, arch))
    return 'ok'

def on_request(ch, method, props, body):
    data = json.loads(body)

    branding_variables = data.get('branding_variables')
    platform = data.get('platform')
    arch = data.get('arch')
    package_type = data.get('package_type')

    response = build_package(props.correlation_id, platform, arch, branding_variables, package_type)

    ch.basic_publish(exchange='',
                     routing_key=props.reply_to,
                     properties=pika.BasicProperties(correlation_id = props.correlation_id),
                     body=response)
    ch.basic_ack(delivery_tag = method.delivery_tag)

channel.basic_qos(prefetch_count=1)
channel.basic_consume(on_request, queue='rpc_queue')

print(" [x] Awaiting RPC build requests")
channel.start_consuming()
