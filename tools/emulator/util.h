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

#ifndef UTIL_H
#define UTIL_H

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define INT8_PTR(memory, address) ((int8_t*)(memory) + (address))
#define UINT8_PTR(memory, address) ((uint8_t*)(memory) + (address))
#define INT16_PTR(memory, address) ((int16_t*)(memory) + (address) / 2)
#define UINT16_PTR(memory, address) ((uint16_t*)(memory) + (address) / 2)
#define UINT32_PTR(memory, address) ((uint32_t*)(memory) + (address) / 4)

static inline uint32_t endian_swap32(uint32_t value)
{
    return ((value & 0x000000ff) << 24)
           | ((value & 0x0000ff00) << 8)
           | ((value & 0x00ff0000) >> 8)
           | ((value & 0xff000000) >> 24);
}

static inline uint32_t extract_unsigned_bits(uint32_t word, uint32_t low_bit, uint32_t size)
{
    return (word >> low_bit) & ((1u << size) - 1);
}

static inline uint32_t extract_signed_bits(uint32_t word, uint32_t low_bit, uint32_t size)
{
    uint32_t mask = (1u << size) - 1;
    uint32_t value = (word >> low_bit) & mask;
    if (value & (1u << (size - 1)))
        value |= ~mask;	// Sign extend

    return value;
}

// Treat integer bitpattern as float without converting
// This is legal in C99
static inline float value_as_float(uint32_t value)
{
    union
    {
        float f;
        uint32_t i;
    } u = { .i = value };

    return u.f;
}

// Treat floating point bitpattern as int without converting
static inline uint32_t value_as_int(float value)
{
    union
    {
        float f;
        uint32_t i;
    } u = { .f = value };

    // x86 at least propagates NaN as recommended (but not required) by IEEE754,
    // but Nyuzi uses a consistent NaN representation for simplicity.
    if (isnan(value))
        return 0x7fffffff;

    return u.i;
}

// This returns true if there are bytes available for reading *or*
// reading it would return an error.
static inline bool can_read_file_descriptor(int fd)
{
    fd_set read_fds;
    int result;
    struct timeval timeout;

    do
    {
        // The read_fds will be signaled when read will not block,
        // including error conditions.
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    }
    while (result < 0 && errno == EINTR);

    return result != 0;
}

// Returns 0 if successful, -1 if there was a format error in the string.
int parse_hex_vector(const char *str, uint32_t *vector_values, bool endian_swap);

// Return the index of the next lowest set bit at or below index (with 0
// being the least signficant bit). If there is no set bits below the index,
// wrap around and search from the the most significand bit position.
// The result is undefined if bitmask is 0
static inline uint32_t next_set_bit(uint32_t bitmask, uint32_t index)
{
    const uint32_t lower_mask = ((1 << (index + 1)) - 1) & bitmask;
    if (lower_mask)
        return 31 - (uint32_t) __builtin_clz(lower_mask);

    // Couldn't find a set bit, search from topmost bit
    return 31 - (uint32_t) __builtin_clz(bitmask);
}

void seed_random(uint64_t value);
uint64_t next_random(void);

uint64_t current_time_us(void);

int read_hex_file(const char *filename, uint32_t *memory, uint32_t memory_size);

#endif
