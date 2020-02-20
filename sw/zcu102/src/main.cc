/*
 * main.cc -    The program entry point for converting a 3D scene to a
 *              filesystem for Nyuzi on the Zedboard
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Edwin Willegger (1326324), Jonas Ferdigg (1226597), Markus Kessler (1225380)
 *      Mail:   e[matrNr]@student.tuwien.ac.at
 *
 */

#include "xil_printf.h"
#include "xil_io.h"
#include "sleep.h"
#include "xscugic.h"
#include "xsdps.h"

#include "logger.hpp"

#include "gpio.h"
#include "shared_mem_itf.hpp"
#include "sdcard.hpp"
#include "util.hpp"
#include "snes.h"

#define DEBUG           1

#define MIDDLE_BUTTON   1
#define DOWN_BUTTON     2
#define RIGHT_BUTTON    4
#define UP_BUTTON       8
#define LEFT_BUTTON     16

#define CHANNEL_PB      1
#define CHANNEL_RST     1
#define CHANNEL_SW      1
#define CHANNEL_LED     2

#define DIRECTION_WRITE 0x00
#define DIRECTION_READ  0xFF

// The position of the reset signals in the reset_vector
#define RESET_DDR4          0
#define RESET_NYUZI         1
#define RESET_AXI           2
#define RESET_VGA           3
#define RESET_AXI_INTER     4

#define SLEEP_TIME_US   10000

/* GPIO instances of push_button, reset and leds/switches */
XGpio gpio_push_buttons, gpio_reset, gpio_switches_leds;
/* Instance of the Interrupt controller driver */
XScuGic intc, intc2;
/* The current and previous button press */
extern u8 button_pressed, button_pressed_last, button_activation;
/* The snes button pressed state */
u32 snes_buttons_pressed;
/* Use channel 1 for the interrupt */
const u32 globalIntrMask = 0x01;
/* Channel 2 for snes interrupt */
const u32 globalIntrMaskSnes = 0x02;
/* Pointer to the shared memory area */
smdb_t* smdb = (smdb_t*)(SMDB_ADDR);
/* A struct that models the content of a sdcard */
unique_ptr<folder> sdcard = nullptr;
/* Stores the status of the four reset signals */
u32 reset_vector = 0;
/* If TRUE, a program file has been sent to Nyuzi. */
bool program_loaded;
/* TRUE, if Nyuzi is running, FALSE otherwise */
bool nyuzi_is_running;

// PROTOTYPES

int init(void);
void reset(void);
int write_file(const string &filename, u64 base_address, bool ascii);
void interrupt_handler(void* callback);
u64 endian_swap64(u64 value);

// IMPLEMENTATION

/**
 * @brief       Main entry point of the program. Initializes the system and controls Nyuzi.
 * @return      XST_SUCCESS if no error occurred, XST_FAILURE otherwise.
 */
