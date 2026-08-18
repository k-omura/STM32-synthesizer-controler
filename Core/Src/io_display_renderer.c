/**
 * @file io_display_renderer.c
 * @author k-omura
 * @brief Display renderer implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "io_display_renderer.h"

#include <stdio.h>
#include <string.h>

#include "app_midi_constants.h"
#include "domain_display_model.h"
#include "domain_sequencer_engine.h"
#include "io_display_driver.h"
#include "io_midi_types.h"

#define IO_DISPLAY_MIDI_CHANNEL_COUNT 16U
#define IO_DISPLAY_MIDI_CHANNEL_MAX (IO_DISPLAY_MIDI_CHANNEL_COUNT - 1U)
#define IO_DISPLAY_NOTE_COUNT 128U
#define IO_DISPLAY_NOTE_RING_MAX 8U
#define IO_DISPLAY_NOTE_RING_STEP_MS 30UL
#define IO_DISPLAY_BOOT_LOGO_DURATION_MS 2500UL
#define IO_DISPLAY_BOOT_LOGO_PHASE_MS 1250UL

typedef struct
{
    const char *text;
    uint8_t len;
} io_display_text_entry_t;

typedef struct
{
    uint8_t active;
    uint8_t ring;
    io_display_driver_color_t color;
    uint32_t ring_tick;
} io_display_note_anim_t;

typedef enum
{
    IO_DISPLAY_SEQPARA_BPM = 0,
    IO_DISPLAY_SEQPARA_BEAT,
    IO_DISPLAY_SEQPARA_BAR,
    IO_DISPLAY_SEQPARA_CHANNEL,
    IO_DISPLAY_SEQPARA_METRONOME,
} io_display_seqpara_kind_t;

typedef void (*io_display_seqpara_value_draw_fn_t)(uint16_t para_val, io_display_driver_color_t *col_text);

static SPI_HandleTypeDef *g_hspi_neopixel;
static uint8_t g_seqpara_channel = 0U;
static uint8_t g_seqplay_mode = 0U;
static uint8_t g_seqinput_target_bar = 0U;
static uint32_t g_boot_logo_start_tick = 0UL;
static io_display_note_anim_t g_note_anim[IO_DISPLAY_NOTE_COUNT];

static const io_display_text_entry_t g_seq_input_action_texts[] = {
    {"UNDO", 4},
    {"CH CLR", 6},
    {"BARCLR", 6},
    {"ALLCLR", 6},
};

static const io_display_text_entry_t g_seqpara_select_texts[] = {
    {"BPM", 3},
    {"BEAT", 4},
    {"BAR", 3},
    {"CHANL", 5},
    {"MTRON", 5},
};

static const io_display_text_entry_t g_seq_metronome_texts[] = {
    {"MTROFF", 6},
    {"MTRON", 5},
};

static const io_display_text_entry_t g_control_hint_texts[] = {
    {"", 0},
    {"MODE", 4},
    {"VOLUME", 6},
    {"PROGRAM", 7},
    {"CHANNEL", 7},
    {"REVERB", 6},
    {"MUTE ON", 7},
    {"MUTEOFF", 7},
};

/**
 * @brief Get the note color from the velocity.
 *
 * @param velocity The velocity of the note.
 * @return The corresponding display driver color.
 */
static io_display_driver_color_t io_display_renderer_note_color_from_velocity(uint8_t velocity)
{
    io_display_driver_color_t out;

    out.green = (uint8_t)(velocity & 0x03U);
    out.red = (uint8_t)((velocity >> 2) & 0x03U);
    out.blue = (uint8_t)((velocity >> 4) & 0x03U);

    if ((out.green == 0U) && (out.red == 0U) && (out.blue == 0U))
    {
        out.green = 1U;
    }

    return out;
}

/**
 * @brief Update the note animation state.
 */
static void io_display_renderer_update_note_animation(void)
{
    domain_display_midi_context_t midi_ctx;
    io_display_note_anim_t *anim;
    uint8_t note;
    uint32_t now_tick = HAL_GetTick();

    domain_display_model_get_midi_context(&midi_ctx);
    if ((midi_ctx.valid != 0U) && ((midi_ctx.midi_msg_type == IO_MIDI_EVT0_NOTEON) || (midi_ctx.midi_msg_type == IO_MIDI_EVT0_NOTEOFF)))
    {
        note = midi_ctx.data1;
        if (note < IO_DISPLAY_NOTE_COUNT)
        {
            anim = &g_note_anim[note];
            anim->ring = 1U;
            anim->ring_tick = now_tick;

            if ((midi_ctx.midi_msg_type == IO_MIDI_EVT0_NOTEON) && (midi_ctx.data2 != 0U))
            {
                anim->active = 1U;
                anim->color = io_display_renderer_note_color_from_velocity(midi_ctx.data2);
            }
            else
            {
                anim->active = 0U;
            }
        }

        domain_display_model_clear_midi_context();
    }
}

