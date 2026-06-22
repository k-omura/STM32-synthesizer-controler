/*
 * stm32_synth_dfu.c
 *
 *  Created on: July 1, 2025
 *      Author: k-omura
 */

#include "stm32_synth_dfu.h"

stm32synth_res_t stm32_synth_goto_dfu(void)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	void (*SysMemBootJump)(void);

	/* Disable all interrupts */
	__disable_irq();

	/* Disable Systick timer */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

	/* Set the clock to the default state */
	HAL_RCC_DeInit();

	/* Clear Interrupt Enable Register & Interrupt Pending Register */
    for (uint16_t i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

	/* Re-enable all interrupts */
	__enable_irq();

	/* Set up the jump to booloader address + 4 */
	SysMemBootJump = (void (*)(void)) (*((uint32_t *) ((SYSTEM_MEMORY_RESET_VECTOR + 4))));

	/* Set the main stack pointer to the bootloader stack */
	__set_MSP(*(uint32_t *)SYSTEM_MEMORY_RESET_VECTOR);

	/* Call the function to jump to bootloader location */
	SysMemBootJump();

	/* Jump is done successfully */
	while (1)
	{
		/* Code should never reach this loop */
	}

	return res;
}