int main(void) {
    int status = 0;

    program_loaded = false;
    nyuzi_is_running = false;
    u32 switches = 0;

    int programFileSize = -1;
    int dataFileSize = -1;

    // Initialize the GPIO connections and push-button interrupt
    status = init();
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }
    XGpio_DiscreteWrite(&gpio_switches_leds, CHANNEL_LED, 0x55);
    reset();

    // Try to mount and read the SD card
    sdcard = unique_ptr<folder>(new folder);
    SDSTATUS sdcard_status = sdcard_init();
    if (sdcard_status == XST_SUCCESS) {
        sdcard_load(sdcard.get());
        selectFirstElement(*sdcard.get());
    }
    printMenu(*sdcard.get());


    while (true) {
        if (button_pressed_last != button_pressed) {
            // Remount SD card if an error occured
            sdcard_status = sdcard_detect();
            if (sdcard_status == SD_NEED_RESET) {
                sdcard = unique_ptr<folder>(new folder);
                sdcard_status = sdcard_init();
                if (sdcard_status == SD_MOUNTED) {
                    sdcard_load(sdcard.get());
                    selectFirstElement(*sdcard.get());
                }
                printMenu(*sdcard.get());
            }

            if (sdcard_status == SD_NOT_FOUND || sdcard_status == SD_NEED_RESET) {
                xil_printf("\n\r### Please insert a SD card! ###\n\r");
            }
            else if (sdcard_status == SD_UNKNOWN) {
                xil_printf("\n\r### Unknown SD card error! ###\n\r");
            }


            if (nyuzi_is_running) {
                // ********************************
                //              SCENE MODE
                // ********************************

                // Read the state of the switches
                switches = XGpio_DiscreteRead(&gpio_switches_leds, 1) & 0x01;
                u32 action = Xil_In32BE((u64)&smdb->camera_actions);
                if (action & (1 << ACTION_BIT_ROTATE) && switches > 0) {
                    action &= ~(1 << ACTION_BIT_ROTATE);
                    Xil_Out32BE((u64)&smdb->camera_actions, action);
                }
                else if ((action & (1 << ACTION_BIT_ROTATE)) == 0 && switches == 0) {
                    action |= (1 << ACTION_BIT_ROTATE);
                    Xil_Out32BE((u64)&smdb->camera_actions, action);
                }

                if ((button_pressed & MIDDLE_BUTTON) && !(button_pressed_last & MIDDLE_BUTTON)) {
                    reset();
                    nyuzi_is_running = false;
                    xil_printf("\n\r### Nyuzi has been stopped! ###\n\r");
                    printMenu(*sdcard.get());
                }
                else if ((button_pressed & DOWN_BUTTON) && !(button_pressed_last & DOWN_BUTTON)) {
                    u32 action = Xil_In32BE((u64)&smdb->camera_actions);
                    if (action & (1 << ACTION_BIT_DOWN)) {
                        action &= ~(1 << ACTION_BIT_DOWN);
                    }
                    else if (action & (1 << ACTION_BIT_UP)) {
                        action &= ~(1 << ACTION_BIT_UP);
                    }
                    else {
                        action |= (1 << ACTION_BIT_DOWN);
                    }
                    Xil_Out32BE((u64)&smdb->camera_actions, action);
                }
                else if ((button_pressed & RIGHT_BUTTON) && !(button_pressed_last & RIGHT_BUTTON)) {
                    u32 action = Xil_In32BE((u64)&smdb->camera_actions);
                    if (action & (1 << ACTION_BIT_RIGHT)) {
                        action &= ~(1 << ACTION_BIT_RIGHT);
                    }
                    else if (action & (1 << ACTION_BIT_LEFT)) {
                        action &= ~(1 << ACTION_BIT_LEFT);
                    }
                    else {
                        action |= (1 << ACTION_BIT_RIGHT);
                    }
                    Xil_Out32BE((u64)&smdb->camera_actions, action);
                }
                else if ((button_pressed & UP_BUTTON) && !(button_pressed_last & UP_BUTTON)) {
                    u32 action = Xil_In32BE((u64)&smdb->camera_actions);
                    if (action & (1 << ACTION_BIT_UP)) {
                        action &= ~(1 << ACTION_BIT_UP);
                    }
                    else if (action & (1 << ACTION_BIT_DOWN)) {
                        action &= ~(1 << ACTION_BIT_DOWN);
                    }
                    else {
                        action |= (1 << ACTION_BIT_UP);
                    }
                    Xil_Out32BE((u64)&smdb->camera_actions, action);
                }
                else if ((button_pressed & LEFT_BUTTON) && !(button_pressed_last & LEFT_BUTTON)) {
                    u32 action = Xil_In32BE((u64)&smdb->camera_actions);
                    if (action & (1 << ACTION_BIT_LEFT)) {
                        action &= ~(1 << ACTION_BIT_LEFT);
                    }
                    else if (action & (1 << ACTION_BIT_RIGHT)) {
                        action &= ~(1 << ACTION_BIT_RIGHT);
                    }
                    else {
                        action |= (1 << ACTION_BIT_LEFT);
                    }
                    Xil_Out32BE((u64)&smdb->camera_actions, action);
                }
            }
            else {
                // ********************************
                //             MENU MODE
                // ********************************

                if ((button_pressed & MIDDLE_BUTTON) && !(button_pressed_last & MIDDLE_BUTTON) && sdcard_status == SD_MOUNTED) {
                    file* program = sdcard->getProgramFile();
                    if (program != nullptr) {
                        // Write the selected program file to the memory
                        string filepath(program->getPath());
                        log(DEBUG, "[main] Writing program file...\n\r (%s) --> 0x%llx\n\r", filepath.c_str(), (u64)PROG_ADDR);
                        programFileSize = write_file(filepath, PROG_ADDR, program->is_ascii());

                        if (programFileSize <= 0) {
                            // An error occured. This happens when the SD card has been plugged out.
                            log(DEBUG, "[main] Writing program file...failed\n\r");
                            sdcard_status = SD_UNKNOWN;
                            program_loaded = false;
                        }
                        else {
                            // Success! Update the shared memory database
                            Xil_Out32BE((u64)&smdb->program_fil_st_addr,  (u32)PROG_ADDR);
                            Xil_Out32BE((u64)&smdb->program_fil_size,     programFileSize);
                            Xil_Out32BE((u64)&smdb->camera_actions,         0);
                            program_loaded = true;
                        }
                    }

                    if (sdcard_status == XST_SUCCESS) { // Do not continue if the program transfer has been failed
                        file* resource = sdcard->getResourceFile();
                        if (resource != nullptr) {
                            // Write the selected resource file to the memory
                            string filepath(resource->getPath());
                            log(DEBUG, "[main] Witing resource file...\n\r (%s) --> 0x%llx\n\r", filepath.c_str(), (u64)RES_ADDR);
                            dataFileSize = write_file(filepath, RES_ADDR, resource->is_ascii());
                            if (dataFileSize <= 0) {
                                // An error occured. This happens when the SD card has been plugged out.
                                log(DEBUG, "[main] Writing resource file...failed\n\r");
                                sdcard_status = SD_UNKNOWN;;
                                program_loaded = false;
                            }
                            else {
                                // Success! Update the memory database
                                Xil_Out32BE((u64)&smdb->resource_fil_st_addr, (u32)RES_ADDR);
                                Xil_Out32BE((u64)&smdb->resource_fil_size,    dataFileSize);
                            }
                        }
                        if (program_loaded) {
                            // Start Nyuzi
                            reset_vector &= ~(1UL << RESET_NYUZI);
                            XGpio_DiscreteWrite(&gpio_reset, CHANNEL_RST, reset_vector);
                            nyuzi_is_running = true;
                            xil_printf("\n\r### Nyuzi has been started! ###\n\r");
                        }
                    }
                }
                else if ((button_pressed & DOWN_BUTTON) && !(button_pressed_last & DOWN_BUTTON)) {
                    increaseSelection(*sdcard);
                    printMenu(*sdcard);
                }
                else if ((button_pressed & RIGHT_BUTTON) && !(button_pressed_last & RIGHT_BUTTON)) {
                    increaseFileMode(*sdcard);
                    printMenu(*sdcard);
                }
                else if ((button_pressed & UP_BUTTON) && !(button_pressed_last & UP_BUTTON)) {
                    decreaseSelection(*sdcard);
                    printMenu(*sdcard);
                }
                else if ((button_pressed & LEFT_BUTTON) && !(button_pressed_last & LEFT_BUTTON)) {
                    decreaseFileMode(*sdcard);
                    printMenu(*sdcard);
                }
            }
            // Save pressed button state for next round
            button_pressed_last = button_pressed;
        }
        usleep(SLEEP_TIME_US);
    }
    xil_printf("Shutting down!\n\r");
    return XST_SUCCESS;
}

