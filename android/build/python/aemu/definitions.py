#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from enum import Enum
import os
import platform


from absl import flags

FLAGS = flags.FLAGS

# Beware! This depends on this file NOT being installed!
flags.DEFINE_string('aosp_root', os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(
    __file__)), '..', '..', '..', '..', '..', '..')), 'The aosp directory')


EXE_POSTFIX = ''
if platform.system() == 'Windows':
    EXE_POSTFIX = '.exe'


def get_aosp_root():
    return FLAGS.aosp_root


def get_qemu_root():
    return os.path.abspath(os.path.join(get_aosp_root(), 'external', 'qemu'))


def get_cmake_dir():
    '''Return the cmake directory for the current platform.'''
    return os.path.join(
        get_aosp_root(), 'prebuilts/cmake/%s-x86/bin/' % platform.system().lower())


def get_cmake():
    '''Return the path of cmake executable.'''
    return os.path.join(get_cmake_dir(), 'cmake%s' % EXE_POSTFIX)


def get_ctest():
    '''Return the path of ctest executable.'''
    return os.path.join(get_cmake_dir(), 'ctest%s' % EXE_POSTFIX)



# Functions that determine if a file is executable.
is_executable = {
    'Windows': [lambda f: f.endswith('.exe') or f.endswith('.dll')],
    'Linux': [lambda f: os.access(f, os.X_OK), lambda f: f.endswith('.so')],
    'Darwin': [lambda f: os.access(f, os.X_OK), lambda f: f.endswith('.dylib')],
}


def recursive_iglob(rootdir, pattern_fns):
    '''Recursively glob the rootdir for any file that matches the pattern functions.'''
    for root, _, filenames in os.walk(rootdir):
        for filename in filenames:
            fname = os.path.join(root, filename)
            for pattern_fn in pattern_fns:
                if pattern_fn(fname):
                    yield fname


class ArgEnum(Enum):
    '''A class that can parse argument enums'''

    def __str__(self):
        return self.name

    def to_cmd(self):
        return self.value

    @classmethod
    def values(clzz):
        return [n.name for n in clzz]

    @classmethod
    def from_string(clzz, s):
        try:
            return clzz[s.lower()]
        except KeyError:
            raise ValueError()


class Crash(ArgEnum):
    '''All supported crash backends.'''
    prod = ['-DOPTION_CRASHUPLOAD=PROD']
    staging = ['-DOPTION_CRASHUPLOAD=STAGING']
    none = ['-DOPTION_CRASHUPLOAD=NONE']


class BuildConfig(ArgEnum):
    '''Supported build configurations.'''
    debug = ['-DCMAKE_BUILD_TYPE=Debug']
    release = ['-DCMAKE_BUILD_TYPE=Release']


class Make(ArgEnum):
    config = 'configure only'
    check = 'check'


class Generator(ArgEnum):
    '''Supported generators.'''
    visualstudio = ['-G', 'Visual Studio 15 2017 Win64']
    xcode = ['-G', 'Xcode']
    ninja = ['-G', 'Ninja']
    make = ['-G', 'Unix Makefiles']


class Toolchain(ArgEnum):
    '''Supported toolchains.'''
    windows = 'toolchain-windows_msvc-x86_64.cmake'
    linux = 'toolchain-linux-x86_64.cmake'
    darwin = 'toolchain-darwin-x86_64.cmake'
    mingw = 'toolchain-windows-x86_64.cmake'

    def to_cmd(self):
        return ['-DCMAKE_TOOLCHAIN_FILE=%s' % os.path.join(get_qemu_root(), 'android', 'build', 'cmake', self.value)]


class SymbolUris(ArgEnum):
    staging = 'https://clients2.google.com/cr/staging_symbol'
    prod = 'https://clients2.google.com/cr/symbol'
    none = None
