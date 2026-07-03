/**
 * @file domain_mode_def.c
 * @author k-omura
 * @brief Mode definition implementation
 * @version 0.1
 * @date 2026-07-02
 */

#include "domain_mode_def.h"
#include "io_midi_types.h"

/* Keep in sync with APP_COMMAND_BUFFER_CAPACITY without depending on app layer. */
#define DOMAIN_MODE_COMMAND_CAPACITY 16U

static void domain_mode_push_display(domain_display_state_t display, app_command_t *out_cmds, uint8_t *out_count)
{
    if ((out_cmds == 0) || (out_count == 0))
    {
        return;
    }

    if (*out_count >= DOMAIN_MODE_COMMAND_CAPACITY)
    {
        return;
    }

    out_cmds[*out_count].type = CMD_DISPLAY_UPDATE;
    out_cmds[*out_count].payload.display = (app_display_state_t)display;
    (*out_count)++;
}

static void domain_mode_on_enter_freeplay(void *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count)
{
    (void)ctx;
    (void)ev;
    domain_mode_push_display(DOMAIN_DISPLAY_NOTE, out_cmds, out_count);
}

static void domain_mode_on_enter_metronome(void *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count)
{
    (void)ctx;
    (void)ev;
    domain_mode_push_display(DOMAIN_DISPLAY_METRONOME, out_cmds, out_count);
}

static void domain_mode_on_enter_seqpara(void *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count)
{
    (void)ctx;
    (void)ev;
    domain_mode_push_display(DOMAIN_DISPLAY_SEQUENCER_PARA, out_cmds, out_count);
}

static void domain_mode_on_enter_seqinput(void *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count)
{
    (void)ctx;
    (void)ev;
    domain_mode_push_display(DOMAIN_DISPLAY_SEQUENCER_INPUT, out_cmds, out_count);
}

static void domain_mode_on_enter_seqplay(void *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count)
{
    (void)ctx;
    (void)ev;
    domain_mode_push_display(DOMAIN_DISPLAY_SEQUENCER_PLAY, out_cmds, out_count);
}

static void domain_mode_on_enter_setting(void *ctx, const app_event_t *ev, app_command_t *out_cmds, uint8_t *out_count)
{
    (void)ctx;
    (void)ev;
    domain_mode_push_display(DOMAIN_DISPLAY_SETTING, out_cmds, out_count);
}

/* Mode state constants */
#define MODE_STATE_FREEPLAY IO_MIDI_MODE_STATE_FREEPLAY               //!< Freeplay mode state
#define MODE_STATE_METRONOME IO_MIDI_MODE_STATE_METRONOME             //!< Metronome mode state
#define MODE_STATE_SEQUENCER_PARA IO_MIDI_MODE_STATE_SEQUENCER_PARA   //!< Sequencer parameter mode state
#define MODE_STATE_SEQUENCER_INPUT IO_MIDI_MODE_STATE_SEQUENCER_INPUT //!< Sequencer input mode state
#define MODE_STATE_SEQUENCER_PLAY IO_MIDI_MODE_STATE_SEQUENCER_PLAY   //!< Sequencer play mode state
#define MODE_STATE_SETTING IO_MIDI_MODE_STATE_SETTING                 //!< Setting mode state

static const domain_mode_def_t g_mode_defs[] = {
    {MODE_STATE_FREEPLAY, DOMAIN_DISPLAY_NOTE, 0, 0, domain_mode_on_enter_freeplay, 0, 0, 0},
    {MODE_STATE_METRONOME, DOMAIN_DISPLAY_METRONOME, 0, 0, domain_mode_on_enter_metronome, 0, 0, 0},
    {MODE_STATE_SEQUENCER_PARA, DOMAIN_DISPLAY_SEQUENCER_PARA, 0, 0, domain_mode_on_enter_seqpara, 0, 0, 0},
    {MODE_STATE_SEQUENCER_INPUT, DOMAIN_DISPLAY_SEQUENCER_INPUT, 0, 0, domain_mode_on_enter_seqinput, 0, 0, 0},
    {MODE_STATE_SEQUENCER_PLAY, DOMAIN_DISPLAY_SEQUENCER_PLAY, 0, 0, domain_mode_on_enter_seqplay, 0, 0, 0},
    {MODE_STATE_SETTING, DOMAIN_DISPLAY_SETTING, 0, 0, domain_mode_on_enter_setting, 0, 0, 0},
};

/**
 * @brief Gets the mode definition for a given mode ID. This function returns a pointer to the mode definition.
 * @param mode_id The mode ID to look up.
 * @return Pointer to the mode definition.
 */
const domain_mode_def_t *domain_mode_def_get(app_mode_state_t mode_id)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)(sizeof(g_mode_defs) / sizeof(g_mode_defs[0])); i++)
    {
        if (g_mode_defs[i].mode_id == mode_id)
        {
            return &g_mode_defs[i];
        }
    }

    return &g_mode_defs[0];
}

/**
 * @brief Gets the default mode definition. This function returns a pointer to the default mode definition.
 * @return Pointer to the default mode definition.
 */
const domain_mode_def_t *domain_mode_def_get_default(void)
{
    return &g_mode_defs[0];
}

app_mode_state_t domain_mode_def_resolve_mode_for_display(domain_display_state_t state, app_mode_state_t fallback_mode)
{
    uint8_t i;

    if (state == DOMAIN_DISPLAY_OVERLAY)
    {
        return fallback_mode;
    }

    for (i = 0; i < (uint8_t)(sizeof(g_mode_defs) / sizeof(g_mode_defs[0])); i++)
    {
        if (g_mode_defs[i].base_display == state)
        {
            return g_mode_defs[i].mode_id;
        }
    }

    return fallback_mode;
}
