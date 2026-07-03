/**
 * @file app_midi_constants.c
 * @author k-omura
 * @brief MIDI constants and text tables implementation
 * @version 0.1
 * @date 2026-06-22
 *
 */

#include "app_midi_constants.h"

/* ===== Color Definitions ===== */
const app_midi_color_t APP_MIDI_COLOR_RESET = {0, 0, 0};
const app_midi_color_t APP_MIDI_COLOR_STRING = {8, 8, 8};
const app_midi_color_t APP_MIDI_COLOR_BLE = {0, 0, 10};
const app_midi_color_t APP_MIDI_COLOR_BAR_ALL = {6, 3, 3};
const app_midi_color_t APP_MIDI_COLOR_CURSORX = {2, 2, 2};
const app_midi_color_t APP_MIDI_COLOR_CURSORY = {1, 2, 1};
const app_midi_color_t APP_MIDI_COLOR_SEL_BAR = {1, 1, 10};
const app_midi_color_t APP_MIDI_COLOR_SEL_CHANNEL = {1, 10, 1};
const app_midi_color_t APP_MIDI_COLOR_SEL_PLAY = {10, 1, 1};
const app_midi_color_t APP_MIDI_COLOR_SEL_STOP = {8, 8, 8};
const app_midi_color_t APP_MIDI_COLOR_SEL_REC = {0, 10, 0};

const app_midi_color_t APP_MIDI_COLOR_SEL_RECNOTE[8] = {
    {20, 4, 4},
    {4, 20, 4},
    {4, 4, 20},
    {20, 20, 4},
    {4, 20, 20},
    {20, 4, 20},
    {20, 12, 0},
    {0, 12, 20},
};

/* ===== Control Change Text (103 entries) ===== */
const char stm32synth_cc_text[][3] = {
    "BNK", // 0: BANKSLECT
    "MWL", // 1: MODWHELL
    "BCT", // 2: BREATHCONT
    "G03", // 3: GENERAL3
    "FCT", // 4: FOOTCONT
    "PTM", // 5: PORTAMENTOTIME
    "DEN", // 6: DATAENTRY
    "VOL", // 7: VOLUME
    "BAL", // 8: BALANCE
    "G09", // 9: GENERAL9
    "PAN", // 10: PAN
    "EXP", // 11: EXPRESSION
    "E1C", // 12: EFFECTCONT1
    "E2C", // 13: EFFECTCONT2
    "G14", // 14: GENERAL14
    "G15", // 15: GENERAL15
    "G16", // 16: GENERAL16
    "G17", // 17: GENERAL17
    "G18", // 18: GENERAL18
    "G19", // 19: GENERAL19
    "AKL", // 20: USER_ATTACKLEVEL
    "1NA", // 21: USER_WAVE1SINEAMP
    "1QA", // 22: USER_WAVE1SQUAMP
    "1QD", // 23: USER_WAVE1SQUDUTY
    "1TA", // 24: USER_WAVE1TRIAMP
    "1TP", // 25: USER_WAVE1TRIPEAK
    "1PT", // 26: USER_WAVE1PITCH
    "1OC", // 27: USER_WAVE1OCTAVE
    "2NA", // 28: USER_WAVE2SINEAMP
    "2QA", // 29: USER_WAVE2SQUAMP
    "2QD", // 30: USER_WAVE2SQUDUTY
    "2TA", // 31: USER_WAVE2TRIAMP
    "2TP", // 32: USER_WAVE2TRIPEAK
    "2PT", // 33: USER_WAVE2PITCH
    "2OC", // 34: USER_WAVE2OCTAVE
    "DLV", // 35: USER_DISTORTION_LEVEL
    "PBN", // 36: USER_PITCHBYNOTE
    "MCH", // 37: USER_CH0MIRRORCH
    "TTY", // 38: USER_TRE_TYPE
    "TAM", // 39: USER_TRE_AMP
    "TPE", // 40: USER_TRE_PEAK
    "TFR", // 41: USER_TRE_FREQ
    "VTY", // 42: USER_VIB_TYPE
    "VPE", // 43: USER_VIB_PEAK
    "VAM", // 44: USER_VIB_AMP
    "VFR", // 45: USER_VIB_FREQ
    "WOW", // 46: USER_WOW_TYPE
    "WPK", // 47: USER_WOW_PEAK
    "WAM", // 48: USER_WOW_AMP
    "WFR", // 49: USER_WOW_FREQ
    "EVD", // 50: USER_ENV_VOL_DIFF
    "EVT", // 51: USER_ENV_VOL_TIME
    "EFD", // 52: USER_ENV_FREQ_DIFF
    "EFT", // 53: USER_ENV_FREQ_TIME
    "EWD", // 54: USER_ENV_WOW_DIFF
    "EWT", // 55: USER_ENV_WOW_TIME
    "DRT", // 56: USER_DRUM_TYPE
    "DLF", // 57: USER_DRUM_LPF_FCNN
    "DHF", // 58: USER_DRUM_HPF_FCNN
    "DLQ", // 59: USER_DRUM_LPF_Q
    "DHQ", // 60: USER_DRUM_HPF_Q
    "G61", // 61: GENERAL61
    "G62", // 62: GENERAL62
    "G63", // 63: GENERAL63
    "DMP", // 64: DAMPERPEDAL
    "SST", // 65: SOSTENTOPEDAL
    "SFT", // 66: SOFTPEDAL
    "LFS", // 67: LEGATOFOOTSWITCH
    "HLD", // 68: HOLD2
    "SC1", // 69: SOUNDCONT1
    "SC2", // 70: SOUNDCONT2
    "LPQ", // 71: USER_LPFQFACTOR
    "RLT", // 72: USER_RELEASETIME
    "AKT", // 73: USER_ATTACKTIME
    "LPC", // 74: USER_LPFCUTOFF
    "G75", // 75: GENERAL75
    "G76", // 76: GENERAL76
    "G77", // 77: GENERAL77
    "G78", // 78: GENERAL78
    "G79", // 79: GENERAL79
    "DCT", // 80: USER_DECAYTIME
    "G81", // 81: GENERAL81
    "G82", // 82: GENERAL82
    "G83", // 83: GENERAL83
    "PTC", // 84: PORTAMENTOCC
    "G85", // 85: GENERAL85
    "G86", // 86: GENERAL86
    "G87", // 87: GENERAL87
    "HRV", // 88: HIGHRESVELOCITYPREFIX
    "G89", // 89: GENERAL89
    "G90", // 90: GENERAL90
    "RVB", // 91: USER_REVERB_LEVEL
    "ED2", // 92: EFFECTDEPTH2
    "ED3", // 93: EFFECTDEPTH3
    "ED4", // 94: EFFECTDEPTH4
    "ED5", // 95: EFFECTDEPTH5
    "DIN", // 96: DATAINCREMENT
    "DDE", // 97: DATADECREMENT
    "NLS", // 98: NRPNLSB
    "NMS", // 99: NRPNMSB
    "RLS", // 100: RPNLSB
    "RMS", // 101: RPNMSB
    "MTT", // 102: USER_MTRE_TYPE
};

