/**
 * @file domain_display_model.h
 * @author k-omura
 * @brief Display model definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef DOMAIN_DISPLAY_MODEL_H
#define DOMAIN_DISPLAY_MODEL_H

#include <stdint.h>

typedef enum
{
    DOMAIN_DISPLAY_NOTE = 0,
    DOMAIN_DISPLAY_METRONOME,
    DOMAIN_DISPLAY_SEQUENCER_PARA,
    DOMAIN_DISPLAY_SEQUENCER_PROGRAM,
    DOMAIN_DISPLAY_SEQUENCER_REC,
    DOMAIN_DISPLAY_SEQUENCER_PLAY,
    DOMAIN_DISPLAY_SETTING,
    DOMAIN_DISPLAY_OVERLAY
} domain_display_state_t;

typedef enum
{
    DOMAIN_OVERLAY_CHANNEL = 1,
    DOMAIN_OVERLAY_VOLUME,
    DOMAIN_OVERLAY_MODE,
    DOMAIN_OVERLAY_MIDI_IN,
    DOMAIN_OVERLAY_CONTROL,
    DOMAIN_OVERLAY_REVERB,
    DOMAIN_OVERLAY_BPM,
    DOMAIN_OVERLAY_SEQ_INPUT,
    DOMAIN_OVERLAY_SEQ_PLAY,
    DOMAIN_OVERLAY_SETTING,
    DOMAIN_OVERLAY_SEQ_INPUT_ACTION,
    DOMAIN_OVERLAY_SEQ_PLAY_ACTION,
    DOMAIN_OVERLAY_SEQ_PARA_SELECT,
    DOMAIN_OVERLAY_PROGRAM,
    DOMAIN_OVERLAY_SEQ_PARA_VALUE,
} domain_overlay_type_t;

typedef struct
{
    domain_display_state_t state;
    int16_t message;
    uint8_t dirty;
} domain_display_model_t;

typedef struct
{
    uint8_t valid;
    uint8_t midi_msg_type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
    uint8_t external;
} domain_display_midi_context_t;

void domain_display_model_init(void);
void domain_display_model_set_state(domain_display_state_t state, int16_t message);
domain_display_state_t domain_display_model_get_state(void);
int16_t domain_display_model_get_message(void);
uint8_t domain_display_model_consume_dirty(void);
void domain_display_model_get_current(domain_display_model_t *out_model);
void domain_display_model_clear_midi_context(void);
void domain_display_model_set_midi_context(uint8_t midi_msg_type, uint8_t channel, uint8_t data1, uint8_t data2, uint8_t external);
void domain_display_model_get_midi_context(domain_display_midi_context_t *out_context);

#endif
