/**
 * @file domain_sequencer_engine.c
 * @author k-omura
 * @brief Sequencer engine implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "domain_sequencer_engine.h"
#include "app_midi_constants.h"
#include <string.h>

static domain_sequencer_state_t g_seq;
static uint32_t g_last_tick;
static uint32_t g_beat_accum;
static uint32_t g_metronome_accum;

static uint32_t domain_sequencer_engine_calc_beat_period(void);

typedef struct
{
    uint8_t valid;
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
    uint8_t layer;
    uint32_t start_tick;
    uint32_t release_tick;
    uint8_t playback_stage;
} domain_sequencer_note_record_t;

static domain_sequencer_note_record_t g_records[SEQUENCER_MAXBAR][SEQUENCER_MAXCHORDPARBAR];
static uint8_t g_playback_bar_index;
static uint8_t g_channel_muted[16];
static uint8_t g_selected_bar;
static uint8_t g_layer_count[SEQUENCER_MAXBAR];

static uint8_t domain_sequencer_engine_recalculate_bar_layer_count(uint8_t bar)
{
    uint16_t i;
    uint8_t max_layer = 0U;

    if (bar >= SEQUENCER_MAXBAR)
    {
        return 0U;
    }

    for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
    {
        if ((g_records[bar][i].valid != 0U) && (g_records[bar][i].layer > max_layer))
        {
            max_layer = g_records[bar][i].layer;
        }
    }

    g_layer_count[bar] = max_layer;
    return max_layer;
}

static uint32_t domain_sequencer_engine_get_bar_duration(void)
{
    uint32_t beat_period = g_seq.beat_period;
    uint8_t beat = (g_seq.beat == 0U) ? 1U : g_seq.beat;

    if (beat_period == 0U)
    {
        beat_period = domain_sequencer_engine_calc_beat_period();
    }
    if (beat_period == 0U)
    {
        beat_period = 1U;
    }

    return beat_period * (uint32_t)beat;
}

static uint32_t domain_sequencer_engine_get_current_bar_position(void)
{
    uint32_t beat_period = g_seq.beat_period;
    uint8_t beat = (g_seq.beat == 0U) ? 1U : g_seq.beat;
    uint32_t pos;
    uint32_t bar_duration;

    if (beat_period == 0U)
    {
        beat_period = domain_sequencer_engine_calc_beat_period();
    }
    if (beat_period == 0U)
    {
        beat_period = 1U;
    }

    pos = ((uint32_t)(g_seq.beat_count % beat) * beat_period) + g_seq.beat_elapsed;
    bar_duration = beat_period * (uint32_t)beat;
    if ((bar_duration != 0U) && (pos >= bar_duration))
    {
        pos = bar_duration - 1U;
    }

    return pos;
}

static uint32_t domain_sequencer_engine_calc_beat_period(void)
{
    uint16_t bpm = (g_seq.bpm == 0U) ? 120U : g_seq.bpm;

    return (uint32_t)((60U * 1000U) / bpm);
}

static void domain_sequencer_engine_reset_bar_playback(uint8_t bar)
{
    uint16_t i;

    if (bar >= SEQUENCER_MAXBAR)
    {
        return;
    }

    for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
    {
        g_records[bar][i].playback_stage = 0U;
    }
}

static void domain_sequencer_engine_reset_all_playback(void)
{
    uint8_t bar;

    for (bar = 0; bar < SEQUENCER_MAXBAR; bar++)
    {
        domain_sequencer_engine_reset_bar_playback(bar);
    }
    g_playback_bar_index = 0xFFU;
}

/**
 * @brief Initializes the sequencer engine. This function sets up the initial state of the sequencer.
 */
void domain_sequencer_engine_init(void)
{
    g_seq.playing = 0;
    g_seq.bpm = 120;
    g_seq.beat = 4;
    g_seq.bar = 1;
    g_seq.beat_count = 0;
    g_seq.bar_count = 0;
    g_seq.beat_elapsed = 0;
    g_seq.beat_period = domain_sequencer_engine_calc_beat_period();
    g_seq.metronome_count = 0;
    g_seq.metronome_elapsed = 0;
    g_seq.tick = 0;
    g_last_tick = 0;
    g_beat_accum = 0;
    g_metronome_accum = 0;
    g_playback_bar_index = 0xFFU;
    memset(g_records, 0, sizeof(g_records));
    memset(g_channel_muted, 0, sizeof(g_channel_muted));
    memset(g_layer_count, 0, sizeof(g_layer_count));
    g_selected_bar = 0U;
}

/**
 * @brief Sets the playing state of the sequencer. This function updates whether the sequencer is currently playing.
 * @param playing The new playing state (1 for playing, 0 for stopped).
 */