/**
 * @brief Draw the note on the display.
 *
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_note(io_display_driver_color_t *col_text)
{
    uint32_t now_tick = HAL_GetTick();

    io_display_renderer_update_note_animation();

    for (uint16_t i = 0; i < IO_DISPLAY_NOTE_COUNT; i++)
    {
        io_display_note_anim_t *anim = &g_note_anim[i];
        int16_t center_x = (int16_t)(i % 32U);
        int16_t center_y = (int16_t)(2U + (i / 32U));
        uint8_t ring_len;

        while ((anim->ring > 0U) && (anim->ring < IO_DISPLAY_NOTE_RING_MAX) && ((now_tick - anim->ring_tick) >= IO_DISPLAY_NOTE_RING_STEP_MS))
        {
            anim->ring++;
            anim->ring_tick += IO_DISPLAY_NOTE_RING_STEP_MS;
        }

        if (anim->ring >= IO_DISPLAY_NOTE_RING_MAX)
        {
            anim->ring = 0U;
        }

        if (anim->active != 0U)
        {
            io_display_driver_pixel(center_x, center_y, &anim->color);
        }

        if (anim->ring > 0U)
        {
            ring_len = (uint8_t)(1U + (2U * anim->ring));
            io_display_driver_ybar(center_x, (int16_t)(center_y - (int16_t)anim->ring), ring_len, &anim->color);
        }
    }

    (void)col_text;
}

/**
 * @brief Convert a MIDI color to a display driver color.
 *
 * @param c The MIDI color.
 * @return The corresponding display driver color.
 */
static io_display_driver_color_t io_display_renderer_color_from_midi(app_midi_color_t c)
{
    io_display_driver_color_t out;

    out.green = c.green;
    out.red = c.red;
    out.blue = c.blue;
    return out;
}

/**
 * @brief Draw the BLE indicator on the display.
 */
static void io_display_renderer_draw_ble_indicator(void)
{
    if (HAL_GPIO_ReadPin(bleConnect_GPIO_Port, bleConnect_Pin) == GPIO_PIN_SET)
    {
        io_display_driver_color_t col_ble = io_display_renderer_color_from_midi(APP_MIDI_COLOR_BLE);
        io_display_driver_pixel(31, 7, &col_ble);
    }
}

/**
 * @brief Draw the sequencer channel persistent graphics on the display with a specified lane length.
 *
 * @param ch The channel to draw.
 * @param lane_length The length of the channel lane.
 * @param col_lane The color of the lane.
 * @param col_bank_dot The color of the bank dot.
 */
static void io_display_renderer_draw_seq_channel_lane_limited(uint8_t ch, uint8_t lane_length, io_display_driver_color_t *col_lane, io_display_driver_color_t *col_bank_dot)
{
    uint8_t row = (uint8_t)(ch % 8U);
    uint8_t bank_dot_y = (uint8_t)((ch < 8U) ? 0U : 1U);

    io_display_driver_xbar(0, row, lane_length, col_lane);
    io_display_driver_pixel(7, (int16_t)bank_dot_y, col_bank_dot);
}

/**
 * @brief Draw the recorded notes for the sequencer on the display.
 *
 * @param state The current display state.
 * @param bar_index The index of the bar to draw.
 * @param bar_period The period of the bar.
 */
