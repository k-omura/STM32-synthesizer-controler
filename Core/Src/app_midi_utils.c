/**
 * @file app_midi_utils.c
 * @author k-omura
 * @brief MIDI utility functions implementation
 * @version 0.1
 * @date 2026-07-02
 */

#include "app_midi_utils.h"
#include "io_midi_transport.h"
#include "io_midi_types.h"

#define APP_MIDI_CHANNEL_COUNT 16U

/* ===== MIDI Message Generation ===== */
/**
 * @brief Builds a 4-byte MIDI packet and sends it through the shared transport queue.
 * @param cin The MIDI packet code index.
 * @param status The MIDI status byte.
 * @param data1 The first data byte.
 * @param data2 The second data byte.
 */
static void app_midi_send_packet(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2)
{
    uint8_t midi_tx[4];

    midi_tx[0] = cin;
    midi_tx[1] = status;
    midi_tx[2] = data1;
    midi_tx[3] = data2;
    io_midi_transport_send_packet(midi_tx);
}

/* ===== Public API: Basic MIDI ===== */
/**
 * @brief Sends a Note On message for a specific MIDI channel. This function triggers a note on event.
 * @param ch The MIDI channel to send the note on message.
 * @param notenum The note number to play.
 * @param velocity The velocity of the note.
 */
void app_midi_noteon(uint8_t ch, uint8_t notenum, uint8_t velocity)
{
    app_midi_send_packet((uint8_t)((0x0U << 4) | ((IO_MIDI_EVT0_NOTEON >> 4) & 0x0f)),
                         (uint8_t)(IO_MIDI_EVT0_NOTEON | (ch & 0x0f)),
                         (uint8_t)(notenum & 0x7f),
                         (uint8_t)(velocity & 0x7f));
}

/**
 * @brief Sends a Note Off message for a specific MIDI channel. This function triggers a note off event.
 * @param ch The MIDI channel to send the note off message.
 * @param notenum The note number to stop.
 */
void app_midi_noteoff(uint8_t ch, uint8_t notenum)
{
    app_midi_send_packet((uint8_t)((0x0U << 4) | ((IO_MIDI_EVT0_NOTEOFF >> 4) & 0x0f)),
                         (uint8_t)(IO_MIDI_EVT0_NOTEOFF | (ch & 0x0f)),
                         (uint8_t)(notenum & 0x7f),
                         0);
}

/**
 * @brief Sends a Control Change message for a specific MIDI channel. This function updates a control parameter.
 * @param ch The MIDI channel to send the control change message.
 * @param cc The control change number.
 * @param val The value to set for the control change.
 */
void app_midi_controlchange(uint8_t ch, uint8_t cc, uint8_t val)
{
    app_midi_send_packet((uint8_t)((0x0U << 4) | ((IO_MIDI_EVT0_CONTROLCHANGE >> 4) & 0x0f)),
                         (uint8_t)(IO_MIDI_EVT0_CONTROLCHANGE | (ch & 0x0f)),
                         cc,
                         val);
}

/**
 * @brief Sends a Program Change message for a specific MIDI channel. This function updates the program of the specified channel.
 * @param ch The MIDI channel to send the program change message.
 * @param program The program number to set.
 */
void app_midi_programchange(uint8_t ch, uint8_t program)
{
    app_midi_send_packet((uint8_t)((0x0U << 4) | ((IO_MIDI_EVT0_PROGRAMCHANGE >> 4) & 0x0f)),
                         (uint8_t)(IO_MIDI_EVT0_PROGRAMCHANGE | (ch & 0x0f)),
                         program,
                         0x00);
}

/**
 * @brief Sends a Pitch Bend message for a specific MIDI channel. This function updates the pitch bend value.
 * @param ch The MIDI channel to send the pitch bend message.
 * @param pitch The pitch bend value to set.
 */
void app_midi_pitchbend(uint8_t ch, uint8_t pitch)
{
    app_midi_send_packet((uint8_t)((0x0U << 4) | ((IO_MIDI_EVT0_PITCHBEND >> 4) & 0x0f)),
                         (uint8_t)(IO_MIDI_EVT0_PITCHBEND | (ch & 0x0f)),
                         0x00,
                         pitch);
}

/**
 * @brief Sets the MIDI mode for the synthesizer. This function updates the MIDI mode.
 * @param mode The MIDI mode to set.
 */
void app_midi_modeControl(uint8_t mode)
{
    app_midi_send_packet((uint8_t)((0x1U << 4) | ((IO_MIDI_EVT0_CONTROLCHANGE >> 4) & 0x0f)),
                         (uint8_t)(IO_MIDI_EVT0_CONTROLCHANGE | 0x00),
                         IO_MIDI_CC_USER_MODECONTROL,
                         mode);
}

void app_midi_send_control(uint8_t channel, uint8_t cc, uint8_t value)
{
    if (cc == IO_MIDI_CC_USER_MODECONTROL)
    {
        app_midi_modeControl(value);
        return;
    }

    app_midi_controlchange(channel, cc, value);
}
