/******************************************************************************
* Copyright (c) 2010 - 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
*
* @file xil_types.h
*
* @addtogroup common_types Basic Data types for Xilinx&reg; Software IP
*
* The xil_types.h file contains basic types for Xilinx software IP. These data types
* are applicable for all processors supported by Xilinx.
* @{
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date   Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00a hbm  07/14/09 First release
* 3.03a sdm  05/30/11 Added Xuint64 typedef and XUINT64_MSW/XUINT64_LSW macros
* 5.00  pkp  05/29/14 Made changes for 64 bit architecture
*   srt  07/14/14 Use standard definitions from stdint.h and stddef.h
*             Define LONG and ULONG datatypes and mask values
* 7.00  mus  01/07/19 Add cpp extern macro
* 7.1   aru  08/19/19 Shift the value in UPPER_32_BITS only if it
*                     is 64-bit processor
* </pre>
*
******************************************************************************/

/**
 *@cond nocomments
 */
#ifndef XIL_TYPES_H /* prevent circular inclusions */
#define XIL_TYPES_H /* by using protection macros */

/************************** Constant Definitions *****************************/

#ifndef TRUE
#  define TRUE      1U
#endif

#ifndef FALSE
#  define FALSE     0U
#endif

#ifndef NULL
#define NULL        0U
#endif

#define XIL_COMPONENT_IS_READY     0x11111111U  /**< In device drivers, This macro will be
                                                 assigend to "IsReady" member of driver
                                                 instance to indicate that driver
                                                 instance is initialized and ready to use. */
#define XIL_COMPONENT_IS_STARTED   0x22222222U  /**< In device drivers, This macro will be assigend to
                                                 "IsStarted" member of driver instance
                                                 to indicate that driver instance is
                                                 started and it can be enabled. */


#include <linux/types.h>

typedef long INTPTR;
typedef uintptr_t UINTPTR;
typedef char char8;

#define ULONG64_HI_MASK 0xFFFFFFFF00000000U
#define ULONG64_LO_MASK ~ULONG64_HI_MASK


/** @{ */
/**
 * This data type defines an interrupt handler for a device.
 * The argument points to the instance of the component
 */
typedef void (*XInterruptHandler) (void *InstancePtr);

/**
 * This data type defines an exception handler for a processor.
 * The argument points to the instance of the component
 */
typedef void (*XExceptionHandler) (void *InstancePtr);

/**
 * @brief  Returns 32-63 bits of a number.
 * @param  n : Number being accessed.
 * @return Bits 32-63 of number.
 *
 * @note    A basic shift-right of a 64- or 32-bit quantity.
 *          Use this to suppress the "right shift count >= width of type"
 *          warning when that quantity is 32-bits.
 */
#if defined (__aarch64__) || defined (__arch64__)
#define UPPER_32_BITS(n) ((u32)(((n) >> 16) >> 16))
#else
#define UPPER_32_BITS(n) 0U
#endif
/**
 * @brief  Returns 0-31 bits of a number
 * @param  n : Number being accessed.
 * @return Bits 0-31 of number
 */
#define LOWER_32_BITS(n) ((u32)(n))




/************************** Constant Definitions *****************************/

#ifndef TRUE
#define TRUE        1U
#endif

#ifndef FALSE
#define FALSE       0U
#endif

#ifndef NULL
#define NULL        0U
#endif

#endif  /* end of protection macro */
/**
 *@endcond
 */
/**
* @} End of "addtogroup common_types".
*/
