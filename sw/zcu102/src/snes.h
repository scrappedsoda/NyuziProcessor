
#ifndef SNES_H
#define SNES_H


/****************** Include Files ********************/
#include "xil_types.h"
#include "xstatus.h"
#include "xscugic.h"

#define SNES_S00_AXI_SLV_REG0_OFFSET 0
#define SNES_S00_AXI_SLV_REG1_OFFSET 4
#define SNES_S00_AXI_SLV_REG2_OFFSET 8
#define SNES_S00_AXI_SLV_REG3_OFFSET 12


/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a SNES register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the SNESdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void SNES_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define SNES_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a SNES register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the SNES device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 SNES_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define SNES_mReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))

/************************** Function Prototypes ****************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the SNES instance to be worked on.
 *
 * @return
 *
 *    - XST_SUCCESS   if all self-test code passed
 *    - XST_FAILURE   if any self-test code failed
 *
 * @note    Caching must be turned off for this function to work.
 * @note    Self test may fail if data memory and device are not on the same bus.
 *
 */
XStatus SNES_Reg_SelfTest(void * baseaddr_p);


/**
 * @brief               Initializes, configures and enables an interrupt
 * @param gpioPtr       is a pointer to the SNES Driver Instance
 * @param intcPtr       is a pointer to the Interrupt Controller Driver Instance
 * @param devId         is a device id from xparameters.h
 * @param intrId        is a XPAR_<INTC_instance>_<SNES_instance>_VEC value from xparameters.h
 * @param intrMask      is the SNES channel mask
 * @param handler       is the method that will be called in case of an interrupt
 * @return              XST_SUCCESS if no error occurred, XST_FAILURE otherwise
 */
int snes_interrupt_init(XScuGic* intcPtr,
        u16 devId, u16 intrId, Xil_InterruptHandler handler);

/**
* @brief            This function disables the interrupts for the SNES
* @param intcPtr    is a pointer to the Interrupt Controller Driver Instance
* @param intrId     is a XPAR_<INTC_instance>_<SNES_instance>_VEC value from xparameters.h
* @param intrMask   is the SNES channel mask
*/
void snes_interrupt_disable(XScuGic *intcPtr, u16 intrId, u32 intrMask);

#endif // SNES_H
