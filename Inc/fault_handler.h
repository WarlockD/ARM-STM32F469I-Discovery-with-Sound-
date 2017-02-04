/*
 * fault_handler.h
 *
 *  Created on: Feb 3, 2017
 *      Author: Paul
 */

#ifndef FAULT_HANDLER_H_
#define FAULT_HANDLER_H_

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CPU_ARCH_CORTEX_M3) || defined(CPU_ARCH_CORTEX_M4) || \
    defined(CPU_ARCH_CORTEX_M4F)
#define ARCH_HAS_ATOMIC_COMPARE_AND_SWAP 1
#endif

#define STACK_CANARY_WORD   (0xE7FEE7FEu)
#define CPU_DEFAULT_IRQ_PRIO            (1U)
#define CPU_IRQ_NUMOF                   (82U)
#define CPU_FLASH_BASE                  FLASH_BASE

#ifndef THREAD_EXTRA_STACKSIZE_PRINTF
#define THREAD_EXTRA_STACKSIZE_PRINTF   (512)
#endif
#ifndef THREAD_STACKSIZE_DEFAULT
#define THREAD_STACKSIZE_DEFAULT        (1024)
#endif
#ifndef THREAD_STACKSIZE_IDLE
#define THREAD_STACKSIZE_IDLE           (256)
#endif
/** @} */

/**
 * @brief   Stack size used for the exception (ISR) stack
 * @{
 */
#ifndef ISR_STACKSIZE
#define ISR_STACKSIZE                   (512U)
#endif
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* FAULT_HANDLER_H_ */
