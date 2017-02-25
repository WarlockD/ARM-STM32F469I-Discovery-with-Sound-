/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>
#include "stm32f4xx_it.h"
#include <callbacks.h>
#include <os\platform\irq.h>
#include <os\debug.h>

extern SAI_HandleTypeDef haudio_out_sai; /* SAI handler declared in "stm32469i_discovery_audio.c" file */
extern I2S_HandleTypeDef haudio_in_i2s; /* I2S handler declared in "stm32469i_discovery_audio.c" file */

extern DMA_HandleTypeDef hdma_usart3_tx;

callback_t irq_callbacks[128];

#if 0
void __loader_start(void);

void nointerrupt(void)
{
	while (1)
		/* wait */ ;
}

void dummy_handler(void)
{
	return;
}

__ISR_VECTOR
void (* const g_pfnVectors[])(void) = {
	/* Core Level - ARM Cortex-M */
	(void *) &stack_end,	/* initial stack pointer */
	__loader_start,		/* reset handler */
	nointerrupt,		/* NMI handler */
	nointerrupt,		/* hard fault handler */
	nointerrupt,		/* MPU fault handler */
	nointerrupt,		/* bus fault handler */
	nointerrupt,		/* usage fault handler */
	0,			/* Reserved */
	0,			/* Reserved */
	0,			/* Reserved */
	0,			/* Reserved */
	nointerrupt,		/* SVCall handler */
	nointerrupt,		/* Debug monitor handler */
	0,			/* Reserved */
	dummy_handler,		/* PendSV handler */
	dummy_handler, 		/* SysTick handler */
	/* Chip Level: vendor specific */
	/* FIXME: use better IRQ vector generator */
#include INC_PLAT(nvic_table.h)
};
#endif
int EmptyCallback(void* udata) {
	(void)udata;
	dbg_panic("No Callback for: %s", dbg_irq_name(__get_IPSR()));
	return 0;
}
//void Default_Handler() __attribute__((naked));
void Default_Handler() {
//	irq_enter();
	volatile uint32_t irq =  __get_IPSR();
	if(irq < 16) {
		dbg_panic("System IRQ called: %s", dbg_irq_name(__get_IPSR()));
		assert(0);
	}
	if(irq >128) {
		dbg_panic("IRQ to big?: %X", irq);
		assert(0);
	}
	callback_t* callback = &irq_callbacks[irq];
	if(!callback->callback){
		dbg_panic("No Callback for: %s", dbg_irq_name(__get_IPSR()));
		assert(0);
	}
	callback->callback(callback->udata);
	//irq_return();
}

int RegesterCallback(IRQn_Type type, int(*func)(void*), void* udata) {
	if(type <0) return 0;
	assert(func);
	safe_lock_begin(_regcallback);
	callback_t* callback = &irq_callbacks[type+16];
	callback->callback = func;
	callback->udata = udata;
	safe_lock_end(_regcallback);
	return 1;
}
void IrqCallbackInits() {
	for(uint32_t i=0;i < 128; i++) {
		irq_callbacks[i].callback = EmptyCallback;
	}
}
void DMA1_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

void AUDIO_SAIx_DMAx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}






/**
  * @brief This function handles DMA1 Stream 2 interrupt request.
  * @param None
  * @retval None
  */
void AUDIO_I2Sx_DMAx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(haudio_in_i2s.hdmarx);
}
void DMA2_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

/**
  * @brief This function handles DMA1 Stream 2 interrupt request.
  * @param None
  * @retval None
  */
void DMA1_Stream2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(haudio_in_i2s.hdmarx);
}
void EXTI2_IRQHandler(void)
{
   HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System service call via SWI instruction.
*/


/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/
void NMI_Handler(void)
{
}
void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}
void HardFault_Handler( void ) __attribute__( ( naked ) );
/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
	/* The prototype shows it is a naked function - in effect this is just an
	assembly function. */


	/* The fault handler implementation calls a function called
	prvGetRegistersFromStack(). */
	__asm volatile
	(
		" tst lr, #4                                                \n"
		" ite eq                                                    \n"
		" mrseq r0, msp                                             \n"
		" mrsne r0, psp                                             \n"
		" ldr r1, [r0, #24]                                         \n"
		" ldr r2, handler2_address_const                            \n"
		" bx r2                                                     \n"
		" handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/* USER CODE BEGIN 1 */
extern LTDC_HandleTypeDef hltdc_eval;
extern DMA2D_HandleTypeDef hdma2d_eval;
extern HCD_HandleTypeDef hhcd;
void LTDC_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&hltdc_eval);
}
void DMA2D_IRQHandler(void) {
	HAL_DMA2D_IRQHandler(&hdma2d_eval);
}
void OTG_HS_IRQHandler(){
	assert("OTG_HS_IRQHandler"==0);
	HAL_HCD_IRQHandler(&hhcd);
}


/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
