/*
 * gpio.h - This header file defines functions for GPIO interrupt handling
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Markus Kessler (1225380)
 *      Mail:   markus.kessler@student.tuwien.ac.at
 */

#ifndef GPIO_H
#define GPIO_H

#include "xgpio.h"
#include "xscugic.h"

// PROTOTYPES

/**
 * @brief               Initializes, configures and enables an interrupt
 * @param gpioPtr       is a pointer to the GPIO Driver Instance
 * @param intcPtr       is a pointer to the Interrupt Controller Driver Instance
 * @param devId         is a device id from xparameters.h
 * @param intrId        is a XPAR_<INTC_instance>_<GPIO_instance>_VEC value from xparameters.h
 * @param intrMask      is the GPIO channel mask
 * @param handler       is the method that will be called in case of an interrupt
 * @return              XST_SUCCESS if no error occurred, XST_FAILURE otherwise
 */
int gpio_interrupt_init(XGpio* gpioPtr, XScuGic* intcPtr, u16 devId, u16 intrId, u32 intrMask, Xil_InterruptHandler handler);

/**
* @brief            This function disables the interrupts for the GPIO
* @param instPtr    is a pointer to the GPIO Driver Instance
* @param intcPtr    is a pointer to the Interrupt Controller Driver Instance
* @param intrId     is a XPAR_<INTC_instance>_<GPIO_instance>_VEC value from xparameters.h
* @param intrMask   is the GPIO channel mask
*/
void gpio_interrupt_disable(XGpio *instPtr, XScuGic *intcPtr, u16 intrId, u32 intrMask);

#endif
