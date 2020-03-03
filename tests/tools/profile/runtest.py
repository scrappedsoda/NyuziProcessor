#!/usr/bin/env python3
#
# Copyright 2017 Jeff Bush
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

"""Test profiling capabilities of hardware simulator."""

import os
import subprocess
import sys

sys.path.insert(0, '../..')
import test_harness

@test_harness.test(['verilator'])
def profile(*unused):
    hexfile = test_harness.build_program(['test_program.c'])

    elffile = test_harness.get_elf_file_for_hex(hexfile)
    profile_file = os.path.join(test_harness.WORK_DIR, 'profile.out')

    test_harness.run_program(hexfile, 'verilator', profile_file=profile_file)

    symbol_file = os.path.join(test_harness.WORK_DIR, 'symbols.txt')
    objdump_args = [
        os.path.join(test_harness.COMPILER_BIN_DIR, 'llvm-objdump'),
        '-t', elffile
    ]
    symbols = subprocess.check_output(objdump_args)
    with open(symbol_file, 'w') as f:
        f.write(symbols.decode())

    profile_args = [
        os.path.join(test_harness.TOOL_BIN_DIR, 'profile.py'),
        symbol_file,
        profile_file
    ]
    profile_output = subprocess.check_output(' '.join(profile_args), shell=True)
    profile_lines = profile_output.decode().split('\n')
    profile_tuples = [line.split() for line in profile_lines if line]
    profile_map = {func: int(count) for count, _, func in profile_tuples}

    # These tests don't end up being exactly 2x the number of samples. Because
    # the system samples randomly, it can vary. I could have ran the test longer
    # to get more samples, but this is really just a smoke test and I didn't want
    # to bloat the test time unnecessarily.
    loop5k = profile_map['loop5000']
    loop10k = profile_map['loop10000']
    loop20k = profile_map['loop20000']
    test_harness.assert_greater(loop5k, 0)
    test_harness.assert_greater(loop10k, loop5k * 1.75)
    test_harness.assert_greater(loop20k, loop10k * 1.75)

test_harness.execute_tests()
