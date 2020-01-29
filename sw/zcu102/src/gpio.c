/*
 * gpio.c - Implementation of the GPIO interrupt handling
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Markus Kessler (1225380)
 *      Mail:   markus.kessler@student.tuwien.ac.at
 */

#include "gpio.h"

/* The current and previous button press */
u8 button_pressed = 0;
u8 button_pressed_last = 0;
u8 button_activation = 0;

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
int gpio_interrupt_init(XGpio* gpioPtr, XScuGic* intcPtr,
        u16 devId, u16 intrId, u32 intrMask, Xil_InterruptHandler handler) {

    int result;
    XScuGic_Config *intcConfig;

    /* Initialize the interrupt controller driver */
    intcConfig = XScuGic_LookupConfig(devId);
    if (intcConfig == NULL) {
        return XST_FAILURE;
    }
    result = XScuGic_CfgInitialize(intcPtr, intcConfig, intcConfig->CpuBaseAddress);
    if (result != XST_SUCCESS) {
        return XST_FAILURE;
    }
    XScuGic_SetPriorityTriggerType(intcPtr, intrId, 0xA0, 0x3);

    /* Connect the interrupt handler */
    result = XScuGic_Connect(intcPtr, intrId, handler, gpioPtr);
    if (result != XST_SUCCESS) {
        return result;
    }

    /* Enable the interrupt for the GPIO device.*/
    XScuGic_Enable(intcPtr, intrId);

    /* Enable interrupts for the specified channels from the GPIO device */
    XGpio_InterruptEnable(gpioPtr, intrMask);
    XGpio_InterruptGlobalEnable(gpioPtr);

    /*
     * Initialize the exception table and register the interrupt
     * controller handler with the exception table
     */
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
             (Xil_ExceptionHandler)XScuGic_InterruptHandler, intcPtr);

    /* Enable non-critical exceptions */
    Xil_ExceptionEnable();

    return XST_SUCCESS;
}

/**
* @brief            This function disables the interrupts for the GPIO
* @param instPtr    is a pointer to the GPIO Driver Instance
* @param intcPtr    is a pointer to the Interrupt Controller Driver Instance
* @param intrId     is a XPAR_<INTC_instance>_<GPIO_instance>_VEC value from xparameters.h
* @param intrMask   is the GPIO channel mask
*/
void gpio_interrupt_disable(XGpio *instPtr, XScuGic *intcPtr, u16 intrId, u32 intrMask) {
    XGpio_InterruptDisable(instPtr, intrMask);
    XScuGic_Disable(intcPtr, intrId);
    XScuGic_Disconnect(intcPtr, intrId);
}




