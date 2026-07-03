/**
 * @file domain_metronome_engine.c
 * @author k-omura
 * @brief Metronome engine implementation
 * @version 0.1
 * @date 2026-07-02
 */

#include "domain_metronome_engine.h"

static domain_metronome_state_t g_metro;

/**
 * @brief Initializes the metronome engine. This function sets up the initial state of the metronome.
 */
void domain_metronome_engine_init(void)
{
    g_metro.bpm = 120;
    g_metro.last_tick = 0;
}

/**
 * @brief Sets the BPM (beats per minute) of the metronome. This function updates the BPM value.
 * @param bpm The new BPM value.
 */
void domain_metronome_engine_set_bpm(uint16_t bpm)
{
    g_metro.bpm = (bpm == 0U) ? 1U : bpm;
}

/**
 * @brief Checks if the metronome should tick based on the current time. This function updates the metronome state.
 * @param now_tick The current tick value.
 * @return 1 if the metronome ticked, 0 otherwise.
 */
uint8_t domain_metronome_engine_tick(uint32_t now_tick)
{
    uint32_t interval;

    interval = 60000UL / g_metro.bpm;
    if ((now_tick - g_metro.last_tick) >= interval)
    {
        g_metro.last_tick = now_tick;
        return 1;
    }

    return 0;
}

/**
 * @brief Gets the current state of the metronome. This function returns a pointer to the current metronome state.
 * @return Pointer to the current metronome state.
 */
const domain_metronome_state_t *domain_metronome_engine_get_state(void)
{
    return &g_metro;
}
