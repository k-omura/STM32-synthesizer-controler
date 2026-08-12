/**
 * @file io_system_control.c
 * @author k-omura
 * @brief System control implementation for STM32 microcontroller.
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "io_system_control.h"
#include "io_display_driver.h"
#include "main.h"

/**
 * @brief Jumps to the DFU (Device Firmware Upgrade) bootloader.
 * @return Result of the operation.
 */
io_system_control_result_t io_system_control_goto_dfu(void)
{
    io_system_control_result_t res = IO_SYSTEM_CONTROL_RES_OK;
    void (*SysMemBootJump)(void);

    io_display_driver_shutdown();

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
    SysMemBootJump = (void (*)(void))(*((uint32_t *)((IO_SYSTEM_CONTROL_SYSTEM_MEMORY_RESET_VECTOR + 4))));

    /* Set the main stack pointer to the bootloader stack */
    __set_MSP(*(uint32_t *)IO_SYSTEM_CONTROL_SYSTEM_MEMORY_RESET_VECTOR);

    /* Call the function to jump to bootloader location */
    SysMemBootJump();

    /* Jump is done successfully */
    while (1)
    {
        /* Code should never reach this loop */
    }

    return res;
}