void domain_sequencer_engine_set_playing(uint8_t playing)
{
    uint8_t next_playing = (playing != 0U) ? 1U : 0U;

    if ((g_seq.playing == 0U) && (next_playing != 0U))
    {
        g_seq.beat_count = 0;
        g_seq.bar_count = 0;
        g_seq.beat_elapsed = 0;
        g_seq.metronome_count = 0;
        g_seq.metronome_elapsed = 0;
        g_seq.tick = 0;
        g_beat_accum = 0;
        g_metronome_accum = 0;
        g_last_tick = 0;
        domain_sequencer_engine_reset_all_playback();
    }

    g_seq.playing = next_playing;
}

/**
 * @brief Sets the tempo of the sequencer. This function updates the BPM and beat of the sequencer.
 * @param bpm The new BPM (beats per minute) value.
 * @param beat The new beat value.
 */
void domain_sequencer_engine_set_tempo(uint16_t bpm, uint8_t beat)
{
    g_seq.bpm = bpm;
    g_seq.beat = beat;
    g_seq.beat_period = domain_sequencer_engine_calc_beat_period();
}

/**
 * @brief Sets the bar count for the sequencer. This function updates the current bar length.
 * @param bar The new bar count.
 */
void domain_sequencer_engine_set_bar(uint8_t bar)
{
    g_seq.bar = (bar == 0U) ? 1U : bar;
    if (g_seq.bar_count >= g_seq.bar)
    {
        g_seq.bar_count = 0;
    }
    if (g_selected_bar >= g_seq.bar)
    {
        g_selected_bar = (uint8_t)(g_seq.bar - 1U);
    }
}

void domain_sequencer_engine_set_selected_bar(uint8_t bar)
{
    uint8_t bar_len = (g_seq.bar == 0U) ? 1U : g_seq.bar;

    g_selected_bar = (uint8_t)(bar % bar_len);
}

uint8_t domain_sequencer_engine_get_selected_bar(void)
{
    return g_selected_bar;
}

/**
 * @brief Ticks the sequencer engine. This function updates the sequencer state based on the current tick.
 * @param now_tick The current tick value.
 */
void domain_sequencer_engine_tick(uint32_t now_tick)
{
    uint32_t elapsed;
    uint32_t beat_period;
    uint32_t metro_period;

    g_seq.tick = now_tick;
    g_seq.beat_period = domain_sequencer_engine_get_beat_period();
    metro_period = g_seq.beat_period;

    if (metro_period == 0U)
    {
        metro_period = 1U;
    }

    if (g_last_tick == 0U)
    {
        g_last_tick = now_tick;
        g_seq.beat_elapsed = 0;
        g_seq.metronome_elapsed = 0;
        g_seq.beat_period = metro_period;
        g_seq.beat_elapsed = 0;
        return;
    }

    elapsed = now_tick - g_last_tick;
    g_last_tick = now_tick;
    g_metronome_accum += elapsed;
    if (g_seq.playing != 0U)
    {
        g_beat_accum += elapsed;
    }
    g_seq.metronome_elapsed = g_metronome_accum;
    g_seq.beat_elapsed = g_beat_accum;

    while (g_metronome_accum >= metro_period)
    {
        g_metronome_accum -= metro_period;
        g_seq.metronome_count++;
    }

    g_seq.metronome_elapsed = g_metronome_accum;

    if (g_seq.playing == 0U)
    {
        g_seq.beat_elapsed = g_beat_accum;
        g_seq.beat_period = metro_period;
        return;
    }

    beat_period = domain_sequencer_engine_calc_beat_period();
    if (beat_period == 0U)
    {
        g_seq.beat_period = 1U;
        return;
    }

    g_seq.beat_period = beat_period;

    while (g_beat_accum >= beat_period)
    {
        g_beat_accum -= beat_period;
        g_seq.beat_count++;
        if (g_seq.beat != 0U)
        {
            if (g_seq.beat_count >= g_seq.beat)
            {
                g_seq.beat_count = 0;
                g_seq.bar_count++;
                if (g_seq.bar != 0U && g_seq.bar_count >= g_seq.bar)
                {
                    g_seq.bar_count = 0;
                }
            }
        }
    }

    g_seq.beat_elapsed = g_beat_accum;
}

/**
 * @brief Gets the current state of the sequencer. This function returns a pointer to the current sequencer state.
 * @return Pointer to the current sequencer state.
 */
const domain_sequencer_state_t *domain_sequencer_engine_get_state(void)
{
    return &g_seq;
}

uint16_t domain_sequencer_engine_get_bpm(void)
{
    return g_seq.bpm;
}

uint16_t domain_sequencer_engine_get_beat_count(void)
{
    return g_seq.beat_count;
}

uint16_t domain_sequencer_engine_get_metronome_count(void)
{
    return g_seq.metronome_count;
}

uint32_t domain_sequencer_engine_get_beat_elapsed(void)
{
    return g_seq.beat_elapsed;
}

