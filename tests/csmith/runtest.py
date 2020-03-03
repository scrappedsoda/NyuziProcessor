#!/usr/bin/env python3
#
# Copyright 2011-2015 Jeff Bush
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""Use the Csmith random program generate to validate the compiler.

This first compiles and executes the program using the host system. The
program outputs a checksum of its data structures. It then compiles and
executes it under the emulator. It compares the output to that produced
by the host and flags an error if they don't match.
"""

import os
import re
import subprocess
import sys

sys.path.insert(0, '..')
import test_harness

VERSION_RE = re.compile(r'csmith (?P<version>[0-9\.]+)')
CHECKSUM_RE = re.compile(r'checksum = (?P<checksum>[0-9A-Fa-f]+)')
HOST_EXE = os.path.join(test_harness.WORK_DIR, 'a.out')


@test_harness.test(['emulator'])
def run_csmith_test(_, target):
    # Find version of csmith
    result = subprocess.check_output(['csmith', '-v']).decode()
    got = VERSION_RE.search(result)
    if got is None:
        raise test_harness.TestException(
            'Could not determine csmith version ' + result)

    version_str = got.group('version')
    csmith_include = '-I/usr/local/include/csmith-' + version_str

    for x in range(100):
        source_file = os.path.join(test_harness.WORK_DIR, 'test{}.c'.format(x))
        print('running ' + source_file)

        # Disable packed structs because we don't support unaligned accesses.
        # Disable longlong to avoid incompatibilities between 32-bit Nyuzi
        # and 64-bit hosts.
        subprocess.check_call(['csmith', '-o', source_file, '--no-longlong',
                               '--no-packed-struct'])

        # Compile and run on host
        subprocess.check_call(
            ['cc', '-w', source_file, '-o', HOST_EXE, csmith_include])
        result = subprocess.check_output(HOST_EXE).decode()

        got = CHECKSUM_RE.search(result)
        if got is None:
            raise test_harness.TestException('no checksum in host output')

        host_checksum = int(got.group('checksum'), 16)
        print('host checksum {}'.format(host_checksum))

        # Compile and run under emulator
        hex_file = test_harness.build_program([source_file], cflags=[csmith_include])
        result = test_harness.run_program(hex_file, target)
        got = CHECKSUM_RE.search(result)
        if got is None:
            raise test_harness.TestException('no checksum in host output')

        emulator_checksum = int(got.group('checksum'), 16)
        print('emulator checksum {}'.format(emulator_checksum))
        if host_checksum != emulator_checksum:
            raise test_harness.TestException('checksum mismatch')

        print('PASS')

test_harness.execute_tests()
