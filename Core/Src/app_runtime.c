/**
 * @file app_runtime.c
 * @author k-omura
 * @brief Runtime management for the application
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "app_runtime.h"

#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "app_dispatcher.h"
#include "app_input_router.h"
#include "app_midi_constants.h"
#include "app_midi_utils.h"
#include "domain_display_model.h"
#include "domain_sequencer_engine.h"
#include "domain_mode_def.h"
#include "app_node_id.h"
#include "io_display_renderer.h"
#include "io_system_control.h"
#include "io_midi_transport.h"
#include "io_midi_types.h"

/* Mode state constants */
#define APP_BPM_STEP 5U                                    //!< Step size for BPM adjustments
#define APP_MIDI_CHANNEL_COUNT 16U                         //!< Total number of MIDI channels
#define APP_MIDI_CHANNEL_MAX (APP_MIDI_CHANNEL_COUNT - 1U) //!< Maximum MIDI channel index
#define APP_MIDI_CHANNEL_ALL APP_MIDI_CHANNEL_COUNT        //!< Special value representing all MIDI channels
#define APP_OVERLAY_TIMEOUT_MS 1000U                       //!< Timeout duration for overlay display in milliseconds
#define APP_OVERLAY_HINT_TIMEOUT_MS 650U
#define APP_SEQ_PARA_ENCODE(kind, val) (uint16_t)((((uint16_t)(kind) & 0x7U) << 9) | ((uint16_t)(val) & 0x1FFU)) //!< Macro to encode sequencer parameter kind and value into a single 16-bit value
#define APP_SEQ_PROGRAM_DEFAULT_NOTE 60U
#define APP_SEQ_PROGRAM_PREVIEW_MS 100U
#define APP_SEQ_PROGRAM_NOTE_VELOCITY 100U

#define APP_CONTROL_HINT_RE1_MODE 1U
#define APP_CONTROL_HINT_RE1_VOLUME 2U
#define APP_CONTROL_HINT_RE1_PROGRAM 3U
#define APP_CONTROL_HINT_RE2_CHANNEL 4U
#define APP_CONTROL_HINT_RE2_REVERB 5U
#define APP_CONTROL_HINT_MUTE_ON 6U
#define APP_CONTROL_HINT_MUTE_OFF 7U
#define APP_CONTROL_HINT_HOLD_PROGRESS_BASE 0x100U
#define APP_CONTROL_HINT_HOLD_PROGRESS_SRC_SHIFT 7U
#define APP_CONTROL_HINT_HOLD_PROGRESS_SRC_MASK 0x1U
#define APP_CONTROL_HINT_HOLD_PROGRESS_PCT_MASK 0x7FU
#define APP_OVERLAY_HOLD_PROGRESS_TIMEOUT_MS 140U

typedef enum
{
    APP_RE1_CTRL_MODE = 0, //!< Control for RE1 mode
    APP_RE1_CTRL_VOLUME,   //!< Control for RE1 volume
    APP_RE1_CTRL_PROGRAM,  //!< Control for RE1 program
    APP_RE1_CTRL_COUNT,    //!< Total number of RE1 controls
} app_re1_ctrl_t;

typedef enum
{
    APP_RE2_CTRL_CHANNEL = 0, //!< Control for RE2 channel
    APP_RE2_CTRL_REVERB,      //!< Control for RE2 reverb
    APP_RE2_CTRL_COUNT,       //!< Total number of RE2 controls
} app_re2_ctrl_t;

typedef enum
{
    APP_SEQPARA_SELECT_BPM = 0,   //!< Selection for sequencer parameter BPM
    APP_SEQPARA_SELECT_BEAT,      //!< Selection for sequencer parameter beat
    APP_SEQPARA_SELECT_BAR,       //!< Selection for sequencer parameter bar
    APP_SEQPARA_SELECT_CHANNEL,   //!< Selection for sequencer parameter channel
    APP_SEQPARA_SELECT_METRONOME, //!< Selection for sequencer parameter metronome
    APP_SEQPARA_SELECT_COUNT,     //!< Total number of sequencer parameter selections
} app_seqpara_select_t;

typedef struct
{
    uint8_t channel;
    uint8_t volume;
    uint8_t program;
    uint8_t reverb;
} app_midi_control_state_t;

typedef struct
{
    uint16_t metronome_bpm;
    uint8_t seqpara_select;
    uint16_t seqpara_beat;
    uint16_t seqpara_bar;
    uint16_t seqpara_sel_bar;
    uint16_t seqpara_sel_channel;
    uint16_t seqpara_metronome;
    uint16_t seqpara_playmode;
    io_display_setting_item_t setting_item;
    io_display_animation_mode_t animation_mode;
} app_mode_config_state_t;

typedef struct
{
    uint8_t current_mode;
    uint8_t re1_mode_select;
    uint32_t last_tap_tick;
    uint8_t re1_ctrl;
    uint8_t re2_ctrl;
    uint8_t modeparam_latch;
    uint32_t overlay_expire_tick;
} app_runtime_state_t;

static app_runtime_context_t g_ctx;
static app_event_t g_event_queue[APP_EVENT_QUEUE_CAPACITY];
static uint16_t g_event_wpos;
static uint16_t g_event_rpos;
static app_midi_control_state_t g_midi_ctrl;
static app_mode_config_state_t g_mode_cfg;
static app_runtime_state_t g_runtime_state;
static uint8_t g_seqinput_rec_in_bar;
static uint8_t g_seqinput_last_bar;
static uint8_t g_seqprogram_note;
static uint8_t g_seqprogram_preview_active;
static uint8_t g_seqprogram_preview_note;
static uint32_t g_seqprogram_preview_off_tick;
static uint8_t g_seqprogram_hold_active;
static uint8_t g_seqprogram_hold_note;
static uint8_t g_channel_volume[APP_MIDI_CHANNEL_COUNT];
static uint8_t g_volume_before_mute[APP_MIDI_CHANNEL_COUNT];
static uint8_t g_volume_muted;

#define g_current_mode (g_runtime_state.current_mode)       //!< Macro to access the current mode from the runtime state
#define g_re1_mode_select (g_runtime_state.re1_mode_select) //!< Macro to access the RE1 mode selection from the runtime state
#define g_last_tap_tick (g_runtime_state.last_tap_tick)     //!< Macro to access the last tap tick from the runtime state
#define g_re1_ctrl (g_runtime_state.re1_ctrl)               //!< Macro to access the RE1 control state from the runtime state
#define g_re2_ctrl (g_runtime_state.re2_ctrl)               //!< Macro to access the RE2 control state from the runtime state
#define g_modeparam_latch (g_runtime_state.modeparam_latch)
#define g_overlay_expire_tick (g_runtime_state.overlay_expire_tick) //!< Macro to access the overlay expire tick from the runtime state

static const uint16_t g_re1_ctrl_node_table[] = {
    APP_NODE_ID_RE1_MODE,
    APP_NODE_ID_RE1_VOLUME,
    APP_NODE_ID_RE1_PROGRAM,
};

static const uint16_t g_re2_ctrl_node_table[] = {
    APP_NODE_ID_RE2_CHANNEL,
    APP_NODE_ID_RE2_REVERB,
};

static uint8_t app_runtime_handle_re2_button_modeparam_metronome(const app_event_t *ev);
static uint8_t app_runtime_handle_re2_button_modeparam_seqpara(const app_event_t *ev);
static uint8_t app_runtime_handle_re2_long_press(const app_event_t *ev, uint8_t effective_mode);

typedef uint8_t (*app_runtime_simple_action_t)(const app_event_t *ev);

typedef struct app_runtime_mode_action_entry
{
    uint8_t mode;
    app_runtime_simple_action_t action;
} app_runtime_mode_action_entry_t;

static const app_runtime_mode_action_entry_t g_re2_button_modeparam_actions[] = {
    {IO_MIDI_MODE_STATE_METRONOME, app_runtime_handle_re2_button_modeparam_metronome},
    {IO_MIDI_MODE_STATE_SEQUENCER_PARA, app_runtime_handle_re2_button_modeparam_seqpara},
};

static void app_runtime_send_cc(uint8_t channel, uint8_t cc, uint8_t value);
static void app_runtime_send_all_notes_off(void);
static void app_runtime_send_note(uint8_t channel, uint8_t note, uint8_t velocity, uint8_t is_note_on);
static void app_runtime_toggle_volume_mute(uint32_t now_tick);
static void app_runtime_request_mode_change(uint8_t new_mode, uint32_t now_tick);
static void app_runtime_request_node_change(uint16_t new_node, uint32_t now_tick);
static uint16_t app_runtime_re1_ctrl_to_node(uint8_t ctrl);
static uint16_t app_runtime_re2_ctrl_to_node(uint8_t ctrl);
static uint16_t app_runtime_clamp_add_u16(uint16_t current, int16_t delta, uint16_t min, uint16_t max, uint16_t step);