/**
 * @brief        Resets the system to be ready for starting a new program.
 *               After this method returns, only the reset signal for NYUZI is asserted.
 */
void reset(void) {
    // Halt Nyuzi and reset the system
    reset_vector |= (1UL << RESET_NYUZI);
    XGpio_DiscreteWrite(&gpio_reset, CHANNEL_RST, RESET_NYUZI);
    reset_vector |= (1UL << RESET_VGA);
    reset_vector |= (1UL << RESET_AXI);
    reset_vector |= (1UL << RESET_AXI_INTER);
    XGpio_DiscreteWrite(&gpio_reset, CHANNEL_RST, reset_vector);
    usleep(1);
    // Restart the system
    reset_vector &= ~(1UL << RESET_AXI_INTER);
    reset_vector &= ~(1UL << RESET_AXI);
    reset_vector &= ~(1UL << RESET_VGA);
    XGpio_DiscreteWrite(&gpio_reset, CHANNEL_RST, reset_vector);
}

/**
 * @brief       Initialize the GPIO drivers and the interrupt system
 * @return      XST_SUCCESS if no error occurred, XST_FAILURE otherwise
 */
int init(void) {
    int ret = XST_SUCCESS;
    int status;

    log(DEBUG, "[main] Initializing GPIO push buttons...\n\r");
    status = XGpio_Initialize(&gpio_push_buttons, XPAR_AXI_GPIO_1_DEVICE_ID);
    if (status != XST_SUCCESS) {
        log(DEBUG, "[main] Initializing GPIO push buttons...failed!\n\r");
        ret = XST_FAILURE;
        return ret;
    }

	log(DEBUG, "[main] Initalizing interrupt routine for push buttons...\n\r");
	status = gpio_interrupt_init(&gpio_push_buttons, &intc, XPAR_SCUGIC_0_DEVICE_ID,
	        XPAR_FABRIC_AXI_GPIO_1_IP2INTC_IRPT_INTR, globalIntrMask, interrupt_handler);
	if (status != XST_SUCCESS) {
	    log(DEBUG, "[main] Initalizing interrupt routine for push buttons...failed!\n\r");
	    ret = XST_FAILURE;
	    return ret;
	}

	/*
    log(DEBUG, "[main] Initalizing interrupt routine for snes buttons...\n\r");
        status = snes_interrupt_init(&intc2, XPAR_SCUGIC_0_DEVICE_ID, XPAR_FABRIC_SNES_0_BTN_CHANGE_INTR, snes_interrupt_handler);
        if (status != XST_SUCCESS) {
            log(DEBUG, "[main] Initalizing interrupt routine for snes buttons...failed!\n\r");
            ret = XST_FAILURE;
            return ret;
        }
	*/

    log(DEBUG, "[main] Initializing GPIO reset...\n\r");
    status = XGpio_Initialize(&gpio_reset, XPAR_AXI_GPIO_0_DEVICE_ID);
    if (status != XST_SUCCESS) {
        log(DEBUG, "[main] Initializing GPIO reset...failed!\n\r");
        ret = XST_FAILURE;
        return ret;
    }
    log(DEBUG, "[main] Initalizing GPIO switches and leds...\n\r");
    status = XGpio_Initialize(&gpio_switches_leds, XPAR_AXI_GPIO_2_DEVICE_ID);
    if (status != XST_SUCCESS) {
        log(DEBUG, "[main] Initalizing GPIO switches and leds...failed!\n\r");
        ret = XST_FAILURE;
        return ret;
    }

    // Set the data direction for push_buttons (read), reset (write), switches (read) and leds (write)
    XGpio_SetDataDirection(&gpio_push_buttons,  CHANNEL_PB,     DIRECTION_READ);
    XGpio_SetDataDirection(&gpio_reset,         CHANNEL_RST,       DIRECTION_WRITE);
    XGpio_SetDataDirection(&gpio_switches_leds, CHANNEL_SW,     DIRECTION_READ);
    XGpio_SetDataDirection(&gpio_switches_leds, CHANNEL_LED,    DIRECTION_WRITE);
    
    return ret;
}

