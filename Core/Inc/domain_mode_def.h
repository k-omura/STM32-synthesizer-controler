/**
 * @file domain_mode_def.h
 * @author k-omura
 * @brief Mode definition structures and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef DOMAIN_MODE_DEF_H
#define DOMAIN_MODE_DEF_H

#include <stdint.h>
#include "app_event.h"
#include "domain_display_model.h"

typedef enum
{
    NODE_MENU = 0,
    NODE_ACTION,
    NODE_MODE
} node_type_t;

typedef struct
{
    uint8_t source;
    app_event_type_t event_type;
    uint16_t target_id;
} input_map_t;

typedef struct
{
    uint16_t param_id;
    int32_t min;
    int32_t max;
    int32_t step;
} param_set_t;

typedef void (*handler_t)(void *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count);

typedef struct
{
    app_mode_state_t mode_id;
    domain_display_state_t base_display;
    const input_map_t *input_map;
    const param_set_t *param_set;
    handler_t on_enter;
    handler_t on_event;
    handler_t on_tick;
    handler_t on_exit;
} domain_mode_def_t;

const domain_mode_def_t *domain_mode_def_get(app_mode_state_t mode_id);
const domain_mode_def_t *domain_mode_def_get_default(void);
app_mode_state_t domain_mode_def_resolve_mode_for_display(domain_display_state_t state, app_mode_state_t fallback_mode);

#endif
