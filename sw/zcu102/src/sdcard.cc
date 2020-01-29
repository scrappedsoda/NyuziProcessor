/*
 * sdcard.cc -     Implementation of the SD card using the XILFFS library
 *                 provided by xilinx.
 *
 *      Course: Embedded Systems in FPGA, 2017S
 *      Author: Markus Kessler (1225380)
 *      Mail:   markus.kessler@student.tuwien.ac.at
 */

#include <cstring>
#include "xsdps.h"

#include "sdcard.hpp"
#include "logger.hpp"
#include "xil_printf.h"
#include "sleep.h"

#define DEBUG   0
#define DEBUG2  0

// ++++++++++++++++++++++++++++
// ++++++ IMPLEMENTATION ++++++
// ++++++++s++++++++++++++++++++

static FATFS fatfs;
static XSdPs sd_ps;

/**
 * @brief           Initialize and mount the SD card.
 * @return          SD_MOUNTED, if the SD card has been initialized and mounted, SD_NOT_FOUND otherwise.
 */
SDSTATUS sdcard_init(void) {
    FRESULT ret;

    // Unmount and reset
    f_mount(0, "", 0);
    memset(&fatfs, 0, sizeof(FATFS));

    XSdPs_Config* sd_ps_config = XSdPs_LookupConfig(XPAR_XSDPS_0_DEVICE_ID);
    switch (XSdPs_CfgInitialize(&sd_ps, sd_ps_config, XPAR_XSDPS_0_BASEADDR)) {
    case XST_DEVICE_IS_STARTED:
        log(DEBUG, "[sdcard] CfgInitialize returned with XST_DEVICE_IS_STARTED\n\r");
        return SD_UNKNOWN;
        break;
    case XST_FAILURE:
        log(DEBUG, "[sdcard] CfgInitialize returned with XST_FAILURE\n\r");
        return SD_UNKNOWN;
        break;
    case XST_SUCCESS:
        log(DEBUG2, "[sdcard] CfgInitialize returned with XST_SUCCESS\n\r");
        break;
    default:
        log(DEBUG, "[sdcard] CfgInitialize returned with UNKNOWN\n\r");
        return SD_UNKNOWN;
        break;
    }

    switch (XSdPs_SdCardInitialize(&sd_ps)) {
    case XST_FAILURE:
        log(DEBUG, "[sdcard] XSdPs_SdCardInitialize returned with XST_FAILURE\n\r");
        return SD_NOT_FOUND;
        break;
    case XST_SUCCESS:
        log(DEBUG2, "[sdcard] XSdPs_SdCardInitialize returned with XST_SUCCESS\n\r");
        break;
    default:
        log(DEBUG, "[sdcard] XSdPs_SdCardInitialize returned with UNKNOWN\n\r");
        return SD_UNKNOWN;
        break;
    }

    switch (XSdPs_CardInitialize(&sd_ps)) {
    case XST_FAILURE:
        log(DEBUG, "[sdcard] XSdPs_CardInitialize returned with XST_FAILURE\n\r");
        return SD_UNKNOWN;
        break;
    case XST_SUCCESS:
        log(DEBUG2, "[sdcard] XSdPs_CardInitialize returned with XST_SUCCESS\n\r");
        break;
    default:
        log(DEBUG, "XSdPs_CardInitialize returned with UNKNOWN\n\r");
        return SD_UNKNOWN;
        break;
    }

    // Trying to mount with the XILFFS library
    ret = f_mount(&fatfs, "", 1);
    if (ret == FR_NOT_READY) {
        log(DEBUG, "[sdcard] SD card is not ready\n\r");
        return SD_UNKNOWN;
    }
    else if (ret == FR_DISK_ERR) {
        log(DEBUG, "[sdcard] SD card has been unplugged\n\r");
        return SD_NOT_FOUND;
    }
    else if (ret == FR_NO_FILESYSTEM) {
        log(DEBUG, "[sdcard] SD card has not been found\n\r");
        return SD_NOT_FOUND;
    }
    else if (ret == FR_OK) {
        log(DEBUG2, "[sdcard] SD card has been mounted\n\r");
        return SD_MOUNTED;
    }
    else {
        log(DEBUG, "[sdcard] SD card unknown error %d\n\r", ret);
        return SD_UNKNOWN;
    }
}