static void app_runtime_set_local_midi_context(uint8_t midi_msg_type, uint8_t data1, uint8_t data2);
static uint8_t app_runtime_handle_re2_rotate_modeparam(const app_event_t *ev, uint8_t effective_mode);
static uint8_t app_runtime_handle_seqpara_rotate(const app_event_t *ev);
static void app_runtime_advance_seqpara_select(void);
static void app_runtime_get_seqinput_selection(uint8_t *input_action, uint8_t *target_bar);
static void app_runtime_apply_seqinput_action(uint8_t input_action, uint8_t target_bar);
static void app_runtime_toggle_seqplay_mute_channel(uint8_t mute_channel);
static void app_runtime_handle_seqplay_button_action(void);
static void app_runtime_apply_tap_tempo(uint32_t now_tick);
static uint8_t app_runtime_dispatch_re2_rotate_modeparam(const app_event_t *ev, uint8_t effective_mode);
static uint8_t app_runtime_handle_re1_rotate(const app_event_t *ev);
static uint8_t app_runtime_handle_re1_button_down(const app_event_t *ev);
static uint8_t app_runtime_handle_re1_long_press(const app_event_t *ev, uint8_t effective_mode);
static uint8_t app_runtime_handle_re2_rotate_normal(const app_event_t *ev);
static uint8_t app_runtime_handle_re2_button_special_mode(const app_event_t *ev, uint8_t effective_mode);
static uint8_t app_runtime_handle_re2_button_modeparam(const app_event_t *ev, uint8_t effective_mode);
static uint8_t app_runtime_handle_re2_button_normal(const app_event_t *ev);
static uint8_t app_runtime_handle_re2_button_up_program(const app_event_t *ev, uint8_t effective_mode);
static uint8_t app_runtime_dispatch_mode_action(const app_event_t *ev, uint8_t effective_mode, const struct app_runtime_mode_action_entry *entries, size_t entry_count);
static uint8_t app_runtime_mode_supports_param_edit(uint8_t effective_mode);
static void app_runtime_show_overlay_with_timeout(uint16_t msg_type, uint16_t value, uint32_t now_tick, uint32_t timeout_ms);
static void app_runtime_show_overlay(uint16_t msg_type, uint16_t value, uint32_t now_tick);
static void app_runtime_show_overlay_midi_rx(uint16_t msg_type, uint16_t value, uint32_t now_tick, uint8_t midi_msg_type, uint8_t channel, uint8_t data1, uint8_t data2);
static uint16_t app_runtime_encode_hold_progress_overlay(uint8_t source, uint8_t progress_pct);
static void app_runtime_update_seqinput_auto_advance(void);
static void app_runtime_seqprogram_stop_preview(void);
static void app_runtime_seqprogram_start_preview(uint32_t now_tick);
static uint8_t app_runtime_seqprogram_get_target_bar(uint8_t *out_bar);
static void app_runtime_seqprogram_handle_button_down(uint32_t now_tick);
static void app_runtime_seqprogram_handle_button_up(uint32_t now_tick);
static void app_runtime_process_seqprogram_preview(uint32_t now_tick);
static void app_runtime_seqprogram_abort_active_notes(void);
static uint8_t app_runtime_is_sequencer_mode(uint8_t mode);

static uint16_t app_runtime_encode_hold_progress_overlay(uint8_t source, uint8_t progress_pct)
{
    uint16_t value = APP_CONTROL_HINT_HOLD_PROGRESS_BASE;

    value |= (uint16_t)(((uint16_t)source & APP_CONTROL_HINT_HOLD_PROGRESS_SRC_MASK) << APP_CONTROL_HINT_HOLD_PROGRESS_SRC_SHIFT);
    value |= (uint16_t)(progress_pct & APP_CONTROL_HINT_HOLD_PROGRESS_PCT_MASK);
    return value;
}

static const app_input_router_handlers_t g_input_router_handlers = {
    .re1 = {
        .rotate = app_runtime_handle_re1_rotate,
        .button_down = app_runtime_handle_re1_button_down,
        .long_press = app_runtime_handle_re1_long_press,
    },
    .re2 = {
        .rotate_modeparam = app_runtime_handle_re2_rotate_modeparam,
        .rotate_normal = app_runtime_handle_re2_rotate_normal,
        .button_special = app_runtime_handle_re2_button_special_mode,
        .button_modeparam = app_runtime_handle_re2_button_modeparam,
        .button_normal = app_runtime_handle_re2_button_normal,
        .long_press = app_runtime_handle_re2_long_press,
    },
};

/**
 * @brief Gets the effective mode of the application runtime. This function determines the current mode based on the display state.
 * @return The effective mode of the application runtime.
 */
static uint8_t app_runtime_mode_supports_param_edit(uint8_t effective_mode)
{
    if ((effective_mode == IO_MIDI_MODE_STATE_FREEPLAY) ||
        (effective_mode >= IO_MIDI_MODE_STATE_COUNT))
    {
        return 0U;
    }

    return 1U;
}

static void app_runtime_reset_default_state(void)
{
    uint8_t channel;

    g_midi_ctrl.channel = APP_MIDI_CHANNEL_ALL;
    g_midi_ctrl.volume = 100;
    g_midi_ctrl.program = 0;
    g_midi_ctrl.reverb = 64;
    g_current_mode = IO_MIDI_MODE_STATE_FREEPLAY;
    g_re1_mode_select = g_current_mode;
    g_mode_cfg.metronome_bpm = 120;
    g_last_tap_tick = 0;
    g_re1_ctrl = APP_RE1_CTRL_VOLUME;
    g_re2_ctrl = APP_RE2_CTRL_REVERB;
    g_modeparam_latch = 0U;
    g_mode_cfg.seqpara_select = APP_SEQPARA_SELECT_BPM;
    g_mode_cfg.seqpara_beat = 4;
    g_mode_cfg.seqpara_bar = 2;
    g_mode_cfg.seqpara_sel_bar = 0;
    g_mode_cfg.seqpara_sel_channel = 0;
    g_mode_cfg.seqpara_metronome = 1;
    g_mode_cfg.seqpara_playmode = 0;
    g_mode_cfg.setting_item = IO_DISPLAY_SETTING_ITEM_ANIMATION;
    g_mode_cfg.animation_mode = IO_DISPLAY_ANIMATION_MODE_1;
    g_overlay_expire_tick = 0;
    g_seqinput_rec_in_bar = 0U;
    g_seqinput_last_bar = 0U;
    g_seqprogram_note = APP_SEQ_PROGRAM_DEFAULT_NOTE;
    g_seqprogram_preview_active = 0U;
    g_seqprogram_preview_note = APP_SEQ_PROGRAM_DEFAULT_NOTE;
    g_seqprogram_preview_off_tick = 0U;
    g_seqprogram_hold_active = 0U;
    g_seqprogram_hold_note = APP_SEQ_PROGRAM_DEFAULT_NOTE;
    g_volume_muted = 0U;
    for (channel = 0U; channel < APP_MIDI_CHANNEL_COUNT; channel++)
    {
        g_channel_volume[channel] = g_midi_ctrl.volume;
        g_volume_before_mute[channel] = g_midi_ctrl.volume;
    }
    g_ctx.current_node = app_runtime_re1_ctrl_to_node(g_re1_ctrl);
}

static void app_runtime_sync_setting_view(void)
{
    io_display_renderer_set_setting_view(g_mode_cfg.setting_item, g_mode_cfg.animation_mode);
}

static uint8_t app_runtime_handle_seqpara_rotate(const app_event_t *ev)
{
    switch (g_mode_cfg.seqpara_select)
    {
    case APP_SEQPARA_SELECT_BPM:
        g_mode_cfg.metronome_bpm = app_runtime_clamp_add_u16(g_mode_cfg.metronome_bpm, ev->delta, IO_MIDI_DEFAULT_BPM_MIN, IO_MIDI_DEFAULT_BPM_MAX, APP_BPM_STEP);
        domain_sequencer_engine_set_tempo(g_mode_cfg.metronome_bpm, (uint8_t)g_mode_cfg.seqpara_beat);
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PARA_VALUE, APP_SEQ_PARA_ENCODE(APP_SEQPARA_SELECT_BPM, g_mode_cfg.metronome_bpm), ev->tick);
        return 1U;

    case APP_SEQPARA_SELECT_BEAT:
        g_mode_cfg.seqpara_beat = app_runtime_clamp_add_u16(g_mode_cfg.seqpara_beat, ev->delta, 2, 32, 1);
        domain_sequencer_engine_set_tempo(g_mode_cfg.metronome_bpm, (uint8_t)g_mode_cfg.seqpara_beat);
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PARA_VALUE, APP_SEQ_PARA_ENCODE(APP_SEQPARA_SELECT_BEAT, g_mode_cfg.seqpara_beat), ev->tick);
        return 1U;

    case APP_SEQPARA_SELECT_BAR:
        g_mode_cfg.seqpara_bar = app_runtime_clamp_add_u16(g_mode_cfg.seqpara_bar, ev->delta, 1, IO_MIDI_SEQUENCER_MAXBAR, 1);
        if (g_mode_cfg.seqpara_sel_bar >= (g_mode_cfg.seqpara_bar * 4U))
        {
            g_mode_cfg.seqpara_sel_bar = (uint16_t)((g_mode_cfg.seqpara_bar * 4U) - 1U);
        }
        domain_sequencer_engine_set_bar((uint8_t)g_mode_cfg.seqpara_bar);
        domain_sequencer_engine_set_selected_bar((uint8_t)(g_mode_cfg.seqpara_sel_bar % g_mode_cfg.seqpara_bar));
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PARA_VALUE, APP_SEQ_PARA_ENCODE(APP_SEQPARA_SELECT_BAR, g_mode_cfg.seqpara_bar), ev->tick);
        return 1U;

    case APP_SEQPARA_SELECT_CHANNEL:
        g_mode_cfg.seqpara_sel_channel = app_runtime_clamp_add_u16(g_mode_cfg.seqpara_sel_channel, ev->delta, 0, APP_MIDI_CHANNEL_MAX, 1);
        io_display_renderer_set_seq_channel((uint8_t)g_mode_cfg.seqpara_sel_channel);
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PARA_VALUE, APP_SEQ_PARA_ENCODE(APP_SEQPARA_SELECT_CHANNEL, (uint16_t)(g_mode_cfg.seqpara_sel_channel + 1U)), ev->tick);
        return 1U;

    case APP_SEQPARA_SELECT_METRONOME:
    default:
        g_mode_cfg.seqpara_metronome = app_runtime_clamp_add_u16(g_mode_cfg.seqpara_metronome, ev->delta, 0, 1, 1);
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PARA_VALUE, APP_SEQ_PARA_ENCODE(APP_SEQPARA_SELECT_METRONOME, g_mode_cfg.seqpara_metronome), ev->tick);
        return 1U;
    }
}

