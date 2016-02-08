#!/usr/bin/env python

import platform
import re

NO_ERROR = 0
ERROR = -1

class Error:
    def __init__(self, error = NO_ERROR, description = 'No error'):
        self.error = error
        self.descr = description

    def isError(self):
        return self.error == ERROR

    def description(self):
        return self.descr

class Platform(object):
    def __init__(self, platform, archs, package_types):
        self.platform = platform
        self.archs = archs
        self.package_types = package_types

SUPPORTED_PLATFORMS = [Platform('linux', [32, 64], ['DEB', 'RPM', 'TGZ']),
                       Platform('windows', [32, 64], ['NSIS', 'ZIP']),
                       Platform('macosx', [64], ['DragNDrop', 'ZIP']),
                       Platform('freebsd', [64], ['TGZ']) ]

def get_os():
    uname_str = platform.system()
    if uname_str == 'MINGW64_NT-6.1':
        return 'windows'
    elif uname_str == 'Linux':
        return 'linux'
    elif uname_str == 'Darwin':
        return 'macosx'
    elif uname_str == 'FreeBSD':
        return 'freebsd'
    else:
        return None

def get_arch():
    arch = platform.architecture()
    return re.search(r'\d+', arch[0]).group()

def get_supported_platform_by_name(platform):
    return next((x for x in SUPPORTED_PLATFORMS if x.platform == platform), None)