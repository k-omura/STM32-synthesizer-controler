/**
 * @file app_transition.h
 * @author k-omura
 * @brief Transition handling between modes and nodes
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_TRANSITION_H
#define APP_TRANSITION_H

#include <stdint.h>
#include "app_runtime.h"

void app_transition_to_mode(app_runtime_context_t *ctx, app_mode_state_t mode);
void app_transition_to_node(app_runtime_context_t *ctx, uint16_t node_id);
void app_transition_resolve(const app_runtime_context_t *ctx, const app_event_t *ev, app_mode_state_t *next_mode, uint16_t *next_node);

#endif
