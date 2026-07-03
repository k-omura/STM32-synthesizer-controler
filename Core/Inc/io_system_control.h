/**
 * @file io_system_control.h
 * @author k-omura
 * @brief System control definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef IO_SYSTEM_CONTROL_H
#define IO_SYSTEM_CONTROL_H

#include <stdint.h>

#define IO_SYSTEM_CONTROL_SYSTEM_MEMORY_RESET_VECTOR (0x1FFF0000) //!< Address of the system memory reset vector for STM32F4 series

typedef enum
{
    IO_SYSTEM_CONTROL_RES_OK = 0,
    IO_SYSTEM_CONTROL_RES_ERROR,
} io_system_control_result_t;

/**
 * Jump to DFU bootloader mode
 * Disables all interrupts and jumps to system memory
 * This function does not return
 */
io_system_control_result_t io_system_control_goto_dfu(void);

#endif /* IO_SYSTEM_CONTROL_H */
