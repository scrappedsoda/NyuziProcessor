/*
 * logger.h  -  Provide a simple logging function
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Markus Kessler (1225380)
 *      Mail:   [matrNr]@student.tuwien.ac.at
 *
 */
#include <string>
#include "xil_printf.h"

#include "logger.hpp"

/**
 * @brief           A wrapper for the xil_printf command
 * @param print     if TRUE, the message will be printed.
 * @param format    is the format string for xil_printf
 */
void log(bool print, const char8 *format, ...) {
    if (!print) {
        return;
    }
    char dest[1024];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(dest, 1024, format, argptr);
    va_end(argptr);
    xil_printf(dest);
}
