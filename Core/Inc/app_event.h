/**
 * @file app_event.h
 * @author k-omura
 * @brief Event module for handling application events and commands
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_EVENT_H
#define APP_EVENT_H

#include <stdint.h>
#include "domain_display_model.h"

typedef uint8_t app_mode_state_t;

typedef enum
{
    APP_INPUT_SOURCE_NONE = 0,
    APP_INPUT_SOURCE_RE1,
    APP_INPUT_SOURCE_RE2,
    APP_INPUT_SOURCE_USB_MIDI,
    APP_INPUT_SOURCE_SPI_MIDI,
    APP_INPUT_SOURCE_SYSTEM
} app_input_source_t;

typedef enum
{
    EV_NONE = 0,
    EV_ROTATE,
    EV_BUTTON_DOWN,
    EV_BUTTON_UP,
    EV_LONG_PRESS,
    EV_TICK,
    EV_SYSTEM_MODE_CHANGE,
    EV_SYSTEM_NODE_CHANGE
} app_event_type_t;

typedef struct
{
    app_event_type_t type;
    app_input_source_t source;
    int16_t delta;
    uint16_t code;
    uint32_t tick;
} app_event_t;

typedef enum
{
    CMD_NONE = 0,
    CMD_MIDI_SEND,
    CMD_DISPLAY_UPDATE,
    CMD_MODE_CHANGE,
    CMD_NODE_CHANGE
} app_command_type_t;

typedef struct
{
    app_command_type_t type;
    union
    {
        uint8_t midi[4];
        domain_display_state_t display;
        app_mode_state_t mode;
        uint16_t node;
    } payload;
} app_command_t;

#endif
