/*
 * logger.hpp  -  Provide a simple logging function
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Markus Kessler (1225380)
 *      Mail:   markus.kessler@student.tuwien.ac.at
 *
 */
#ifndef SRC_LOGGER_HPP_
#define SRC_LOGGER_HPP_

/**
 * @brief           A wrapper for the xil_printf command
 * @param print     if TRUE, the message will be printed.
 * @param format    is the format string for xil_printf
 */
void log(bool print, const char* format, ...);

#endif /* SRC_LOGGER_HPP_ */
