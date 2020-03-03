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

"""Utility functions for functional tests.

This is imported into test runner scripts in subdirectories under this one.
"""

import argparse
import binascii
import configparser
import hashlib
import os
import random
import re
import shutil
import subprocess
import sys
import termios
import traceback
from PIL import Image

TEST_DIR = os.path.dirname(os.path.abspath(__file__))

# Read configuration file. This is written by the build system.
# It contains paths to dependencies of the tests, and allows
# them to operate no matter where the build directory is.
config = configparser.ConfigParser()
config.read(os.path.join(TEST_DIR, 'tests.conf'))
default_config = config['DEFAULT']
TOOL_BIN_DIR = default_config['TOOL_BIN_DIR']
LIB_DIR = default_config['LIB_DIR']
KERNEL_DIR = default_config['KERNEL_DIR']
WORK_DIR = default_config['WORK_DIR']
LIB_INCLUDE_DIR = default_config['LIB_INCLUDE_DIR']
COMPILER_BIN_DIR = default_config['COMPILER_BIN_DIR']
HARDWARE_INCLUDE_DIR = default_config['HARDWARE_INCLUDE_DIR']

VSIM_PATH = os.path.join(TOOL_BIN_DIR, 'nyuzi_vsim')
EMULATOR_PATH = os.path.join(TOOL_BIN_DIR, 'nyuzi_emulator')
SERIAL_BOOT_PATH = os.path.join(TOOL_BIN_DIR, 'serial_boot')
ALL_TARGETS = ['verilator', 'emulator']
DEFAULT_TARGETS = ['verilator', 'emulator']

class TestException(Exception):
    """This exception is raised for test failures"""
    pass

parser = argparse.ArgumentParser()
parser.add_argument('--target', dest='target',
                    help='restrict to only executing tests on this target',
                    nargs=1)
parser.add_argument('--debug', action='store_true',
                    help='enable verbose output to debug test failures')
parser.add_argument('--list', action='store_true',
                    help='list availble tests')
parser.add_argument('--randseed', dest='randseed',
                    help='set verilator random seed', nargs=1)
parser.add_argument('names', nargs=argparse.REMAINDER,
                    help='names of specific tests to run')
test_args = parser.parse_args()
DEBUG = test_args.debug


def build_program(source_files, image_type='bare-metal', opt_level='-O3', cflags=None):
    """Compile and or assemble one or more files.

    If there are any .c files in the list, this will link in crt0, libc,
    and libos libraries. It converts the binary to a hex file that can be
    loaded into memory.

    Args:
        source_files: list of str
            Paths to files to compile, which can be C/C++ or assembly files.
        image_type: str
            Can be:
            - 'bare-metal', Runs standalone, but with elf linkage
            - 'raw', Has no header and is linked at address 0
            - 'user', ELF binary linked at 0x1000, linked against kernel libs
        opt_level: str
            Optimization level (-O0, -O1, -O2, or -O3)
        cflags: list of str
            Additional command line flags to pass to C compiler.

    Returns:
        str Name of output file

    Raises:
        TestException if compilation failed, will contain compiler output
    """
    assert isinstance(source_files, list)
    assert image_type in ['bare-metal', 'raw', 'user']

    elf_file = os.path.join(WORK_DIR, 'program.elf')
    compiler_args = [
        os.path.join(COMPILER_BIN_DIR, 'clang'),
        '-o', elf_file,
        '-w',
        opt_level,
        '-I', TEST_DIR  # To include asm_macros.h
    ]

    if cflags is not None:
        compiler_args += cflags

    if image_type == 'raw':
        compiler_args += ['-Wl,--script,{},--oformat,binary'.format(os.path.join(TEST_DIR, 'one-segment.ld'))]
    elif image_type == 'user':
        compiler_args += ['-Wl,--image-base=0x1000']

    compiler_args += source_files

    if any(name.endswith(('.c', '.cpp')) for name in source_files):
        compiler_args += [
            '-I' + os.path.join(LIB_INCLUDE_DIR, 'libc/include'),
            '-I' + os.path.join(LIB_INCLUDE_DIR, 'libos'),
            os.path.join(LIB_DIR, 'libc/libc.a')
        ]
        if image_type == 'user':
            compiler_args += [os.path.join(LIB_DIR, 'libos/kernel/libos-kern.a')]
        else:
            compiler_args += [os.path.join(LIB_DIR, 'libos/bare-metal/libos-bare.a')]

    try:
        subprocess.check_output(compiler_args, stderr=subprocess.STDOUT)
        hex_file = os.path.join(WORK_DIR, 'program.hex')
        if image_type == 'raw':
            dump_hex(input_file=elf_file, output_file=hex_file)
            return hex_file

        if image_type == 'bare-metal':
            subprocess.check_output([os.path.join(COMPILER_BIN_DIR, 'elf2hex'),
                                    '-o', hex_file, elf_file],
                                    stderr=subprocess.STDOUT)
            return hex_file

        return elf_file
    except subprocess.CalledProcessError as exc:
        raise TestException('Compilation failed:\n' + exc.output.decode())


