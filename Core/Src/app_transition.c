/**
 * @file app_transition.c
 * @author k-omura
 * @brief Transition management implementation
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "app_transition.h"
#include <stdint.h>

#include "app_node_id.h"
#include "io_midi_types.h"

static uint8_t app_transition_is_valid_node(uint16_t node)
{
    switch (node)
    {
    case APP_NODE_ID_NONE:
    case APP_NODE_ID_RE1_MODE:
    case APP_NODE_ID_RE1_VOLUME:
    case APP_NODE_ID_RE1_PROGRAM:
    case APP_NODE_ID_RE2_CHANNEL:
    case APP_NODE_ID_RE2_REVERB:
        return 1U;
    default:
        break;
    }

    /* Backward-compatible acceptance for legacy untagged RE1 values. */
    if (node <= 2U)
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief Transitions to a new mode. This function updates the runtime context to the specified mode.
 * @param ctx Pointer to the runtime context.
 * @param mode The mode to transition to.
 */
void app_transition_to_mode(app_runtime_context_t *ctx, app_mode_state_t mode)
{
    if (ctx == 0)
    {
        return;
    }

    ctx->current_mode = mode;
}

/**
 * @brief Transitions to a new node. This function updates the runtime context to the specified node.
 * @param ctx Pointer to the runtime context.
 * @param node_id The node to transition to.
 */
void app_transition_to_node(app_runtime_context_t *ctx, uint16_t node_id)
{
    if (ctx == 0)
    {
        return;
    }

    ctx->current_node = node_id;
}

/**
 * @brief Resolves the next mode and node based on the current context and event. This function determines the next state of the system.
 * @param ctx Pointer to the runtime context.
 * @param ev Pointer to the event.
 * @param next_mode Pointer to store the next mode.
 * @param next_node Pointer to store the next node.
 */
void app_transition_resolve(const app_runtime_context_t *ctx, const app_event_t *ev, app_mode_state_t *next_mode, uint16_t *next_node)
{
    if ((ctx == 0) || (ev == 0) || (next_mode == 0) || (next_node == 0))
    {
        return;
    }

    *next_mode = ctx->current_mode;
    *next_node = ctx->current_node;

    /* System event path: requested mode is carried in ev->code. */
    if ((ev->source == APP_INPUT_SOURCE_SYSTEM) &&
        (ev->type == EV_SYSTEM_MODE_CHANGE) &&
        (ev->code < IO_MIDI_MODE_STATE_COUNT))
    {
        *next_mode = (app_mode_state_t)ev->code;
    }

    if ((ev->source == APP_INPUT_SOURCE_SYSTEM) &&
        (ev->type == EV_SYSTEM_NODE_CHANGE))
    {
        if (app_transition_is_valid_node(ev->code) != 0U)
        {
            *next_node = ev->code;
        }
    }
}
