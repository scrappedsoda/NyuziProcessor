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

"""Test LLDB symbolic debugger.

LLDB is built as part of the LLVM toolchain: https://lldb.llvm.org/
"""

import os
import re
import subprocess
import sys

sys.path.insert(0, '../..')
import test_harness


class EmulatorProcess(object):
    """Spawns the emulator process and LLDB in MI (machine interface) mode.

    This allows communication with LLDB with it via stdin and stdout. It has the
    __enter__ and __exit__ methods allowing it to be used in the 'with'
    construct so it will automatically be torn down when the test is done.
    """

    def __init__(self, hexfile):
        self.hexfile = hexfile

        self.elf_file = test_harness.get_elf_file_for_hex(hexfile)
        self.output = None
        self.emulator_proc = None
        self.lldb_proc = None
        self.outstr = None
        self.instr = None

    def __enter__(self):
        emulator_args = [
            test_harness.EMULATOR_PATH,
            '-m',
            'gdb',
            '-v',
            self.hexfile
        ]

        if test_harness.DEBUG:
            self.output = None
        else:
            self.output = open(os.devnull, 'w')

        self.emulator_proc = subprocess.Popen(emulator_args, stdout=self.output,
                                              stderr=subprocess.STDOUT)

        lldb_args = [
            os.path.join(test_harness.COMPILER_BIN_DIR, 'lldb-mi')
        ]

        # XXX race condition: the emulator needs to be ready before
        # lldb tries to connect to it.

        try:
            self.lldb_proc = subprocess.Popen(lldb_args, stdout=subprocess.PIPE,
                                              stdin=subprocess.PIPE)
            self.outstr = self.lldb_proc.stdin
            self.instr = self.lldb_proc.stdout
        except:
            self.emulator_proc.kill()
            raise

        return self

    def __exit__(self, *unused):
        self.emulator_proc.kill()
        self.lldb_proc.kill()

    def send_command(self, cmd):
        if test_harness.DEBUG:
            print('LLDB send: ' + cmd)

        self.outstr.write(str.encode(cmd + '\n'))
        self.outstr.flush()
        return self.wait_response()

    def wait_response(self):
        response = ''
        while True:
            response += self.instr.read(1).decode('utf-8')
            if response.endswith('^done'):
                break

        if test_harness.DEBUG:
            print('LLDB recv: ' + response)

        return response

    def wait_stop(self):
        current_line = ''
        while True:
            inchar = self.instr.read(1).decode('utf-8')
            current_line += inchar
            if inchar == '\n':
                if test_harness.DEBUG:
                    print('LLDB recv: ' + current_line[:-1])

                if current_line.startswith('*stopped'):
                    break

                current_line = ''

FRAME_RE = re.compile(
    'frame #[0-9]+:( 0x[0-9a-f]+)? [a-zA-Z_\\.0-9]+`(?P<function>[a-zA-Z_0-9][a-zA-Z_0-9]+)')
AT_RE = re.compile(' at (?P<filename>[a-z_A-Z][a-z\\._A-Z]+):(?P<line>[0-9]+)')


def parse_stack_crawl(response):
    """Convert blob of stack crawl text to a list of tuples.

    Args:
        response: str
            Text from debugger containing the stack crawl.

    Returns:
        list of (function name: str, filename: str, line number: int))
    """

    stack_info = []
    for line in response.split('\\n'):
        frame_match = FRAME_RE.search(line)
        if frame_match is not None:
            func = frame_match.group('function')
            at_match = AT_RE.search(line)
            if at_match is not None:
                stack_info += [(func, at_match.group('filename'),
                                int(at_match.group('line')))]
            else:
                stack_info += [(func, '', 0)]

    return stack_info


@test_harness.test(['emulator'])
def lldb(*unused):
    """This mainly validates that LLDB is reading symbols correctly."""

    hexfile = test_harness.build_program(
        ['test_program.c'], opt_level='-O0', cflags=['-g'])
    with EmulatorProcess(hexfile) as conn:
        conn.send_command('file "' + os.path.join(test_harness.WORK_DIR, 'program.elf"'))
        conn.send_command('gdb-remote 8000\n')
        response = conn.send_command(
            'breakpoint set --file test_program.c --line 27')
        if 'Breakpoint 1: where = program.elf`func2 + 100 at test_program.c:27' not in response:
            raise test_harness.TestException(
                'breakpoint: did not find expected value ' + response)

        conn.send_command('c')
        conn.wait_stop()

        expected_stack = [
            ('func2', 'test_program.c', 27),
            ('func1', 'test_program.c', 35),
            ('main', 'test_program.c', 41),
            ('do_main', '', 0)
        ]

        response = conn.send_command('bt')
        crawl = parse_stack_crawl(response)
        if crawl != expected_stack:
            raise test_harness.TestException(
                'stack crawl mismatch ' + str(crawl))

        response = conn.send_command('print value')
        if '= 67' not in response:
            raise test_harness.TestException(
                'print value: Did not find expected value ' + response)

        response = conn.send_command('print result')
        if '= 128' not in response:
            raise test_harness.TestException(
                'print result: Did not find expected value ' + response)

        # Up to previous frame
        conn.send_command('frame select --relative=1')

        response = conn.send_command('print a')
        if '= 12' not in response:
            raise test_harness.TestException(
                'print a: Did not find expected value ' + response)

        response = conn.send_command('print b')
        if '= 67' not in response:
            raise test_harness.TestException(
                'print b: Did not find expected value ' + response)

        conn.send_command('step')
        conn.wait_stop()

        response = conn.send_command('print result')
        if '= 64' not in response:
            raise test_harness.TestException(
                'print b: Did not find expected value ' + response)


test_harness.execute_tests()
