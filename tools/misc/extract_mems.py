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

"""Create non-parameterized instances of all FIFOs and SRAMS in the design.

This is required to use ASIC memory compiler tools. It is invoked by the
Makefile in the hardware directory. It outputs a new Verilog file that
is included in the source files for memories.
"""

import re
import sys

def main():
    patterns = [
        [re.compile(r'sram1r1w\s+(?P<width>\d+)\s+(?P<depth>\d+)'),
        [], 'sram1r1w_', '_GENERATE_SRAM1R1W'],
        [re.compile(r'sram2r1w\s+(?P<width>\d+)\s+(?P<depth>\d+)'),
        [], 'sram2r1w_', '_GENERATE_SRAM2R1W'],
        [re.compile(r'sync_fifo\s+(?P<width>\d+)\s+(?P<depth>\d+)'),
        [], 'fifo_', '_GENERATE_FIFO']
    ]

    for line in sys.stdin.readlines():
        for regexp, itemlist, name, macro in patterns:
            match = regexp.search(line)
            if match is not None:
                pair = (match.group('width'), match.group('depth'))
                if pair not in itemlist:
                    itemlist.append(pair)

    for regexp, itemlist, prefix, macro in patterns:
        print('`ifdef ' + macro)
        first = True
        for width, depth in itemlist:
            if first:
                first = False
            else:
                print('else ', end='')

            print('if (WIDTH == {} && SIZE == {})'.format(width, depth))
            instancename = '{}{}x{}'.format(prefix, width, depth)
            print('    {0} {0}(.*);'.format(instancename))

        print('\n`endif')

if __name__ == '__main__':
    main()
