
/***************************** Include Files *******************************/
#include "snes.h"
#include "xscugic.h"

/************************** Function Definitions ***************************/

/**
 * @brief               Initializes, configures and enables an interrupt
 * @param intcPtr       is a pointer to the Interrupt Controller Driver Instance
 * @param devId         is a device id from xparameters.h
 * @param intrId        is a XPAR_<INTC_instance>_<SNES_instance>_VEC value from xparameters.h
 * @param intrMask      is the SNES channel mask
 * @param handler       is the method that will be called in case of an interrupt
 * @return              XST_SUCCESS if no error occurred, XST_FAILURE otherwise
 */
int snes_interrupt_init(XScuGic* intcPtr, u16 devId, u16 intrId,
		Xil_InterruptHandler handler) {

	int result;
	XScuGic_Config *intcConfig;

	/* Initialize the interrupt controller driver */
	intcConfig = XScuGic_LookupConfig(devId);
	if (intcConfig == NULL) {
		return XST_FAILURE;
	}
	result = XScuGic_CfgInitialize(intcPtr, intcConfig,
			intcConfig->CpuBaseAddress);
	if (result != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Connect the interrupt handler */
	result = XScuGic_Connect(intcPtr, intrId, handler, intcPtr);
	if (result != XST_SUCCESS) {
		return result;
	}

	/* Enable the interrupt for the SNES device.*/
	XScuGic_Enable(intcPtr, intrId);

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 * */
	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler, (void*) intcPtr);

	/* Enable non-critical exceptions */
	Xil_ExceptionEnable();

	XScuGic_SetPriorityTriggerType(intcPtr, intrId, 0x00, 0x3);

	return XST_SUCCESS;
}

/**
 * @brief            This function disables the interrupts for the SNES
 * @param instPtr    is a pointer to the SNES Driver Instance
 * @param intcPtr    is a pointer to the Interrupt Controller Driver Instance
 * @param intrId     is a XPAR_<INTC_instance>_<SNES_instance>_VEC value from xparameters.h
 * @param intrMask   is the SNES channel mask
 */
void snes_interrupt_disable(XScuGic *intcPtr, u16 intrId, u32 intrMask) {
	XScuGic_Disable(intcPtr, intrId);
	XScuGic_Disconnect(intcPtr, intrId);
}
