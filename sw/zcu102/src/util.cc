/*
 * util.cpp - Implementation of all util functions
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Markus Kessler (1225380)
 *      Mail:   markus.kessler@student.tuwien.ac.at
 * 
 */
#include "xil_printf.h"

#include "util.hpp"
#include "logger.hpp"

#define DEBUG 0

// IMPLEMENTATION

/**
 * @return    The absolute path to this file.
 */
string file::getPath() {
    string parents;
    if (parent != nullptr) {
        return string(parent->getPath() + name);
    }
    else {
        return string(name);
    }
}

/**
 * @return    TRUE, if this file is in ASCII mode, FALSE if in binary mode.
 */
bool file::is_ascii() {
    switch (file_mode) {
        case FM_PROG_ASCII: case FM_RES_ASCII: case FM_PROG_RES_ASCII:
            return true;
            break;
        default:
            return false;
            break;
    }
}

/**
 * @return    TRUE, if this file is selected as program file, FALSE otherwise.
 */
bool file::is_program() {
    switch (file_mode) {
        case FM_PROG_BIN: case FM_PROG_ASCII: case FM_PROG_RES_BIN: case FM_PROG_RES_ASCII:
            return true;
            break;
        default:
            return false;
            break;
    }
}

/**
 * @return    TRUE, if this file is selected as resource file, FALSE otherwise.
 */
bool file::is_resource() {
    switch (file_mode) {
        case FM_RES_BIN: case FM_RES_ASCII: case FM_PROG_RES_BIN: case FM_PROG_RES_ASCII:
            return true;
            break;
        default:
            return false;
            break;
    }
}

/**
 * @return    A pointer to the current selected file
 */
file* folder::getSelectedFile() {
    // Check if a file is selected
    for (size_t i = 0; i < files.size(); i++) {
        if (files.at(i)->is_selected) {
            return files.at(i).get();
        }
    }
    // Search for a selection in every subdirectory
    for (size_t i = 0; i < subfolders.size(); i++) {
        file* sel = subfolders.at(i)->getSelectedFile();
        if (sel > nullptr) {
            return sel;
        }
    }
    // No selected file or directory has been found
    return nullptr;
}

/**
 * @return    A pointer to the current resource file
 */
file* folder::getResourceFile() {
    // Check if a file is marked as a resource file
    for (size_t i = 0; i < files.size(); i++) {
        if (files.at(i)->is_resource()) {
            return files.at(i).get();
        }
    }
    // Search for a resource file in every subfolder
    for (size_t i = 0; i < subfolders.size(); i++) {
        file* res = subfolders.at(i)->getResourceFile();
        if (res > nullptr) {
            return res;
        }
    }
    // No resource file has been found
    return nullptr;
}

/**
 * @return    A pointer to the current program file
 */
file* folder::getProgramFile() {
    // Check if a file is marked as a program file
    for (size_t i = 0; i < files.size(); i++) {
        if (files.at(i)->is_program()) {
            return files.at(i).get();
        }
    }
    // Search for a program file in every subfolder
    for (size_t i = 0; i < subfolders.size(); i++) {
        file* prog = subfolders.at(i)->getProgramFile();
        if (prog > nullptr) {
            return prog;
        }
    }
    // No program file has been found
    return nullptr;
}

/**
 * @return    The absolute path to this directory
 */
string folder::getPath() {
    if (parent == nullptr) {
        return string(name + "/");
    }
    else {
        string p(parent->getPath());
        return string(parent->getPath() + name + "/");
    }
}

void folder::do_reset() {
}

/**
 * @brief           Print an overview of all files
 * @param files     is an array holding all files
 */
void printMenu(const folder &f) {
    xil_printf("+++++++ LIST OF FILES +++++++\n\r");
    xil_printf("([P]rogram, [R]esource, [B]inary, [A]scii)\n\r");

    printDirectory(f, "");

    xil_printf("\n\r+++++++++++++++++++++++++++++\n\r");
}

/**
 * @brief           Print the content of a folder
 * @param f         is the folder that should be printed
 * @param prefix    is the prefix, that will be added before any output
 */
