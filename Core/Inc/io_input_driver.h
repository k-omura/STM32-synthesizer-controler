/**
 * @file io_input_driver.h
 * @author k-omura
 * @brief Input driver definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef IO_INPUT_DRIVER_H
#define IO_INPUT_DRIVER_H

#include <stdint.h>
#include "main.h"

typedef enum
{
    IO_INPUT_DRIVER_BTN_RELEASE = 0,
    IO_INPUT_DRIVER_BTN_PUSHED,
    IO_INPUT_DRIVER_BTN_PUSHING,
} io_input_driver_btn_state_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;

    io_input_driver_btn_state_t state;
    io_input_driver_btn_state_t prev_state;
    uint32_t tick;
} io_input_driver_btn_t;

typedef struct
{
    GPIO_TypeDef *port[2];
    uint16_t pin[2];

    uint16_t firstpin;
    uint16_t secondpin;

    uint32_t tick;
    int16_t count;
    uint8_t control;
} io_input_driver_rot_t;

/* Callback functions from ISR */
void io_input_driver_exti_callback(uint16_t gpio_pin);
void io_input_driver_timer_callback(void);

/* Main functions */
void io_input_driver_init(uint8_t led_type);
void io_input_driver_handle(uint32_t now_tick);
uint8_t io_input_driver_btn_check(uint8_t encoder_id);

/* Accessor functions */
io_input_driver_btn_t *io_input_driver_get_btn(uint8_t id);
io_input_driver_rot_t *io_input_driver_get_rot(uint8_t id);

#endif
