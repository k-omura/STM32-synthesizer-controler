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
    IO_MIDI_MODE_STATE_SEQUENCER_INPUT,
    IO_MIDI_MODE_STATE_SEQUENCER_PLAY,
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

/* ===== Legacy Compatibility Aliases ===== */
/* These aliases maintain backward compatibility with old code */

/* Event type aliases */
#define STM32SYNTH_MIDIEVT0_NOTEOFF IO_MIDI_EVT0_NOTEOFF                     //!< Legacy alias for Note Off event
#define STM32SYNTH_MIDIEVT0_NOTEON IO_MIDI_EVT0_NOTEON                       //!< Legacy alias for Note On event
#define STM32SYNTH_MIDIEVT0_AFTERTOUCH IO_MIDI_EVT0_AFTERTOUCH               //!< Legacy alias for Aftertouch event
#define STM32SYNTH_MIDIEVT0_CONTROLCHANGE IO_MIDI_EVT0_CONTROLCHANGE         //!< Legacy alias for Control Change event
#define STM32SYNTH_MIDIEVT0_PROGRAMCHANGE IO_MIDI_EVT0_PROGRAMCHANGE         //!< Legacy alias for Program Change event
#define STM32SYNTH_MIDIEVT0_CHANNELAFTERTOUCH IO_MIDI_EVT0_CHANNELAFTERTOUCH //!< Legacy alias for Channel Aftertouch event
#define STM32SYNTH_MIDIEVT0_PITCHBEND IO_MIDI_EVT0_PITCHBEND                 //!< Legacy alias for Pitch Bend event

/* Mode control aliases */
#define STM32SYNTH_MODECONTROL_CABLENUMFILTON IO_MIDI_MODECONTROL_CABLENUMFILTON //!< Legacy alias for Cable Number Filter On

/* Result type aliases */
#define STM32SYNTH_RES_OK IO_MIDI_RES_OK  //!< Legacy alias for OK result
#define STM32SYNTH_RES_NG IO_MIDI_RES_NG  //!< Legacy alias for NG result
#define stm32synth_res_t io_midi_result_t //!< Legacy alias for MIDI result type

/* Mode state aliases */
#define STM32SYNTH_MIDIMODE_FREEPLAY IO_MIDI_MODE_STATE_FREEPLAY               //!< Legacy alias for Freeplay mode
#define STM32SYNTH_MIDIMODE_METRONOME IO_MIDI_MODE_STATE_METRONOME             //!< Legacy alias for Metronome mode
#define STM32SYNTH_MIDIMODE_SEQUENCER_PARA IO_MIDI_MODE_STATE_SEQUENCER_PARA   //!< Legacy alias for Sequencer Parameter mode
#define STM32SYNTH_MIDIMODE_SEQUENCER_INPUT IO_MIDI_MODE_STATE_SEQUENCER_INPUT //!< Legacy alias for Sequencer Input mode
#define STM32SYNTH_MIDIMODE_SEQUENCER_PLAY IO_MIDI_MODE_STATE_SEQUENCER_PLAY   //!< Legacy alias for Sequencer Play mode
#define STM32SYNTH_MIDIMODE_SETTING IO_MIDI_MODE_STATE_SETTING                 //!< Legacy alias for Setting mode

/* Buffer and timing aliases */
#define STM32SYNTH_MIDIBUFF_SIZE IO_MIDI_MIDIBUFF_SIZE //!< Legacy alias for MIDI buffer size
#define STM32SYNTH_MIDINN_DRUMCH IO_MIDI_DRUMCH        //!< Legacy alias for MIDI drum channel
#define STM32SYNTH_LOWESTDRUM IO_MIDI_LOWESTDRUM       //!< Legacy alias for lowest drum note
#define STM32SYNTH_DEFAULT_BPM IO_MIDI_DEFAULT_BPM     //!< Legacy alias for default BPM

