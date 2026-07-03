/**
 * @file app_dispatcher.h
 * @author k-omura
 * @brief Dispatcher module for handling application events and ticks
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_DISPATCHER_H
#define APP_DISPATCHER_H

#include <stdint.h>
#include "app_event.h"

typedef struct app_runtime_context app_runtime_context_t;

void app_dispatcher_handle_event(const app_runtime_context_t *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count);
void app_dispatcher_handle_tick(const app_runtime_context_t *ctx, uint32_t now_tick, app_command_t *out_cmds, uint8_t *out_count);

#endif