static void io_display_renderer_draw_sequencer_recorded_notes(domain_display_state_t state, uint8_t bar_index, uint32_t bar_period)
{
    uint16_t i;
    uint8_t rec_ch;
    uint32_t rec_start_tick;
    uint8_t selected_ch = 0xFFU;
    uint8_t selected_bank = 0U;
    uint8_t use_bank_filter = 0U;

    if (bar_index >= SEQUENCER_MAXBAR)
    {
        return;
    }

    if ((state == DOMAIN_DISPLAY_SEQUENCER_PARA) ||
        (state == DOMAIN_DISPLAY_SEQUENCER_PROGRAM) ||
        (state == DOMAIN_DISPLAY_SEQUENCER_REC))
    {
        selected_ch = g_seqpara_channel;
        selected_bank = (selected_ch < 8U) ? 0U : 1U;
        use_bank_filter = 1U;
    }
    else if ((state == DOMAIN_DISPLAY_SEQUENCER_PLAY) && (g_seqplay_mode > 0U))
    {
        selected_ch = (uint8_t)(g_seqplay_mode - 1U);
        selected_bank = (selected_ch < 8U) ? 0U : 1U;
        use_bank_filter = 1U;
    }

    for (i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
    {
        io_display_driver_color_t col_temp;
        uint8_t draw_x;

        if (domain_sequencer_engine_get_record_position(bar_index, i, &rec_ch, &rec_start_tick) == 0U)
        {
            continue;
        }

        if (use_bank_filter != 0U)
        {
            if ((selected_bank == 0U) && (rec_ch >= 8U))
            {
                continue;
            }
            if ((selected_bank != 0U) && (rec_ch < 8U))
            {
                continue;
            }
        }

        col_temp = io_display_renderer_color_from_midi(APP_MIDI_COLOR_SEL_RECNOTE[rec_ch % 8U]);

        if (domain_sequencer_engine_is_channel_muted(rec_ch) != 0U)
        {
            col_temp.green >>= 2;
            col_temp.red >>= 2;
            col_temp.blue >>= 2;
        }
        else if ((selected_ch != 0xFFU) && (rec_ch == selected_ch))
        {
            col_temp.green <<= 2;
            col_temp.red <<= 2;
            col_temp.blue <<= 2;
        }

        if (rec_start_tick > bar_period)
        {
            rec_start_tick = bar_period;
        }

        draw_x = (uint8_t)(9U + ((22UL * rec_start_tick) / bar_period));
        if (draw_x > 31U)
        {
            draw_x = 31U;
        }
        io_display_driver_pixel((int16_t)draw_x, (int16_t)(rec_ch % 8U), &col_temp);
    }
}

/**
 * @brief Draw the sequencer channel persistent graphics on the display with a specified lane length.
 *
 * @param state The current display state.
 * @param lane_length The length of the channel lane.
 */
static void io_display_renderer_draw_seq_channel_persistent_with_length(domain_display_state_t state, uint8_t lane_length)
{
    io_display_driver_color_t col_lane = io_display_renderer_color_from_midi(APP_MIDI_COLOR_CURSORY);
    io_display_driver_color_t col_sel_channel = io_display_renderer_color_from_midi(APP_MIDI_COLOR_SEL_CHANNEL);

    if ((state == DOMAIN_DISPLAY_SEQUENCER_PARA) ||
        (state == DOMAIN_DISPLAY_SEQUENCER_PROGRAM) ||
        (state == DOMAIN_DISPLAY_SEQUENCER_REC))
    {
        io_display_renderer_draw_seq_channel_lane_limited(g_seqpara_channel, lane_length, &col_lane, &col_sel_channel);
    }
    else if ((state == DOMAIN_DISPLAY_SEQUENCER_PLAY) && (g_seqplay_mode > 0U))
    {
        io_display_renderer_draw_seq_channel_lane_limited((uint8_t)(g_seqplay_mode - 1U), lane_length, &col_lane, &col_sel_channel);
    }
}

/**
 * @brief Draw the sequencer channel persistent graphics on the display.
 *
 * @param state The current display state.
 */
static void io_display_renderer_draw_seq_channel_persistent(domain_display_state_t state)
{
    io_display_renderer_draw_seq_channel_persistent_with_length(state, 32U);
}

/**
 * @brief Draw the sequencer parameter BPM value on the display.
 *
 * @param para_val The BPM parameter value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_seqpara_value_with_format(const char *label, uint8_t label_len, uint8_t value_width, uint16_t para_val, io_display_driver_color_t *col_text)
{
    char text[12];

    snprintf(text, sizeof(text), "%.*s%*u", label_len, label, value_width, para_val);
    io_display_driver_string(9, 1, text, (uint8_t)strlen(text), col_text, 0);
}

static void io_display_renderer_draw_seqpara_value_bpm(uint16_t para_val, io_display_driver_color_t *col_text)
{
    io_display_renderer_draw_seqpara_value_with_format("BPM", 3U, 3U, para_val, col_text);
}

/**
 * @brief Draw the sequencer parameter BPM value on the display.
 *
 * @param para_val The BPM parameter value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_seqpara_value_beat(uint16_t para_val, io_display_driver_color_t *col_text)
{
    io_display_renderer_draw_seqpara_value_with_format("BEAT", 4U, 2U, para_val, col_text);
}

/**
 * @brief Draw the sequencer parameter bar value on the display.
 *
 * @param para_val The bar parameter value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_seqpara_value_bar(uint16_t para_val, io_display_driver_color_t *col_text)
{
    io_display_renderer_draw_seqpara_value_with_format("BAR ", 4U, 2U, para_val, col_text);
}

/**
 * @brief Draw the sequencer parameter channel value on the display.
 *
 * @param para_val The channel parameter value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_seqpara_value_channel(uint16_t para_val, io_display_driver_color_t *col_text)
{
    uint8_t ch = (uint8_t)((para_val > 0U) ? (para_val - 1U) : 0U);

    g_seqpara_channel = ch;

    (void)col_text;
}

/**
 * @brief Draw the sequencer parameter metronome value on the display.
 *
 * @param para_val The metronome parameter value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_seqpara_value_metronome(uint16_t para_val, io_display_driver_color_t *col_text)
{
    const io_display_text_entry_t *entry = &g_seq_metronome_texts[(para_val == 0U) ? 0U : 1U];

    io_display_driver_string(9, 1, entry->text, entry->len, col_text, 0);
}

static const io_display_seqpara_value_draw_fn_t g_seqpara_value_draw_table[] = {
    io_display_renderer_draw_seqpara_value_bpm,
    io_display_renderer_draw_seqpara_value_beat,
    io_display_renderer_draw_seqpara_value_bar,
    io_display_renderer_draw_seqpara_value_channel,
    io_display_renderer_draw_seqpara_value_metronome,
};

/**
 * @brief Draw the metronome on the display.
 *
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_metronome(io_display_driver_color_t *col_text)
{
    uint32_t beat_period = domain_sequencer_engine_get_beat_period();
    uint32_t beat_elapsed = domain_sequencer_engine_get_metronome_elapsed();
    uint8_t needle_pos;
    char text[10];

    if (beat_period == 0U)
    {
        beat_period = 1U;
    }

    if (beat_elapsed > beat_period)
    {
        beat_elapsed = beat_period;
    }

    needle_pos = (uint8_t)(((31UL * beat_elapsed) / beat_period) & 0xFFUL);
    if ((domain_sequencer_engine_get_metronome_count() & 1U) != 0U)
    {
        needle_pos = (uint8_t)(31U - needle_pos);
    }

    io_display_driver_ybar((int16_t)needle_pos, 0, 2, col_text);
    snprintf(text, sizeof(text), "BPM%3u", domain_sequencer_engine_get_bpm());
    io_display_driver_string(4, 3, text, (uint8_t)strlen(text), col_text, 0);
}

/**
 * @brief Draw the base bar on the display.
 *
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_base_bar(io_display_driver_color_t *col_text)
{
    io_display_driver_ybar(8, 0, 8, col_text);
}

/**
 * @brief Draw the sequencer left pane on the display.
 *
 * @param state The current display state.
 * @param col_text The color for the text.
 * @param draw_divider Flag indicating whether to draw the divider.
 */
static void io_display_renderer_draw_sequencer_left_pane(domain_display_state_t state, io_display_driver_color_t *col_text, uint8_t draw_divider)
{
    const domain_sequencer_state_t *seq = domain_sequencer_engine_get_state();
    io_display_driver_color_t col_icon;
    io_display_driver_icon_t icon = IO_DISPLAY_DRIVER_ICON_STOP;

    if ((state == DOMAIN_DISPLAY_SEQUENCER_REC) ||
        (state == DOMAIN_DISPLAY_SEQUENCER_PROGRAM))
    {
        icon = IO_DISPLAY_DRIVER_ICON_RECORD;
        col_icon = io_display_renderer_color_from_midi(APP_MIDI_COLOR_SEL_REC);
    }
    else if ((seq != 0) && (seq->playing != 0U))
    {
        icon = IO_DISPLAY_DRIVER_ICON_PLAY;
        col_icon = io_display_renderer_color_from_midi(APP_MIDI_COLOR_SEL_PLAY);
    }
    else
    {
        icon = IO_DISPLAY_DRIVER_ICON_STOP;
        col_icon = io_display_renderer_color_from_midi(APP_MIDI_COLOR_SEL_STOP);
    }

    io_display_driver_icon(icon, &col_icon);
    if (draw_divider != 0U)
    {
        io_display_renderer_draw_base_bar(col_text);
    }
}

/**
 * @brief Draw the sequencer left pane on the display.
 *
 * @param state The current display state.
 * @param col_text The color for the text.
 * @param draw_divider Flag indicating whether to draw the divider.
 */
static void io_display_renderer_draw_sequencer_bar_meter(domain_display_state_t state)
{
    const domain_sequencer_state_t *seq = domain_sequencer_engine_get_state();
    io_display_driver_color_t col_bar_all = io_display_renderer_color_from_midi(APP_MIDI_COLOR_BAR_ALL);
    io_display_driver_color_t col_sel_pos = io_display_renderer_color_from_midi(APP_MIDI_COLOR_CURSORX);
    io_display_driver_color_t col_sel_bar = io_display_renderer_color_from_midi(APP_MIDI_COLOR_SEL_BAR);
    io_display_driver_color_t col_sel_rec = io_display_renderer_color_from_midi(APP_MIDI_COLOR_SEL_REC);
    io_display_driver_color_t col_temp;
    uint8_t bar_len;
    uint8_t bar_index;
    uint8_t target_bar_index;
    uint16_t progress_bar;
    uint16_t target_bar;
    uint8_t beat_division;

    if (seq == 0)
    {
        return;
    }

    bar_len = (seq->bar == 0U) ? 1U : seq->bar;
    if (bar_len > 8U)
    {
        io_display_driver_xbar(0, 7, 8, &col_bar_all);

        col_temp = col_bar_all;
        col_temp.red = col_temp.green;
        io_display_driver_xbar(0, 7, (uint8_t)(bar_len - 8U), &col_temp);
    }
    else
    {
        io_display_driver_xbar(0, 7, bar_len, &col_bar_all);
    }

    beat_division = (seq->beat == 0U) ? 1U : seq->beat;
    if ((state == DOMAIN_DISPLAY_SEQUENCER_PLAY) && (seq->playing != 0U))
    {
        progress_bar = seq->bar_count;
    }
    else
    {
        progress_bar = (uint16_t)(seq->metronome_count / beat_division);
    }

    if (seq->bar != 0U)
    {
        progress_bar = (uint16_t)(progress_bar % seq->bar);
    }

    if ((seq->playing == 0U) && (state == DOMAIN_DISPLAY_SEQUENCER_PLAY))
    {
        return;
    }

    col_temp = col_sel_pos;

    bar_index = (uint8_t)(progress_bar % 8U);
    io_display_driver_pixel((int16_t)bar_index, 7, &col_temp);

    if ((state == DOMAIN_DISPLAY_SEQUENCER_REC) ||
        (state == DOMAIN_DISPLAY_SEQUENCER_PROGRAM))
    {
        target_bar = domain_sequencer_engine_get_selected_bar();
        if (seq->bar != 0U)
        {
            target_bar = (uint16_t)(target_bar % seq->bar);
        }

        col_temp = col_sel_bar;
        if ((seq->playing != 0U) && (target_bar == progress_bar))
        {
            col_temp = col_sel_rec;
        }
        else if (target_bar >= 8U)
        {
            col_temp.red = col_temp.blue;
        }

        target_bar_index = (uint8_t)(target_bar % 8U);
        io_display_driver_pixel((int16_t)target_bar_index, 7, &col_temp);
    }
}

/**
 * @brief Draw the sequencer graphics on the display.
 *
 * @param state The current display state.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_sequencer_graphics(domain_display_state_t state, io_display_driver_color_t *col_text)
{
    const domain_sequencer_state_t *seq = domain_sequencer_engine_get_state();
    io_display_driver_color_t col_cursor = io_display_renderer_color_from_midi(APP_MIDI_COLOR_CURSORY);
    uint8_t cursor_x;
    uint32_t beat_period;
    uint32_t beat_elapsed;
    uint32_t bar_period;
    uint32_t bar_elapsed;
    uint16_t beat_count_in_bar;
    uint16_t progress_bar;
    uint8_t beat_division;
    uint8_t draw_bar;

    if (seq == 0)
    {
        return;
    }

    io_display_renderer_draw_seq_channel_persistent(state);
    io_display_renderer_draw_sequencer_left_pane(state, col_text, 1U);
    io_display_renderer_draw_sequencer_bar_meter(state);

    beat_division = (seq->beat == 0U) ? 1U : seq->beat;
    if ((state == DOMAIN_DISPLAY_SEQUENCER_PLAY) && (seq->playing != 0U))
    {
        progress_bar = seq->bar_count;
    }
    else
    {
        progress_bar = (uint16_t)(seq->metronome_count / beat_division);
    }

    if (seq->bar != 0U)
    {
        progress_bar = (uint16_t)(progress_bar % seq->bar);
    }

    beat_period = domain_sequencer_engine_get_beat_period();
    if (beat_period == 0U)
    {
        beat_period = 1U;
    }

    bar_period = beat_period * (uint32_t)beat_division;
    if (bar_period == 0U)
    {
        bar_period = 1U;
    }

    if ((state == DOMAIN_DISPLAY_SEQUENCER_PLAY) && (seq->playing != 0U))
    {
        beat_count_in_bar = seq->beat_count;
        beat_elapsed = domain_sequencer_engine_get_beat_elapsed();
    }
    else if (state == DOMAIN_DISPLAY_SEQUENCER_PLAY)
    {
        beat_count_in_bar = seq->beat_count;
        beat_elapsed = seq->beat_elapsed;
    }
    else
    {
        beat_count_in_bar = (uint16_t)(seq->metronome_count % beat_division);
        beat_elapsed = domain_sequencer_engine_get_metronome_elapsed();
    }

    if (beat_elapsed > beat_period)
    {
        beat_elapsed = beat_period;
    }

    bar_elapsed = ((uint32_t)beat_count_in_bar * beat_period) + beat_elapsed;
    if (bar_elapsed > bar_period)
    {
        bar_elapsed = bar_period;
    }

    cursor_x = (uint8_t)(9U + ((22UL * bar_elapsed) / bar_period));
    if (cursor_x > 31U)
    {
        cursor_x = 31U;
    }
    io_display_driver_ybar((int16_t)cursor_x, 0, 8, &col_cursor);

    draw_bar = (uint8_t)progress_bar;
    io_display_renderer_draw_sequencer_recorded_notes(state, draw_bar, bar_period);
}

/**
 * @brief Draw the sequencer graphics on the display.
 *
 * @param state The current display state.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_base_setting(io_display_driver_color_t *col_text)
{
    io_display_driver_string(2, 1, "GOTODFU", 7, col_text, 0);
}

/**
 * @brief Draw the base display for the given state.
 *
 * @param state The current display state.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_base(domain_display_state_t state, io_display_driver_color_t *col_text)
{
    switch (state)
    {
    case DOMAIN_DISPLAY_NOTE:
        io_display_renderer_draw_note(col_text);
        break;
    case DOMAIN_DISPLAY_METRONOME:
        io_display_renderer_draw_metronome(col_text);
        break;
    case DOMAIN_DISPLAY_SEQUENCER_PARA:
    case DOMAIN_DISPLAY_SEQUENCER_PROGRAM:
    case DOMAIN_DISPLAY_SEQUENCER_REC:
    case DOMAIN_DISPLAY_SEQUENCER_PLAY:
        io_display_renderer_draw_sequencer_graphics(state, col_text);
        break;
    case DOMAIN_DISPLAY_SETTING:
        io_display_renderer_draw_base_setting(col_text);
        break;
    case DOMAIN_DISPLAY_OVERLAY:
    default:
        break;
    }
}

/**
 * @brief Draw the overlay for the MIDI channel on the display.
 *
 * @param value The MIDI channel value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_overlay_channel(uint16_t value, io_display_driver_color_t *col_text)
{
    char text[12];

    if (value > IO_DISPLAY_MIDI_CHANNEL_MAX)
    {
        io_display_driver_string(2, 1, "CHNLALL", 7, col_text, 0);
        return;
    }

    if (value == IO_MIDI_DRUMCH)
    {
        io_display_driver_string(2, 1, "CH DRUM", 7, col_text, 0);
        io_display_driver_xbar((int16_t)(2U * value), 7, 2, col_text);
        return;
    }

    snprintf(text, sizeof(text), "CHNL.%2u", value + 1U);
    io_display_driver_string(2, 1, text, (uint8_t)strlen(text), col_text, 0);
    io_display_driver_xbar((int16_t)(2U * value), 7, 2, col_text);
}

/**
 * @brief Draw the overlay for the MIDI channel on the display.
 *
 * @param value The MIDI channel value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_overlay_seq_input(uint16_t value, io_display_driver_color_t *col_text)
{
    uint16_t action = (uint16_t)((value >> 8) & 0x3U);
    uint16_t target_bar = (uint16_t)(value & 0xFFU);
    const io_display_text_entry_t *entry = &g_seq_input_action_texts[(action <= 3U) ? action : 3U];

    g_seqinput_target_bar = (uint8_t)((target_bar <= 0x0FU) ? target_bar : 0x0FU);
    io_display_driver_string(9, 1, entry->text, entry->len, col_text, 0);
}

/**
 * @brief Draw the overlay for sequencer input on the display.
 *
 * @param value The value containing the input action and target bar.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_overlay_seq_play(uint16_t value, io_display_driver_color_t *col_text)
{
    const domain_sequencer_state_t *seq = domain_sequencer_engine_get_state();
    char text[12];

    if (value == 0U)
    {
        g_seqplay_mode = 0U;

        if ((seq != 0) && (seq->playing != 0U))
        {
            io_display_driver_string(9, 1, "PLAYON", 6, col_text, 0);
        }
        else
        {
            io_display_driver_string(9, 1, "PLAYOF", 6, col_text, 0);
        }
        return;
    }

    g_seqplay_mode = (value > IO_DISPLAY_MIDI_CHANNEL_COUNT) ? IO_DISPLAY_MIDI_CHANNEL_COUNT : (uint8_t)value;
    snprintf(text, sizeof(text), "MUTE%2u", g_seqplay_mode);
    io_display_driver_string(9, 1, text, (uint8_t)strlen(text), col_text, 0);
}

/**
 * @brief Draw the overlay for sequencer play on the display.
 *
 * @param value The value containing the play state.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_overlay_seqpara_select(uint16_t value, io_display_driver_color_t *col_text)
{
    const io_display_text_entry_t *entry = &g_seqpara_select_texts[(value <= IO_DISPLAY_SEQPARA_METRONOME) ? value : IO_DISPLAY_SEQPARA_METRONOME];
    io_display_driver_string(9, 1, entry->text, entry->len, col_text, 0);
}

/**
 * @brief Draw the overlay for sequencer parameter value on the display.
 *
 * @param value The value containing the parameter and its value.
 * @param col_text The color for the text.
 */
static void io_display_renderer_draw_overlay_seqpara_value(uint16_t value, io_display_driver_color_t *col_text)
{
    uint16_t para = (uint16_t)((value >> 9) & 0x7U);
    uint16_t para_val = (uint16_t)(value & 0x1FFU);
    char text[12];

    if (para == IO_DISPLAY_SEQPARA_CHANNEL)
    {
        if (para_val > 0U)
        {
            g_seqpara_channel = (uint8_t)(para_val - 1U);
        }
        return;
    }

    if (para <= IO_DISPLAY_SEQPARA_METRONOME)
    {
        g_seqpara_value_draw_table[para](para_val, col_text);
        return;
    }

    snprintf(text, sizeof(text), "VAL%u", para_val);
    io_display_driver_string(8, 1, text, (uint8_t)strlen(text), col_text, 0);
}

/**
 * @brief Draw the overlay on the display.
 *
 * @param message The message containing the overlay type and value.
 * @param col_text The color for the text.
 * @param col_accent The color for the accent.
 */
static void io_display_renderer_draw_overlay(int16_t message, io_display_driver_color_t *col_text, io_display_driver_color_t *col_accent)
{
    uint8_t type = (uint8_t)((message >> 12) & 0x0FU);
    uint16_t value = (uint16_t)(message & 0x0FFFU);
    char text[12];
    io_display_driver_color_t col_valuebar;
    domain_display_midi_context_t midi_ctx;

    domain_display_model_get_midi_context(&midi_ctx);

    (void)col_accent;

    switch (type)
    {
    case DOMAIN_OVERLAY_CHANNEL:
        io_display_renderer_draw_overlay_channel(value, col_text);
        break;
    case DOMAIN_OVERLAY_VOLUME:
        snprintf(text, sizeof(text), "VOL.%3u", value);
        io_display_driver_string(2, 1, text, (uint8_t)strlen(text), col_text, 0);
        col_valuebar.green = (uint8_t)((10U * ((value > 127U) ? 127U : value)) / 127U);
        col_valuebar.red = 0U;
        col_valuebar.blue = (uint8_t)(10U - col_valuebar.green);
        io_display_driver_xbar(0, 7, (uint8_t)(((value > 127U) ? 127U : value) * 32U / 127U), &col_valuebar);
        if ((midi_ctx.valid != 0U) && (midi_ctx.channel <= IO_DISPLAY_MIDI_CHANNEL_MAX))
        {
            io_display_driver_xbar((int16_t)(2U * midi_ctx.channel), 6, 2, col_text);
        }
        break;
    case DOMAIN_OVERLAY_MODE:
        if (value < IO_MIDI_MODE_STATE_COUNT)
        {
            io_display_driver_string(2, 1, mode_text[value], 7, col_text, 0);
        }
        break;
    case DOMAIN_OVERLAY_CONTROL:
        if ((value & 0x100U) != 0U)
        {
            uint8_t hold_step = (uint8_t)(value & 0x7FU);

            // io_display_driver_string(2, 1, "HOLD", 4, col_text, 0);

            if (hold_step > 32U)
            {
                hold_step = 32U;
            }

            if (hold_step >= 16U)
            {
                io_display_driver_xbar(0, 7, hold_step, col_text);
            }
        }
        else if (value < (sizeof(g_control_hint_texts) / sizeof(g_control_hint_texts[0])))
        {
            const io_display_text_entry_t *entry = &g_control_hint_texts[value];
            io_display_driver_string(2, 1, entry->text, entry->len, col_text, 0);
        }
        break;
    case DOMAIN_OVERLAY_MIDI_IN:
        if ((midi_ctx.valid != 0U) && (midi_ctx.midi_msg_type == IO_MIDI_EVT0_CONTROLCHANGE) && (midi_ctx.data1 <= IO_MIDI_CC_USER_MODECONTROL))
        {
            snprintf(text, sizeof(text), "%.3s.%3u", stm32synth_cc_text[midi_ctx.data1], midi_ctx.data2);
            io_display_driver_string(2, 1, text, (uint8_t)strlen(text), col_text, 0);
            col_valuebar.green = (uint8_t)((10U * midi_ctx.data2) / 127U);
            col_valuebar.red = 0U;
            col_valuebar.blue = (uint8_t)(10U - col_valuebar.green);
            io_display_driver_xbar(0, 7, (uint8_t)((midi_ctx.data2 * 32U) / 127U), &col_valuebar);
            if ((midi_ctx.external != 0U) && (midi_ctx.channel <= IO_DISPLAY_MIDI_CHANNEL_MAX))
            {
                io_display_driver_xbar((int16_t)(2U * midi_ctx.channel), 6, 2, col_text);
            }
        }
        break;
    case DOMAIN_OVERLAY_REVERB:
        snprintf(text, sizeof(text), "RVB.%3u", value);
        io_display_driver_string(2, 1, text, (uint8_t)strlen(text), col_text, 0);
        col_valuebar.green = (uint8_t)((10U * ((value > 127U) ? 127U : value)) / 127U);
        col_valuebar.red = 0U;
        col_valuebar.blue = (uint8_t)(10U - col_valuebar.green);
        io_display_driver_xbar(0, 7, (uint8_t)(((value > 127U) ? 127U : value) * 32U / 127U), &col_valuebar);
        if ((midi_ctx.valid != 0U) && (midi_ctx.channel <= IO_DISPLAY_MIDI_CHANNEL_MAX))
        {
            io_display_driver_xbar((int16_t)(2U * midi_ctx.channel), 6, 2, col_text);
        }
        break;
    case DOMAIN_OVERLAY_BPM:
        io_display_renderer_draw_metronome(col_text);
        break;
    case DOMAIN_OVERLAY_SEQ_INPUT:
    case DOMAIN_OVERLAY_SEQ_INPUT_ACTION:
        io_display_renderer_draw_overlay_seq_input(value, col_text);
        break;
    case DOMAIN_OVERLAY_SEQ_PLAY:
    case DOMAIN_OVERLAY_SEQ_PLAY_ACTION:
        io_display_renderer_draw_overlay_seq_play(value, col_text);
        break;
    case DOMAIN_OVERLAY_SEQ_PARA_SELECT:
        io_display_renderer_draw_overlay_seqpara_select(value, col_text);
        break;
    case DOMAIN_OVERLAY_PROGRAM:
        if (value < 128U)
        {
            io_display_driver_string(2, 1, stm32synth_prog_text[value], 7, col_text, 0);
            if ((midi_ctx.valid != 0U) && (midi_ctx.channel <= IO_DISPLAY_MIDI_CHANNEL_MAX))
            {
                io_display_driver_xbar((int16_t)(2U * midi_ctx.channel), 7, 2, col_text);
            }
        }
        break;
    case DOMAIN_OVERLAY_SEQ_PARA_VALUE:
        io_display_renderer_draw_overlay_seqpara_value(value, col_text);
        break;
    default:
        io_display_driver_string(0, 0, "OVR", 3, col_text, 0);
        io_display_driver_xbar(0, 7, 8, col_accent);
        break;
    }
}

/**
 * @brief Draw the base overlay on the display.
 *
 * @param message The message containing the overlay type and value.
 * @param col_text The color for the text.
 * @return uint8_t Returns 1 if the overlay was drawn, 0 otherwise.
 */
static uint8_t io_display_renderer_draw_overlay_base(int16_t message, io_display_driver_color_t *col_text)
{
    uint8_t type = (uint8_t)((message >> 12) & 0x0FU);
    uint16_t value = (uint16_t)(message & 0x0FFFU);
    uint16_t para;

    switch (type)
    {
    case DOMAIN_OVERLAY_SEQ_INPUT:
    case DOMAIN_OVERLAY_SEQ_INPUT_ACTION:
        io_display_renderer_draw_seq_channel_persistent_with_length(DOMAIN_DISPLAY_SEQUENCER_REC, 8U);
        io_display_renderer_draw_sequencer_left_pane(DOMAIN_DISPLAY_SEQUENCER_REC, col_text, 0U);
        io_display_renderer_draw_sequencer_bar_meter(DOMAIN_DISPLAY_SEQUENCER_REC);
        return 1U;
    case DOMAIN_OVERLAY_SEQ_PLAY:
    case DOMAIN_OVERLAY_SEQ_PLAY_ACTION:
        io_display_renderer_draw_sequencer_graphics(DOMAIN_DISPLAY_SEQUENCER_PLAY, col_text);
        return 1U;
    case DOMAIN_OVERLAY_SEQ_PARA_SELECT:
        if (value == IO_DISPLAY_SEQPARA_CHANNEL)
        {
            io_display_renderer_draw_sequencer_graphics(DOMAIN_DISPLAY_SEQUENCER_PARA, col_text);
        }
        else
        {
            io_display_renderer_draw_sequencer_left_pane(DOMAIN_DISPLAY_SEQUENCER_PARA, col_text, 0U);
            io_display_renderer_draw_sequencer_bar_meter(DOMAIN_DISPLAY_SEQUENCER_PARA);
        }
        return 1U;
    case DOMAIN_OVERLAY_SEQ_PARA_VALUE:
        para = (uint16_t)((value >> 9) & 0x7U);
        if (para == IO_DISPLAY_SEQPARA_CHANNEL)
        {
            io_display_renderer_draw_sequencer_graphics(DOMAIN_DISPLAY_SEQUENCER_PARA, col_text);
        }
        else
        {
            io_display_renderer_draw_sequencer_left_pane(DOMAIN_DISPLAY_SEQUENCER_PARA, col_text, 0U);
            io_display_renderer_draw_sequencer_bar_meter(DOMAIN_DISPLAY_SEQUENCER_PARA);
        }
        return 1U;
    default:
        return 0U;
    }
}

/**
 * @brief Initialize the display renderer.
 *
 * @param hspi_neopixel The SPI handle for the NeoPixel.
 */
void io_display_renderer_init(SPI_HandleTypeDef *hspi_neopixel)
{
    if (hspi_neopixel == 0)
    {
        return;
    }

    g_hspi_neopixel = hspi_neopixel;
    g_seqpara_channel = 0U;
    g_seqplay_mode = 0U;
    g_seqinput_target_bar = 0U;
    g_boot_logo_start_tick = HAL_GetTick();
    memset(g_note_anim, 0, sizeof(g_note_anim));
}

/**
 * @brief Set the sequencer channel for the display renderer.
 *
 * @param channel The sequencer channel to set.
 */
void io_display_renderer_set_seq_channel(uint8_t channel)
{
    g_seqpara_channel = (uint8_t)(channel & 0x0FU);
}

/**
 * @brief Set the sequencer play mode for the display renderer.
 *
 * @param playmode The sequencer play mode to set.
 */
void io_display_renderer_set_seq_playmode(uint8_t playmode)
{
    g_seqplay_mode = (playmode > IO_DISPLAY_MIDI_CHANNEL_COUNT) ? IO_DISPLAY_MIDI_CHANNEL_COUNT : playmode;
}

/**
 * @brief Render the display based on the current model.
 *
 * @param model The display model containing the state and message.
 */
void io_display_renderer_render(const domain_display_model_t *model)
{
    io_display_driver_color_t col_bg = io_display_renderer_color_from_midi(APP_MIDI_COLOR_RESET);
    io_display_driver_color_t col_text = io_display_renderer_color_from_midi(APP_MIDI_COLOR_STRING);
    io_display_driver_color_t col_accent = io_display_renderer_color_from_midi(APP_MIDI_COLOR_CURSORX);
    uint32_t elapsed;
    uint8_t logo_phase;

    if ((model == 0) || (g_hspi_neopixel == 0))
    {
        return;
    }

    io_display_driver_clear(&col_bg);

    elapsed = HAL_GetTick() - g_boot_logo_start_tick;
    if (elapsed < IO_DISPLAY_BOOT_LOGO_DURATION_MS)
    {
        logo_phase = (uint8_t)((elapsed / IO_DISPLAY_BOOT_LOGO_PHASE_MS) & 1U);
        io_display_driver_logo(logo_phase, &col_text);
        return;
    }

    if (model->state == DOMAIN_DISPLAY_OVERLAY)
    {
        (void)io_display_renderer_draw_overlay_base(model->message, &col_text);
        io_display_renderer_draw_overlay(model->message, &col_text, &col_accent);
        io_display_renderer_draw_ble_indicator();
        return;
    }

    io_display_renderer_draw_base(model->state, &col_text);
    io_display_renderer_draw_ble_indicator();
}

/**
 * @brief Flush the display renderer, ensuring all pending updates are applied.
 */
void io_display_renderer_flush(void)
{
    if (g_hspi_neopixel == 0)
    {
        return;
    }

    io_display_driver_flush();
}