def get_elf_file_for_hex(hexfile):
    """Get the path to a .elf file that was used to generate the .hex file.

    This is a bit of a hack. build_program only returns the hex file, but
    creates the ELF file in the same directory.
    """
    return os.path.splitext(hexfile)[0] + '.elf'


class TerminalStateRestorer(object):
    """This saves and restores the configuration of POSIX terminals.

    The emulator process disables local echo.  If it crashes or
    times out, it can leave the terminal in a bad state. This will
    save and restore the state automatically."""
    def __init__(self):
        self.attrs = None

    def __enter__(self):
        try:
            self.attrs = termios.tcgetattr(sys.stdin.fileno())
        except termios.error:
            # This may throw an exception if the process doesn't
            # have a controlling TTY (continuous integration). In
            # this case, self.attrs will remain None.
            pass

    def __exit__(self, *unused):
        if self.attrs is not None:
            termios.tcsetattr(sys.stdin.fileno(), termios.TCSANOW, self.attrs)


def run_test_with_timeout(args, timeout):
    """Run program specified by args with timeout.

    If it does not complete in time, throw a TestException. If it
    returns a non-zero result, it will also throw a TestException.

    Args:
        args: list of str
            Arguments to called program the first being the path to the
            executable
        timeout: float
            Number of seconds to wait for the program to exit normally
            before throwing exception.

    Returns:
        str All data printed to stdout by the process.

    Raises:
        TestException if the test fails or times out.
    """
    process = subprocess.Popen(args, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        raise TestException('Test timed out')

    if process.poll():
        # Non-zero return code. Probably target program crash.
        raise TestException(
            'Process returned error: ' + output.decode())

    return output.decode()


def reset_fpga():
    args = ['quartus_stp', '-t', os.path.join(TEST_DIR, 'reset_altera.tcl')]

    try:
        subprocess.check_output(args)
    except subprocess.CalledProcessError as exc:
        raise TestException(
            'Failed to reset dev board:\n' + exc.output.decode())


def run_program(
        executable,
        target='emulator',
        *,
        block_device=None,
        dump_file=None,
        dump_base=None,
        dump_length=None,
        timeout=60,
        flush_l2=False,
        trace=False,
        profile_file=None):
    """Run test program.

    This uses the hex file produced by build_program.

    Args:
        executable: str
            Path in host filesystem to file that should be executed.
            This is usually the return value from build_program.
        target: str
            Which target will run the program. Can be 'verilator'
            or 'emulator'.
        block_device: str
            Relative path to a file that contains a filesystem image.
            If passed, contents will appear as a virtual SDMMC device.
        dump_file: str
            Path to a file to write memory contents into after
            execution completes.
        dump_base: int
            if dump_file is specified, base physical memory address to start
            writing mempry from.
        dump_length: int
            number of bytes of memory to write to dump_file

    Returns:
        str Output from program, anything written to virtual serial device,
        as a str.

    Raises:
        TestException if emulated program crashes or the program cannot
        execute for some other reason.
    """
    if target == 'emulator':
        args = [EMULATOR_PATH]
        args += ['-a']  # Enable thread scheduling randomization by default
        if block_device is not None:
            args += ['-b', block_device]

        if dump_file is not None:
            args += ['-d', '{},0x{:x},0x{:x}'.format(dump_file, dump_base, dump_length)]

        args += [executable]
        if DEBUG:
            print('running emulator with args ' + str(args))

        output = run_test_with_timeout(args, timeout)
    elif target == 'verilator':
        random_seed = random.randint(0, 0xffffffff)
        if DEBUG:
            print('random seed is {}'.format(random_seed))

        args = [
            VSIM_PATH,
            '+bin=' + executable,
            '+verilator+rand+reset+2',
            '+verilator+seed+' + str(random_seed)
        ]
        if block_device is not None:
            args += ['+block=' + block_device]

        if dump_file is not None:
            args += [
                '+memdumpfile=' + dump_file,
                '+memdumpbase={:x}'.format(dump_base),
                '+memdumplen={:x}'.format(dump_length)
            ]

        if flush_l2:
            args += ['+autoflushl2=1']

        if trace:
            args += ['+trace']

        if profile_file is not None:
            args += ['+profile=' + profile_file]

        if DEBUG:
            print('running verilator with args ' + str(args))

        output = run_test_with_timeout(args, timeout)
        if '***HALTED***' not in output:
            raise TestException(output + '\nProgram did not halt normally')
    elif target == 'fpga':
        if block_device is not None:
            args += [block_device]

        if dump_file is not None:
            raise TestException('dump file is not supported on FPGA')

        if flush_l2:
            raise TestException('flush_l2 is not supported on FPGA')

        if 'SERIAL_PORT' not in os.environ:
            raise TestException(
                'Need to set SERIAL_PORT to device path in environment')

        args = [
            SERIAL_BOOT_PATH,
            os.environ['SERIAL_PORT'],
            executable
        ]

        reset_fpga()

        output = run_test_with_timeout(args, timeout)
    else:
        raise TestException('Unknown execution target')

    if DEBUG:
        print('Program Output:\n' + output)

    return output


def run_kernel(
        exe_file,
        target='emulator',
        *,
        timeout=60):
    """Run test program as a user space program under the kernel.

    This uses the elf file produced by build_program. The kernel reads
    the file 'program.elf' from the filesystem. This will build a filesystem
    with that image automatically.

    Args:
        target: str
            Which target to execute on. Can be 'verilator' or 'emulator'.

    Returns:
        str Output from program, anything written to virtual serial device

    Raises:
        TestException if emulated program crashes or the program cannot
        execute for some other reason.
    """
    block_file = os.path.join(WORK_DIR, 'fsimage.bin')
    subprocess.check_output([os.path.join(TOOL_BIN_DIR, 'mkfs'), block_file, exe_file],
                            stderr=subprocess.STDOUT)

    output = run_program(os.path.join(KERNEL_DIR, 'kernel.hex'),
                         target, block_device=block_file,
                         timeout=timeout, )

    if DEBUG:
        print('Program Output:\n' + output)

    return output


def assert_greater(a, b):
    if a <= b:
        raise TestException('assert_greater failed: {}, {}'.format(a, b))


def assert_greater_equal(a, b):
    if a < b:
        raise TestException('assert_greater_equal failed: {}, {}'.format(a, b))


def assert_less(a, b):
    if a >= b:
        raise TestException('assert_less failed: {}, {}'.format(a, b))


def assert_less_equal(a, b):
    if a > b:
        raise TestException('assert_less_equal failed: {}, {}'.format(a, b))


def assert_equal(a, b):
    if a != b:
        raise TestException('assert_equal failed: {}, {}'.format(a, b))


def assert_not_equal(a, b):
    if a == b:
        raise TestException('assert_not_equal failed: {}, {}'.format(a, b))


def assert_files_equal(file1, file2, error_msg='file mismatch'):
    """Read two files and throw a TestException if they are not the same

    Args:
        file1: str
            relative path to first file
        file2: str
            relative path to second file
        error_msg: str
            If there is a file mismatch, prepend this to error output

    Returns:
        Nothing

    Raises:
        TestException if the files don't match. Exception test contains
        details about where the mismatch occurred.
    """

    bufsize = 0x1000
    block_offset = 0
    with open(file1, 'rb') as fp1, open(file2, 'rb') as fp2:
        while True:
            block1 = bytearray(fp1.read(bufsize))
            block2 = bytearray(fp2.read(bufsize))
            if len(block1) < len(block2):
                raise TestException(error_msg + ': file1 shorter than file2')
            elif len(block1) > len(block2):
                raise TestException(error_msg + ': file1 longer than file2')

            if block1 != block2:
                for offset, (val1, val2) in enumerate(zip(block1, block2)):
                    if val1 != val2:
                        # Show the difference
                        exception_text = error_msg + ':\n'
                        rounded_offset = offset & ~15
                        exception_text += '{:08x} '.format(block_offset +
                                                           rounded_offset)
                        for lineoffs in range(16):
                            exception_text += '{:02x}'.format(
                                block1[rounded_offset + lineoffs])

                        exception_text += '\n{:08x} '.format(
                            block_offset + rounded_offset)
                        for lineoffs in range(16):
                            exception_text += '{:02x}'.format(
                                block2[rounded_offset + lineoffs])

                        exception_text += '\n         '
                        for lineoffs in range(16):
                            if block1[rounded_offset + lineoffs] \
                                    != block2[rounded_offset + lineoffs]:
                                exception_text += '^^'
                            else:
                                exception_text += '  '

                        raise TestException(exception_text)

            if not block1:
                return

            block_offset += len(block1)


registered_tests = []


def register_tests(func, names, targets=None):
    """Add a list of tests to be run when execute_tests is called.

    This function can be called multiple times, it will append passed
    tests to the existing list.

    Args:
        func: function
            A function that will be called for each of the elements
            in the names list.
        names: list of str
            List of tests to run.

    Returns:
        Nothing

    Raises:
        Nothing
     """

    global registered_tests
    if not targets:
        targets = ALL_TARGETS[:]

    registered_tests += [(func, name, targets) for name in names]


def test(param=None):
    """decorator @test automatically registers test to be run.

    Args:
        param: list of str
            optional list of targets that are valid for this test
    """
    if callable(param):
        # If the test decorator is used without a target list,
        # this will just pass the function as the parameter.
        # Run all targtes
        register_tests(param, [param.__name__], ALL_TARGETS)
        return param

    # decorator is called with a list of targets. Return
    # a fuction that will be called on the actual function.
    def register_func(func):
        register_tests(func, [func.__name__], param)
        return func

    return register_func


def disable(func):
    """Decorator that disables a test.

    Useful to temporarily disable failing tests.

    Args:
        func: function
            Callback of function to disable
    """
    assert registered_tests[-1][0] == func
    del registered_tests[-1] # This should always called right after @test


def find_files(extensions):
    """Find all files in the current directory that have the passed extensions.

    Args:
        extensions: list of str
            File extensions, each starting with a dot. For example
            ['.c', '.cpp']

    Returns:
        list of str Filenames.

    Raises:
        Nothing
    """

    return [fname for fname in os.listdir('.') if fname.endswith(extensions)]

COLOR_RED = '[\x1b[31m'
COLOR_GREEN = '[\x1b[32m'
COLOR_NONE = '\x1b[0m]'
OUTPUT_ALIGN = 50


def execute_tests():
    """All tests are called from here.

    Run all tests that have been registered with the register_tests functions
    and print results. If this fails, it will call sys.exit with a non-zero
    status.

    Args:
        None

    Returns:
        Nothing

    Raises:
        NOthing
    """

    global DEBUG

    if test_args.list:
        for _, param, targets in registered_tests:
            print(param + ': ' + ', '.join(targets))

        return

    if test_args.target:
        targets_to_run = test_args.target
    else:
        targets_to_run = DEFAULT_TARGETS

    # Filter based on names and targets
    if test_args.names:
        tests_to_run = []
        for requested in test_args.names:
            for func, param, targets in registered_tests:
                if param == requested:
                    tests_to_run += [(func, param, targets)]
                    break
            else:
                print('Unknown test ' + requested)
                sys.exit(1)
    else:
        tests_to_run = registered_tests

    test_run_count = 0
    test_pass_count = 0
    failing_tests = []
    for func, param, targets in tests_to_run:
        for target in targets:
            if target not in targets_to_run:
                continue

            label = '{}({})'.format(param, target)
            print(label + (' ' * (OUTPUT_ALIGN - len(label))), end='')
            try:
                # Clean out working directory and re-create
                shutil.rmtree(path=WORK_DIR, ignore_errors=True)
                os.makedirs(WORK_DIR)

                test_run_count += 1
                sys.stdout.flush()
                with TerminalStateRestorer():
                    func(param, target)

                print(COLOR_GREEN + 'PASS' + COLOR_NONE)
                test_pass_count += 1
            except KeyboardInterrupt:
                sys.exit(1)
            except TestException as exc:
                print(COLOR_RED + 'FAIL' + COLOR_NONE)
                failing_tests += [(param, target, exc.args[0])]
            except Exception as exc:  # pylint: disable=W0703
                print(COLOR_RED + 'FAIL' + COLOR_NONE)
                failing_tests += [(param, target, 'Test threw exception:\n' +
                                   traceback.format_exc())]

    if failing_tests:
        print('Failing tests:')
        for name, target, output in failing_tests:
            print('{} ({})'.format(name, target))
            print(output)

    print('{}/{} tests failed'.format(test_run_count - test_pass_count,
                                      test_run_count))
    if failing_tests:
        sys.exit(1)

CHECK_PREFIX = 'CHECK: '
CHECKN_PREFIX = 'CHECKN: '


def check_result(source_file, program_output):
    """Check output of a program based on embedded comments in source code.

    For each pattern in a source file that begins with 'CHECK: ', search
    to see if the regular expression that follows it occurs in program_output.
    The strings must occur in order, but this ignores anything between them.
    If there is a pattern 'CHECKN: ', the test will fail if the string *does*
    occur in the output.

    Args:
        source_file: str
            relative path to a source file that contains patterns

    Returns:
        Nothing

    Raises:
        TestException if a string is not found.
    """

    output_offset = 0
    line_num = 1
    found_check_lines = False
    with open(source_file, 'r') as infile:
        for line in infile:
            chkoffs = line.find(CHECK_PREFIX)
            if chkoffs != -1:
                found_check_lines = True
                expected = line[chkoffs + len(CHECK_PREFIX):].strip()
                if DEBUG:
                    print('searching for pattern "{}", line {}'.format(expected,
                          line_num))

                regexp = re.compile(expected)
                got = regexp.search(program_output, output_offset)
                if got is not None:
                    output_offset = got.end()
                else:
                    error = 'FAIL: line {} expected string {} was not found\n'.format(
                        line_num, expected)
                    error += 'searching here:' + program_output[output_offset:]
                    raise TestException(error)
            else:
                chkoffs = line.find(CHECKN_PREFIX)
                if chkoffs != -1:
                    found_check_lines = True
                    nexpected = line[chkoffs + len(CHECKN_PREFIX):].strip()
                    if DEBUG:
                        print('ensuring absence of pattern "' + nexpected +
                            '", line ' + str(line_num))

                    regexp = re.compile(nexpected)
                    got = regexp.search(program_output, output_offset)
                    if got is not None:
                        error = 'FAIL: line {} string {} should not be here:\n'.format(
                            line_num, nexpected)
                        error += program_output
                        raise TestException(error)

            line_num += 1

    if not found_check_lines:
        raise TestException('FAIL: no lines with CHECK: were found')

    return True


def dump_hex(output_file, input_file):
    """Read binary input file and encode as hexadecimal file.

    Each line of the output file is 4 bytes.

    Args:
        output_file: str
            Path of hex file to to write
        input_file: str
            Path of binary file to read

    Returns:
        Nothing

    Raises:
        IOError if there is a problem reading or writing files.
    """

    with open(input_file, 'rb') as ifile, open(output_file, 'wb') as ofile:
        while True:
            word = ifile.read(4)
            if not word:
                break

            ofile.write(binascii.hexlify(word))
            ofile.write(b'\n')


def endian_swap(value):
    """"Given a 32-bit integer value, swap it to the opposite endianness.

    Args:
        value: int
            Value to endian swap

    Returns:
        int Endian swapped result
    """

    return (((value >> 24) & 0xff) | ((value >> 8) & 0xff00)
            | ((value << 8) & 0xff0000) | (value << 24))


def _run_generic_test(name, target):
    """Compile a file, run the program, and call check_result on it.

    check_result will match expected strings in the source
    file with the programs output.

    Args:
        name: str
            Filename of the file to run, expected to be in the same
            directory is the runtest script
        target: str
            Name of the target (e.g. emulator, verilator)

    Returns:
        Nothing

    Raises:
        TestException if the test fails.
    """

    hex_file = build_program([name])
    result = run_program(hex_file, target)
    check_result(name, result)


def register_generic_test(name, targets=None):
    """Register a test without a custom test handler function.

    This will compile the passed filename, then use check_result to validate
    it against comment strings embedded in the file. It runs it both in
    verilator and emulator configurations.

    Args:
        names: list of str
            Source file names. Each is compiled as a separate test.

    Returns:
        Nothing

    Raises:
        Nothing
    """
    if targets is None:
        targets = ALL_TARGETS[:]

    register_tests(_run_generic_test, name, targets)


def _run_generic_assembly_test(name, target):
    hex_file = build_program([name])
    result = run_program(hex_file, target)
    if 'PASS' not in result or 'FAIL' in result:
        raise TestException('Test failed ' + result)


def register_generic_assembly_tests(tests, targets=None):
    """Register a test written in assembler without a custom test handler.

    This will assemble the passed program, then look for PASS or FAIL
    strings. It runs it both in verilator and emulator configurations.

    Args:
        tests: list of str
            Source file names. Each is assembled as a separate test.

    Returns:
        Nothing

    Raises:
        Nothing
    """

    if targets is None:
        targets = ALL_TARGETS[:]

    register_tests(_run_generic_assembly_test, tests, targets)


def register_render_test(name, source_files, expected_hash, targets=None):
    """Register a test that renders graphics.

    This will compile the source files, run the program, then compute a
    hash of memory starting at 2M, which is expected to be a framebuffer
    with the format 640x480x32bpp. This hash will be compared to a reference
    value to ensure the output is pixel accurate.

    Args:
        name: str
            Display name of the test in test result output
        source_files: list of str
            List of source files to compile (unlike other calls, these
            are all compiled into one executable, this call only registers
            one test, not multiple).
        expected_hash: str
            this is an ASCII hex string that the computed hash should match.

    Returns:
        Nothing

    Raises:
        TestException if the computed hash doesn't match or there is some other
        test failure.
    """

    # This closure captures parameters source_files and
    # expected_checksum.
    def run_render_test(_, target):
        RAW_FB_DUMP_FILE = os.path.join(WORK_DIR, 'fb.bin')
        PNG_DUMP_FILE = os.path.join(WORK_DIR, 'actual-output.png')

        render_cflags = [
            '-I' + os.path.join(LIB_INCLUDE_DIR, 'librender'),
            os.path.join(LIB_DIR, 'librender/librender.a'),
            '-ffast-math'
        ]

        hex_file = build_program(source_files=source_files,
                      cflags=render_cflags)
        run_program(hex_file,
                    target,
                    dump_file=RAW_FB_DUMP_FILE,
                    dump_base=0x200000,
                    dump_length=0x12c000,
                    flush_l2=True)
        with open(RAW_FB_DUMP_FILE, 'rb') as f:
            contents = f.read()

        sha = hashlib.sha1()
        sha.update(contents)
        actual_hash = sha.hexdigest()
        if actual_hash != expected_hash:
            with open(RAW_FB_DUMP_FILE, 'rb') as fp:
                contents = fp.read()
                image = Image.frombytes('RGBA', (640, 480), contents)
                image.save(PNG_DUMP_FILE)

            raise TestException('render test failed, bad checksum {} output image written to {}'.format(
                actual_hash, PNG_DUMP_FILE))

    register_tests(run_render_test, [name], targets)
