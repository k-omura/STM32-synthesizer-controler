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
    IO_MIDI_MODECONTROL_SAVECONFIG = 10, // Save config
    IO_MIDI_MODECONTROL_ERASECONFIG,     // Erase config

    IO_MIDI_MODECONTROL_CABLENUMFILTOFF = 20, // CableN
    IO_MIDI_MODECONTROL_CABLENUMFILTON,       // CableN
    IO_MIDI_MODECONTROL_CABLENUMFILTONDEEP,   // Deep C

    IO_MIDI_MODECONTROL_GOTODFU = 100, // Goto DFU mode
} io_midi_mode_control_t;

/* MIDI Control Change (CC) Constants */
typedef enum
{
    IO_MIDI_CC_BANKSLECT = 0,              // Allows user to switch bank for patch selection. Program change used with Bank Select. MIDI can access 16,384 patches per MIDI channel.
    IO_MIDI_CC_MODWHELL,                   // Generally this CC controls a vibrato effect (pitch, loudness, brighness). What is modulated is based on the patch.
    IO_MIDI_CC_BREATHCONT,                 // Oftentimes associated with aftertouch messages. It was originally intended for use with a breath MIDI controller in which blowing harder produced higher MIDI control values. It can be used for modulation as well.
    IO_MIDI_CC_FOOTCONT = 4,               // Often used with aftertouch messages. It can send a continuous stream of values based on how the pedal is used.
    IO_MIDI_CC_PORTAMENTOTIME,             // Controls portamento rate to slide between 2 notes played subsequently.
    IO_MIDI_CC_DATAENTRY,                  // Controls Value for NRPN or RPN parameters.
    IO_MIDI_CC_VOLUME,                     // Controls the volume of the channel.
    IO_MIDI_CC_BALANCE,                    // Controls the left and right balance, generally for stereo patches. A value of 64 equals the center.
    IO_MIDI_CC_PAN = 10,                   // Controls the left and right balance, generally for mono patches. A value of 64 equals the center.
    IO_MIDI_CC_EXPRESSION,                 // Expression is a percentage of volume (CC7).
    IO_MIDI_CC_EFFECTCONT1,                // Usually used to control a parameter of an effect within the synth or workstation.
    IO_MIDI_CC_EFFECTCONT2,                // Usually used to control a parameter of an effect within the synth or workstation.
    IO_MIDI_CC_GENERAL16 = 16,             //
    IO_MIDI_CC_GENERAL17,                  //
    IO_MIDI_CC_GENERAL18,                  //
    IO_MIDI_CC_GENERAL19,                  //
    IO_MIDI_CC_USER_ATTACKLEVEL = 20,      //
    IO_MIDI_CC_USER_WAVE1SINEAMP,          //
    IO_MIDI_CC_USER_WAVE1SQUAMP,           //
    IO_MIDI_CC_USER_WAVE1SQUDUTY,          //
    IO_MIDI_CC_USER_WAVE1TRIAMP,           //
    IO_MIDI_CC_USER_WAVE1TRIPEAK,          //
    IO_MIDI_CC_USER_WAVE1PITCH,            //
    IO_MIDI_CC_USER_WAVE1OCTAVE,           //
    IO_MIDI_CC_USER_WAVE2SINEAMP,          //
    IO_MIDI_CC_USER_WAVE2SQUAMP,           //
    IO_MIDI_CC_USER_WAVE2SQUDUTY,          //
    IO_MIDI_CC_USER_WAVE2TRIAMP,           //
    IO_MIDI_CC_USER_WAVE2TRIPEAK,          //
    IO_MIDI_CC_USER_WAVE2PITCH,            //
    IO_MIDI_CC_USER_WAVE2OCTAVE,           //
    IO_MIDI_CC_USER_DISTORTION_LEVEL,      //
    IO_MIDI_CC_USER_PITCHBYNOTE,           //
    IO_MIDI_CC_USER_FILTERTYPE,            //
    IO_MIDI_CC_USER_CH0MIRRORCH,           //
    IO_MIDI_CC_USER_TRE_TYPE,              //
    IO_MIDI_CC_USER_TRE_PEAK,              //
    IO_MIDI_CC_USER_TRE_FREQ,              //
    IO_MIDI_CC_USER_VIB_TYPE,              //
    IO_MIDI_CC_USER_VIB_PEAK,              //
    IO_MIDI_CC_USER_VIB_AMP,               //
    IO_MIDI_CC_USER_VIB_FREQ,              //
    IO_MIDI_CC_USER_WOW_TYPE,              //
    IO_MIDI_CC_USER_WOW_PEAK,              //
    IO_MIDI_CC_USER_WOW_AMP,               //
    IO_MIDI_CC_USER_WOW_FREQ,              //
    IO_MIDI_CC_USER_ENV_VOL_DIFF,          //
    IO_MIDI_CC_USER_ENV_VOL_TIME,          //
    IO_MIDI_CC_USER_ENV_FREQ_DIFF,         //
    IO_MIDI_CC_USER_ENV_FREQ_TIME,         //
    IO_MIDI_CC_USER_ENV_WOW_DIFF,          //
    IO_MIDI_CC_USER_ENV_WOW_TIME,          //
    IO_MIDI_CC_USER_DRUM_NN,               //
    IO_MIDI_CC_USER_DRUM_TYPE,             //
    IO_MIDI_CC_USER_DRUM_LPF_FCNN,         //
    IO_MIDI_CC_USER_DRUM_HPF_FCNN,         //
    IO_MIDI_CC_USER_DRUM_LPF_Q,            //
    IO_MIDI_CC_USER_DRUM_HPF_Q,            //
    IO_MIDI_CC_DAMPERPEDAL = 64,           // ≤63 off, ≥64 on. On/off switch that controls sustain pedal. Nearly every synth will react to CC 64. (See also Sostenuto CC 66)
    IO_MIDI_CC_PORTAMENTO,                 // ≤63 off, ≥64 on. On/off switch
    IO_MIDI_CC_SOSTENTOPEDAL,              // ≤63 off, ≥64 on. On/off switch – Like the Sustain controller (CC 64), However, it only holds notes that were “On” when the pedal was pressed. People use it to “hold” chords” and play melodies over the held chord.
    IO_MIDI_CC_SOFTPEDAL,                  // ≤63 off, ≥64 on.	On/off switch – Lowers the volume of notes played.
    IO_MIDI_CC_LEGATOFOOTSWITCH,           // ≤63 off, ≥64 on. On/off switch – Turns Legato effect between 2 subsequent notes on or off.
    IO_MIDI_CC_HOLD2,                      // ≤63 off, ≥64 on	Another way to “hold notes” (see MIDI CC 64 and MIDI CC 66). However notes fade out according to their release parameter rather than when the pedal is released.
    IO_MIDI_CC_SOUNDCONT1,                 // Usually controls the way a sound is produced. Default = Sound Variation.
    IO_MIDI_CC_SOUNDCONT2,                 // Allows shaping the Voltage Controlled Filter (VCF). Default = Resonance also (Timbre or Harmonics)
    IO_MIDI_CC_USER_FILTERQFACTOR = 71,    // Resonance of the filter(q factor)
    IO_MIDI_CC_SOUNDCONT3,                 // Controls release time of the Voltage controlled Amplifier (VCA). Default = Release Time.
    IO_MIDI_CC_USER_RELEASETIME = 72,      // Release Time
    IO_MIDI_CC_SOUNDCONT4,                 // Controls the “Attack’ of a sound. The attack is the amount of time it takes for the sound to reach maximum amplitude.
    IO_MIDI_CC_USER_ATTACKTIME = 73,       // Controls the “Attack’ of a sound. The attack is the amount of time it takes for the sound to reach maximum amplitude.
    IO_MIDI_CC_SOUNDCONT5,                 // Controls VCFs cutoff frequency of the filter.
    IO_MIDI_CC_USER_FILTERCUTOFF = 74,     // cutoff frequency of the filter
    IO_MIDI_CC_GENERAL75,                  // Generic – Some manufacturers may use to further shave their sounds.
    IO_MIDI_CC_GENERAL76,                  // Generic – Some manufacturers may use to further shave their sounds.
    IO_MIDI_CC_GENERAL77,                  // Generic – Some manufacturers may use to further shave their sounds.
    IO_MIDI_CC_GENERAL78,                  // Generic – Some manufacturers may use to further shave their sounds.
    IO_MIDI_CC_GENERAL79,                  // Generic – Some manufacturers may use to further shave their sounds.
    IO_MIDI_CC_GENERAL80,                  // Decay Generic or on/off switch ≤63 off, ≥64 on
    IO_MIDI_CC_USER_DECAYTIME = 80,        // Decay time
    IO_MIDI_CC_GENERAL81,                  // Hi-Pass Filter Frequency or Generic on/off switch ≤63 off, ≥64 on
    IO_MIDI_CC_GENERAL82,                  // Generic on/off switch ≤63 off, ≥64 on
    IO_MIDI_CC_GENERAL83,                  // Generic on/off switch ≤63 off, ≥64 on
    IO_MIDI_CC_PORTAMENTOCC,               // Controls the amount of Portamento.
    IO_MIDI_CC_HIGHRESVELOCITYPREFIX = 88, // Extends the range of possible velocity values
    IO_MIDI_CC_EFFECTDEPTH1 = 91,          // Usually controls reverb send amount
    IO_MIDI_CC_USER_REVERB_LEVEL = 91,     // Reverb amount
    IO_MIDI_CC_EFFECTDEPTH2,               // Usually controls tremolo amount
    IO_MIDI_CC_USER_TRE_AMP = 92,          //
    IO_MIDI_CC_EFFECTDEPTH3,               // Usually controls chorus amount
    IO_MIDI_CC_EFFECTDEPTH4,               // Usually controls detune amount
    IO_MIDI_CC_EFFECTDEPTH5,               // Usually controls phaser amount
    IO_MIDI_CC_DATAINCREMENT,              // Usually used to increment data for RPN and NRPN messages.
    IO_MIDI_CC_DATADECREMENT,              // Usually used to decrement data for RPN and NRPN messages.
    IO_MIDI_CC_NRPNLSB,                    // For controllers 6, 38, 96, and 97, it selects the NRPN parameter.
    IO_MIDI_CC_NRPNMSB,                    // For controllers 6, 38, 96, and 97, it selects the NRPN parameter.
    IO_MIDI_CC_RPNLSB,                     // For controllers 6, 38, 96, and 97, it selects the RPN parameter.
    IO_MIDI_CC_RPNMSB,                     // For controllers 6, 38, 96, and 97, it selects the RPN parameter.
    IO_MIDI_CC_USER_MTRE_TYPE,             //
    IO_MIDI_CC_USER_MTRE_PEAK,             //
    IO_MIDI_CC_USER_MTRE_AMP,              //
    IO_MIDI_CC_USER_MTRE_FREQ,             //
    IO_MIDI_CC_USER_MVIB_TYPE,             //
    IO_MIDI_CC_USER_MVIB_PEAK,             //
    IO_MIDI_CC_USER_MVIB_AMP,              //
    IO_MIDI_CC_USER_MVIB_FREQ,             //
    IO_MIDI_CC_USER_MWOW_TYPE,             //
    IO_MIDI_CC_USER_MWOW_PEAK,             //
    IO_MIDI_CC_USER_MWOW_AMP,              //
    IO_MIDI_CC_USER_MWOW_FREQ,             //
    IO_MIDI_CC_USER_MFILTER_TYPE,          //
    IO_MIDI_CC_USER_MFILTER_QFACTOR,       //
    IO_MIDI_CC_USER_MFILTER_CUTOFF,        //
    IO_MIDI_CC_USER_PRESET = 118,          // Preset
    IO_MIDI_CC_USER_MODECONTROL = 119,     //
    IO_MIDI_CC_ALLSOUNDOFF = 120,          // Mutes all sound. It does so regardless of release time or sustain. (See MIDI CC 123)
    IO_MIDI_CC_RESETALL,                   // It will reset all controllers to their default.
    IO_MIDI_CC_LOCALSWITCH,                // Turns internal connection of a MIDI keyboard or workstation, etc. on or off. If you use a computer, you will most likely want local control off to avoid notes being played twice. Once locally and twice when the note is sent back from the computer to your keyboard.
    IO_MIDI_CC_ALLNOTESOFF,                // Mutes all sounding notes. Release time will still be maintained, and notes held by sustain will not turn off until sustain pedal is depressed.
    IO_MIDI_CC_OMNIMODEOFF,                // Sets to “Omni Off” mode.
    IO_MIDI_CC_OMNIMODEON,                 // Sets to “Omni On” mode.
    IO_MIDI_CC_MONOMODE,                   // Sets device mode to Monophonic. The value equals the number of channels, or 0 if the number of channels equals the number of voices in the receiver.
    IO_MIDI_CC_POLYMODE                    // Sets device mode to Polyphonic.
} io_midi_cc_t;

#endif /* IO_MIDI_TYPES_H */
