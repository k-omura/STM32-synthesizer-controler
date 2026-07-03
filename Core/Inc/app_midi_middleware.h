/**
 * @file app_midi_middleware.h
 * @author k-omura
 * @brief MIDI middleware module for STM32 synthesizer
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_MIDI_MIDDLEWARE_H
#define APP_MIDI_MIDDLEWARE_H

#include <stdint.h>
#include "main.h"
#include "io_midi_types.h"
#include "io_system_control.h"
#include "io_midi_transport.h"
#include "app_midi_utils.h"

/* ===== Type Definitions ===== */
typedef io_midi_result_t stm32synth_res_t;
typedef io_midi_mode_control_t stm32synth_midi_modecontrol_t;

/* ===== Public API: Main MIDI Handler ===== */
void stm32synth_hundle(SPI_HandleTypeDef *hspi_neopixel, SPI_HandleTypeDef *hspi_synth, uint32_t tick);
/* Compatibility wrappers around the shared MIDI transport queue. */
static inline stm32synth_res_t stm32synth_midi_writebuff(const uint8_t *midi)
{
    io_midi_transport_send_packet(midi);
    return STM32SYNTH_RES_OK;
}

static inline void stm32synth_midi_mode(stm32synth_midi_modecontrol_t mode)
{
    app_midi_modeControl((uint8_t)mode);
}

/* ===== Public API: Display Control ===== */
void stm32synth_disp_mode(uint8_t mode);
void stm32synth_disp_channel(uint8_t ch);
void stm32synth_disp_metronome(uint16_t bpm);
void stm32synth_disp_setting(uint16_t target);

/* ===== Public API: Sequencer Control ===== */
void stm32synth_seq_bpm(uint16_t bpm);
void stm32synth_seq_beat(uint8_t beat);
void stm32synth_seq_bar(uint16_t bar);
void stm32synth_seq_channel(uint8_t ch);
void stm32synth_seq_select(uint8_t select);
void stm32synth_seq_selbar(uint16_t bar, uint8_t mode);
void stm32synth_seq_metronome(uint8_t mode);
void stm32synth_seq_progpush(uint8_t mode);
void stm32synth_seq_set_playing(uint8_t playing);
void stm32synth_seq_undo(void);
void stm32synth_seq_chclear(uint8_t ch);
void stm32synth_seq_cleanrecs(void);
void stm32synth_seq_mutech(uint8_t ch);
void stm32synth_seq_mutechtoggle(uint8_t ch);

/* ===== Public API: MIDI Volume Control ===== */
static inline void stm32synth_midi_volume(uint8_t ch, uint8_t volume)
{
    app_midi_volume(ch, volume);
}

static inline void stm32synth_midi_volumeAll(uint8_t volume)
{
    app_midi_volumeAll(volume);
}

static inline void stm32synth_midi_program(uint8_t ch, uint8_t program)
{
    if (ch != 9U)
    {
        app_midi_program(ch, program);
    }
}

static inline void stm32synth_midi_programAll(uint8_t program)
{
    app_midi_programAll(program);
}

static inline void stm32synth_midi_pitch(uint8_t ch, uint8_t pitch)
{
    app_midi_pitch(ch, pitch);
}

static inline void stm32synth_midi_reverb(uint8_t level)
{
    app_midi_reverb(level);
}

/* ===== Public API: Note On/Off ===== */
static inline void midi_spi_noteon(uint8_t ch, uint8_t notenum, uint8_t velocity)
{
    app_midi_noteon(ch, notenum, velocity);
}

static inline void midi_spi_noteoff(uint8_t ch, uint8_t notenum)
{
    app_midi_noteoff(ch, notenum);
}

static inline void midi_spi_allnoteoff(void)
{
    app_midi_allnotesoff();
}

static inline void midi_play_noteon(uint8_t ch, uint8_t notenum, uint8_t velocity)
{
    app_midi_noteon(ch, notenum, velocity);
}

static inline void midi_play_noteoff(uint8_t ch, uint8_t notenum)
{
    app_midi_noteoff(ch, notenum);
}

/* ===== DFU compatibility wrapper ===== */
static inline stm32synth_res_t stm32_synth_goto_dfu(void)
{
    return (stm32synth_res_t)io_system_control_goto_dfu();
}

#endif /* APP_MIDI_MIDDLEWARE_H */
