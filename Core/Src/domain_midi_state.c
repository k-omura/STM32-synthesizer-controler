/**
 * @file domain_midi_state.c
 * @author k-omura
 * @brief Unified MIDI state management implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "domain_midi_state.h"
static domain_midi_state_t g_midi_state = {0};

/**
 * @brief Initializes the MIDI state. This function sets up the initial state of the MIDI system.
 */
void domain_midi_state_init(void)
{
    g_midi_state.reserved = 0U;
}

/**
 * @brief Gets the current MIDI state. This function returns a pointer to the current MIDI state.
 * @return Pointer to the current MIDI state.
 */
domain_midi_state_t *domain_midi_state_get(void)
{
    return &g_midi_state;
}

/**
 * @brief Resets the MIDI state. This function clears the current MIDI state and reinitializes it.
 */
void domain_midi_state_reset(void)
{
    domain_midi_state_init();
}
