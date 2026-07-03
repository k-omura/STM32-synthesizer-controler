/**
 * @file domain_metronome_engine.h
 * @author k-omura
 * @brief Metronome engine definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef DOMAIN_METRONOME_ENGINE_H
#define DOMAIN_METRONOME_ENGINE_H

#include <stdint.h>

typedef struct
{
    uint16_t bpm;
    uint32_t last_tick;
} domain_metronome_state_t;

void domain_metronome_engine_init(void);
void domain_metronome_engine_set_bpm(uint16_t bpm);
uint8_t domain_metronome_engine_tick(uint32_t now_tick);
const domain_metronome_state_t *domain_metronome_engine_get_state(void);

#endif
