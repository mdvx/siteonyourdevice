#!/usr/bin/env python
import pika
import json
import subprocess
import os
import shutil
import sys
import shlex
import re
import base
import config

def print_usage():
    print("Usage:\n"
        "[optional] argv[1] platform\n"
        "[optional] argv[3] architecture\n")

def run_command(cmd):
    """run_command(cmd) returns a error."""
    try:
        output = subprocess.check_call(cmd)
    except subprocess.CalledProcessError as ex:
        return base.Error(base.ERROR, str(ex))
    else:
        return base.Error()

class BuildRpcServer(object):
    def __init__(self, platform):
        credentials = pika.PlainCredentials(config.USER_NAME, config.PASSWORD)
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(host = config.REMOTE_HOST, credentials = credentials))
        self.channel = self.connection.channel()
        self.channel.queue_declare(queue = platform)
        self.channel.basic_qos(prefetch_count = 1)
        self.channel.basic_consume(self.on_request, queue = platform)
        print("Build server for %s created!\n" % platform)

    def start(self):
        print("Awaiting RPC build requests")
        self.channel.start_consuming()

    def build_package(self, op_id, platform, arch, branding_variables, package_type, destination):

        platform_or_none = base.get_supported_platform_by_name(platform)

        if platform_or_none == None:
            return (base.Error(base.ERROR, 'invalid platform'), None)

        if not arch in platform_or_none.archs:
            return (base.Error(base.ERROR, 'invalid arch'), None)
        if not package_type in platform_or_none.package_types:
            return (base.Error(base.ERROR, 'invalid package_type'), None)

        pwd = os.getcwd()
        dir_name = 'build_{0}_for_{1}'.format(platform, op_id)
        if os.path.exists(dir_name):
            shutil.rmtree(dir_name)

        os.mkdir(dir_name)
        os.chdir(dir_name)

        arch_args = '-DOS_ARCH={0}'.format(arch)
        generator_args = '-DCPACK_GENERATOR={0}'.format(package_type)
        branding_variables_list = shlex.split(branding_variables)
        cmake_line = ['cmake', '../../', '-GUnix Makefiles', '-DCMAKE_BUILD_TYPE=RELEASE', generator_args, arch_args]
        cmake_line.extend(branding_variables_list)
        err = run_command(cmake_line)
        if err.isError():
            os.chdir(pwd)
            return (err, None)

        make_line = ['make', 'package', '-j2']
        err = run_command(make_line)
        if err.isError():
            os.chdir(pwd)
            return (err, None)

        in_file = open('CPackConfig.cmake', 'r')
        for line in in_file.readlines():
            res = re.search(r'SET\(CPACK_SOURCE_PACKAGE_FILE_NAME "(.+)"\)', line)
            if res != None:
                filename = res.group(1) + '.' + base.get_extension_by_package(package_type)
        in_file.close()

        err, result = config.post_install_step(filename, destination)
        os.chdir(pwd)
        return (err, result)

    def on_request(self, ch, method, props, body):
        data = json.loads(body)

        branding_variables = data.get('branding_variables')
        platform = data.get('platform')
        arch = data.get('arch')
        package_type = data.get('package_type')
        destination = data.get('destination')
        op_id = props.correlation_id

        print('build started for: {0}, platform: {1}_{2}'.format(op_id, platform, arch))
        err, response = self.build_package(op_id, platform, arch, branding_variables, package_type, destination)
        print('build finished for: {0}, platform: {1}_{2}, responce: {3}'.format(op_id, platform, arch, response))

        ch.basic_publish(exchange = '',
                         routing_key = props.reply_to,
                         properties = pika.BasicProperties(content_type= 'application/json', correlation_id = op_id),
                         body = {error: err.destination(), body: responce})
        ch.basic_ack(delivery_tag = method.delivery_tag)

if __name__ == "__main__":
    argc = len(sys.argv)

    if argc > 1:
        platform_str = sys.argv[1]
    else:
        platform_str = base.get_os()

    if argc > 2:
        arch_str = sys.argv[2]
    else:
        arch_str = base.get_arch()
        
    server = BuildRpcServer(base.gen_routing_key(platform_str, arch_str))
    server.start()