/**
 * @file app_input_router.c
 * @brief Input source router for app-layer event dispatch.
 */

#include "app_input_router.h"

uint8_t app_input_router_dispatch_input_event(const app_event_t *ev,
                                              uint8_t effective_mode,
                                              const app_input_router_handlers_t *handlers)
{
    if (ev == 0)
    {
        return 0U;
    }

    if (handlers == 0)
    {
        return 0U;
    }

    switch (ev->source)
    {
    case APP_INPUT_SOURCE_RE1:
        return app_input_router_dispatch_re1_event(ev,
                                                   handlers->re1.rotate,
                                                   handlers->re1.button_down);

    case APP_INPUT_SOURCE_RE2:
        return app_input_router_dispatch_re2_event(ev,
                                                   effective_mode,
                                                   handlers->re2.rotate_modeparam,
                                                   handlers->re2.rotate_normal,
                                                   handlers->re2.button_special,
                                                   handlers->re2.button_modeparam,
                                                   handlers->re2.button_normal,
                                                   handlers->re2.long_press);

    default:
        break;
    }

    return 0U;
}

uint8_t app_input_router_dispatch_re2_event(const app_event_t *ev,
                                            uint8_t effective_mode,
                                            app_input_router_re2_mode_handler_t rotate_modeparam,
                                            app_input_router_re2_normal_handler_t rotate_normal,
                                            app_input_router_re2_mode_handler_t button_special,
                                            app_input_router_re2_mode_handler_t button_modeparam,
                                            app_input_router_re2_normal_handler_t button_normal,
                                            app_input_router_re2_mode_handler_t long_press)
{
    if (ev == 0)
    {
        return 0U;
    }

    if (ev->type == EV_ROTATE)
    {
        if ((rotate_modeparam != 0) && (rotate_modeparam(ev, effective_mode) != 0U))
        {
            return 1U;
        }

        if (rotate_normal != 0)
        {
            (void)rotate_normal(ev);
            return 1U;
        }

        return 0U;
    }

    if (ev->type == EV_BUTTON_DOWN)
    {
        if ((button_special != 0) && (button_special(ev, effective_mode) != 0U))
        {
            return 1U;
        }

        if ((button_modeparam != 0) && (button_modeparam(ev, effective_mode) != 0U))
        {
            return 1U;
        }

        if (button_normal != 0)
        {
            (void)button_normal(ev);
            return 1U;
        }

        return 0U;
    }

    if (ev->type == EV_LONG_PRESS)
    {
        if ((long_press != 0) && (long_press(ev, effective_mode) != 0U))
        {
            return 1U;
        }

        return 0U;
    }

    return 0U;
}

uint8_t app_input_router_dispatch_re1_event(const app_event_t *ev,
                                            app_input_router_re1_normal_handler_t rotate_handler,
                                            app_input_router_re1_normal_handler_t button_down_handler)
{
    if (ev == 0)
    {
        return 0U;
    }

    if (ev->type == EV_ROTATE)
    {
        if (rotate_handler != 0)
        {
            (void)rotate_handler(ev);
            return 1U;
        }

        return 0U;
    }

    if (ev->type == EV_BUTTON_DOWN)
    {
        if (button_down_handler != 0)
        {
            (void)button_down_handler(ev);
            return 1U;
        }

        return 0U;
    }

    return 0U;
}
