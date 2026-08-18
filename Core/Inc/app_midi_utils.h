/**
 * @file app_midi_utils.h
 * @author k-omura
 * @brief MIDI message generation and buffer utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_MIDI_UTILS_H
#define APP_MIDI_UTILS_H

#include <stdint.h>

/* ===== API: MIDI Message Generation ===== */
void app_midi_noteon(uint8_t ch, uint8_t notenum, uint8_t velocity);
void app_midi_noteoff(uint8_t ch, uint8_t notenum);
void app_midi_controlchange(uint8_t ch, uint8_t cc, uint8_t val);
void app_midi_programchange(uint8_t ch, uint8_t program);
void app_midi_pitchbend(uint8_t ch, uint8_t pitch);

/* ===== API: System Control ===== */
void app_midi_modeControl(uint8_t mode);
void app_midi_send_control(uint8_t channel, uint8_t cc, uint8_t value);

#endif /* APP_MIDI_UTILS_H */