static void app_runtime_advance_seqpara_select(void)
{
    g_mode_cfg.seqpara_select = (uint8_t)((g_mode_cfg.seqpara_select + 1U) % APP_SEQPARA_SELECT_COUNT);
}

static void app_runtime_get_seqinput_selection(uint8_t *input_action, uint8_t *target_bar)
{
    if ((input_action == 0) || (target_bar == 0) || (g_mode_cfg.seqpara_bar == 0U))
    {
        return;
    }

    *target_bar = (uint8_t)(g_mode_cfg.seqpara_sel_bar % g_mode_cfg.seqpara_bar);
    *input_action = (uint8_t)(g_mode_cfg.seqpara_sel_bar / g_mode_cfg.seqpara_bar);
}

static void app_runtime_apply_seqinput_action(uint8_t input_action, uint8_t target_bar)
{
    if (input_action == 0U)
    {
        domain_sequencer_engine_undo(target_bar);
    }
    else if (input_action == 1U)
    {
        domain_sequencer_engine_clear_channel_in_bar(target_bar, (uint8_t)g_mode_cfg.seqpara_sel_channel);
        app_runtime_send_all_notes_off();
    }
    else if (input_action == 2U)
    {
        domain_sequencer_engine_clear_bar(target_bar);
        app_runtime_send_all_notes_off();
    }
    else
    {
        domain_sequencer_engine_clear_all();
        app_runtime_send_all_notes_off();
    }
}

static void app_runtime_toggle_seqplay_mute_channel(uint8_t mute_channel)
{
    domain_sequencer_engine_toggle_channel_muted(mute_channel);
    if (domain_sequencer_engine_is_channel_muted(mute_channel) != 0U)
    {
        app_runtime_send_cc(mute_channel, IO_MIDI_CC_ALLNOTESOFF, 0U);
    }
}

static void app_runtime_handle_seqplay_button_action(void)
{
    if (g_mode_cfg.seqpara_playmode == 0U)
    {
        uint8_t next_playing = (uint8_t)!domain_sequencer_engine_get_state()->playing;
        domain_sequencer_engine_set_playing(next_playing);
    }
    else
    {
        uint8_t mute_channel = (uint8_t)(g_mode_cfg.seqpara_playmode - 1U);
        app_runtime_toggle_seqplay_mute_channel(mute_channel);
    }
}

static void app_runtime_apply_tap_tempo(uint32_t now_tick)
{
    if (g_last_tap_tick != 0U)
    {
        uint32_t diff = now_tick - g_last_tap_tick;
        if (diff > 0U)
        {
            uint16_t bpm = (uint16_t)(60000UL / diff);
            bpm = (uint16_t)(bpm - (bpm % APP_BPM_STEP));
            if ((bpm >= IO_MIDI_DEFAULT_BPM_MIN) && (bpm <= IO_MIDI_DEFAULT_BPM_MAX))
            {
                g_mode_cfg.metronome_bpm = bpm;
            }
        }
    }

    g_last_tap_tick = now_tick;
    domain_sequencer_engine_set_tempo(g_mode_cfg.metronome_bpm, (uint8_t)g_mode_cfg.seqpara_beat);
}

static uint8_t app_runtime_dispatch_re2_rotate_modeparam(const app_event_t *ev, uint8_t effective_mode)
{
    if (effective_mode == IO_MIDI_MODE_STATE_SETTING)
    {
        int32_t next_item;

        if (ev->delta == 0)
        {
            return 1U;
        }

        next_item = ((int32_t)g_mode_cfg.setting_item + (int32_t)ev->delta) % (int32_t)IO_DISPLAY_SETTING_ITEM_COUNT;
        if (next_item < 0)
        {
            next_item += (int32_t)IO_DISPLAY_SETTING_ITEM_COUNT;
        }
        g_mode_cfg.setting_item = (io_display_setting_item_t)next_item;
        app_runtime_sync_setting_view();
        g_overlay_expire_tick = 0U;
        domain_display_model_set_state(DOMAIN_DISPLAY_SETTING, 0);
        return 1U;
    }

    if (effective_mode == IO_MIDI_MODE_STATE_METRONOME)
    {
        g_mode_cfg.metronome_bpm = app_runtime_clamp_add_u16(g_mode_cfg.metronome_bpm, ev->delta, IO_MIDI_DEFAULT_BPM_MIN, IO_MIDI_DEFAULT_BPM_MAX, APP_BPM_STEP);
        domain_sequencer_engine_set_tempo(g_mode_cfg.metronome_bpm, (uint8_t)g_mode_cfg.seqpara_beat);
        app_runtime_show_overlay(DOMAIN_OVERLAY_BPM, g_mode_cfg.metronome_bpm, ev->tick);
        return 1U;
    }

    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_PARA)
    {
        return app_runtime_handle_seqpara_rotate(ev);
    }

    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM)
    {
        uint16_t next_note = app_runtime_clamp_add_u16(g_seqprogram_note, ev->delta, 0U, 127U, 1U);

        if (next_note != g_seqprogram_note)
        {
            g_seqprogram_note = (uint8_t)next_note;
            app_runtime_seqprogram_start_preview(ev->tick);
        }
        return 1U;
    }

    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_REC)
    {
        uint16_t max_sel = (uint16_t)((g_mode_cfg.seqpara_bar * 4U) - 1U);
        uint8_t input_action = 0U;
        uint8_t target_bar = 0U;

        g_mode_cfg.seqpara_sel_bar = app_runtime_clamp_add_u16(g_mode_cfg.seqpara_sel_bar, ev->delta, 0, max_sel, 1);
        app_runtime_get_seqinput_selection(&input_action, &target_bar);
        domain_sequencer_engine_set_selected_bar(target_bar);
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_INPUT, (uint16_t)((((uint16_t)input_action & 0x3U) << 8) | ((uint16_t)target_bar & 0xFFU)), ev->tick);
        return 1U;
    }

    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_PLAY)
    {
        g_mode_cfg.seqpara_playmode = app_runtime_clamp_add_u16(g_mode_cfg.seqpara_playmode, ev->delta, 0, APP_MIDI_CHANNEL_ALL, 1);
        io_display_renderer_set_seq_playmode((uint8_t)g_mode_cfg.seqpara_playmode);
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PLAY, g_mode_cfg.seqpara_playmode, ev->tick);
        return 1U;
    }

    app_runtime_show_overlay(DOMAIN_OVERLAY_MODE, effective_mode, ev->tick);
    return 1U;
}

/**
 * @brief Handles the RE2 button press event for the metronome mode parameter. This function applies the tap tempo based on the event tick and shows the BPM overlay.
 *
 * @param ev Pointer to the app_event_t structure containing the event information.
 * @return uint8_t Returns 1U to indicate successful handling of the event.
 */
static uint8_t app_runtime_handle_re2_button_modeparam_metronome(const app_event_t *ev)
{
    app_runtime_apply_tap_tempo(ev->tick);
    app_runtime_show_overlay(DOMAIN_OVERLAY_BPM, g_mode_cfg.metronome_bpm, ev->tick);
    return 1U;
}

static uint8_t app_runtime_handle_re2_button_modeparam_seqpara(const app_event_t *ev)
{
    app_runtime_advance_seqpara_select();
    app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PARA_SELECT, g_mode_cfg.seqpara_select, ev->tick);
    return 1U;
}

static uint8_t app_runtime_handle_re2_long_press(const app_event_t *ev, uint8_t effective_mode)
{
    (void)effective_mode;
    app_runtime_toggle_volume_mute(ev->tick);
    return 1U;
}

static uint8_t app_runtime_dispatch_mode_action(const app_event_t *ev,
                                                uint8_t effective_mode,
                                                const app_runtime_mode_action_entry_t *entries,
                                                size_t entry_count)
{
    size_t index;

    if ((ev == 0) || (entries == 0))
    {
        return 0U;
    }

    for (index = 0; index < entry_count; ++index)
    {
        if (entries[index].mode == effective_mode)
        {
            if (entries[index].action != 0)
            {
                return entries[index].action(ev);
            }

            return 0U;
        }
    }

    return 0U;
}