/**
 * @brief           Read the file structure of the SD card.
 * @param f_out     is a pointer to a folder, that will be used to output the root folder
 * @param type      defines the filetype that should be listed
 * @return          XST_SUCCESS if no error occured, XST_FAILURE otherwise.
 */
int sdcard_load(folder* f_out, const string &type) {
    // Trying to read the file structure
    f_out->name = "0";
    f_out->parent = nullptr;

    string path = "0:/";
    string drive = "0:";

    if (sdcard_readFolder(drive, "/", f_out, type) != XST_SUCCESS) {
        log(DEBUG, "[sdcard] Reading root folder on drive %d failed!\n\r", drive.c_str());
        return XST_FAILURE;
    }
    else {
        log(DEBUG2, "[sdcard] SDCard %s has been mounted and loaded!\n\r", drive.c_str());
        return XST_SUCCESS;
    }
}

/**
 * @brief       Detects if a SD card is plugged in and ready to use.
 * @return      The status of the SD card.
 */
SDSTATUS sdcard_detect(void) {
    DIR dir;

    // Force the driver to skip the cache and read from the memory
    fatfs.winsect = 0;

    string path = "0:/";
    FRESULT res = f_opendir(&dir, path.c_str());

    if (res == FR_NOT_ENABLED) {
        log(DEBUG, "[sdcard] SD card has been unplugged\n\r");
        return SD_NEED_RESET;
    }
    else if (res != FR_OK) {
        log(DEBUG, "[sdcard] Unknown detection error %d\n\r", res);
        return SD_UNKNOWN;
    }
    else {
        static FILINFO fno;
        res = f_readdir(&dir, &fno);
        if (res == FR_DISK_ERR) {
            log(DEBUG, "[sdcard] Card has been unplugged\n\r");
            return SD_NEED_RESET;
        }
        else if (res != FR_OK) {
            log(DEBUG, "[sdcard] (2) Unknown detection error %d\n\r", res);
            return SD_UNKNOWN;
        }
        else {
            log(DEBUG2, "[sdcard] SD card is mounted!\n\r");
            f_closedir(&dir);
            return SD_MOUNTED;
        }
    }
}

