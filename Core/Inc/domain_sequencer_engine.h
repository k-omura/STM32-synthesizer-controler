/**
 * @file domain_sequencer_engine.h
 * @author k-omura
 * @brief Sequencer engine definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef DOMAIN_SEQUENCER_ENGINE_H
#define DOMAIN_SEQUENCER_ENGINE_H

#include <stdint.h>

typedef struct
{
    uint8_t playing;
    uint16_t bpm;
    uint8_t beat;
    uint8_t bar;
    uint16_t beat_count;
    uint16_t bar_count;
    uint32_t beat_elapsed;
    uint32_t beat_period;
    uint16_t metronome_count;
    uint32_t metronome_elapsed;
    uint32_t tick;
} domain_sequencer_state_t;

typedef enum
{
    DOMAIN_SEQUENCER_PLAYBACK_EVENT_NOTEON = 0,
    DOMAIN_SEQUENCER_PLAYBACK_EVENT_NOTEOFF,
} domain_sequencer_playback_event_type_t;

typedef struct
{
    domain_sequencer_playback_event_type_t type;
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
} domain_sequencer_playback_event_t;

void domain_sequencer_engine_init(void);
void domain_sequencer_engine_set_playing(uint8_t playing);
void domain_sequencer_engine_set_tempo(uint16_t bpm, uint8_t beat);
void domain_sequencer_engine_set_bar(uint8_t bar);
void domain_sequencer_engine_set_selected_bar(uint8_t bar);
uint8_t domain_sequencer_engine_get_selected_bar(void);
void domain_sequencer_engine_tick(uint32_t now_tick);
const domain_sequencer_state_t *domain_sequencer_engine_get_state(void);
uint16_t domain_sequencer_engine_get_bpm(void);
uint16_t domain_sequencer_engine_get_beat_count(void);
uint16_t domain_sequencer_engine_get_metronome_count(void);
uint32_t domain_sequencer_engine_get_beat_elapsed(void);
uint32_t domain_sequencer_engine_get_metronome_elapsed(void);
uint32_t domain_sequencer_engine_get_beat_period(void);
uint8_t domain_sequencer_engine_collect_playback_events(domain_sequencer_playback_event_t *out_events, uint8_t max_events);
void domain_sequencer_engine_set_channel_muted(uint8_t channel, uint8_t muted);
void domain_sequencer_engine_toggle_channel_muted(uint8_t channel);
uint8_t domain_sequencer_engine_is_channel_muted(uint8_t channel);
uint8_t domain_sequencer_engine_get_record_position(uint8_t bar, uint16_t index, uint8_t *channel, uint32_t *start_tick);
void domain_sequencer_engine_begin_record_layer(uint8_t bar);
void domain_sequencer_engine_record_note_on(uint8_t bar, uint8_t channel, uint8_t note, uint8_t velocity, uint32_t now_tick);
void domain_sequencer_engine_record_note_off(uint8_t bar, uint8_t channel, uint8_t note, uint32_t now_tick);
void domain_sequencer_engine_undo(uint8_t bar);
void domain_sequencer_engine_clear_bar(uint8_t bar);
void domain_sequencer_engine_clear_channel_in_bar(uint8_t bar, uint8_t channel);
void domain_sequencer_engine_clear_channel(uint8_t channel);
void domain_sequencer_engine_clear_all(void);

#endif