static uint8_t app_runtime_handle_re1_rotate(const app_event_t *ev)
{
    if (g_re1_ctrl == APP_RE1_CTRL_MODE)
    {
        uint8_t next_mode = g_re1_mode_select;

        if (ev->delta > 0)
        {
            if ((uint8_t)(g_re1_mode_select + 1U) < IO_MIDI_MODE_STATE_COUNT)
            {
                next_mode = (uint8_t)(g_re1_mode_select + 1U);
            }
        }
        else if (ev->delta < 0)
        {
            if (g_re1_mode_select > IO_MIDI_MODE_STATE_FREEPLAY)
            {
                next_mode = (uint8_t)(g_re1_mode_select - 1U);
            }
        }

        if (next_mode != g_re1_mode_select)
        {
            g_re1_mode_select = next_mode;
            app_runtime_show_overlay(DOMAIN_OVERLAY_MODE, g_re1_mode_select, ev->tick);
        }
        else
        {
            app_runtime_show_overlay(DOMAIN_OVERLAY_MODE, g_re1_mode_select, ev->tick);
        }

        g_modeparam_latch = 0U;
        return 1U;
    }

    if (g_re1_ctrl == APP_RE1_CTRL_VOLUME)
    {
        g_midi_ctrl.volume = (uint8_t)app_runtime_clamp_add_u16(g_midi_ctrl.volume, ev->delta, 0, 127, 4);
        if (g_midi_ctrl.channel > APP_MIDI_CHANNEL_MAX)
        {
            uint8_t ch;
            for (ch = 0U; ch < APP_MIDI_CHANNEL_COUNT; ch++)
            {
                g_channel_volume[ch] = g_midi_ctrl.volume;
                if (g_volume_muted != 0U)
                {
                    g_volume_before_mute[ch] = g_midi_ctrl.volume;
                }
                app_runtime_send_cc(ch, IO_MIDI_CC_VOLUME, (g_volume_muted != 0U) ? 0U : g_midi_ctrl.volume);
            }
        }
        else
        {
            g_channel_volume[g_midi_ctrl.channel] = g_midi_ctrl.volume;
            if (g_volume_muted != 0U)
            {
                g_volume_before_mute[g_midi_ctrl.channel] = g_midi_ctrl.volume;
            }
            app_runtime_send_cc(g_midi_ctrl.channel, IO_MIDI_CC_VOLUME, (g_volume_muted != 0U) ? 0U : g_midi_ctrl.volume);
        }
        app_runtime_set_local_midi_context(IO_MIDI_EVT0_CONTROLCHANGE, IO_MIDI_CC_VOLUME, g_midi_ctrl.volume);
        app_runtime_show_overlay(DOMAIN_OVERLAY_VOLUME, g_midi_ctrl.volume, ev->tick);
        return 1U;
    }

    if (g_re1_ctrl == APP_RE1_CTRL_PROGRAM)
    {
        g_midi_ctrl.program = (uint8_t)app_runtime_clamp_add_u16(g_midi_ctrl.program, ev->delta, 0, 127, 1);
        if (g_midi_ctrl.channel > APP_MIDI_CHANNEL_MAX)
        {
            uint8_t ch;
            for (ch = 0; ch < APP_MIDI_CHANNEL_COUNT; ch++)
            {
                app_midi_programchange(ch, g_midi_ctrl.program);
            }
        }
        else
        {
            app_midi_programchange(g_midi_ctrl.channel, g_midi_ctrl.program);
        }
        app_runtime_set_local_midi_context(IO_MIDI_EVT0_PROGRAMCHANGE, g_midi_ctrl.program, 0U);
        app_runtime_show_overlay(DOMAIN_OVERLAY_PROGRAM, g_midi_ctrl.program, ev->tick);
        return 1U;
    }

    return 1U;
}

static uint8_t app_runtime_handle_re1_button_down(const app_event_t *ev)
{
    if (g_re1_ctrl == APP_RE1_CTRL_MODE)
    {
        if (g_re1_mode_select != g_current_mode)
        {
            app_runtime_request_mode_change(g_re1_mode_select, ev->tick);
            app_runtime_send_cc(0, IO_MIDI_CC_USER_MODECONTROL, g_re1_mode_select);
        }
        app_runtime_request_node_change(app_runtime_re1_ctrl_to_node(APP_RE1_CTRL_VOLUME), ev->tick);
        app_runtime_show_overlay(DOMAIN_OVERLAY_MODE, g_re1_mode_select, ev->tick);
        g_modeparam_latch = 0U;
        return 1U;
    }

    uint16_t next_ctrl;
    if (g_re1_ctrl == APP_RE1_CTRL_VOLUME)
    {
        next_ctrl = APP_RE1_CTRL_PROGRAM;
    }
    else
    {
        next_ctrl = APP_RE1_CTRL_VOLUME;
    }

    app_runtime_request_node_change(app_runtime_re1_ctrl_to_node((uint8_t)next_ctrl), ev->tick);
    g_modeparam_latch = 0U;

    if (next_ctrl == APP_RE1_CTRL_VOLUME)
    {
        app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL, APP_CONTROL_HINT_RE1_VOLUME, ev->tick, APP_OVERLAY_HINT_TIMEOUT_MS);
    }
    else
    {
        app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL, APP_CONTROL_HINT_RE1_PROGRAM, ev->tick, APP_OVERLAY_HINT_TIMEOUT_MS);
    }

    return 1U;
}

static uint8_t app_runtime_handle_re1_long_press(const app_event_t *ev, uint8_t effective_mode)
{
    (void)effective_mode;

    g_modeparam_latch = 0U;

    if (g_re1_ctrl != APP_RE1_CTRL_MODE)
    {
        app_runtime_request_node_change(app_runtime_re1_ctrl_to_node(APP_RE1_CTRL_MODE), ev->tick);
        g_re1_mode_select = g_current_mode;
        app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL, APP_CONTROL_HINT_RE1_MODE, ev->tick, APP_OVERLAY_HINT_TIMEOUT_MS);
        return 1U;
    }

    g_re1_mode_select = g_current_mode;
    app_runtime_show_overlay(DOMAIN_OVERLAY_MODE, g_re1_mode_select, ev->tick);
    return 1U;
}

/**
 * @brief Sends a Volume Change message for a specific MIDI channel. This function updates the volume of the specified channel.
 * @param channel The MIDI channel to send the volume change message.
 * @param volume The volume level to set.
 */
static void app_runtime_toggle_volume_mute(uint32_t now_tick)
{
    uint8_t channel;

    if (g_volume_muted == 0U)
    {
        for (channel = 0U; channel < APP_MIDI_CHANNEL_COUNT; channel++)
        {
            g_volume_before_mute[channel] = g_channel_volume[channel];
            app_runtime_send_cc(channel, IO_MIDI_CC_VOLUME, 0U);
        }
        g_volume_muted = 1U;
        app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL,
                                              APP_CONTROL_HINT_MUTE_ON,
                                              now_tick,
                                              APP_OVERLAY_HINT_TIMEOUT_MS);
        return;
    }

    for (channel = 0U; channel < APP_MIDI_CHANNEL_COUNT; channel++)
    {
        g_channel_volume[channel] = g_volume_before_mute[channel];
        app_runtime_send_cc(channel, IO_MIDI_CC_VOLUME, g_channel_volume[channel]);
    }
    g_volume_muted = 0U;
    app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL,
                                          APP_CONTROL_HINT_MUTE_OFF,
                                          now_tick,
                                          APP_OVERLAY_HINT_TIMEOUT_MS);
}

/**
 * @brief Clamps and adds a delta to a 16-bit unsigned integer. This function ensures the result stays within the specified range.
 * @param current The current value.
 * @param delta The delta to add.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @param step The step size for the delta.
 * @return The clamped result.
 */
static uint16_t app_runtime_clamp_add_u16(uint16_t current, int16_t delta, uint16_t min, uint16_t max, uint16_t step)
{
    if ((current == 127U) && (delta < 0) && (step == 4U))
    {
        return 124U;
    }

    int32_t next = (int32_t)current + ((int32_t)delta * (int32_t)step);
    if (next < (int32_t)min)
    {
        return min;
    }
    if (next > (int32_t)max)
    {
        return max;
    }
    return (uint16_t)next;
}

/**
 * @brief Requests a mode change in the application runtime. This function creates a system event to change the current mode to the specified new mode.
 *
 * @param new_mode The new mode to transition to.
 * @param now_tick The current tick count, used for timing purposes.
 */
static void app_runtime_request_mode_change(uint8_t new_mode, uint32_t now_tick)
{
    app_event_t mode_ev;

    if ((new_mode >= IO_MIDI_MODE_STATE_COUNT) || (new_mode == g_current_mode))
    {
        return;
    }

    memset(&mode_ev, 0, sizeof(mode_ev));
    mode_ev.type = EV_SYSTEM_MODE_CHANGE;
    mode_ev.source = APP_INPUT_SOURCE_SYSTEM;
    mode_ev.code = new_mode;
    mode_ev.tick = now_tick;

    app_runtime_post_event(&mode_ev);
}

/**
 * @brief Requests a node change in the application runtime. This function creates a system event to change the current node to the specified new node.
 *
 * @param new_node The new node to transition to.
 * @param now_tick The current tick count, used for timing purposes.
 */
