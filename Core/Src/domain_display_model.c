/**
 * @file domain_display_model.c
 * @author k-omura
 * @brief Display model implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "domain_display_model.h"

static domain_display_model_t g_global_model;
static domain_display_midi_context_t g_midi_context;

/**
 * @brief Initializes the display model. This function sets up the initial state of the display model.
 */
void domain_display_model_init(void)
{
    g_global_model.state = DOMAIN_DISPLAY_NOTE;
    g_global_model.message = 0;
    g_global_model.dirty = 1;

    g_midi_context.valid = 0;
    g_midi_context.midi_msg_type = 0;
    g_midi_context.channel = 0;
    g_midi_context.data1 = 0;
    g_midi_context.data2 = 0;
    g_midi_context.external = 0;
}

/**
 * @brief
 * @param state The new display state to set.
 * @param message The message associated with the new display state.
 */
void domain_display_model_set_state(domain_display_state_t state, int16_t message)
{
    g_global_model.state = state;
    g_global_model.message = message;
    g_global_model.dirty = 1;
}

/**
 * @brief Gets the current state of the display model. This function returns the current state.
 * @return The current display state.
 */
domain_display_state_t domain_display_model_get_state(void)
{
    return g_global_model.state;
}

/**
 * @brief Gets the current message of the display model. This function returns the current message.
 * @return The current display message.
 */
int16_t domain_display_model_get_message(void)
{
    return g_global_model.message;
}

/**
 * @brief Consumes the dirty flag of the display model. This function returns the current dirty flag and clears it.
 * @return The current dirty flag.
 */
uint8_t domain_display_model_consume_dirty(void)
{
    uint8_t dirty = g_global_model.dirty;
    g_global_model.dirty = 0;
    return dirty;
}

/**
 * @brief Gets the current display model. This function copies the current display model to the provided output.
 * @param out_model Pointer to the output display model.
 */
void domain_display_model_get_current(domain_display_model_t *out_model)
{
    if (out_model == 0)
    {
        return;
    }

    *out_model = g_global_model;
}

void domain_display_model_clear_midi_context(void)
{
    g_midi_context.valid = 0;
    g_midi_context.midi_msg_type = 0;
    g_midi_context.channel = 0;
    g_midi_context.data1 = 0;
    g_midi_context.data2 = 0;
    g_midi_context.external = 0;
}

void domain_display_model_set_midi_context(uint8_t midi_msg_type, uint8_t channel, uint8_t data1, uint8_t data2, uint8_t external)
{
    g_midi_context.valid = 1;
    g_midi_context.midi_msg_type = midi_msg_type;
    g_midi_context.channel = channel;
    g_midi_context.data1 = data1;
    g_midi_context.data2 = data2;
    g_midi_context.external = external;
}

void domain_display_model_get_midi_context(domain_display_midi_context_t *out_context)
{
    if (out_context == 0)
    {
        return;
    }

    *out_context = g_midi_context;
}
