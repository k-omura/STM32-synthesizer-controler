/**
 * @file app_midi_display.c
 * @author k-omura
 * @brief
 * @version 0.1
 * @date 2026-06-22
 *
 * @copyright Copyright (c) 2026
 */

#include "app_midi_display.h"

/* ===== Main Render Function ===== */
/**
 * @brief Renders the MIDI display based on the current state. This function should be called periodically (e.g., in a main loop or timer callback) to update the display according to the current mode and state of the MIDI application. The `state` parameter contains all relevant information about the current MIDI state, while `tick` can be used for timing animations or updates. The `ble_connected` flag indicates whether a BLE connection is active, which may affect certain display modes.
 *
 * @param state
 * @param tick
 * @param ble_connected
 */
void app_midi_display_render(domain_midi_state_t *state, uint32_t tick, uint8_t ble_connected)
{
    (void)state;
    (void)tick;
    (void)ble_connected;
}

/* ===== Update Functions (Periodic Callbacks) ===== */
/**
 * @brief Updates the note states for the display. This function should be called periodically to update the visual representation of notes on the display. It checks each note's velocity and count to determine if it should be displayed as active or if it should show a fading animation. The `tick` parameter can be used to time the updates, ensuring that notes fade out after a certain duration.
 *
 * @param state
 * @param tick
 */
void app_midi_display_update_notes(domain_midi_state_t *state, uint32_t tick)
{
    (void)state;
    (void)tick;
}

/**
 * @brief Updates the metronome state for the display. This function should be called periodically to update the metronome's visual representation on the display. It checks the time elapsed since the last metronome tick and updates the metronome count accordingly. This allows for a visual indication of the metronome's timing, which can be used in conjunction with animations or indicators on the display.
 *
 * @param state
 * @param tick
 */
void app_midi_display_update_metronome(domain_midi_state_t *state, uint32_t tick)
{
    (void)state;
    (void)tick;
}
