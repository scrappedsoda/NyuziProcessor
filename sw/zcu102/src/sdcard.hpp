/*
 * sdcard.hpp - Contain the basic function declarations to use the SD card
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Markus Kessler (1225380)
 *      Mail:   markus.kessler@student.tuwien.ac.at
 */

#ifndef SDCARD_HPP
#define SDCARD_HPP

#include <string>

#include "util.hpp"

#include "ff.h"
#include "xstatus.h"

typedef enum {
	SD_MOUNTED,
	SD_NOT_FOUND,
	SD_NEED_RESET,
	SD_UNKNOWN
} SDSTATUS;

// ++++++++++++++++++++++++++++
// ++++++++ PROTOTYPES ++++++++
// ++++++++++++++++++++++++++++

/**
 * @brief           Initialize and mount the SD card.
 * @return          XST_SUCCESS, if the SD card has been initialized, XST_FAILURE otherwise.
 */
SDSTATUS sdcard_init(void);

/**
 * @brief			Read the file structure of the SD card.
 * @param f_out     is a pointer to a folder, that will be used to output the root folder
 * @param type      defines the filetype that should be listed
 * @return			XST_SUCCESS if no error occured, XST_FAILURE otherwise.
 */
int sdcard_load(folder* f_out, const string &type = "");

/**
 * @brief			Detects if a SD card is plugged in and ready to use.
 * @return			The status of the SD card.
 */
SDSTATUS sdcard_detect(void);

/**
 * @brief           Checks if a specified file exists
 * @param path      is the absolute path to the file
 * @param exist_out is the output variable
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_fileexist(const string &path, bool &exist_out);

/**
 * @brief           Determine the size of a file
 * @param path      is the absolute path to the file
 * @param size_out  is a pointer to the output variable
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_getsize(const string &path, int &size_out);

/**
 * @brief           Collects all files and sub-folders from a specified directory
 * @param path      is the absolute path to the folder
 * @param name      is the name of the folder
 * @param f_out     is a pointer to a folder, that will be used to output the folder
 * @param type      defines the filetype that should be listed
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_readFolder(const string &path, const string &name, folder* f_out, const string &type);

/**
 * @brief           Opens a file from the SD card for reading or writing
 * @param filename  is the name of the file
 * @param filemode  defines how to open the file (read, write, binary)
 *                  You can use the standard convention like "r" (read), "w" (write),
 *                  "rb" (read-binary), "wb" (write-binary)
 * @param file_out  is a pointer to the file object struct that will be used to
 *                  open the file
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_open(const string &filename, const string &filemode, FIL* &file_out);

/**
 * @brief           Reads a specified number of bytes from the file object
 *                  and stores it into the specified buffer
 * @param file      is a pointer to the file object struct
 * @param buff      is a pointer to the buffer
 * @param size      is the number of bytes that should be read
 * @return          The number of bytes, that have been read or -1 if an error occurred
 */
int sdcard_read(FIL* &file, u8* buff, size_t size);

/**
 * @brief           Reads a characters from the file object until eol is
 *                  reached and stores them into the specified buffer
 * @param file      is a pointer to the file object struct
 * @param buffer    is a pointer to the buffer
 * @param size      is the size of the buffer
 * @return          The number of bytes, that have been read or -1 if an error occurred
 */
int sdcard_readLine(FIL* &file, u8* buffer, size_t size);

/**
 * @brief           Write to the file object
 * @param file      is a pointer to the file object struct
 * @param buffer    is a pointer to the buffer
 * @param size      is the size of the buffer
 * @return          The number of bytes, that have been written or -1 if an error occurred
 */
int sdcard_write(FIL* &file, const char* buffer, size_t size);

/**
 * @brief           Closes the file
 * @param file      is a pointer to the file object sruct
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_close(FIL* &file);

/**
 * @brief           Move the file pointer to the new position
 * @param file      is a pointer to the file object struct
 * @param position  is the new position of the file pointer
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_seek(FIL* &file, const u32 &position);

#endif
