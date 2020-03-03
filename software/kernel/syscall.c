//
// Copyright 2016 Jeff Bush
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "errno.h"
#include "thread.h"
#include "libc.h"
#include "registers.h"
#include "syscalls.h"
#include "vga.h"

#define NUM_PERF_COUNTERS 4

extern int user_copy(void *dest, const void *src, int count);
extern int user_strlcpy(char *dest, const char *src, int count);

int handle_syscall(int index, int arg0, int arg1, int arg2, int arg3, int arg4,
                   int arg5)
{
    char tmp[64];

    (void) arg4;
    (void) arg5;

    switch (index)
    {
        // int write_console(const char *data, int length);
        case SYS_write_console:
            if ((unsigned int) arg1 >= sizeof(tmp) - 2)
            {
                kprintf("size out of range\n");
                return -EINVAL;
            }

            if (user_copy(tmp, (void*) arg0, arg1) < 0)
            {
                kprintf("user copy failed\n");
                return -EFAULT;
            }

            tmp[arg1] = '\0';
            kprintf("%s", tmp);
            return 0;

        // int spawn_user_thread(const char *name, function, void *arg);
        case SYS_spawn_thread:
            spawn_user_thread((const char*) arg0, current_thread()->proc, arg1,
                              (void*) arg2);
            return 0;

        // int get_current_thread_id();
        case SYS_get_current_thread_id:
            return current_thread()->id;

        // int exec(const char *path);
        case SYS_exec:
        {
            // XXX unsafe user copy. Need copy_from_user
            struct process *proc = exec_program((const char*) arg0);
            if (proc)
            {
                int id = proc->id;
                dec_proc_ref(proc);
                return id;
            }
            else
                return -ENOEXEC;
        }

        // void thread_exit(int code)
        case SYS_thread_exit:
            thread_exit(arg0);  // This will not return

        // void *init_vga(int mode);
        case SYS_init_vga:
            return (int) init_vga(arg0);

        // void *create_area(unsigned int address, unsigned int size, int placement,
        //                   const char *name, int flags);
        case SYS_create_area:
        {
            if (user_strlcpy(tmp, (const char*) arg3, sizeof(tmp)) < 0)
                return 0;

            struct vm_area *area = create_area(current_thread()->proc->space,
                (unsigned int) arg0, // Address
                (unsigned int) arg1, // size
                arg2, // Placement
                tmp,
                arg4, // flags,
                0, 0);

            if (area == 0)
                return 0;

            return area->low_address;
        }

        // void set_perf_counter_event(int counter, enum performance_event event)
        case SYS_set_perf_counter:
            if (arg0 >= 0 && arg0 < NUM_PERF_COUNTERS)
                REGISTERS[REG_PERF0_SEL + arg0] = arg1;

            return 0;

        // unsigned int read_perf_counter(int counter)
        case SYS_read_perf_counter:
            if (arg0 >= 0 && arg0 < NUM_PERF_COUNTERS)
                return REGISTERS[REG_PERF0_VAL + arg0];
            else
                return 0;

        case SYS_get_cycle_count:
            return __builtin_nyuzi_read_control_reg(6);

        default:
            kprintf("Unknown syscall %d\n", index);
            return -EINVAL;
    }
}
