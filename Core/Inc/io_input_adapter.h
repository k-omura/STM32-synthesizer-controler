/**
 * @file io_input_adapter.h
 * @author k-omura
 * @brief Input adapter definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef IO_INPUT_ADAPTER_H
#define IO_INPUT_ADAPTER_H

#include <stdint.h>
#include "app_event.h"

void io_input_adapter_init(void);
void io_input_adapter_on_exti(uint16_t gpio_pin, uint32_t now_tick);
void io_input_adapter_on_timer(uint32_t now_tick);
uint8_t io_input_adapter_poll(app_event_t *out_event);

#endif