static void app_runtime_request_node_change(uint16_t new_node, uint32_t now_tick)
{
    app_event_t node_ev;

    if (new_node == g_ctx.current_node)
    {
        return;
    }

    memset(&node_ev, 0, sizeof(node_ev));
    node_ev.type = EV_SYSTEM_NODE_CHANGE;
    node_ev.source = APP_INPUT_SOURCE_SYSTEM;
    node_ev.code = new_node;
    node_ev.tick = now_tick;

    app_runtime_post_event(&node_ev);
}

static uint16_t app_runtime_re1_ctrl_to_node(uint8_t ctrl)
{
    if (ctrl < APP_RE1_CTRL_COUNT)
    {
        return g_re1_ctrl_node_table[ctrl];
    }

    return APP_NODE_ID_RE1_VOLUME;
}

static uint16_t app_runtime_re2_ctrl_to_node(uint8_t ctrl)
{
    if (ctrl < APP_RE2_CTRL_COUNT)
    {
        return g_re2_ctrl_node_table[ctrl];
    }

    return APP_NODE_ID_RE2_CHANNEL;
}

/**
 * @brief Processes received MIDI messages. This function handles incoming MIDI messages and updates the system state accordingly.
 * @param now_tick The current tick count.
 */
static void app_runtime_process_rx_midi(uint32_t now_tick)
{
    uint8_t packet[4];

    while (io_midi_transport_pop_rx_packet(packet) != 0U)
    {
        uint8_t status = packet[1];
        uint8_t data1 = packet[2];
        uint8_t data2 = packet[3];
        uint8_t msg_type = (uint8_t)(status & 0xF0U);
        uint8_t channel = (uint8_t)(status & 0x0FU);
        uint8_t is_seq_mode = (uint8_t)((g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PARA) ||
                                        (g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM) ||
                                        (g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_REC) ||
                                        (g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PLAY));

        if ((msg_type == IO_MIDI_EVT0_NOTEON) || (msg_type == IO_MIDI_EVT0_NOTEOFF))
        {
            if (g_current_mode == IO_MIDI_MODE_STATE_FREEPLAY)
            {
                domain_display_model_set_midi_context(msg_type, channel, data1, data2, 1U);
            }

            if (g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_REC)
            {
                const domain_sequencer_state_t *seq_state = domain_sequencer_engine_get_state();
                uint16_t bar_len = (g_mode_cfg.seqpara_bar == 0U) ? 1U : g_mode_cfg.seqpara_bar;
                uint8_t target_bar = (uint8_t)(g_mode_cfg.seqpara_sel_bar % bar_len);

                if ((seq_state != 0) && (seq_state->playing != 0U) && (target_bar == seq_state->bar_count))
                {
                    uint8_t record_channel = (uint8_t)(g_mode_cfg.seqpara_sel_channel & 0x0FU);

                    if ((msg_type == IO_MIDI_EVT0_NOTEON) && (data2 != 0U))
                    {
                        if (g_seqinput_rec_in_bar == 0U)
                        {
                            domain_sequencer_engine_begin_record_layer(target_bar);
                        }
                        domain_sequencer_engine_record_note_on(target_bar, record_channel, data1, data2, now_tick);
                        g_seqinput_rec_in_bar = 1U;
                    }
                    else
                    {
                        domain_sequencer_engine_record_note_off(target_bar, record_channel, data1, now_tick);
                    }
                }
            }
        }

        if (is_seq_mode != 0U)
        {
            if ((msg_type == IO_MIDI_EVT0_NOTEON) || (msg_type == IO_MIDI_EVT0_NOTEOFF))
            {
                uint8_t route_channel = (uint8_t)(g_mode_cfg.seqpara_sel_channel & 0x0FU);
                packet[1] = (uint8_t)((packet[1] & 0xF0U) | route_channel);
                channel = route_channel;
            }
        }

        if ((msg_type == IO_MIDI_EVT0_CONTROLCHANGE) && (data1 == IO_MIDI_CC_VOLUME) && (channel < APP_MIDI_CHANNEL_COUNT))
        {
            g_channel_volume[channel] = data2;
            if (g_volume_muted != 0U)
            {
                g_volume_before_mute[channel] = data2;
                packet[3] = 0U;
            }
        }

        io_midi_transport_send_packet(packet);

        if ((msg_type == IO_MIDI_EVT0_CONTROLCHANGE) && (data1 == IO_MIDI_CC_USER_MODECONTROL))
        {
            if (data2 < IO_MIDI_MODE_STATE_COUNT)
            {
                app_runtime_request_mode_change(data2, now_tick);
                app_runtime_show_overlay(DOMAIN_OVERLAY_MODE, data2, now_tick);
            }
        }
        else if ((msg_type == IO_MIDI_EVT0_PROGRAMCHANGE) && (g_current_mode != IO_MIDI_MODE_STATE_SEQUENCER_PLAY))
        {
            app_runtime_show_overlay_midi_rx(DOMAIN_OVERLAY_PROGRAM, data1, now_tick, msg_type, channel, data1, data2);
        }
        else if ((msg_type == IO_MIDI_EVT0_CONTROLCHANGE) && (g_current_mode != IO_MIDI_MODE_STATE_SEQUENCER_PLAY))
        {
            if ((data1 == IO_MIDI_CC_DAMPERPEDAL) || (data1 == IO_MIDI_CC_USER_MODECONTROL))
            {
                continue;
            }

            if (data1 == IO_MIDI_CC_VOLUME)
            {
                app_runtime_show_overlay_midi_rx(DOMAIN_OVERLAY_VOLUME, data2, now_tick, msg_type, channel, data1, data2);
            }
            else if (data1 == IO_MIDI_CC_USER_REVERB_LEVEL)
            {
                app_runtime_show_overlay_midi_rx(DOMAIN_OVERLAY_REVERB, data2, now_tick, msg_type, channel, data1, data2);
            }
            else
            {
                app_runtime_show_overlay_midi_rx(DOMAIN_OVERLAY_MIDI_IN, (uint16_t)(channel + 1U), now_tick, msg_type, channel, data1, data2);
            }
        }
    }
}

static void app_runtime_update_seqinput_auto_advance(void)
{
    const domain_sequencer_state_t *seq_state = domain_sequencer_engine_get_state();
    uint8_t current_bar;
    uint16_t bar_len;
    uint16_t mode_offset;
    uint16_t target_bar;

    if (seq_state == 0)
    {
        return;
    }

    current_bar = (uint8_t)((seq_state->bar == 0U) ? 0U : (seq_state->bar_count % seq_state->bar));
    if (current_bar == g_seqinput_last_bar)
    {
        return;
    }

    if ((g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM) && (g_seqinput_rec_in_bar != 0U))
    {
        bar_len = (g_mode_cfg.seqpara_bar == 0U) ? 1U : g_mode_cfg.seqpara_bar;
        target_bar = (uint16_t)(g_mode_cfg.seqpara_sel_bar % bar_len);
        mode_offset = (uint16_t)((g_mode_cfg.seqpara_sel_bar / bar_len) * bar_len);

        target_bar = (uint16_t)((target_bar + 1U) % bar_len);
        g_mode_cfg.seqpara_sel_bar = (uint16_t)(mode_offset + target_bar);
        domain_sequencer_engine_set_selected_bar((uint8_t)target_bar);
    }

    g_seqinput_rec_in_bar = 0U;
    g_seqinput_last_bar = current_bar;
}

static void app_runtime_seqprogram_stop_preview(void)
{
    if (g_seqprogram_preview_active == 0U)
    {
        return;
    }

    app_runtime_send_note((uint8_t)g_mode_cfg.seqpara_sel_channel, g_seqprogram_preview_note, 0U, 0U);
    g_seqprogram_preview_active = 0U;
    g_seqprogram_preview_off_tick = 0U;
}

static void app_runtime_seqprogram_start_preview(uint32_t now_tick)
{
    app_runtime_seqprogram_stop_preview();
    app_runtime_send_note((uint8_t)g_mode_cfg.seqpara_sel_channel, g_seqprogram_note, APP_SEQ_PROGRAM_NOTE_VELOCITY, 1U);
    g_seqprogram_preview_note = g_seqprogram_note;
    g_seqprogram_preview_active = 1U;
    g_seqprogram_preview_off_tick = now_tick + APP_SEQ_PROGRAM_PREVIEW_MS;
}

static uint8_t app_runtime_seqprogram_get_target_bar(uint8_t *out_bar)
{
    const domain_sequencer_state_t *seq_state = domain_sequencer_engine_get_state();
    uint16_t bar_len;
    uint8_t target_bar;

    if ((out_bar == 0) || (seq_state == 0) || (seq_state->playing == 0U))
    {
        return 0U;
    }

    bar_len = (g_mode_cfg.seqpara_bar == 0U) ? 1U : g_mode_cfg.seqpara_bar;
    target_bar = (uint8_t)(g_mode_cfg.seqpara_sel_bar % bar_len);
    if (target_bar != seq_state->bar_count)
    {
        return 0U;
    }

    *out_bar = target_bar;
    return 1U;
}