/**
 * @brief           Checks if a specified file exists
 * @param path      is the absolute path to the file
 * @param exist_out is the output variable
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_fileexist(const string &path, bool &exist_out) {
    FILINFO f;
    int ret = f_stat(path.c_str(), &f);
    exist_out = (ret == FR_OK);
    return XST_SUCCESS;
}

/**
 * @brief           Determine the size of a file
 * @param path      is the absolute path to the file
 * @param size_out  is the output variable
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_getsize(const string &path, int &size_out) {
    FILINFO f;
    int ret = f_stat(path.c_str(), &f);
    if (ret != FR_OK) {
        log(DEBUG, "[sdcard] Error! Cannot determine filesize of '%s' (%d)\n\r!", path.c_str(), ret);
        return XST_FAILURE;
    }
    size_out = f.fsize;
    return XST_SUCCESS;
}

/**
 * @brief           Collects all files and sub-folders from a specified directory
 * @param path      is the absolute path to the folder
 * @param name      is the name of the folder
 * @param f_out     is a pointer to a folder, that will be used to output the folder
 * @param type      defines the filetype that should be listed
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_readFolder(const string &path, const string &name, folder* f_out, const string &type) {

    DIR dir;
    DIR* dir_ptr = &dir;
    static FILINFO fno;

    log(DEBUG2, "[sdcard] Reading directory: %s with name %s\r\n", path.c_str(), name.c_str());

    FRESULT res = f_opendir(dir_ptr, path.c_str());
    if (res == FR_OK) {
        // Copy data to stack
        memcpy(&dir, dir_ptr, sizeof(DIR));

        // Convert type to lower case
        string type_lower = type;
        for (size_t i = 0; i < type_lower.size(); i++) {
            type_lower[i] = tolower(type_lower[i]);
        }

        // Set path and name of current directory
        if (name.compare("/") == 0) {
            // The root folder must be treated differently
            f_out->name = path;
        }
        else {
            f_out->name = name;
        }

        // Read until end of directory
        for (;;) {
            res = f_readdir(&dir, &fno);

            if (res != FR_OK || fno.fname[0] == 0) {
                /* Break on error or end of dir */
                break;
            }
            if (fno.fattrib & AM_DIR) {
                // Add a new directory, if it is not the system directory
                string foldername = string(fno.fname);
                if (foldername.compare("SYSTEM~1") != 0) {
                    unique_ptr<folder> f_child(new folder);
                    f_child->parent = f_out;
                    f_child->name = foldername;

                    if (sdcard_readFolder(path + "/" + f_child->name, f_child->name, f_child.get(), type) == XST_SUCCESS) {
                        log(DEBUG2, "[sdcard] Added directory %s to directory %s [%s]\n\r", f_child->name.c_str(), f_out->name.c_str(), f_child->getPath().c_str());
                        f_out->subfolders.push_back(move(f_child));
                    }
                }
            }
            else {
                // Add new file
                string name(fno.fname);
                if (type_lower.size() > 0) {
                    // Check if the filetype should be added
                    size_t dotpos = name.find_last_of('.');
                    if (dotpos < name.size()) {
                        // Convert filetype to lower case
                        string filetype = name.substr(dotpos + 1, name.size() - dotpos);
                        for (size_t i = 0; i < filetype.size(); i++) {
                            filetype.at(i) = tolower(filetype.at(i));
                        }
                        if (type_lower.compare(filetype) != 0) {
                            continue;
                        }
                    }
                    else {
                        continue;
                    }
                }

                // Add new file
                unique_ptr<file> f_child(new file);
                f_child->name = string(fno.fname);
                f_child->size = fno.fsize/1000;
                f_child->parent = f_out;
                f_child->is_selected = false;
                log(DEBUG2, "[sdcard] Add file: \n\r   --> name = %s\n\r   --> size = %d kB\n\r   --> directory = %s\n\r",
                        f_child->name.c_str(), f_child->size, f_out->name.c_str());
                const char* test = f_child->getPath().c_str();
                log(DEBUG2, "   --> path = %s\n\r", test);
                log(DEBUG2, "   --> parent_address %6x\n\r   --> parent name = %s\n\r",
                		f_child->parent, f_child->parent->name.c_str(), f_child->parent);
                f_out->files.push_back(move(f_child));
            }
        }

        res = f_closedir(&dir);
    }
    else {
        log(DEBUG, "[sdcard] Directory %s cannot be opened (%d)!\n\r", path.c_str(), res);
        return XST_FAILURE;
    }

    // Set return variable
    log(DEBUG2, "[sdcard] Read folder %s success!\n\r", f_out->name.c_str());
    return XST_SUCCESS;
}

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
int sdcard_open(const string &filename, const string &filemode, FIL* &file_out) {

    FRESULT ret;
    FIL f;

    // Trying to open the file. xilffs provides only read and write, therefore
    // we use a wrapper to map "r[b]" and "w[b]" to "FA_READ" or "FA_WRITE".
    if (filemode.size() == 0) {
        log(DEBUG, "[sdcard] Error! No filemode specified!\n\r");
        return XST_FAILURE;
    }
    else if (filemode[0] == 'r') {
        // Check if the file is available
        bool fileexists = false;
        int ret = sdcard_fileexist(filename, fileexists);
        if (!fileexists) {
            log(DEBUG, "[sdcard] Error! File '%s' has not been found (%d)!\n\r", filename.c_str(), ret);
            return XST_FAILURE;
        }
        ret = f_open(&f, filename.c_str(), FA_READ);
    }
    else if (filemode[0] == 'w') {
        ret = f_open(&f, filename.c_str() , FA_WRITE | FA_CREATE_ALWAYS);
    }
    else {
        log(DEBUG, "[sdcard] Error! Filemode '%s' not valid!\n\r", filemode.c_str());
        return XST_FAILURE;
    }

    // Check return value from f_open
    if (ret != FR_OK) {
        log(DEBUG, "[sdcard] Error! File '%s' cannot be opened (%d)!\n\r", filename.c_str(), ret);
        return XST_FAILURE;
    }
    else {
        memcpy(file_out, &f, sizeof(FIL));
        return XST_SUCCESS;
    }
}

