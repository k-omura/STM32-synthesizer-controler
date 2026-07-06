/**
 * @file io_midi_types.h
 * @author k-omura
 * @brief MIDI type definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef IO_MIDI_TYPES_H
#define IO_MIDI_TYPES_H

#include <stdint.h>
#include "main.h"

/* MIDI Buffer Configuration */
#define IO_MIDI_MIDIBUFF_SIZE (256) //!< Size of the MIDI buffer for incoming and outgoing messages
#define IO_MIDI_DRUMCH (9)          //!< Default MIDI channel for drum sounds (channel 10 in 1-based indexing)
#define IO_MIDI_LOWESTDRUM (35)     //!< Lowest MIDI note number for drum sounds

/* Timing & Sequencer Configuration */
#define IO_MIDI_DEFAULT_BPM (120)                  //!< Default beats per minute for the sequencer
#define IO_MIDI_DEFAULT_BPM_MAX (360)              //!< Maximum beats per minute for the sequencer
#define IO_MIDI_DEFAULT_BPM_MIN (30)               //!< Minimum beats per minute for the sequencer
#define IO_MIDI_SEQUENCER_MAXBAR (16)              //!< Maximum number of bars in the sequencer
#define IO_MIDI_SEQUENCER_MAXCHORDPARBAR (80)      //!< Maximum number of chord parameters per bar in the sequencer
#define IO_MIDI_SEQUENCER_RECTIMERESOLUTION (4095) //!< Resolution of the sequencer's real-time clock timer

/* Result Status */
typedef enum
{
    IO_MIDI_RES_OK = 0,
    IO_MIDI_RES_NG,
} io_midi_result_t;

/* MIDI Event Types */
typedef enum
{
    IO_MIDI_EVT0_NOTEOFF = 0x80,
    IO_MIDI_EVT0_NOTEON = 0x90,
    IO_MIDI_EVT0_AFTERTOUCH = 0xA0,
    IO_MIDI_EVT0_CONTROLCHANGE = 0xB0,
    IO_MIDI_EVT0_PROGRAMCHANGE = 0xC0,
    IO_MIDI_EVT0_CHANNELAFTERTOUCH = 0xD0,
    IO_MIDI_EVT0_PITCHBEND = 0xE0,
} io_midi_event_type_t;

/* Mode States */
typedef enum
{
    IO_MIDI_MODE_STATE_FREEPLAY = 0,
    IO_MIDI_MODE_STATE_METRONOME,
    IO_MIDI_MODE_STATE_SEQUENCER_PARA,
    IO_MIDI_MODE_STATE_SEQUENCER_PLAY,
    IO_MIDI_MODE_STATE_SEQUENCER_PROGRAM,
    IO_MIDI_MODE_STATE_SEQUENCER_REC,
    IO_MIDI_MODE_STATE_SETTING,
    IO_MIDI_MODE_STATE_COUNT,
} io_midi_mode_state_t;

/* Display States */
typedef enum
{
    IO_MIDI_DISPSTATE_HELLO = 0,
    IO_MIDI_DISPSTATE_MODESEL,
    IO_MIDI_DISPSTATE_NOTE,
    IO_MIDI_DISPSTATE_METRONOME,
    IO_MIDI_DISPSTATE_SEQUENCER_PARA,
    IO_MIDI_DISPSTATE_SEQUENCER_INPUT,
    IO_MIDI_DISPSTATE_SEQUENCER_PLAY,
    IO_MIDI_DISPSTATE_CHANNEL,
    IO_MIDI_DISPSTATE_VOLALL,
    IO_MIDI_DISPSTATE_MIDIIN,
    IO_MIDI_DISPSTATE_SETCH,
    IO_MIDI_DISPSTATE_SETTING,
} io_midi_display_state_t;

/* MIDI Mode Control */
typedef enum
{
    IO_MIDI_MODECONTROL_CABLENUMFILTON = 0,
} io_midi_mode_control_t;

/* MIDI Control Change (CC) Constants */
typedef enum
{
    IO_MIDI_CC_BANKSLECT = 0,
    IO_MIDI_CC_MODWHELL,
    IO_MIDI_CC_BREATHCONT,
    IO_MIDI_CC_GENERAL3,
    IO_MIDI_CC_FOOTCONT,
    IO_MIDI_CC_PORTAMENTOTIME,
    IO_MIDI_CC_DATAENTRY,
    IO_MIDI_CC_VOLUME,
    IO_MIDI_CC_BALANCE,
    IO_MIDI_CC_GENERAL9,
    IO_MIDI_CC_PAN,
    IO_MIDI_CC_EXPRESSION,
    IO_MIDI_CC_EFFECTCONT1,
    IO_MIDI_CC_EFFECTCONT2,
    IO_MIDI_CC_GENERAL14,
    IO_MIDI_CC_GENERAL15,
    IO_MIDI_CC_GENERAL16,
    IO_MIDI_CC_GENERAL17,
    IO_MIDI_CC_GENERAL18,
    IO_MIDI_CC_GENERAL19,
    IO_MIDI_CC_USER_ATTACKLEVEL,
    IO_MIDI_CC_USER_WAVE1SINEAMP,
    IO_MIDI_CC_DAMPERPEDAL = 64,
    IO_MIDI_CC_SOSTENTOPEDAL,
    IO_MIDI_CC_SOFTPEDAL,
    IO_MIDI_CC_LEGATOFOOTSWITCH,
    IO_MIDI_CC_HOLD2,
    IO_MIDI_CC_SOUNDCONT1,
    IO_MIDI_CC_SOUNDCONT2,
    IO_MIDI_CC_USER_LPFQFACTOR,
    IO_MIDI_CC_USER_RELEASETIME,
    IO_MIDI_CC_USER_ATTACKTIME,
    IO_MIDI_CC_USER_LPFCUTOFF,
    IO_MIDI_CC_USER_REVERB_LEVEL = 91,
    IO_MIDI_CC_USER_MODECONTROL = 102,
    IO_MIDI_CC_ALLSOUNDOFF = 120,
    IO_MIDI_CC_RESETALL,
    IO_MIDI_CC_LOCALSWITCH,
    IO_MIDI_CC_ALLNOTESOFF,
    IO_MIDI_CC_OMNIMODEOFF,
    IO_MIDI_CC_OMNIMODEON,
    IO_MIDI_CC_MONOMODE,
    IO_MIDI_CC_POLYMODE,
} io_midi_cc_t;

#endif /* IO_MIDI_TYPES_H */