/* MIDI CC aliases */
#define STM32SYNTH_MIDICC_BANKSLECT IO_MIDI_CC_BANKSLECT                 //!< Legacy alias for Bank Select CC
#define STM32SYNTH_MIDICC_MODWHELL IO_MIDI_CC_MODWHELL                   //!< Legacy alias for Mod Wheel CC
#define STM32SYNTH_MIDICC_DAMPERPEDAL IO_MIDI_CC_DAMPERPEDAL             //!< Legacy alias for Damper Pedal CC
#define STM32SYNTH_MIDICC_SOSTENTOPEDAL IO_MIDI_CC_SOSTENTOPEDAL         //!< Legacy alias for Sostenuto Pedal CC
#define STM32SYNTH_MIDICC_SOFTPEDAL IO_MIDI_CC_SOFTPEDAL                 //!< Legacy alias for Soft Pedal CC
#define STM32SYNTH_MIDICC_LEGATOFOOTSWITCH IO_MIDI_CC_LEGATOFOOTSWITCH   //!< Legacy alias for Legato Footswitch CC
#define STM32SYNTH_MIDICC_HOLD2 IO_MIDI_CC_HOLD2                         //!< Legacy alias for Hold 2 CC
#define STM32SYNTH_MIDICC_SOUNDCONT1 IO_MIDI_CC_SOUNDCONT1               //!< Legacy alias for Sound Control 1 CC
#define STM32SYNTH_MIDICC_SOUNDCONT2 IO_MIDI_CC_SOUNDCONT2               //!< Legacy alias for Sound Control 2 CC
#define STM32SYNTH_MIDICC_USER_REVERB_LEVEL IO_MIDI_CC_USER_REVERB_LEVEL //!< Legacy alias for User Reverb Level CC
#define STM32SYNTH_MIDICC_USER_MODECONTROL IO_MIDI_CC_USER_MODECONTROL   //!< Legacy alias for User Mode Control CC
#define STM32SYNTH_MIDICC_ALLSOUNDOFF IO_MIDI_CC_ALLSOUNDOFF             //!< Legacy alias for All Sound Off CC
#define STM32SYNTH_MIDICC_RESETALL IO_MIDI_CC_RESETALL                   //!< Legacy alias for Reset All CC
#define STM32SYNTH_MIDICC_LOCALSWITCH IO_MIDI_CC_LOCALSWITCH             //!< Legacy alias for Local Switch CC
#define STM32SYNTH_MIDICC_ALLNOTESOFF IO_MIDI_CC_ALLNOTESOFF             //!< Legacy alias for All Notes Off CC
#define STM32SYNTH_MIDICC_OMNIMODEOFF IO_MIDI_CC_OMNIMODEOFF             //!< Legacy alias for Omni Mode Off CC
#define STM32SYNTH_MIDICC_OMNIMODEON IO_MIDI_CC_OMNIMODEON               //!< Legacy alias for Omni Mode On CC
#define STM32SYNTH_MIDICC_MONOMODE IO_MIDI_CC_MONOMODE                   //!< Legacy alias for Mono Mode CC
#define STM32SYNTH_MIDICC_POLYMODE IO_MIDI_CC_POLYMODE                   //!< Legacy alias for Poly Mode CC
#define STM32SYNTH_MIDICC_VOLUME IO_MIDI_CC_VOLUME                       //!< Legacy alias for Volume CC
#define STM32SYNTH_MIDICC_USER_LPFCUTOFF IO_MIDI_CC_USER_LPFCUTOFF       //!< Legacy alias for User LPF Cutoff CC
#define STM32SYNTH_MIDICC_USER_LPFQFACTOR IO_MIDI_CC_USER_LPFQFACTOR     //!< Legacy alias for User LPF Q Factor CC
#define STM32SYNTH_MIDICC_USER_ATTACKTIME IO_MIDI_CC_USER_ATTACKTIME     //!< Legacy alias for User Attack Time CC
#define STM32SYNTH_MIDICC_USER_RELEASETIME IO_MIDI_CC_USER_RELEASETIME   //!< Legacy alias for User Release Time CC
#define STM32SYNTH_MIDICC_USER_ATTACKLEVEL IO_MIDI_CC_USER_ATTACKLEVEL   //!< Legacy alias for User Attack Level CC

#endif /* IO_MIDI_TYPES_H */