static void app_runtime_seqprogram_handle_button_down(uint32_t now_tick)
{
    uint8_t target_bar;
    uint8_t record_channel = (uint8_t)(g_mode_cfg.seqpara_sel_channel & 0x0FU);

    app_runtime_seqprogram_stop_preview();
    app_runtime_send_note(record_channel, g_seqprogram_note, APP_SEQ_PROGRAM_NOTE_VELOCITY, 1U);
    g_seqprogram_hold_active = 1U;
    g_seqprogram_hold_note = g_seqprogram_note;

    if (app_runtime_seqprogram_get_target_bar(&target_bar) == 0U)
    {
        return;
    }

    if (g_seqinput_rec_in_bar == 0U)
    {
        domain_sequencer_engine_begin_record_layer(target_bar);
    }
    domain_sequencer_engine_record_note_on(target_bar, record_channel, g_seqprogram_note, APP_SEQ_PROGRAM_NOTE_VELOCITY, now_tick);
    g_seqinput_rec_in_bar = 1U;
}

static void app_runtime_seqprogram_handle_button_up(uint32_t now_tick)
{
    uint8_t target_bar;
    uint8_t record_channel = (uint8_t)(g_mode_cfg.seqpara_sel_channel & 0x0FU);

    if (g_seqprogram_hold_active == 0U)
    {
        return;
    }

    app_runtime_send_note(record_channel, g_seqprogram_hold_note, 0U, 0U);
    g_seqprogram_hold_active = 0U;

    if (app_runtime_seqprogram_get_target_bar(&target_bar) == 0U)
    {
        return;
    }

    domain_sequencer_engine_record_note_off(target_bar, record_channel, g_seqprogram_hold_note, now_tick);
}

static void app_runtime_process_seqprogram_preview(uint32_t now_tick)
{
    if ((g_seqprogram_preview_active != 0U) && (now_tick >= g_seqprogram_preview_off_tick))
    {
        app_runtime_seqprogram_stop_preview();
    }
}

static void app_runtime_seqprogram_abort_active_notes(void)
{
    uint8_t record_channel = (uint8_t)(g_mode_cfg.seqpara_sel_channel & 0x0FU);

    app_runtime_seqprogram_stop_preview();
    if (g_seqprogram_hold_active != 0U)
    {
        app_runtime_send_note(record_channel, g_seqprogram_hold_note, 0U, 0U);
        g_seqprogram_hold_active = 0U;
    }
}

/**
 * @brief Sends a Control Change message for a specific MIDI channel. This function updates a control parameter.
 * @param channel The MIDI channel to send the control change message.
 * @param cc The control change number.
 * @param value The value to set for the control change.
 */
static void app_runtime_send_cc(uint8_t channel, uint8_t cc, uint8_t value)
{
    if (cc == IO_MIDI_CC_USER_MODECONTROL)
    {
        app_midi_modeControl(value);
        return;
    }

    app_midi_controlchange(channel, cc, value);
}

static void app_runtime_send_all_notes_off(void)
{
    uint8_t ch;

    for (ch = 0U; ch < APP_MIDI_CHANNEL_COUNT; ch++)
    {
        app_runtime_send_cc(ch, IO_MIDI_CC_ALLNOTESOFF, 0U);
    }
}

static uint8_t app_runtime_is_sequencer_mode(uint8_t mode)
{
    return (uint8_t)((mode == IO_MIDI_MODE_STATE_SEQUENCER_PARA) ||
                     (mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM) ||
                     (mode == IO_MIDI_MODE_STATE_SEQUENCER_REC) ||
                     (mode == IO_MIDI_MODE_STATE_SEQUENCER_PLAY));
}

/**
 * @brief Sends a Note On or Note Off message for a specific MIDI channel.
 * @param channel The MIDI channel to send the note message.
 * @param note The note number.
 * @param velocity The velocity of the note.
 * @param is_note_on Flag indicating whether the note is on (1) or off (0).
 */
static void app_runtime_send_note(uint8_t channel, uint8_t note, uint8_t velocity, uint8_t is_note_on)
{
    if (is_note_on != 0U)
    {
        app_midi_noteon(channel, note, velocity);
    }
    else
    {
        app_midi_noteoff(channel, note);
    }
}

/**
 * @brief Sets the local MIDI context for display purposes.
 * @param midi_msg_type The type of MIDI message.
 * @param data1 The first data byte of the MIDI message.
 * @param data2 The second data byte of the MIDI message.
 */
static void app_runtime_set_local_midi_context(uint8_t midi_msg_type, uint8_t data1, uint8_t data2)
{
    if (g_midi_ctrl.channel <= APP_MIDI_CHANNEL_MAX)
    {
        domain_display_model_set_midi_context(midi_msg_type, (uint8_t)g_midi_ctrl.channel, data1, data2, 0U);
    }
    else
    {
        domain_display_model_clear_midi_context();
    }
}

/**
 * @brief Processes sequencer playback events. This function handles the playback of sequencer events.
 */
static void app_runtime_process_sequencer_playback(void)
{
    domain_sequencer_playback_event_t events[16];
    uint8_t count;
    uint8_t i;

    if ((g_current_mode != IO_MIDI_MODE_STATE_SEQUENCER_PARA) &&
        (g_current_mode != IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM) &&
        (g_current_mode != IO_MIDI_MODE_STATE_SEQUENCER_REC) &&
        (g_current_mode != IO_MIDI_MODE_STATE_SEQUENCER_PLAY))
    {
        return;
    }

    count = domain_sequencer_engine_collect_playback_events(events, (uint8_t)(sizeof(events) / sizeof(events[0])));
    for (i = 0; i < count; i++)
    {
        if (events[i].type == DOMAIN_SEQUENCER_PLAYBACK_EVENT_NOTEON)
        {
            app_runtime_send_note(events[i].channel, events[i].note, events[i].velocity, 1U);
        }
        else
        {
            app_runtime_send_note(events[i].channel, events[i].note, 0U, 0U);
        }
    }
}

/**
 * @brief Processes metronome click events. This function handles the metronome click based on the current mode and sequencer state.
 */
static void app_runtime_process_metronome_click(void)
{
    static uint16_t last_metronome_count;
    uint16_t metronome_count;
    uint8_t enable_click = 0U;
    const domain_sequencer_state_t *seq_state = domain_sequencer_engine_get_state();
    uint8_t is_sequencer_mode = 0U;
    uint8_t beat_division = 1U;
    uint8_t click_velocity = METRONOME_VELOCITY;

    if (g_current_mode == IO_MIDI_MODE_STATE_METRONOME)
    {
        enable_click = 1U;
    }
    else if ((g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PARA) ||
             (g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM) ||
             (g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_REC) ||
             (g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PLAY))
    {
        is_sequencer_mode = 1U;
        enable_click = (g_mode_cfg.seqpara_metronome != 0U) ? 1U : 0U;
        if ((g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PLAY) &&
            ((seq_state == 0) || (seq_state->playing == 0U)))
        {
            enable_click = 0U;
        }
    }

    if (enable_click == 0U)
    {
        last_metronome_count = domain_sequencer_engine_get_metronome_count();
        return;
    }

    metronome_count = domain_sequencer_engine_get_metronome_count();
    if (metronome_count != last_metronome_count)
    {
        last_metronome_count = metronome_count;
        if ((is_sequencer_mode != 0U) && (seq_state != 0) && (seq_state->beat != 0U))
        {
            beat_division = seq_state->beat;
            click_velocity = ((metronome_count % beat_division) == 0U) ? METRONOME_ACCENT_VELOCITY : METRONOME_SUB_VELOCITY;
        }
        app_runtime_send_note(IO_MIDI_DRUMCH, METRONOME_NOTENUM, 0U, 0U);
        app_runtime_send_note(IO_MIDI_DRUMCH, METRONOME_NOTENUM, click_velocity, 1U);
    }
}

/**
 * @brief Shows an overlay message on the display. This function sets the overlay message and its expiration time.
 * @param msg_type The type of the message.
 * @param value The value associated with the message.
 * @param now_tick The current tick count.
 */
static void app_runtime_show_overlay(uint16_t msg_type, uint16_t value, uint32_t now_tick)
{
    app_runtime_show_overlay_with_timeout(msg_type, value, now_tick, APP_OVERLAY_TIMEOUT_MS);
}

static void app_runtime_show_overlay_with_timeout(uint16_t msg_type, uint16_t value, uint32_t now_tick, uint32_t timeout_ms)
{
    int16_t msg = (int16_t)(((msg_type & 0x0FU) << 12) | (value & 0x0FFFU));

    if ((msg_type != DOMAIN_OVERLAY_VOLUME) &&
        (msg_type != DOMAIN_OVERLAY_PROGRAM))
    {
        domain_display_model_clear_midi_context();
    }

    domain_display_model_set_state(DOMAIN_DISPLAY_OVERLAY, msg);
    g_overlay_expire_tick = now_tick + timeout_ms;
}

/**
 * @brief Shows an overlay message for received MIDI messages. This function sets the overlay message and its expiration time, including MIDI context information.
 *
 * @param msg_type The type of the message.
 * @param value The value associated with the message.
 * @param now_tick The current tick count.
 * @param midi_msg_type The type of the MIDI message.
 * @param channel The MIDI channel.
 * @param data1 The first data byte of the MIDI message.
 * @param data2 The second data byte of the MIDI message.
 */
