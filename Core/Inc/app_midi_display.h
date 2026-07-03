/**
 * @file app_midi_display.h
 * @author k-omura
 * @brief MIDI display and UI rendering module
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_MIDI_DISPLAY_H
#define APP_MIDI_DISPLAY_H

#include <stdint.h>
#include "domain_midi_state.h"

/* ===== Display Mode IDs ===== */
enum
{
    APP_MIDI_DISP_HELLO = 0,
    APP_MIDI_DISP_MODESEL,
    APP_MIDI_DISP_NOTE,
    APP_MIDI_DISP_METRONOME,
    APP_MIDI_DISP_SEQ_PARA,
    APP_MIDI_DISP_SEQ_INPUT,
    APP_MIDI_DISP_SEQ_PLAY,
    APP_MIDI_DISP_CHANNEL,
    APP_MIDI_DISP_MIDIIN,
    APP_MIDI_DISP_SETTING,
};

/* ===== API ===== */
/**
 * Legacy compatibility stub
 * @param state unused compatibility state structure
 * @param tick current tick
 * @param ble_connected BLE connection status
 */
void app_midi_display_render(domain_midi_state_t *state, uint32_t tick, uint8_t ble_connected);

/**
 * Update note display refresh (called periodically)
 * @param state MIDI state structure
 * @param tick current tick
 */
void app_midi_display_update_notes(domain_midi_state_t *state, uint32_t tick);

/**
 * Update metronome display (called periodically)
 * @param state MIDI state structure
 * @param tick current tick
 */
void app_midi_display_update_metronome(domain_midi_state_t *state, uint32_t tick);

#endif /* APP_MIDI_DISPLAY_H */