/**
 * @brief               Copies a file from the SD card to the specified address.
 * @param filename      is the absolute path to a file on the SD card.
 * @param base_address  is the target address where the data should be written to.
 * @param ascii         If TRUE, the file content will be interpreted as ASCII characters representing
 *                      32-bit numbers in hex format (e.g. 400000f4).
 *                      If FALSE, the file content will be interpreted as binary.
 * @return              The number of written bytes or -1 if an error occurred.
 */
int write_file(const string &filename, u64 base_address, bool ascii) {
    int bytes_written = 0;
    int bytes_read = 0;
    FIL file;
    FIL *file_ptr = &file;

    char asciiBuffer[9];
    u8 binaryBuffer[8];
    
    u64 current_address = base_address;

    // Open file and determine size
    int status = sdcard_open(filename, "r", file_ptr);

    if (status == XST_SUCCESS) {
        int file_size = file.fsize;
        // Read from file and write to address until done
        while (bytes_written < file_size) {
            if (ascii) {
                // ASCII mode - Clear buffer and read next 9 chars
                memset(asciiBuffer, 0, sizeof(asciiBuffer));
                bytes_read = sdcard_read(file_ptr, (u8 *)asciiBuffer, 9);
                if (bytes_read < 0) {
                    sdcard_close(file_ptr);
                    return -1;
                }
                if (bytes_read == 9) {
                    u32 data = strtol(asciiBuffer, NULL, 16);
                    Xil_Out32(current_address, data);
                }
                current_address += 4;
                bytes_written += 4;
            }
            else {
                // BINARY mode - Clear buffer and read next 4 bytes
                memset(binaryBuffer, 0, sizeof(binaryBuffer));
                bytes_read = sdcard_read(file_ptr, (u8 *)binaryBuffer, 8);
                if (bytes_read < 0) {
                    sdcard_close(file_ptr);
                    return -1;
                }
                else if (bytes_read <= 8) {
                    u64 data = endian_swap64(*(u64 *)binaryBuffer);
                    Xil_Out64(current_address, data);
                }
                else {
                    log(DEBUG, "[main] write_file error: Unexpected number of bytes received (%d). It should be 8\n\r", bytes_read);
                    sdcard_close(file_ptr);
                    return -1;
                }
                current_address += bytes_read;
                bytes_written += bytes_read;
            }
        }
        sdcard_close(file_ptr);
    }
    return bytes_written;
}