uint32_t domain_sequencer_engine_get_metronome_elapsed(void)
{
    return g_seq.metronome_elapsed;
}

uint32_t domain_sequencer_engine_get_beat_period(void)
{
    return g_seq.beat_period;
}

uint8_t domain_sequencer_engine_collect_playback_events(domain_sequencer_playback_event_t *out_events, uint8_t max_events)
{
    uint8_t out_count = 0U;
    uint16_t i;
    uint8_t current_bar;
    uint32_t current_position;

    if ((out_events == 0) || (max_events == 0U) || (g_seq.playing == 0U))
    {
        return 0U;
    }

    current_bar = (uint8_t)((g_seq.bar == 0U) ? 0U : g_seq.bar_count);
    if (current_bar >= SEQUENCER_MAXBAR)
    {
        current_bar = (uint8_t)(SEQUENCER_MAXBAR - 1U);
    }

    if (g_playback_bar_index != current_bar)
    {
        domain_sequencer_engine_reset_bar_playback(current_bar);
        g_playback_bar_index = current_bar;
    }

    current_position = ((uint32_t)g_seq.beat_count * g_seq.beat_period) + g_seq.beat_elapsed;

    for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
    {
        domain_sequencer_note_record_t *record = &g_records[current_bar][i];

        if (record->valid == 0U)
        {
            continue;
        }

        if ((record->playback_stage == 0U) && (current_position >= record->start_tick))
        {
            if (domain_sequencer_engine_is_channel_muted(record->channel) != 0U)
            {
                record->playback_stage = 2U;
                continue;
            }

            out_events[out_count].type = DOMAIN_SEQUENCER_PLAYBACK_EVENT_NOTEON;
            out_events[out_count].channel = record->channel;
            out_events[out_count].note = record->note;
            out_events[out_count].velocity = record->velocity;
            out_count++;
            record->playback_stage = 1U;

            if (out_count >= max_events)
            {
                break;
            }
        }

        if ((record->playback_stage == 1U) && (record->release_tick != 0U) && (current_position >= record->release_tick))
        {
            out_events[out_count].type = DOMAIN_SEQUENCER_PLAYBACK_EVENT_NOTEOFF;
            out_events[out_count].channel = record->channel;
            out_events[out_count].note = record->note;
            out_events[out_count].velocity = 0U;
            out_count++;
            record->playback_stage = 2U;

            if (out_count >= max_events)
            {
                break;
            }
        }
    }

    return out_count;
}

void domain_sequencer_engine_set_channel_muted(uint8_t channel, uint8_t muted)
{
    g_channel_muted[channel & 0x0FU] = (muted != 0U) ? 1U : 0U;
}

void domain_sequencer_engine_toggle_channel_muted(uint8_t channel)
{
    uint8_t idx = (uint8_t)(channel & 0x0FU);

    g_channel_muted[idx] = (g_channel_muted[idx] == 0U) ? 1U : 0U;
}

uint8_t domain_sequencer_engine_is_channel_muted(uint8_t channel)
{
    return g_channel_muted[channel & 0x0FU];
}

uint8_t domain_sequencer_engine_get_record_position(uint8_t bar, uint16_t index, uint8_t *channel, uint32_t *start_tick)
{
    const domain_sequencer_note_record_t *record;

    if ((bar >= SEQUENCER_MAXBAR) || (index >= SEQUENCER_MAXCHORDPARBAR) || (channel == 0) || (start_tick == 0))
    {
        return 0U;
    }

    record = &g_records[bar][index];
    if (record->valid == 0U)
    {
        return 0U;
    }

    *channel = record->channel;
    *start_tick = record->start_tick;
    return 1U;
}

void domain_sequencer_engine_begin_record_layer(uint8_t bar)
{
    if (bar >= SEQUENCER_MAXBAR)
    {
        return;
    }

    if (g_layer_count[bar] == 0xFFU)
    {
        return;
    }

    g_layer_count[bar]++;
}

void domain_sequencer_engine_record_note_on(uint8_t bar, uint8_t channel, uint8_t note, uint8_t velocity, uint32_t now_tick)
{
    uint16_t i;
    uint32_t position = domain_sequencer_engine_get_current_bar_position();

    (void)now_tick;

    if (bar >= SEQUENCER_MAXBAR)
    {
        return;
    }

    if (g_layer_count[bar] == 0U)
    {
        g_layer_count[bar] = 1U;
    }

    for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
    {
        if (g_records[bar][i].valid == 0U)
        {
            g_records[bar][i].valid = 1U;
            g_records[bar][i].channel = (uint8_t)(channel & 0x0FU);
            g_records[bar][i].note = (uint8_t)(note & 0x7FU);
            g_records[bar][i].velocity = (uint8_t)(velocity & 0x7FU);
            g_records[bar][i].layer = g_layer_count[bar];
            g_records[bar][i].start_tick = position;
            g_records[bar][i].release_tick = 0U;
            g_records[bar][i].playback_stage = 0U;
            domain_sequencer_engine_reset_bar_playback(bar);
            return;
        }
    }

    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].valid = 1U;
    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].channel = (uint8_t)(channel & 0x0FU);
    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].note = (uint8_t)(note & 0x7FU);
    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].velocity = (uint8_t)(velocity & 0x7FU);
    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].layer = g_layer_count[bar];
    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].start_tick = position;
    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].release_tick = 0U;
    g_records[bar][SEQUENCER_MAXCHORDPARBAR - 1U].playback_stage = 0U;
    domain_sequencer_engine_reset_bar_playback(bar);
}