static void app_runtime_show_overlay_midi_rx(uint16_t msg_type, uint16_t value, uint32_t now_tick, uint8_t midi_msg_type, uint8_t channel, uint8_t data1, uint8_t data2)
{
    int16_t msg = (int16_t)(((msg_type & 0x0FU) << 12) | (value & 0x0FFFU));
    domain_display_model_set_midi_context(midi_msg_type, channel, data1, data2, 1U);
    domain_display_model_set_state(DOMAIN_DISPLAY_OVERLAY, msg);
    g_overlay_expire_tick = now_tick + APP_OVERLAY_TIMEOUT_MS;
}

/**
 * @brief Restores the base display state. This function sets the display to the state corresponding to the current mode.
 */
static void app_runtime_restore_base_display(void)
{
    const domain_mode_def_t *mode_def = domain_mode_def_get(g_current_mode);

    if (mode_def != 0)
    {
        domain_display_model_set_state(mode_def->base_display, 0);
        return;
    }

    domain_display_model_set_state(DOMAIN_DISPLAY_NOTE, 0);
}

typedef void (*app_runtime_command_handler_t)(const app_command_t *cmd);

static void app_runtime_handle_cmd_none(const app_command_t *cmd)
{
    (void)cmd;
}

static void app_runtime_handle_cmd_midi_send(const app_command_t *cmd)
{
    io_midi_transport_send_packet(cmd->payload.midi);
}

static void app_runtime_handle_cmd_display_update(const app_command_t *cmd)
{
    if (domain_display_model_get_state() != DOMAIN_DISPLAY_OVERLAY)
    {
        domain_display_model_set_state((domain_display_state_t)cmd->payload.display, 0);
    }
}

static void app_runtime_handle_cmd_mode_change(const app_command_t *cmd)
{
    uint8_t prev_mode = g_current_mode;

    if ((g_current_mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM) &&
        (cmd->payload.mode != IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM))
    {
        app_runtime_seqprogram_abort_active_notes();
    }

    if ((app_runtime_is_sequencer_mode(prev_mode) != 0U) &&
        (app_runtime_is_sequencer_mode((uint8_t)cmd->payload.mode) == 0U))
    {
        app_runtime_send_all_notes_off();
    }

    if ((prev_mode == IO_MIDI_MODE_STATE_FREEPLAY) &&
        (cmd->payload.mode != IO_MIDI_MODE_STATE_FREEPLAY))
    {
        io_display_renderer_reset_note_state();
    }

    g_current_mode = cmd->payload.mode;
    g_re1_mode_select = g_current_mode;

    if ((g_current_mode == IO_MIDI_MODE_STATE_SETTING) &&
        (prev_mode != IO_MIDI_MODE_STATE_SETTING))
    {
        g_mode_cfg.setting_item = IO_DISPLAY_SETTING_ITEM_ANIMATION;
        g_overlay_expire_tick = 0U;
        app_runtime_sync_setting_view();
        app_runtime_restore_base_display();
    }

    if (app_runtime_mode_supports_param_edit(g_current_mode) == 0U)
    {
        g_modeparam_latch = 0U;
    }
}

/**
 * @brief Handles a node change command. This function updates the current node and related control states based on the command payload.
 *
 * @param cmd Pointer to the command containing the node change information.
 */
static void app_runtime_handle_cmd_node_change(const app_command_t *cmd)
{
    g_ctx.current_node = cmd->payload.node;
    if (APP_NODE_GROUP(cmd->payload.node) == APP_NODE_GROUP_RE1)
    {
        uint8_t re1 = (uint8_t)APP_NODE_VALUE(cmd->payload.node);
        if (re1 < APP_RE1_CTRL_COUNT)
        {
            g_re1_ctrl = re1;
        }
    }
    else if (APP_NODE_GROUP(cmd->payload.node) == APP_NODE_GROUP_RE2)
    {
        uint8_t re2 = (uint8_t)APP_NODE_VALUE(cmd->payload.node);
        if (re2 < APP_RE2_CTRL_COUNT)
        {
            g_re2_ctrl = re2;
        }
    }
    else if (cmd->payload.node < APP_RE1_CTRL_COUNT)
    {
        /* Backward-compatible path for legacy untagged RE1 node values. */
        g_re1_ctrl = (uint8_t)cmd->payload.node;
    }
}

static const app_runtime_command_handler_t g_command_handlers[] = {
    app_runtime_handle_cmd_none,
    app_runtime_handle_cmd_midi_send,
    app_runtime_handle_cmd_display_update,
    app_runtime_handle_cmd_mode_change,
    app_runtime_handle_cmd_node_change,
};

/**
 * @brief Pops an event from the event queue. This function retrieves the next event from the queue if available.
 * @param out_event Pointer to store the retrieved event.
 * @return 1 if an event was retrieved, 0 if the queue is empty.
 */
static uint8_t app_runtime_event_pop(app_event_t *out_event)
{
    if (g_event_rpos == g_event_wpos)
    {
        return 0;
    }

    *out_event = g_event_queue[g_event_rpos];
    g_event_rpos = (uint16_t)((g_event_rpos + 1U) % APP_EVENT_QUEUE_CAPACITY);
    return 1;
}

/**
 * @brief Executes a list of commands. This function processes the given commands and performs the corresponding actions.
 * @param cmds Pointer to the array of commands to execute.
 * @param count The number of commands in the array.
 */
static void app_runtime_execute_commands(const app_command_t *cmds, uint8_t count)
{
    uint8_t i;

    if ((cmds == 0) || (count == 0U))
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        if ((cmds[i].type <= CMD_NODE_CHANGE) && (g_command_handlers[cmds[i].type] != 0))
        {
            g_command_handlers[cmds[i].type](&cmds[i]);
        }
    }
}

/**
 * @brief Initializes the runtime context. This function sets up the initial state of the runtime context.
 */
void app_runtime_init(void)
{
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.current_mode = IO_MIDI_MODE_STATE_FREEPLAY;
    g_event_wpos = 0;
    g_event_rpos = 0;

    app_runtime_reset_default_state();

    domain_sequencer_engine_init();
    domain_sequencer_engine_set_playing(1);
    domain_sequencer_engine_set_tempo(g_mode_cfg.metronome_bpm, (uint8_t)g_mode_cfg.seqpara_beat);
    domain_sequencer_engine_set_bar((uint8_t)g_mode_cfg.seqpara_bar);
    domain_sequencer_engine_set_selected_bar(0U);
    io_display_renderer_set_seq_channel((uint8_t)g_mode_cfg.seqpara_sel_channel);
    io_display_renderer_set_seq_playmode((uint8_t)g_mode_cfg.seqpara_playmode);
}

/**
 * @brief Starts the runtime. This function initializes the runtime and sets the initial mode.
 */
void app_runtime_start(void)
{
    const domain_mode_def_t *def = domain_mode_def_get_default();
    if (def != 0)
    {
        g_ctx.current_mode = def->mode_id;
    }

    app_runtime_sync_setting_view();
    app_runtime_restore_base_display();

    /* Startup behavior: initialize synth-side mode control state. */
    app_runtime_send_cc(0, IO_MIDI_CC_USER_MODECONTROL, IO_MIDI_MODECONTROL_CABLENUMFILTON);
}

/**
 * @brief Steps the runtime. This function processes events and updates the system state.
 * @param now_tick The current tick count.
 */
void app_runtime_step(uint32_t now_tick)
{
    app_event_t ev;
    app_command_t cmds[APP_COMMAND_BUFFER_CAPACITY];
    uint8_t cmd_count;

    g_ctx.now_tick = now_tick;
    g_ctx.current_mode = g_current_mode;

    domain_sequencer_engine_tick(now_tick);
    app_runtime_update_seqinput_auto_advance();
    app_runtime_process_metronome_click();
    app_runtime_process_sequencer_playback();
    app_runtime_process_seqprogram_preview(now_tick);

    app_runtime_process_rx_midi(now_tick);

    if ((g_overlay_expire_tick > 0U) && (now_tick >= g_overlay_expire_tick))
    {
        g_overlay_expire_tick = 0U;
        app_runtime_restore_base_display();
    }

    while (app_runtime_event_pop(&ev))
    {
        app_runtime_handle_input_event(&ev);
        g_ctx.current_mode = g_current_mode;

        cmd_count = 0;
        app_dispatcher_handle_event(&g_ctx, &ev, cmds, &cmd_count);
        app_runtime_execute_commands(cmds, cmd_count);
    }

    cmd_count = 0;
    app_dispatcher_handle_tick(&g_ctx, now_tick, cmds, &cmd_count);
    app_runtime_execute_commands(cmds, cmd_count);

    g_ctx.current_mode = g_current_mode;
}

/**
 * @brief Posts an event to the event queue. This function adds a new event to the queue for processing.
 * @param ev The event to post.
 */
void app_runtime_post_event(const app_event_t *ev)
{
    uint16_t next_wpos;
    if (ev == 0)
    {
        return;
    }

    next_wpos = (uint16_t)((g_event_wpos + 1U) % APP_EVENT_QUEUE_CAPACITY);
    if (next_wpos == g_event_rpos)
    {
        /* Queue full: drop newest to preserve current behavior safety. */
        return;
    }

    g_event_queue[g_event_wpos] = *ev;
    g_event_wpos = next_wpos;
}