void printDirectory(const folder &f, const string &prefix) {
    for (size_t i = 0; i < f.files.size(); i++) {
        if (f.files.at(i)->is_selected) {
            xil_printf("->");
        }
        else {
            xil_printf("  ");
        }
        if (f.files.at(i)->is_program()) {
            xil_printf("P");
        }
        else {
            xil_printf(" ");
        }
        if (f.files.at(i)->is_resource()) {
            xil_printf("R");
        }
        else {
            xil_printf(" ");
        }
        if (f.files.at(i)->file_mode != FM_NOTHING) {
            if (f.files.at(i)->is_ascii()) {
                xil_printf("A");
            }
            else {
                xil_printf("B");
            }
        }
        else {
            xil_printf(" ");
        }
        xil_printf("  %s%s (%dkB)\n\r", prefix.c_str(), f.files.at(i)->name.c_str(), f.files.at(i)->size);
    }
    for (size_t i = 0; i < f.subfolders.size(); i++) {
        xil_printf("       %s[%s]\n\r", prefix.c_str(), f.subfolders.at(i)->name.c_str());
        printDirectory(*f.subfolders.at(i), prefix + "- ");
    }
}

/**
 * @brief       Trying to decrease the selection
 * @param f     defines the folder where the selection should be decreased
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS decreaseSelection(folder &f) {
    log(DEBUG, "[util] decreaseSelection: folder %s with %d files and %d subfolders\n\r", f.name.c_str(), f.files.size(), f.subfolders.size());
    // Find selection in files
    for (size_t i = 0; i < f.files.size(); i++) {
        if (f.files.at(i)->is_selected) {
            f.files.at(i)->is_selected = false;
            log(DEBUG, "[util] File %s selected\n\r", f.files.at(i)->name.c_str());
            if (i == 0) {
                // First file in this directory is selected.
                if (f.parent == nullptr) {
                    // The first possible file is selected. Select the last one
                    return selectLastElement(f);
                }
                else {
                    // Signal the caller, that we cannot decrease the selection
                    return S_FIRST_ELEMENT;
                }
            }
            // Select previous file
            f.files.at(i-1)->is_selected = true;
            log(DEBUG, "[util] Selected previously file %s\n\r", f.files.at(i-1)->name.c_str());
            return S_OK;
        }
    }
    // No selected file has been found yet. Search within the directories
    for (size_t i = f.subfolders.size(); i > 0; i--) {
        S_STATUS ret = decreaseSelection(*f.subfolders.at(i-1));
        if (ret == S_OK) {
            return S_OK; // Stop and return with success
        }
        else if (ret == S_FIRST_ELEMENT) {
            // First element in the subdirectory has been unselected.
            for (size_t j = i-1; j > 0; j--) {
                // Try to select the last file in the previous subdirectory
                if (selectLastElement(*f.subfolders.at(j-1)) == S_OK) {
                    return S_OK; // Stop and return with success
                }
            }
            // Try to select the last file in this folder
            if (f.files.size() > 0) {
                f.files.back()->is_selected = true;
                return S_OK;
            }
            else {
                if (f.parent == nullptr) {
                    // This is the root folder. Select the last possible element
                    return selectLastElement(f);
                }
                else {
                    // Signal the caller, that we cannot decrease the selection
                    return S_FIRST_ELEMENT;
                }
            }
        }
    }
    return S_NOT_FOUND;
}

/**
 * @brief       Trying to increase the selection
 * @param f     defines the folder where the selection should be increased
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS increaseSelection(folder &f) {
    // Find selection in files
    for (size_t i = 0; i < f.files.size(); i++) {
        if (f.files.at(i)->is_selected) {
            f.files.at(i)->is_selected = false;
            log(DEBUG, "[util] File %s selected\n\r", f.files.at(i)->name.c_str());
            if (i == f.files.size() - 1) {
                // Last file in this directory is selected.
                // If there are subfolders, try to select a file in them
                for (size_t j = 0; j < f.subfolders.size(); j++) {
                    if (selectFirstElement(*f.subfolders.at(j)) == S_OK) {
                        return S_OK; // Stop and return with success
                    }
                }
                if (f.parent == nullptr) {
                    // This is the root folder. Select the first possible element
                    return selectFirstElement(f);
                }
                else {
                    // Signal the caller, that we cannot increase the selection
                    return S_LAST_ELEMENT;
                }
            }
            else {
                // Select next file
                f.files.at(i+1)->is_selected = true;
                log(DEBUG, "[util] Selected next file %s\n\r", f.files.at(i+1)->name.c_str());
                return S_OK;
            }
        }
    }
    // No selected file has been found yet. Search within the directories
    for (size_t i = 0; i < f.subfolders.size(); i++) {
        // Try recursive with current folder
        S_STATUS ret = increaseSelection(*f.subfolders.at(i));
        if (ret == S_OK) {
            return S_OK; // Stop and return with success
        }
        else if (ret == S_LAST_ELEMENT) {
            // Last element in recursive subtree has been unselected.
            if (i < f.subfolders.size() - 1) {
                // Try to select first element in next subdirectory
                if (selectFirstElement(*f.subfolders.at(i+1)) == S_OK) {
                    return S_OK; // Stop and return with success
                }
            }
            if (f.parent == nullptr) {
                // We are in the root directory and the selection cannot be increased.
                // Instead, select the first file
                return selectFirstElement(f);
            }

            // Signal the caller, that we cannot increase the selection.
            return S_LAST_ELEMENT;
        }
    }
    // Signal the caller, that no selected file has been found
    return S_NOT_FOUND;
}

/**
 * @brief       Trying to decrease the file mode of the current file selection
 * @param f     is the root folder
 * @return      S_OK if the status has been changed, S_NOT_FOUND otherwise.
 */
