/**
 * @file domain_midi_state.h
 * @author k-omura
 * @brief MIDI state definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 */

#ifndef DOMAIN_MIDI_STATE_H
#define DOMAIN_MIDI_STATE_H

#include <stdint.h>

/* ===== Global State Structure ===== */
typedef struct
{
    uint8_t reserved;
} domain_midi_state_t;

/* ===== API ===== */
void domain_midi_state_init(void);
domain_midi_state_t *domain_midi_state_get(void);
void domain_midi_state_reset(void);

#endif /* DOMAIN_MIDI_STATE_H */