void domain_sequencer_engine_record_note_off(uint8_t bar, uint8_t channel, uint8_t note, uint32_t now_tick)
{
    int16_t i;
    uint32_t release_tick = domain_sequencer_engine_get_current_bar_position();
    uint32_t bar_duration = domain_sequencer_engine_get_bar_duration();

    (void)now_tick;

    if (bar >= SEQUENCER_MAXBAR)
    {
        return;
    }

    for (i = (int16_t)SEQUENCER_MAXCHORDPARBAR - 1; i >= 0; i--)
    {
        if ((g_records[bar][(uint16_t)i].valid != 0U) &&
            (g_records[bar][(uint16_t)i].channel == (uint8_t)(channel & 0x0FU)) &&
            (g_records[bar][(uint16_t)i].note == (uint8_t)(note & 0x7FU)) &&
            (g_records[bar][(uint16_t)i].release_tick == 0U))
        {
            if (release_tick <= g_records[bar][(uint16_t)i].start_tick)
            {
                if (bar_duration > 0U)
                {
                    release_tick = bar_duration - 1U;
                }
                else
                {
                    release_tick = g_records[bar][(uint16_t)i].start_tick + 1U;
                }
            }

            g_records[bar][(uint16_t)i].release_tick = release_tick;
            return;
        }
    }
}

void domain_sequencer_engine_undo(uint8_t bar)
{
    uint16_t i;
    uint8_t target_layer;
    uint8_t removed = 0U;

    if (bar >= SEQUENCER_MAXBAR)
    {
        return;
    }

    target_layer = g_layer_count[bar];
    if (target_layer == 0U)
    {
        target_layer = domain_sequencer_engine_recalculate_bar_layer_count(bar);
        if (target_layer == 0U)
        {
            return;
        }
    }

    for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
    {
        if ((g_records[bar][i].valid != 0U) && (g_records[bar][i].layer == target_layer))
        {
            memset(&g_records[bar][i], 0, sizeof(g_records[bar][i]));
            removed = 1U;
        }
    }

    if (removed != 0U)
    {
        domain_sequencer_engine_reset_bar_playback(bar);
    }

    (void)domain_sequencer_engine_recalculate_bar_layer_count(bar);
}

void domain_sequencer_engine_clear_bar(uint8_t bar)
{
    if (bar >= SEQUENCER_MAXBAR)
    {
        return;
    }

    memset(g_records[bar], 0, sizeof(g_records[bar]));
    g_layer_count[bar] = 0U;
    domain_sequencer_engine_reset_bar_playback(bar);
}

void domain_sequencer_engine_clear_channel_in_bar(uint8_t bar, uint8_t channel)
{
    uint16_t i;

    if (bar >= SEQUENCER_MAXBAR)
    {
        return;
    }

    channel &= 0x0FU;
    for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
    {
        if ((g_records[bar][i].valid != 0U) && (g_records[bar][i].channel == channel))
        {
            memset(&g_records[bar][i], 0, sizeof(g_records[bar][i]));
        }
    }

    domain_sequencer_engine_reset_bar_playback(bar);
    (void)domain_sequencer_engine_recalculate_bar_layer_count(bar);
}

void domain_sequencer_engine_clear_channel(uint8_t channel)
{
    uint16_t bar;
    uint16_t i;

    channel &= 0x0FU;
    for (bar = 0; bar < SEQUENCER_MAXBAR; bar++)
    {
        for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
        {
            if ((g_records[bar][i].valid != 0U) && (g_records[bar][i].channel == channel))
            {
                memset(&g_records[bar][i], 0, sizeof(g_records[bar][i]));
            }
        }
        domain_sequencer_engine_reset_bar_playback((uint8_t)bar);
        (void)domain_sequencer_engine_recalculate_bar_layer_count((uint8_t)bar);
    }
}

void domain_sequencer_engine_clear_all(void)
{
    memset(g_records, 0, sizeof(g_records));
    memset(g_layer_count, 0, sizeof(g_layer_count));
    domain_sequencer_engine_reset_all_playback();
}
