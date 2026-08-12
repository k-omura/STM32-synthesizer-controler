/**
 * @file app_input_router.h
 * @brief Input source router for app-layer event dispatch.
 */

#ifndef APP_INPUT_ROUTER_H
#define APP_INPUT_ROUTER_H

#include <stdint.h>

#include "app_event.h"

typedef uint8_t (*app_input_router_re1_normal_handler_t)(const app_event_t *ev);
typedef uint8_t (*app_input_router_re1_mode_handler_t)(const app_event_t *ev, uint8_t effective_mode);
typedef uint8_t (*app_input_router_re2_mode_handler_t)(const app_event_t *ev, uint8_t effective_mode);
typedef uint8_t (*app_input_router_re2_normal_handler_t)(const app_event_t *ev);

typedef struct app_input_router_re1_handlers
{
    app_input_router_re1_normal_handler_t rotate;
    app_input_router_re1_normal_handler_t button_down;
    app_input_router_re1_mode_handler_t long_press;
} app_input_router_re1_handlers_t;

typedef struct app_input_router_re2_handlers
{
    app_input_router_re2_mode_handler_t rotate_modeparam;
    app_input_router_re2_normal_handler_t rotate_normal;
    app_input_router_re2_mode_handler_t button_special;
    app_input_router_re2_mode_handler_t button_modeparam;
    app_input_router_re2_normal_handler_t button_normal;
    app_input_router_re2_mode_handler_t long_press;
} app_input_router_re2_handlers_t;

typedef struct app_input_router_handlers
{
    app_input_router_re1_handlers_t re1;
    app_input_router_re2_handlers_t re2;
} app_input_router_handlers_t;

uint8_t app_input_router_dispatch_input_event(const app_event_t *ev,
                                              uint8_t effective_mode,
                                              const app_input_router_handlers_t *handlers);

uint8_t app_input_router_dispatch_re2_event(const app_event_t *ev,
                                            uint8_t effective_mode,
                                            app_input_router_re2_mode_handler_t rotate_modeparam,
                                            app_input_router_re2_normal_handler_t rotate_normal,
                                            app_input_router_re2_mode_handler_t button_special,
                                            app_input_router_re2_mode_handler_t button_modeparam,
                                            app_input_router_re2_normal_handler_t button_normal,
                                            app_input_router_re2_mode_handler_t long_press);

uint8_t app_input_router_dispatch_re1_event(const app_event_t *ev,
                                            uint8_t effective_mode,
                                            app_input_router_re1_normal_handler_t rotate_handler,
                                            app_input_router_re1_normal_handler_t button_down_handler,
                                            app_input_router_re1_mode_handler_t long_press_handler);

#endif