S_STATUS decreaseFileMode(folder &f) {
    // Find current file selection
    file* selection = f.getSelectedFile();
    if (selection > nullptr) {
        if (selection->file_mode > FM_NOTHING) {
            selection->file_mode = static_cast<FILE_MODE>(((int)selection->file_mode) - 1);
        }
        else {
            selection->file_mode = FM_PROG_RES_ASCII;
        }
        return S_OK;
    }
    else {
        return S_NOT_FOUND;
    }
}

/**
 * @brief       Trying to increase the file mode of the current file selection
 * @param f     is the root folder
 * @return      S_OK if the status has been changed, S_NOT_FOUND otherwise.
 */
S_STATUS increaseFileMode(folder &f) {
    // Find current file selection
    file* selection = f.getSelectedFile();
    if (selection > nullptr) {
        if (selection->file_mode < FM_PROG_RES_ASCII) {
            selection->file_mode = static_cast<FILE_MODE>(((int)selection->file_mode) + 1);
        }
        else {
            selection->file_mode = FM_NOTHING;
        }
        return S_OK;
    }
    else {
        return S_NOT_FOUND;
    }
}

/**
 * @brief       Select the first element in the provided folder. Files are prioritized over subfolders,
 *              that means that the subfolders are only considered if there isn't any file available.
 * @param f     defines the folder where the first element should become selected.
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS selectFirstElement(folder &f) {
    if (f.files.size() > 0) {
        // Use the first file
        f.files.at(0)->is_selected = true;
        return S_OK;
    }
    // Check if there are subdirectories
    for (size_t i = 0; i < f.subfolders.size(); i++) {
        if (selectFirstElement(*f.subfolders.at(i)) == S_OK) {
            return S_OK; // Stop and return with success
        }
    }
    // Signal the caller, that this folder is empty.
    return S_NOT_FOUND;
}

/**
 * @brief       Select the last element in the provided folder. Subfolders are prioritized over subfiles,
 *              that means that the last file becomes selected if there isn't any subfolder available.
 * @param f     defines the folder where the last element should become selected.
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS selectLastElement(folder &f) {
    // Check if there are directories
    for (size_t i = f.subfolders.size(); i > 0; i--) {
        if (selectLastElement(*f.subfolders.at(i-1)) == S_OK) {
            return S_OK; // Stop and return with success
        }
    }
    if (f.files.size() > 0) {
        // Use the last file
        f.files.back()->is_selected = true;
        return S_OK;
    }
    else {
        // Signal the caller, that this folder is empty.
        return S_NOT_FOUND;
    }
}