/**
* @brief            This is the interrupt handler when a GPIO push button has been pressed
* @param callback   is the GPIO callback reference for the handler.
*/
void interrupt_handler(void *callback) {
    // Read the current state of the buttons and clear the interrupt
    button_pressed = XGpio_DiscreteRead((XGpio *)callback, CHANNEL_PB);
    XGpio_InterruptClear((XGpio *)callback, globalIntrMask);

    log(DEBUG, "[main] Received gpio interrupt...\n\r");
}

/**
* @brief            This is the interrupt handler when the state register of the snes controller changes
* @param callback   is the snes callback reference for the handler.
*/
void snes_interrupt_handler(void *callback) {
    // Read the current state of the buttons and clear the interrupt
    snes_buttons_pressed = SNES_mReadReg(XPAR_SNES_0_S00_AXI_BASEADDR, 0);
    XGpio_DiscreteWrite(&gpio_switches_leds, CHANNEL_LED, snes_buttons_pressed>>8);

    log(DEBUG, "[main] Received snes interrupt...\n\r");
}

/**
 * @brief            Switches the high and low 32 bit and perform a endian swap on both parts
 * @param value      is the value that should be modified
 * @return           the provided value in the following format:
 *                   0x0123456789ABCDEF --> 0xEFCDAB8967452301
 */
u64 endian_swap64(u64 value) {
    u32 LoDWord, HiDWord;

    // get each of the half words from the 32 bit word
    LoDWord = (u32) (value & 0x00000000FFFFFFFFU);
    HiDWord = (u32) ((value & 0xFFFFFFFF00000000U) >> 32U);

    // byte swap each of the 32 bit half words
    u32 LoDWord2 = Xil_EndianSwap32(LoDWord);
    u32 HiDWord2 = Xil_EndianSwap32(HiDWord);

    // swap the words before returning the value
    return ((((u64)HiDWord2) << (u64)32U) | (u64)LoDWord2);
}
