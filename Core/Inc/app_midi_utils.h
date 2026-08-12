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
void app_midi_allnotesoff(void);

/* ===== API: Channel-specific MIDI ===== */
void app_midi_volume(uint8_t ch, uint8_t volume);
void app_midi_volumeAll(uint8_t volume);
void app_midi_program(uint8_t ch, uint8_t program);
void app_midi_programAll(uint8_t program);
void app_midi_pitch(uint8_t ch, uint8_t pitch);
void app_midi_reverb(uint8_t level);
void app_midi_modeControl(uint8_t mode);

/* ===== API: Application MIDI output policy ===== */
void app_midi_send_control(uint8_t channel, uint8_t cc, uint8_t value);
void app_midi_send_all_notes_off(void);
void app_midi_send_volume(uint8_t channel, uint8_t volume);
void app_midi_send_program(uint8_t channel, uint8_t program);
void app_midi_send_note(uint8_t channel, uint8_t note, uint8_t velocity, uint8_t is_note_on);

#endif /* APP_MIDI_UTILS_H */
