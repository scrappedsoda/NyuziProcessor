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

import os
import subprocess
import sys

sys.path.insert(0, '..')
import test_harness

FS_IMAGE_PATH = os.path.join(test_harness.WORK_DIR, 'fsimage.bin')

@test_harness.test(['emulator', 'fpga'])
def filesystem(_, target):
    """Test the filesystem implementation in libos.

    This creates a filesystem image with the test file fstest.txt
    in it, the compiles the program fs.c to perform operations on it. The program
    will print 'PASS' if it is successful.
    """

    hex_file = test_harness.build_program(['fs.c'])
    subprocess.check_output(
        [os.path.join(test_harness.TOOL_BIN_DIR, 'mkfs'), FS_IMAGE_PATH,
         'fstest.txt'], stderr=subprocess.STDOUT)
    result = test_harness.run_program(hex_file, target, block_device=FS_IMAGE_PATH)
    if 'PASS' not in result or 'FAIL' in result:
        raise test_harness.TestException(
            'test program did not indicate pass\n' + result)


def run_test(source_file, target):
    hex_file = test_harness.build_program([source_file])
    result = test_harness.run_program(hex_file, target)
    test_harness.check_result(source_file, result)

# hack: register all source files in this directory except for fs test,
# which has special handling.
test_list = [fname for fname in test_harness.find_files(
    ('.c', '.cpp')) if not fname.startswith('_')]
test_list.remove('fs.c')
test_harness.register_tests(run_test, test_list, ['emulator', 'fpga'])
test_harness.execute_tests()
