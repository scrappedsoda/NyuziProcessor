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
import random
import subprocess
import sys
import time

sys.path.insert(0, '..')
import test_harness


VERILATOR_MEM_DUMP = os.path.join(test_harness.WORK_DIR, 'vmem.bin')
EMULATOR_MEM_DUMP = os.path.join(test_harness.WORK_DIR, 'mmem.bin')


def run_cosimulation_test(source_file, *unused):
    if test_harness.test_args.randseed:
        random_seed = int(test_harness.test_args.randseed[0])
    else:
        random_seed = random.randint(0, 0xffffffff)

    if test_harness.DEBUG:
        print('random seed is {}'.format(random_seed))

    verilator_args = [
        test_harness.VSIM_PATH,
        '+trace',
        '+memdumpfile=' + VERILATOR_MEM_DUMP,
        '+memdumpbase=800000',
        '+memdumplen=400000',
        '+autoflushl2',
        '+verilator+rand+reset+2',
        '+verilator+seed+{}'.format(random_seed)
    ]

    emulator_args = [
        test_harness.EMULATOR_PATH,
        '-m',
        'cosim',
        '-d',
        EMULATOR_MEM_DUMP + ',0x800000,0x400000'
    ]

    if test_harness.DEBUG:
        emulator_args += ['-v']

    hexfile = test_harness.build_program([source_file])
    try:
        p1 = subprocess.Popen(
            verilator_args + ['+bin=' + hexfile], stdout=subprocess.PIPE)
        p2 = subprocess.Popen(
            emulator_args + [hexfile], stdin=p1.stdout, stdout=subprocess.PIPE)
        output = ''
        while True:
            got = p2.stdout.read(0x1000)
            if not got:
                break

            if test_harness.DEBUG:
                print(got.decode())
            else:
                output += got.decode()

        p2.wait()
        time.sleep(1)  # Give verilator a chance to clean up
    finally:
        p1.kill()
        p2.kill()

    if p2.returncode:
        raise test_harness.TestException(
            'FAIL: cosimulation mismatch\n' + output)

    test_harness.assert_files_equal(VERILATOR_MEM_DUMP, EMULATOR_MEM_DUMP,
                                    'final memory contents to not match')

test_harness.register_tests(run_cosimulation_test,
                            test_harness.find_files(('.s', '.S')), ['verilator'])

test_harness.execute_tests()