/**
 * @brief Gets the runtime context. This function returns a pointer to the current runtime context.
 * @return Pointer to the runtime context.
 */
const app_runtime_context_t *app_runtime_get_context(void)
{
    return &g_ctx;
}

/**
 * @brief Initializes the input system. This function sets up the initial state of the input controls.
 */
void app_runtime_input_init(void)
{
    app_runtime_reset_default_state();

    io_display_renderer_set_seq_channel((uint8_t)g_mode_cfg.seqpara_sel_channel);
    io_display_renderer_set_seq_playmode((uint8_t)g_mode_cfg.seqpara_playmode);

    app_runtime_restore_base_display();
}

/**
 * @brief Handles the RE2 rotate mode parameter input event. This function processes the input event for the RE2 rotate control when in mode parameter mode.
 *
 * @param ev  The input event to handle.
 * @param effective_mode The effective mode of the application at the time of the event.
 * @return uint8_t
 */
static uint8_t app_runtime_handle_re2_rotate_modeparam(const app_event_t *ev, uint8_t effective_mode)
{
    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM)
    {
        return app_runtime_dispatch_re2_rotate_modeparam(ev, effective_mode);
    }

    if (app_runtime_mode_supports_param_edit(effective_mode) == 0U)
    {
        return 0U;
    }

    return app_runtime_dispatch_re2_rotate_modeparam(ev, effective_mode);
}

/**
 * @brief Handles the RE2 rotate normal input event. This function processes the input event for the RE2 rotate control when in normal mode.
 * @param ev The input event to handle.
 * @return uint8_t
 */
static uint8_t app_runtime_handle_re2_rotate_normal(const app_event_t *ev)
{
    if (g_re2_ctrl == APP_RE2_CTRL_CHANNEL)
    {
        g_midi_ctrl.channel = (uint8_t)app_runtime_clamp_add_u16(g_midi_ctrl.channel, ev->delta, 0, APP_MIDI_CHANNEL_ALL, 1);
        app_runtime_show_overlay(DOMAIN_OVERLAY_CHANNEL, g_midi_ctrl.channel, ev->tick);
        return 1U;
    }

    g_midi_ctrl.reverb = (uint8_t)app_runtime_clamp_add_u16(g_midi_ctrl.reverb, ev->delta, 0, 127, 4);
    app_runtime_send_cc(0, IO_MIDI_CC_USER_REVERB_LEVEL, g_midi_ctrl.reverb);
    app_runtime_show_overlay(DOMAIN_OVERLAY_REVERB, g_midi_ctrl.reverb, ev->tick);
    return 1U;
}

/**
 * @brief Handles the RE2 button special mode input event. This function processes the input event for the RE2 button when in special mode.
 * @param ev The input event to handle.
 * @param effective_mode The effective mode of the application at the time of the event.
 * @return uint8_t
 */
static uint8_t app_runtime_handle_re2_button_special_mode(const app_event_t *ev, uint8_t effective_mode)
{
    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM)
    {
        if (ev->type == EV_BUTTON_DOWN)
        {
            app_runtime_seqprogram_handle_button_down(ev->tick);
        }
        return 1U;
    }

    if (ev->type != EV_SHORT_PRESS)
    {
        return 0U;
    }

    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_REC)
    {
        uint8_t input_action = 0U;
        uint8_t target_bar = 0U;

        app_runtime_get_seqinput_selection(&input_action, &target_bar);
        app_runtime_apply_seqinput_action(input_action, target_bar);
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_INPUT_ACTION,
                                 (uint16_t)((((uint16_t)input_action & 0x3U) << 8) | ((uint16_t)target_bar & 0xFFU)),
                                 ev->tick);

        return 1U;
    }

    if (effective_mode == IO_MIDI_MODE_STATE_SEQUENCER_PLAY)
    {
        app_runtime_handle_seqplay_button_action();
        app_runtime_show_overlay(DOMAIN_OVERLAY_SEQ_PLAY_ACTION, g_mode_cfg.seqpara_playmode, ev->tick);
        return 1U;
    }

    if (effective_mode == IO_MIDI_MODE_STATE_SETTING)
    {
        if (g_mode_cfg.setting_item == IO_DISPLAY_SETTING_ITEM_ANIMATION)
        {
            g_mode_cfg.animation_mode = (g_mode_cfg.animation_mode == IO_DISPLAY_ANIMATION_MODE_1) ? IO_DISPLAY_ANIMATION_MODE_2 : IO_DISPLAY_ANIMATION_MODE_1;
            app_runtime_sync_setting_view();
            g_overlay_expire_tick = 0U;
            domain_display_model_set_state(DOMAIN_DISPLAY_SETTING, 0);
        }
        else if (g_mode_cfg.setting_item == IO_DISPLAY_SETTING_ITEM_GOTODFU)
        {
            app_runtime_show_overlay(DOMAIN_OVERLAY_SETTING, 0U, ev->tick);
            app_midi_modeControl(IO_MIDI_MODECONTROL_GOTODFU);
            io_midi_transport_service();
            (void)io_system_control_goto_dfu();
        }
        return 1U;
    }

    return 0U;
}

/**
 * @brief Handles the RE2 button mode parameter input event. This function processes the input event for the RE2 button when in mode parameter mode.
 * @param ev The input event to handle.
 * @param effective_mode The effective mode of the application at the time of the event.
 * @return uint8_t
 */
static uint8_t app_runtime_handle_re2_button_modeparam(const app_event_t *ev, uint8_t effective_mode)
{
    if (app_runtime_mode_supports_param_edit(effective_mode) == 0U)
    {
        return 0U;
    }

    if (app_runtime_dispatch_mode_action(ev,
                                         effective_mode,
                                         g_re2_button_modeparam_actions,
                                         (size_t)(sizeof(g_re2_button_modeparam_actions) / sizeof(g_re2_button_modeparam_actions[0]))) != 0U)
    {
        return 1U;
    }

    app_runtime_show_overlay(DOMAIN_OVERLAY_MODE, effective_mode, ev->tick);
    return 1U;
}

/**
 * @brief Handles the RE2 button up event for the SEQUENCER_PROGRAM mode. This function processes the button up event for the RE2 button when in SEQUENCER_PROGRAM mode.
 * @param ev The input event to handle.
 * @param effective_mode The effective mode of the application at the time of the event.
 * @return uint8_t
 */
static uint8_t app_runtime_handle_re2_button_up_program(const app_event_t *ev, uint8_t effective_mode)
{
    if ((ev == 0) || (ev->source != APP_INPUT_SOURCE_RE2) || (ev->type != EV_BUTTON_UP))
    {
        return 0U;
    }

    if (effective_mode != IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM)
    {
        return 0U;
    }

    app_runtime_seqprogram_handle_button_up(ev->tick);
    return 1U;
}

/**
 * @brief Handles the RE2 button normal mode input event. This function processes the input event for the RE2 button when in normal mode.
 * @param ev The input event to handle.
 * @return uint8_t
 */
static uint8_t app_runtime_handle_re2_button_normal(const app_event_t *ev)
{
    uint16_t next_ctrl = (uint16_t)((g_re2_ctrl + 1U) % APP_RE2_CTRL_COUNT);

    app_runtime_request_node_change(app_runtime_re2_ctrl_to_node((uint8_t)next_ctrl), ev->tick);
    g_modeparam_latch = 0U;

    if (next_ctrl == APP_RE2_CTRL_CHANNEL)
    {
        app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL, APP_CONTROL_HINT_RE2_CHANNEL, ev->tick, APP_OVERLAY_HINT_TIMEOUT_MS);
    }
    else
    {
        app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL, APP_CONTROL_HINT_RE2_REVERB, ev->tick, APP_OVERLAY_HINT_TIMEOUT_MS);
    }

    return 1U;
}

/**
 * @brief Handles an input event. This function processes the given input event and updates the system state accordingly.
 * @param ev The input event to handle.
 */
void app_runtime_handle_input_event(const app_event_t *ev)
{
    uint8_t effective_mode;
    uint8_t progress_step;
    uint8_t source_idx;

    if (ev == 0)
    {
        return;
    }

    if (ev->type == EV_LONG_PRESS_PROGRESS)
    {
        if (ev->delta < 0)
        {
            progress_step = 0U;
        }
        else if (ev->delta > 32)
        {
            progress_step = 32U;
        }
        else
        {
            progress_step = (uint8_t)ev->delta;
        }

        if (progress_step < 16U)
        {
            return;
        }

        source_idx = (ev->source == APP_INPUT_SOURCE_RE2) ? 1U : 0U;
        app_runtime_show_overlay_with_timeout(DOMAIN_OVERLAY_CONTROL,
                                              app_runtime_encode_hold_progress_overlay(source_idx, progress_step),
                                              ev->tick,
                                              APP_OVERLAY_HOLD_PROGRESS_TIMEOUT_MS);
        return;
    }

    effective_mode = (uint8_t)domain_mode_def_resolve_mode_for_display(domain_display_model_get_state(), (app_mode_state_t)g_current_mode);

    if (app_runtime_handle_re2_button_up_program(ev, effective_mode) != 0U)
    {
        return;
    }

    (void)app_input_router_dispatch_input_event(ev,
                                                effective_mode,
                                                &g_input_router_handlers);
}
