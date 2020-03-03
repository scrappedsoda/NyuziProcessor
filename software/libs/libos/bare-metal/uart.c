//
// Copyright 2011-2015 Jeff Bush
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

#include "registers.h"

#define UART_TX_READY 	(1 << 0)

void _write_uart(char ch)
{
    while ((REGISTERS[REG_UART_STATUS] & UART_TX_READY) == 0)
        ;	// Wait for space

    REGISTERS[REG_UART_TX] = ch;
}

int write_console(const char *str, int length)
{
    int i;

    for (i = 0; i < length; i++)
        _write_uart(str[i]);

    return 0;
}

