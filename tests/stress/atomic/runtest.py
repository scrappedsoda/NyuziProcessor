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

"""Test load_sync/store_sync instructions.

This uses four threads to update variables round-robin.
"""

import os
import struct
import sys

sys.path.insert(0, '../..')
import test_harness

MEM_DUMP_FILE = os.path.join(test_harness.WORK_DIR, 'vmem.bin')

@test_harness.test(['verilator'])
def atomic(_, target):
    hex_file = test_harness.build_program(['atomic.S'])
    test_harness.run_program(
        hex_file,
        target,
        dump_file=MEM_DUMP_FILE,
        dump_base=0x100000,
        dump_length=0x800,
        flush_l2=True)

    with open(MEM_DUMP_FILE, 'rb') as memfile:
        for _ in range(512):
            val = memfile.read(4)
            if len(val) < 4:
                raise test_harness.TestException('output file is truncated')

            num_val, = struct.unpack('<L', val)
            if num_val != 10:
                raise test_harness.TestException(
                    'FAIL: mismatch: ' + str(num_val))

test_harness.execute_tests()