/**
 * @brief           Reads a specified number of bytes from the file object
 *                  and stores it into the specified buffer
 * @param file      is a pointer to the file object struct
 * @param buff      is a pointer to the buffer
 * @param size      is the number of bytes that should be read
 * @return          The number of bytes, that have been read or -1 if an error occurred
 */
int sdcard_read(FIL* &file, u8* buff, size_t size) {
    if (size == 0) {
        return -1;
    }
    UINT NumBytesRead;
    FRESULT ret = f_read(file, (void*)buff, size, &NumBytesRead);
    if (ret != FR_OK) {
        log(DEBUG, "[sdcard] Read error: %d\n\r!", ret);
        return -1;
    }
    return NumBytesRead;
}

/**
 * @brief           Reads a single char from the file object and stores it
 *                  into the specified buffer
 * @param file      is a pointer to the file object struct
 * @param buff      is a pointer to the char buffer
 * @return          The number of bytes, that have been read
 */
int sdcard_readChar(FIL* &file, char* buff) {
    UINT NumBytesRead;
    FRESULT ret = f_read(file, (void*)buff, 1, &NumBytesRead);
    if (ret != FR_OK || NumBytesRead != 1) {
        return -1;
    }
    return NumBytesRead;
}

/**
 * @brief           Reads a characters from the file object until eol is
 *                  reached and stores them into the specified buffer
 * @param file      is a pointer to the file object struct
 * @param buffer    is a pointer to the buffer
 * @param size      is the size of the buffer
 * @return          The number of bytes, that have been read or -1 if an error occurred
 */
int sdcard_readLine(FIL* &file, u8* buffer, size_t size) {
    if (size == 0) {
        return -1;
    }
    
    UINT NumBytesReadTotal = 0;
    char lastChar = 0x00;

    while (NumBytesReadTotal < ((UINT)size - 1) && lastChar != (char)0x0A) {
        if (sdcard_readChar(file, &lastChar) != 1) {
            return -1;
        }
        buffer[NumBytesReadTotal] = (u8)lastChar;
        NumBytesReadTotal++;
    }
    if (NumBytesReadTotal > 0) {
        NumBytesReadTotal--;
        buffer[NumBytesReadTotal] = '\0';
    }
    return NumBytesReadTotal;
}

/**
 * @brief           Write to the file object
 * @param file      is a pointer to the file object struct
 * @param buffer    is a pointer to the buffer
 * @param size      is the size of the buffer
 * @return          The number of bytes, that have been written or -1 if an error occurred
 */
int sdcard_write(FIL* &file, const char* buffer, size_t size) {
    if (size == 0) {
        return -1;
    }
    UINT NumBytesWrite;
    int ret = f_write(file, buffer, size, &NumBytesWrite);
    if (ret != FR_OK) {
        log(DEBUG, "[sdcard] Write error: %d\n\r!", ret);
        return -1;
    }
    return NumBytesWrite;
}

/**
 * @brief           Closes the file
 * @param file      is a pointer to the file object sruct
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_close(FIL* &file) {
    int ret = f_close(file);
    if (ret != FR_OK) {
        log(DEBUG, "[sdcard] Close error: %d\n\r!", ret);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

/**
 * @brief           Move the file pointer to the new position
 * @param file      is a pointer to the file object struct
 * @param position  is the new position of the file pointer
 * @return          XST_SUCCESS, if the file has been opened, XST_FAILURE otherwise.
 */
int sdcard_seek(FIL* &file, const u32 &position) {
    int ret = f_lseek(file, position);
    if (ret != FR_OK) {
        log(DEBUG, "[sdcard] Seek error: %d\n\r!", ret);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