/* ===== Program Text (128 entries) ===== */
const char stm32synth_prog_text[][6] = {
    "ACGRPN",
    "BRACPN",
    "ELGRPN",
    "HONKPN",
    "RHODPN",
    "CHORPN",
    "HAPSCD",
    "CLAVNT",
    "CELEST",
    "GLKSPL",
    "MUSBOX",
    "VIBRPH",
    "MARIMB",
    "XYLOPH",
    "TUBBEL",
    "DULCIM",
    "HAMORG",
    "PERORG",
    "RCKORG",
    "CHRORG",
    "REEORG",
    "ACCORD",
    "HARMON",
    "TNGACD",
    "NYLGTR",
    "STLGTR",
    "JAZGTR",
    "CLNGTR",
    "MUTGTR",
    "OVRGTR",
    "DISGTR",
    "GTRHMN",
    "ACSTBS",
    "FINGBS",
    "PLELBS",
    "FRETBS",
    "SLAPB1",
    "SLAPB2",
    "SYNTB1",
    "SYNTB2",
    "VIOLIN",
    "VIOLA ",
    "CELLO ",
    "CONTRB",
    "TRESTR",
    "PIZSTR",
    "ORCHHP",
    "TIMPNI",
    "STRENS",
    "STREN2",
    "SYNSTR",
    "SYNST2",
    "CHOAAH",
    "CHOOOH",
    "SYNVOC",
    "ORCHIT",
    "TRUMPT",
    "TROMBN",
    "TUBA  ",
    "MUTTRP",
    "FRHORN",
    "BRASSC",
    "SYBRAS",
    "SYBRA2",
    "SOPRSX",
    "ALTOSX",
    "TENOSX",
    "BRTNSX",
    "OBOE  ",
    "ENHORN",
    "BASSON",
    "CLARNT",
    "PICCOL",
    "FLUTE ",
    "RECORD",
    "PANFLT",
    "BOTBLE",
    "SHAKUH",
    "WHISTL",
    "OCARIN",
    "SQLEAD",
    "SWLEAD",
    "CALLIO",
    "CHIFFL",
    "CHARAN",
    "VOLEAD",
    "FIFTHS",
    "BASSLD",
    "NEAGPD",
    "WARMPD",
    "POLYPD",
    "CHORPD",
    "BOWDPD",
    "MTLCPD",
    "HALOPD",
    "SWEPPD",
    "RAINEF",
    "SOUNDE",
    "CRYSTL",
    "ATMOSP",
    "BRIGHT",
    "GOBLIN",
    "ECHOES",
    "SCIFIE",
    "SITAR ",
    "BANJO ",
    "SHAMIS",
    "KOTO  ",
    "KALIMB",
    "BAGPIP",
    "FIDDLE",
    "SHANAI",
    "TINKLE",
    "AGOGO ",
    "STEELD",
    "WOODBL",
    "TAIKOD",
    "MELOTM",
    "SYNDRM",
    "REVCSM",
    "GFRETN",
    "BREATH",
    "SEASHR",
    "BIRDTW",
    "TELEPH",
    "HELICP",
    "APPLSE",
    "GUNSHT",
};

/* ===== Mode Text ===== */
const char mode_text[][7] = {
    "FREEPLY", // MODE_STATE_FREEPLAY
    "METRNOM", // MODE_STATE_METRONOME
    "SQ-PARA", // MODE_STATE_SEQUENCER_PARA
    "SQ-PROG", // MODE_STATE_SEQUENCER_INPUT
    "SQ-PLAY", // MODE_STATE_SEQUENCER_PLAY
    "SETTING", // MODE_STATE_SETTING
};
