/**
 * @file app_runtime.h
 * @author k-omura
 * @brief Runtime context and event handling
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

#include <stdint.h>
#include "app_event.h"

#define APP_EVENT_QUEUE_CAPACITY (32)    //!< Capacity of the event queue
#define APP_COMMAND_BUFFER_CAPACITY (16) //!< Capacity of the command buffer

typedef struct
{
    app_mode_state_t current_mode;
    uint16_t current_node;
    uint32_t now_tick;
} app_runtime_context_t;

void app_runtime_init(void);
void app_runtime_start(void);
void app_runtime_step(uint32_t now_tick);
void app_runtime_post_event(const app_event_t *ev);
const app_runtime_context_t *app_runtime_get_context(void);

/* New-architecture input handling */
void app_runtime_input_init(void);
void app_runtime_handle_input_event(const app_event_t *ev);

#endif
