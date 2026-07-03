/**
 * @file app_midi_constants.h
 * @author k-omura
 * @brief MIDI constants, text tables, and program definitions
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_MIDI_CONSTANTS_H
#define APP_MIDI_CONSTANTS_H

#include <stdint.h>
#include "io_midi_types.h"

/* ===== Color Constants ===== */
typedef struct
{
    uint8_t green, red, blue;
} app_midi_color_t;

extern const app_midi_color_t APP_MIDI_COLOR_RESET;
extern const app_midi_color_t APP_MIDI_COLOR_STRING;
extern const app_midi_color_t APP_MIDI_COLOR_BLE;
extern const app_midi_color_t APP_MIDI_COLOR_BAR_ALL;
extern const app_midi_color_t APP_MIDI_COLOR_CURSORX;
extern const app_midi_color_t APP_MIDI_COLOR_CURSORY;
extern const app_midi_color_t APP_MIDI_COLOR_SEL_BAR;
extern const app_midi_color_t APP_MIDI_COLOR_SEL_CHANNEL;
extern const app_midi_color_t APP_MIDI_COLOR_SEL_PLAY;
extern const app_midi_color_t APP_MIDI_COLOR_SEL_STOP;
extern const app_midi_color_t APP_MIDI_COLOR_SEL_REC;
extern const app_midi_color_t APP_MIDI_COLOR_SEL_RECNOTE[8];

/* ===== Text Tables ===== */
extern const char stm32synth_cc_text[][3];
extern const char stm32synth_prog_text[][6];
extern const char mode_text[][7];

/* ===== MIDI Constants ===== */
#define METRONOME_NOTENUM 37                                      //!< MIDI note number for the metronome click sound
#define METRONOME_VELOCITY 50                                     //!< MIDI velocity for the metronome click sound
#define METRONOME_ACCENT_VELOCITY 90                              //!< MIDI velocity for accented bar-head metronome clicks
#define METRONOME_SUB_VELOCITY 40                                 //!< MIDI velocity for subdivided beat metronome clicks
#define SEQUENCER_MAXBAR IO_MIDI_SEQUENCER_MAXBAR                 //!< Maximum number of bars in the sequencer
#define SEQUENCER_MAXCHORDPARBAR IO_MIDI_SEQUENCER_MAXCHORDPARBAR //!< Maximum number of chord parameters per bar in the sequencer

#endif /* APP_MIDI_CONSTANTS_H */
