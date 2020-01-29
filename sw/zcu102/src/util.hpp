/*
 * util.hpp - Contain additional code to manage files and subfolders
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Markus Kessler (1225380)
 *      Mail:   markus.kessler@student.tuwien.ac.at
 * 
 */
#ifndef MY_UTIL_HPP
#define MY_UTIL_HPP

#include <string>
#include <vector>
#include <memory>

using namespace std;

enum S_STATUS { S_NOT_FOUND, S_OK, S_LAST_ELEMENT, S_FIRST_ELEMENT };
enum FILE_MODE { FM_NOTHING, FM_RES_BIN, FM_RES_ASCII, FM_PROG_BIN, FM_PROG_ASCII, FM_PROG_RES_BIN, FM_PROG_RES_ASCII };

struct file;
struct folder;

struct file {
    string  name		= string("");
    size_t  size		= 0;
    folder* parent 		= nullptr;
    bool    is_selected = false;
    FILE_MODE file_mode = FM_NOTHING;

    /**
     * @return    The absolute path to this file.
     */
    string getPath();

    /**
     * @return    TRUE, if this file is selected as program file, FALSE otherwise.
     */
    bool is_program();

    /**
     * @return    TRUE, if this file is selected as resource file, FALSE otherwise.
     */
    bool is_resource();

    /**
     * @return    TRUE, if this file is in ASCII mode, FALSE if in binary mode.
     */
    bool is_ascii();
};

struct folder {
    string          		   name = string("");
    vector<unique_ptr<file>>   files;
    vector<unique_ptr<folder>> subfolders;
    folder* parent = nullptr;

    /**
     * @return    A pointer to the current selected file
     */
    file* getSelectedFile();

    /**
     * @return    A pointer to the current resource file
     */
    file* getResourceFile();

    /**
     * @return    A pointer to the current program file
     */
    file* getProgramFile();

    /**
     * @return    The absolute path to this directory
     */
    string getPath();

    void do_reset();
};

// PROTOTYPES

/**
 * @brief           Print an overview of all files
 * @param files     is an array holding all files
 */
void printMenu(const folder &f);

/**
 * @brief           Print the content of a folder
 * @param f         is the folder that should be printed
 * @param prefix    is the prefix, that will be added before any output
 */
void printDirectory(const folder &f, const string &prefix);

/**
 * @brief       Trying to decrease the selection
 * @param f     defines the folder where the selection should be decreased
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS decreaseSelection(folder &f);

/**
 * @brief       Trying to increase the selection
 * @param f     defines the folder where the selection should be increased
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS increaseSelection(folder &f);

/**
 * @brief       Trying to decrease the file mode of the current file selection
 * @param f     is the root folder
 * @return      S_OK if the status has been changed, S_NOT_FOUND otherwise.
 */
S_STATUS decreaseFileMode(folder &f);

/**
 * @brief       Trying to increase the file mode of the current file selection
 * @param f     is the root folder
 * @return      S_OK if the status has been changed, S_NOT_FOUND otherwise.
 */
S_STATUS increaseFileMode(folder &f);

/**
 * @brief       Select the first element in the provided folder. Files are prioritized over subfolders,
 *              that means that the subfolders are only considered if there isn't any file available.
 * @param f     defines the folder where the first element should become selected.
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS selectFirstElement(folder &f);

/**
 * @brief       Select the last element in the provided folder. Subfolders are prioritized over subfiles,
 *              that means that the last file becomes selected if there isn't any subfolder available.
 * @param f     defines the folder where the last element should become selected.
 * @return      The result of the selection. Check S_STATUS for all possible outcomes.
 */
S_STATUS selectLastElement(folder &f);

#endif
