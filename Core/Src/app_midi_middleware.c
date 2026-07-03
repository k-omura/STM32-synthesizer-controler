/**
 * @file app_midi_middleware.c
 * @author k-omura
 * @brief MIDI middleware implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "app_midi_middleware.h"

/**
 * @brief Handles the main MIDI processing loop. This function is called periodically to process incoming MIDI messages, update the sequencer state, and manage the display updates. It takes care of reading MIDI packets from the transport layer, handling sequencer events, and updating the display based on the current state of the application.
 *
 * @param _ch The MIDI channel to process.
 */
void stm32synth_disp_channel(uint8_t _ch)
{
    (void)_ch;
}

/**
 * @brief Displays the mode selection screen for the specified mode.
 * @param mode The mode ID to display.
 */
void stm32synth_disp_mode(uint8_t mode)
{
    (void)mode;
}

/**
 * @brief Displays the MIDI input activity. This function updates the display to indicate that a MIDI message has been received. It sets the appropriate display code and message to show the MIDI input status, and triggers a display update to reflect the change on the screen.
 * @param _bpm
 */
void stm32synth_disp_metronome(uint16_t _bpm)
{
    (void)_bpm;
}

/**
 * @brief Displays the settings screen. This function updates the display to show the current settings of the MIDI application. It sets the appropriate display code and message to indicate the settings, and triggers a display update to reflect the change on the screen.
 * @param _target The target setting to display.
 */
void stm32synth_disp_setting(uint16_t _target)
{
    (void)_target;
}

/* ===== Sequencer Control ===== */
/**
 * @brief Sets the BPM (Beats Per Minute) for the sequencer. This function updates the BPM of the sequencer and metronome, recalculates the BPM tick, and updates the display to reflect the change.
 * @param _bpm The BPM value to set.
 */
void stm32synth_seq_bpm(uint16_t _bpm)
{
    (void)_bpm;
}

/**
 * @brief Sets the beat for the sequencer. This function updates the beat of the sequencer and updates the display to reflect the change.
 * @param _beat The beat value to set.
 */
void stm32synth_seq_beat(uint8_t _beat)
{
    (void)_beat;
}

/**
 * @brief Sets the bar for the sequencer. This function updates the bar of the sequencer and updates the display to reflect the change.
 * @param _bar The bar value to set.
 */
void stm32synth_seq_bar(uint16_t _bar)
{
    (void)_bar;
}

/**
 * @brief Sets the MIDI channel for the sequencer. This function updates the selected MIDI channel and updates the display to reflect the change.
 * @param _ch The MIDI channel to set.
 */
void stm32synth_seq_channel(uint8_t _ch)
{
    (void)_ch;
}

/**
 * @brief Selects the sequencer parameter page. This updates the display state so the legacy UI keeps track of the active parameter selection.
 * @param _select The parameter selection index.
 */
void stm32synth_seq_select(uint8_t _select)
{
    (void)_select;
}

/**
 * @brief Sets the selected bar for the sequencer. This function updates the selected bar and recording mode of the sequencer and updates the display to reflect the change.
 * @param _bar The bar value to set.
 * @param _mode The recording mode to set.
 */
void stm32synth_seq_selbar(uint16_t _bar, uint8_t _mode)
{
    (void)_bar;
    (void)_mode;
}

/**
 * @brief Sets the metronome mode for the sequencer. This function updates the metronome mode of the sequencer and updates the display to reflect the change.
 * @param _mode The metronome mode to set.
 */
void stm32synth_seq_metronome(uint8_t _mode)
{
    (void)_mode;
}

/**
 * @brief Handles the program push action for the sequencer. This function updates the sequencer state based on the mode.
 * @param _mode The mode to set.
 */
void stm32synth_seq_progpush(uint8_t _mode)
{
    (void)_mode;
}

void stm32synth_seq_set_playing(uint8_t playing)
{
    (void)playing;
}

/**
 * @brief Removes the most recent sequencer layer from the currently selected bar.
 */
void stm32synth_seq_undo(void)
{
}

/**
 * @brief Clears all sequencer records for the specified channel.
 * @param _ch The MIDI channel to clear.
 */
void stm32synth_seq_chclear(uint8_t _ch)
{
    (void)_ch;
}

/**
 * @brief Cleans the recorded sequences. This function clears all recorded sequences in the sequencer and resets the layer count for each bar.
 */
void stm32synth_seq_cleanrecs(void)
{
}

/**
 * @brief Sets the mute state for a specific channel in the sequencer. This function updates the selected channel and display to reflect the change.
 * @param _ch The channel to mute.
 */
void stm32synth_seq_mutech(uint8_t _ch)
{
    (void)_ch;
}

/**
 * @brief Toggles the mute state for a specific channel in the sequencer. This function updates the mute state of the specified channel and updates the display to reflect the change.
 * @param _ch The channel to toggle mute.
 */
void stm32synth_seq_mutechtoggle(uint8_t _ch)
{
    (void)_ch;
}
