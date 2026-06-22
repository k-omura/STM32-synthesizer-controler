/*
 * stm32_synth_midi.c
 *
 *  Created on: Feb 11, 2024
 *      Author: k-omura
 */

#include "neopixel.h"
#include "stm32_synth_midi.h"
#include <stdio.h>
#include <string.h>

#define METRONOME_NOTENUM (37)
#define METRONOME_VELOCITY (50)

// typedef
typedef struct
{
    uint8_t vel;
    uint8_t cnt;
    uint8_t ch;
    uint32_t tick;
    fb_color24_t col_noteef;
} stm32synth_note_t;

typedef struct
{
    uint16_t bpm;
    uint16_t cnt;
    uint32_t bpmtick;
    uint32_t tick;
} stm32synth_metronome_t;

typedef struct
{
    uint8_t sel_ch;
    uint8_t beat;
    uint8_t cnt_beat;
    uint8_t rec_mode;

    uint16_t bpm;

    uint16_t bar;
    uint16_t sel_bar;
    uint16_t cnt_bar;

    uint32_t tick;
    uint32_t bpmtick;

    uint8_t metromode;
    uint8_t recInThisBar;

    uint8_t layer_cnt[SEQUENCER_MAXBAR];
} stm32synth_sequencer_t;

typedef struct
{
    uint8_t layer;
    uint8_t ch;
    uint8_t nn;
    uint8_t vel;
    
    uint16_t start;
    uint16_t release;

    uint8_t played;
} stm32synth_seqrec_t;

typedef struct
{
    uint8_t state;
    uint8_t play;
    uint8_t mutech[16];
    uint8_t sel_ch;
} stm32synth_seqplay_t;

// global variables
static stm32synth_midiinput_ringbuff_t midibuff;

static fb_color24_t col_reset = {0, 0, 0};
static fb_color24_t col_string = {8, 8, 8};
static fb_color24_t col_needle = {10, 0, 2};
static fb_color24_t col_ble = {0, 0, 10};

static fb_color24_t col_bar_all = {6, 3, 3};
static fb_color24_t col_cursorx = {1, 1, 1};
static fb_color24_t col_cursory = {1, 2, 1};
static fb_color24_t col_sel_bar = {1, 1, 10};
static fb_color24_t col_sel_channel = {1, 10, 1};

static fb_color24_t col_sel_play = {10, 1, 1};
static fb_color24_t col_sel_stop = {8, 8, 8};
static fb_color24_t col_sel_rec = {0, 10, 0};

static fb_color24_t col_sel_recnote[8] = {
    {20, 4, 4},
    {4, 20, 4},
    {4, 4, 20},
    {20, 20, 4},
    {4, 20, 20},
    {20, 4, 20},
    {20, 12, 0},
    {0, 12, 20},
};
static stm32synth_state_t state = {STM32SYNTH_DISPSTATE_HELLO, STM32SYNTH_DISPSTATE_NOTE, 0, 0, 0, STM32SYNTH_STATE_MESSAGE_DEFAULT};
static stm32synth_note_t note[128] = {0};
static uint16_t freeplaymode = 0;
static stm32synth_metronome_t metronome = {
    .bpm = DEFAULT_BPM,
};
static stm32synth_sequencer_t sequencer = {
    .bar = 2,
    .beat = 4,
    .bpm = DEFAULT_BPM,
    .metromode = 1,
};
static stm32synth_seqrec_t sequencer_rec[SEQUENCER_MAXBAR][SEQUENCER_MAXCHORDPARBAR] = {0};
static stm32synth_seqplay_t sequencer_play = {
    .play = 1,
};

// private functions
stm32synth_res_t stm32synth_component_inputmidi(uint8_t *_midi, uint32_t _tick, uint32_t _rectick);
stm32synth_res_t stm32synth_component_midi_writebuff(uint8_t *_midi);
stm32synth_res_t stm32synth_component_midi_readbuff(uint8_t *_midi);
stm32synth_res_t stm32synth_midi_inputmidiCC(uint8_t _im, uint8_t _ch, uint8_t _cc, uint8_t _val);

// Public
// implementation
void stm32synth_hundle(SPI_HandleTypeDef *_hspi_neopixel, SPI_HandleTypeDef *_hspi_synth, uint32_t _tick)
{
    char string[10] = { 0 };
    uint8_t midiRec[4];
    stm32synth_res_t input_midi_res;

    uint32_t tickDiff = _tick - state.tick;
    uint32_t bpmDiff = _tick - metronome.tick;
    uint32_t seqDiff = _tick - sequencer.tick;

    uint8_t bleCone = (HAL_GPIO_ReadPin(bleConnect_GPIO_Port, bleConnect_Pin) == GPIO_PIN_SET) ? (1) : (0);

    if (state.code != STM32SYNTH_DISPSTATE_HELLO)
    {
        stm32synth_res_t midi_res = stm32synth_component_midi_readbuff(midiRec);
        if (midi_res == STM32SYNTH_RES_OK)
        {
            input_midi_res = stm32synth_component_inputmidi(midiRec, _tick, seqDiff);
            HAL_SPI_Transmit_DMA(_hspi_synth, midiRec, 4);
        }
    }

    // refresh
    //note
    for (uint8_t i = 0; i < 128; i++)
    {
        if (((note[i].vel) || (note[i].cnt)) && (note[i].cnt < 8) && ((_tick - note[i].tick) > 20))
        {
            note[i].cnt++;
            note[i].tick = _tick;
        }
    }
    
    //metronome
    if(state.base_code == STM32SYNTH_DISPSTATE_METRONOME)
    {
        if (bpmDiff > metronome.bpmtick)
        {
            metronome.tick = _tick;
            metronome.cnt++;
            bpmDiff = 0;

            //beat sound
            midi_spi_noteon(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM, METRONOME_VELOCITY);
        }
    }

    //sequencer
    if(((sequencer_play.play) || (sequencer_play.state)) && ((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PARA) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY)))
    {
        if (seqDiff > sequencer.bpmtick)
        {
            sequencer.tick = _tick;
            sequencer.cnt_beat++;
            if(sequencer.cnt_beat >= sequencer.beat)
            {
                sequencer.cnt_beat = 0;

                sequencer.cnt_bar++;
                if(sequencer.cnt_bar >= sequencer.bar)
                {
                    sequencer.cnt_bar = 0;
                }

                if((sequencer_play.play == 0) && (sequencer_play.state))
                {
                    sequencer_play.state = 0;
                    sequencer.cnt_beat = 0;
                    sequencer.cnt_bar = 0;
                }

                if(sequencer.recInThisBar > 0)
                {
                    sequencer.recInThisBar = 0;

                    // auto seek
                    sequencer.sel_bar++;
                    if(sequencer.sel_bar >= sequencer.bar)
                    {
                        sequencer.sel_bar = 0;
                    }
                }
            }

            seqDiff -= sequencer.bpmtick;
            
            //beat sound
            if(sequencer.metromode == 1)
            {
                if(sequencer.cnt_beat == 0)
                {
                    midi_play_noteon(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM, METRONOME_VELOCITY);
                }
                else
                {
                    midi_play_noteon(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM, METRONOME_VELOCITY >> 1);
                }
            }
        }

        // sequencer play
        uint16_t seqRecTick = SEQUENCER_RECTIMERESOLUTION * (sequencer.bpmtick * sequencer.cnt_beat + seqDiff) / (sequencer.bpmtick * sequencer.beat);
        uint8_t vel;
        for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
        {
            //note on
            if((sequencer_rec[sequencer.cnt_bar][i].played == 0) && (sequencer_rec[sequencer.cnt_bar][i].nn > 0) && (sequencer_rec[sequencer.cnt_bar][i].start < seqRecTick) && (sequencer_rec[sequencer.cnt_bar][i].release > seqRecTick))
            {
                sequencer_rec[sequencer.cnt_bar][i].played = 1;
                vel = (sequencer_play.mutech[sequencer_rec[sequencer.cnt_bar][i].ch] == 0) ? (sequencer_rec[sequencer.cnt_bar][i].vel) : (0);
                midi_play_noteon(sequencer_rec[sequencer.cnt_bar][i].ch, sequencer_rec[sequencer.cnt_bar][i].nn, vel);
            }

            //note off
            uint8_t beat = sequencer.cnt_bar;
            for(uint8_t b = 0; b < sequencer.bar; b++)
            {
                if((sequencer_rec[beat][i].played == 1) && ((sequencer_rec[beat][i].release - SEQUENCER_RECTIMERESOLUTION * b) < seqRecTick))
                {
                    sequencer_rec[beat][i].played = 0;
                    midi_play_noteoff(sequencer_rec[beat][i].ch, sequencer_rec[beat][i].nn);
                }

                beat = (beat > 0) ? (beat - 1) : (sequencer.bar - 1);
            }
        }
    }

    if(((sequencer_play.play) && (sequencer_play.state)) && ((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PARA) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY)))
    {
        sequencer_play.state = 0;
        sequencer.cnt_beat = 0;
        sequencer.cnt_bar = 0;
    }
    // refresh end

    switch (state.code)
    {
    case STM32SYNTH_DISPSTATE_HELLO:
    {
        if (state.cnt && (tickDiff > 2500))
        {
            state.code = state.base_code;
            state.cnt = 0;
            state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;
        }
        else if ((_tick - state.animate_tick) > 1250)
        {
            neopixel_fb_clear(&col_reset);
            neopixel_fb_logo(state.cnt, &col_string);
            neopixel_flush(_hspi_neopixel);

            if (state.cnt++ == 0)
            {
                state.tick = _tick;
            }
            state.animate_tick = _tick;
        }

        break;
    }
    case STM32SYNTH_DISPSTATE_MODESEL:
    {
        char string[10] = {0};

        neopixel_fb_clear(&col_reset);
        memcpy(string, mode_text[state.message], 7);
        neopixel_fb_stringBitmap(2, 1, string, 0, &col_string, 1);
        neopixel_flush(_hspi_neopixel);

        if (state.cnt == 0)
        {
            state.cnt = 1;
            state.tick = _tick;
        }

        if (state.cnt && (tickDiff > 1000))
        {
            state.code = state.base_code;
            state.cnt = 0;
            state.tick = _tick;
            state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;
        }

        break;
    }
    case STM32SYNTH_DISPSTATE_NOTE:
    {
    	uint16_t freeplayModeChannelMap = 0;

        neopixel_fb_clear(&col_reset);

        for (uint8_t i = 0; i < 128; i++)
        {
            // draw center point
            if (note[i].vel > 0)
            {
                neopixel_fb_pixel(i % 32, 2 + (i / 32), &note[i].col_noteef);
                freeplayModeChannelMap |= (1 << note[i].ch);
            }

            // draw ring
            // if ((note[i].cnt) && (note[i].cnt < 32))
            if ((note[i].cnt) && (note[i].cnt < 8))
            {
                int16_t center_x = i % 32;
                int16_t center_y = 2 + (i / 32);
                uint8_t length = 1 + 2 * note[i].cnt;

                neopixel_fb_ybar(center_x, center_y - note[i].cnt, length, &note[i].col_noteef);

                /*
                neopixel_fb_xbar(center_x - note[i].cnt, center_y - note[i].cnt, length, &note[i].col_noteef);
                neopixel_fb_ybar(center_x - note[i].cnt, center_y - note[i].cnt, length, &note[i].col_noteef);
                neopixel_fb_xbar(center_x - note[i].cnt, center_y + note[i].cnt, length, &note[i].col_noteef);
                neopixel_fb_ybar(center_x + note[i].cnt, center_y - note[i].cnt, length, &note[i].col_noteef);
                */
            }
        }

        //Channel
        if(freeplaymode == 1)
        {
            for (uint8_t i = 0; i < 16; i++)
            {
                if (freeplayModeChannelMap & 0x0001)
                {
                    neopixel_fb_xbar(2 * i, 7, 2, &col_string);
                }
                freeplayModeChannelMap >>= 1;
            }
        }

        //BLE Connected
        if(bleCone)
        {
            neopixel_fb_pixel(31, 7, &col_ble);
        }

        neopixel_flush(_hspi_neopixel);

        break;
    }
    case STM32SYNTH_DISPSTATE_METRONOME:
    {
        uint8_t needlePos;

        neopixel_fb_clear(&col_reset);

        needlePos = (32 - 1) * bpmDiff / metronome.bpmtick;
        if(metronome.cnt % 2)
        {
            needlePos = (32 - 1) - needlePos;
        }
        neopixel_fb_ybar(needlePos, 0, 2, &col_needle);

        sprintf(string, "BPM%3d", metronome.bpm);
        neopixel_fb_stringBitmap(4, 3, string, 0, &col_string, 1);

        //BLE Connected
        if(bleCone)
        {
            neopixel_fb_pixel(31, 7, &col_ble);
        }

        neopixel_flush(_hspi_neopixel);

        if (state.cnt == 0)
        {
            state.cnt = 1;
            state.tick = _tick;
            //metronome.tick = _tick;
        }

        break;
    }
    case STM32SYNTH_DISPSTATE_SEQUENCER_PARA:
    case STM32SYNTH_DISPSTATE_SEQUENCER_INPUT:
    case STM32SYNTH_DISPSTATE_SEQUENCER_PLAY:
    {
        fb_color24_t col_temp;

        neopixel_fb_clear(&col_reset);

        if (state.cnt == 0)
        {
            state.cnt = 1;
            state.tick = _tick;
        }
        else if ((state.cnt == 1) && (tickDiff < 1000))
        {
            uint8_t stringStartX = 8;
            switch(state.message)
            {
                case CNTSEQ_BPM:
                {
                    sprintf(string, "BPM%3d", sequencer.bpm);
                    break;
                }
                case CNTSEQ_BEAT:
                {
                    sprintf(string, "BEAT%2d", sequencer.beat);
                    break;
                }
                case CNTSEQ_BAR:
                {
                    sprintf(string, "BAR %2d", sequencer.bar);
                    break;
                }
                case CNTSEQ_SELBAR:
                {
                    if(sequencer.rec_mode == 0)
                    {
                        sprintf(string, "UNDO");
                    }
                    else
                    {
                        sprintf(string, "CH CLR");
                    }
                    break;
                }
                case CNTSEQ_CHANNEL:
                {
                    sprintf(string, "CHANL");
                    break;
                }
                case CNTSEQ_METRONOME:
                {
                    if(sequencer.metromode == 0)
                    {
                        sprintf(string, "MTROFF");
                    }
                    else
                    {
                        sprintf(string, "MTRON");
                    }
                    break;
                }
                default:
                {
                    string[0] = 0;
                }
            }
            neopixel_fb_stringBitmap(stringStartX, 1, string, 0, &col_string, 1);
        }
        else if((state.cnt == 1) && (state.message != STM32SYNTH_STATE_MESSAGE_DEFAULT))
        {
            state.cnt = 2;
            state.tick = _tick;
            state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;
        }

        // draw frame
        if((state.message == STM32SYNTH_STATE_MESSAGE_DEFAULT))
        {
            neopixel_fb_ybar(8, 0, 8, &col_cursorx);
        }

        // draw selected channel
        if(state.base_code != STM32SYNTH_DISPSTATE_SEQUENCER_PLAY)
        {
            uint8_t length = ((state.message == STM32SYNTH_STATE_MESSAGE_DEFAULT) || ((sequencer.sel_ch % 8) == 7)) ? (32) : (8);
            neopixel_fb_xbar(0, sequencer.sel_ch % 8, length, &col_cursory);
            uint8_t ch_upper_y = (sequencer.sel_ch < 8) ? (0) : (1);
            neopixel_fb_pixel(7, ch_upper_y, &col_sel_channel);
        }

        // draw glaph
        if((state.message == STM32SYNTH_STATE_MESSAGE_DEFAULT))
        {
            uint8_t drawx;

            // draw needle
            if(sequencer_play.play)
            {
                uint8_t needlePos = 8 + 24 * (sequencer.bpmtick * sequencer.cnt_beat + seqDiff) / (sequencer.bpmtick * sequencer.beat);
                neopixel_fb_ybar(needlePos, 0, 8, &col_cursorx);
            }

            // draw selected mute channel
            if((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY) && (sequencer_play.sel_ch > 0))
            {
                uint8_t ch_upper_y = ((sequencer_play.sel_ch - 1) < 8) ? (0) : (1);
                neopixel_fb_xbar(0, ((sequencer_play.sel_ch - 1) % 8), 32, &col_cursory);
                neopixel_fb_pixel(7, ch_upper_y, &col_sel_channel);
            }

            // draw note
            for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
            {
                if((sequencer_rec[sequencer.cnt_bar][i].layer > 0) && (sequencer_rec[sequencer.cnt_bar][i].nn > 0))
                {
                    if(state.base_code != STM32SYNTH_DISPSTATE_SEQUENCER_PLAY)
                    {
                        if((sequencer.sel_ch < 8) && sequencer_rec[sequencer.cnt_bar][i].ch > 7)
                        {
                            continue;
                        }
                        if((sequencer.sel_ch > 7) && sequencer_rec[sequencer.cnt_bar][i].ch < 8)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if(((sequencer_play.sel_ch - 1) < 8) && sequencer_rec[sequencer.cnt_bar][i].ch > 7)
                        {
                            continue;
                        }
                        if(((sequencer_play.sel_ch - 1) > 7) && sequencer_rec[sequencer.cnt_bar][i].ch < 8)
                        {
                            continue;
                        }
                    }

                    col_temp = col_sel_recnote[sequencer_rec[sequencer.cnt_bar][i].ch % 8];

                    if(sequencer_play.mutech[sequencer_rec[sequencer.cnt_bar][i].ch] == 1)
                    {
                        col_temp.green >>= 2;
                        col_temp.blue >>= 2;
                        col_temp.red >>= 2;
                    }
                    else
                    {
                        if(
                            ((state.base_code != STM32SYNTH_DISPSTATE_SEQUENCER_PLAY) && (sequencer_rec[sequencer.cnt_bar][i].ch == sequencer.sel_ch))
                        || ((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY) && (sequencer_rec[sequencer.cnt_bar][i].ch == (sequencer_play.sel_ch - 1)))
                        )
                        {
                            col_temp.green <<= 2;
                            col_temp.blue <<= 2;
                            col_temp.red <<= 2;
                        }
                    }

                    drawx = 8 + 24 * sequencer_rec[sequencer.cnt_bar][i].start / SEQUENCER_RECTIMERESOLUTION;
                    neopixel_fb_pixel(drawx, (sequencer_rec[sequencer.cnt_bar][i].ch % 8), &col_temp);
                }
            }
        }

        // draw bar
        if(sequencer.bar > 8)
        {
            neopixel_fb_xbar(0, 7, 8, &col_bar_all);

            col_temp = col_bar_all;
            col_temp.red = col_temp.green;
            neopixel_fb_xbar(0, 7, (sequencer.bar - 8), &col_temp);
        }
        else
        {
            neopixel_fb_xbar(0, 7, sequencer.bar, &col_bar_all);
        }

        // draw current bar position
        if(sequencer_play.play)
        {
            col_temp = col_sel_play;
            if(sequencer.cnt_bar >= 8)
            {
                col_temp.red = col_temp.green;
            }
            neopixel_fb_pixel((sequencer.cnt_bar % 8), 7, &col_temp);
        }

        // draw selected bar position
        if(state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT)
        {
            col_temp = col_sel_bar;
            if((sequencer.sel_bar == sequencer.cnt_bar) && (sequencer_play.play))
            {
                col_temp = col_sel_rec;
            }
            else if((sequencer.sel_bar > 7))
            {
                col_temp.red = col_temp.blue;
            }

            neopixel_fb_pixel((sequencer.sel_bar % 8), 7, &col_temp);
        }

        // draw icon
        switch(state.base_code)
        {
            case STM32SYNTH_DISPSTATE_SEQUENCER_INPUT:
            {
                col_temp = (sequencer_play.play) ? (col_sel_rec) : (col_sel_stop);
                neopixel_fb_icon(SEQ_ICON_RECORD, &col_temp);
                break;
            }
            case STM32SYNTH_DISPSTATE_SEQUENCER_PARA:
            case STM32SYNTH_DISPSTATE_SEQUENCER_PLAY:
            default:
            {
                if(sequencer_play.play)
                {
                    neopixel_fb_icon(SEQ_ICON_PLAY, &col_sel_play);
                }
                else
                {
                    neopixel_fb_icon(SEQ_ICON_STOP, &col_sel_stop);
                }
                break;
            }
        }

        //BLE Connected
        if(bleCone)
        {
            neopixel_fb_pixel(0, 0, &col_ble);
        }
        
        neopixel_flush(_hspi_neopixel);

        break;
    }
    case STM32SYNTH_DISPSTATE_CHANNEL:
    {
        neopixel_fb_clear(&col_reset);

        if (state.message > 15)
        {
            sprintf(string, "CHNLALL");
            neopixel_fb_stringBitmap(2, 1, string, 0, &col_string, 1);
            // neopixel_fb_xbar(0, 7, 32, &col_string);
        }
        else if (state.message == 9)
        {
            sprintf(string, "CH DRUM");
            neopixel_fb_stringBitmap(2, 1, string, 0, &col_string, 1);
            neopixel_fb_xbar(2 * state.message, 7, 2, &col_string);
        }
        else if (state.message >= 0)
        {
            sprintf(string, "CHNL.%2d", state.message + 1);
            neopixel_fb_stringBitmap(2, 1, string, 0, &col_string, 1);
            neopixel_fb_xbar(2 * state.message, 7, 2, &col_string);
        }

        neopixel_flush(_hspi_neopixel);

        if (state.cnt == 0)
        {
            state.cnt = 1;
            state.tick = _tick;
        }

        if (state.cnt && (tickDiff > 1000))
        {
            state.code = state.base_code;
            state.cnt = 0;
            state.tick = _tick;
            state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;
        }

        break;
    }
    case STM32SYNTH_DISPSTATE_MIDIIN:
    {
        neopixel_flush(_hspi_neopixel);

        if (state.cnt && (tickDiff > 1000))
        {
            state.code = state.base_code;
            state.cnt = 0;
            state.tick = _tick;
            state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;
        }

        break;
    }
    case STM32SYNTH_DISPSTATE_SETTING:
    {
        neopixel_fb_clear(&col_reset);

        switch(state.message)
        {
            case 0:
            default:
            {
                sprintf(string, "GOTODFU");
                neopixel_fb_stringBitmap(2, 1, string, 0, &col_string, 1);
                break;
            }
        }

        neopixel_flush(_hspi_neopixel);

        if (state.cnt == 0)
        {
            state.cnt = 1;
            state.tick = _tick;
        }

        if (state.cnt && (tickDiff > 1000))
        {
            state.code = state.base_code;
            state.cnt = 0;
            state.tick = _tick;
            state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;
        }
        break;
    }
    default:
    {

        break;
    }
    }

    return;
}

stm32synth_res_t stm32synth_midi_writebuff(uint8_t *_midi)
{
    return stm32synth_component_midi_writebuff(_midi);
}

void stm32synth_disp_mode(uint8_t _mode)
{
    state.code = STM32SYNTH_DISPSTATE_MODESEL;

    switch (_mode)
    {
        case MODE_STATE_METRONOME:
            state.base_code = STM32SYNTH_DISPSTATE_METRONOME;
            metronome.bpmtick = (60 * 1000) / metronome.bpm;
            midi_play_noteoff(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM);
            break;
        case MODE_STATE_SEQUENCER_PARA:
            state.base_code = STM32SYNTH_DISPSTATE_SEQUENCER_PARA;
            sequencer.bpmtick = (60 * 1000) / sequencer.bpm;

            if((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PARA) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY))
            {
                break;
            }
            midi_play_noteoff(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM);
            break;
        case MODE_STATE_SEQUENCER_INPUT:
            state.base_code = STM32SYNTH_DISPSTATE_SEQUENCER_INPUT;
            sequencer.bpmtick = (60 * 1000) / sequencer.bpm;
            
            if(sequencer.sel_bar >= sequencer.bar)
            {
                sequencer.sel_bar = sequencer.bar - 1;
            }

            if((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PARA) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY))
            {
                break;
            }
            midi_play_noteoff(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM);
            break;
        case MODE_STATE_SEQUENCER_PLAY:
            state.base_code = STM32SYNTH_DISPSTATE_SEQUENCER_PLAY;
            sequencer.bpmtick = (60 * 1000) / sequencer.bpm;

            if((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PARA) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY))
            {
                break;
            }
            midi_play_noteoff(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM);
            break;
        case MODE_STATE_SETTING:
            state.base_code = STM32SYNTH_DISPSTATE_SETTING;
            midi_play_noteoff(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM);
            break;
        case MODE_STATE_FREEPLAY:
        default:
            state.base_code = STM32SYNTH_DISPSTATE_NOTE;
            midi_play_noteoff(STM32SYNTH_MIDINN_DRUMCH, METRONOME_NOTENUM);
            break;
    }
    state.cnt = 0;
    state.message = _mode;

    return;
}

void stm32synth_disp_channel(uint8_t _ch)
{
    state.code = STM32SYNTH_DISPSTATE_CHANNEL;
    state.cnt = 0;
    state.message = _ch;

    return;
}

void stm32synth_disp_metronome(uint16_t _bpm)
{
    metronome.bpm = _bpm;
    metronome.bpmtick = (60 * 1000) / _bpm;

    //mirror to sequencer
    sequencer.bpm = _bpm;
    sequencer.bpmtick = metronome.bpmtick;

    return;
}

void stm32synth_disp_freeplay(uint16_t _mode)
{
    freeplaymode = _mode;
    return;
}

void stm32synth_disp_setting(uint16_t _target)
{
    state.message = _target;
    return;
}

void stm32synth_seq_bpm(uint16_t _bpm)
{
    sequencer.bpm = _bpm;
    sequencer.bpmtick = (60 * 1000) / sequencer.bpm;

    //mirror to metronome
    metronome.bpm = sequencer.bpm;
    metronome.bpmtick = sequencer.bpmtick;

    state.code = STM32SYNTH_DISPSTATE_SEQUENCER_PARA;
    state.cnt = 0;
    state.message = CNTSEQ_BPM;
    return;
}

void stm32synth_seq_beat(uint8_t _beat)
{
    sequencer.beat = _beat;

    state.code = STM32SYNTH_DISPSTATE_SEQUENCER_PARA;
    state.cnt = 0;
    state.message = CNTSEQ_BEAT;
    return;
}

void stm32synth_seq_bar(uint16_t _bar)
{
    sequencer.bar = _bar;

    state.code = STM32SYNTH_DISPSTATE_SEQUENCER_PARA;
    state.cnt = 0;
    state.message = CNTSEQ_BAR;
    return;
}

void stm32synth_seq_channel(uint8_t _ch)
{
    sequencer.sel_ch = _ch;

    state.code = STM32SYNTH_DISPSTATE_SEQUENCER_PARA;
    state.cnt = 0;
    state.message = CNTSEQ_CHANNEL;
    return;
}

void stm32synth_seq_selbar(uint16_t _bar, uint8_t _mode)
{
    sequencer.sel_bar = _bar;
    if(sequencer.sel_bar >= sequencer.bar)
    {
        sequencer.sel_bar = sequencer.bar - 1;
    }

    sequencer.rec_mode = _mode;
    
    state.code = STM32SYNTH_DISPSTATE_SEQUENCER_INPUT;
    state.cnt = 0;
    state.message = CNTSEQ_SELBAR;
    return;
}

void stm32synth_seq_metronome(uint8_t _mode)
{
    sequencer.metromode = _mode;

    state.code = STM32SYNTH_DISPSTATE_SEQUENCER_PARA;
    state.cnt = 0;
    state.message = CNTSEQ_METRONOME;
    return;
}

void stm32synth_seq_progpush(uint8_t _mode)
{
    if(_mode == 0)
    {
        if(sequencer.layer_cnt[sequencer.sel_bar] > 0)
        {
            sequencer.layer_cnt[sequencer.sel_bar]--;
        }
        else
        {
            return;
        }

        // Clear all note in this layer
        for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
        {
            if(sequencer_rec[sequencer.sel_bar][i].layer > sequencer.layer_cnt[sequencer.sel_bar])
            {
                sequencer_rec[sequencer.sel_bar][i].nn = 0;
                if(sequencer_rec[sequencer.sel_bar][i].played > 0)
                {
                    midi_play_noteoff(sequencer_rec[sequencer.sel_bar][i].ch, sequencer_rec[sequencer.sel_bar][i].nn);
                    sequencer_rec[sequencer.sel_bar][i].played = 0;
                }
            }
        }
    }
    else if(_mode == 1)
    {
        // Clear all note in this channel
        for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
        {
            if(sequencer_rec[sequencer.sel_bar][i].ch == sequencer.sel_ch)
            {
                sequencer_rec[sequencer.sel_bar][i].nn = 0;
                if(sequencer_rec[sequencer.sel_bar][i].played > 0)
                {
                    midi_play_noteoff(sequencer_rec[sequencer.sel_bar][i].ch, sequencer_rec[sequencer.sel_bar][i].nn);
                    sequencer_rec[sequencer.sel_bar][i].played = 0;
                }
            }
        }
        stm32synth_seq_cleanrecs();
    }

    return;
}

void stm32synth_seq_cleanrecs()
{
    for(uint16_t b = 0; b < sequencer.bar; b++)
    {
        if(sequencer.layer_cnt[b] == 0)
        {
            continue;
        }

        //list used layer
        uint8_t rec_layer[SEQUENCER_MAXCHORDPARBAR] = {0};
        for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
        {
            if((sequencer_rec[b][i].nn > 0) && (rec_layer[sequencer_rec[b][i].layer - 1] == 0))
            {
                rec_layer[sequencer_rec[b][i].layer - 1] = 1;
            }
        }

        //shift layer
        for(int16_t r = (sequencer.layer_cnt[b] - 1); r >= 0; r--)
        {
            if(rec_layer[r] == 0)
            {
                for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
                {
                    if((sequencer_rec[b][i].nn > 0) && (sequencer_rec[b][i].layer > r))
                    {
                        sequencer_rec[b][i].layer--;
                    }
                }
                sequencer.layer_cnt[b]--;
            }
        }
    }

    return;
}

void stm32synth_seq_mutech(uint8_t _ch)
{
    sequencer_play.sel_ch = _ch;
    state.code = STM32SYNTH_DISPSTATE_SEQUENCER_PLAY;
    state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;
    return;
}

void stm32synth_seq_mutechtoggle(uint8_t _ch)
{
    sequencer_play.mutech[_ch]++;
    if(sequencer_play.mutech[_ch] > 1)
    {
        sequencer_play.mutech[_ch] = 0;
    }

    return;
}

void stm32synth_seq_playtoggle()
{
    sequencer_play.play++;
    if(sequencer_play.play > 1)
    {
        sequencer_play.play = 0;
    }

    if(sequencer_play.play == 0)
    {
        sequencer_play.state = 1;
    }
    else
    {
        sequencer_play.state = 1;
    }
    
    return;
}

void stm32synth_midi_volume(uint8_t _ch, uint8_t _volume)
{
    uint8_t midi_tx[4];

    state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;

    _ch = (_ch > 15) ? (0) : (_ch);

    midi_tx[0] = (0x0 << 4) | (STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f);
    midi_tx[1] = ((STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f) << 4) | (_ch & 0x0f);
    midi_tx[2] = STM32SYNTH_MIDICC_VOLUME;
    midi_tx[3] = _volume;
    stm32synth_component_midi_writebuff(midi_tx);

    return;
}

void stm32synth_midi_volumeAll(uint8_t _volume)
{
    uint8_t midi_tx[4];

    state.message = _volume;

    for (uint8_t i = 0; i < 16; i++)
    {
        //if(i != STM32SYNTH_MIDINN_DRUMCH)
        {
            midi_tx[0] = (0x1 << 4) | (STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f);
            midi_tx[1] = ((STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f) << 4) | (i & 0x0f);
            midi_tx[2] = STM32SYNTH_MIDICC_VOLUME;
            midi_tx[3] = _volume;
            stm32synth_component_midi_writebuff(midi_tx);
        }
    }

    return;
}

void stm32synth_midi_program(uint8_t _ch, uint8_t _program)
{
    uint8_t midi_tx[4];

    if(_ch == STM32SYNTH_MIDINN_DRUMCH)
    {
        return;
    }

    state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;

    _ch = (_ch > 15) ? (0) : (_ch);

    midi_tx[0] = (0x0 << 4) | (STM32SYNTH_MIDIEVT0_PROGRAMCHANGE & 0x0f);
    midi_tx[1] = ((STM32SYNTH_MIDIEVT0_PROGRAMCHANGE & 0x0f) << 4) | (_ch & 0x0f);
    midi_tx[2] = _program;
    midi_tx[3] = 0x00;
    stm32synth_component_midi_writebuff(midi_tx);

    return;
}

void stm32synth_midi_programAll(uint8_t _program)
{
    uint8_t midi_tx[4];

    state.message = _program;

    for (uint8_t i = 0; i < 16; i++)
    {
        if(i != STM32SYNTH_MIDINN_DRUMCH)
        {
            midi_tx[0] = (0x1 << 4) | (STM32SYNTH_MIDIEVT0_PROGRAMCHANGE & 0x0f);
            midi_tx[1] = ((STM32SYNTH_MIDIEVT0_PROGRAMCHANGE & 0x0f) << 4) | (i & 0x0f);
            midi_tx[2] = _program;
            midi_tx[3] = 0x00;
            stm32synth_component_midi_writebuff(midi_tx);
        }
    }

    return;
}

void stm32synth_midi_pitch(uint8_t _ch, uint8_t _pitch)
{
    uint8_t midi_tx[4];

    state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;

    _ch = (_ch > 15) ? (0) : (_ch);

    midi_tx[0] = (0x0 << 4) | (STM32SYNTH_MIDIEVT0_PITCHBEND & 0x0f);
    midi_tx[1] = ((STM32SYNTH_MIDIEVT0_PITCHBEND & 0x0f) << 4) | (_ch & 0x0f);
    midi_tx[2] = 0x00;
    midi_tx[3] = /*0x40 + */ _pitch;
    stm32synth_component_midi_writebuff(midi_tx);

    return;
}

void stm32synth_midi_reverb(uint8_t _level)
{
    uint8_t midi_tx[4];

    state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;

    midi_tx[0] = (0x1 << 4) | (STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f);
    midi_tx[1] = ((STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f) << 4) | (0 & 0x0f);
    midi_tx[2] = STM32SYNTH_MIDICC_USER_REVERB_LEVEL;
    midi_tx[3] = _level;

    stm32synth_component_midi_writebuff(midi_tx);

    return;
}

void stm32synth_midi_mode(stm32synth_midi_modecontrol_t _mode)
{
    uint8_t midi_tx[4];

    state.message = STM32SYNTH_STATE_MESSAGE_DEFAULT;

    midi_tx[0] = (0x1 << 4) | (STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f);
    midi_tx[1] = ((STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f) << 4) | (0 & 0x0f);
    midi_tx[2] = STM32SYNTH_MIDICC_USER_MODECONTROL;
    midi_tx[3] = _mode;

    stm32synth_component_midi_writebuff(midi_tx);

    return;
}

void midi_spi_noteon(uint8_t _ch, uint8_t _notenum, uint8_t _velocity)
{
	uint8_t midi_tx[4];

	midi_tx[0] = (0x0 << 4) | (STM32SYNTH_MIDIEVT0_NOTEON & 0x0f);
	midi_tx[1] = ((STM32SYNTH_MIDIEVT0_NOTEON & 0x0f) << 4) | (_ch & 0x0f);
	midi_tx[2] = _notenum & 0x7f;
	midi_tx[3] = _velocity & 0x7f;
	stm32synth_component_midi_writebuff(midi_tx);

	return;
}

void midi_spi_noteoff(uint8_t _ch, uint8_t _notenum)
{
	uint8_t midi_tx[4];

	midi_tx[0] = (0x0 << 4) | (STM32SYNTH_MIDIEVT0_NOTEOFF & 0x0f);
	midi_tx[1] = ((STM32SYNTH_MIDIEVT0_NOTEOFF & 0x0f) << 4) | (_ch & 0x0f);
	midi_tx[2] = _notenum & 0x7f;
	midi_tx[3] = 0 & 0x7f;
	stm32synth_component_midi_writebuff(midi_tx);

	return;
}

void midi_spi_allnoteoff()
{
	uint8_t midi_tx[4];

	midi_tx[0] = (0x0 << 4) | (STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f);
	midi_tx[1] = ((STM32SYNTH_MIDIEVT0_CONTROLCHANGE & 0x0f) << 4) | (0 & 0x0f);
	midi_tx[2] = STM32SYNTH_MIDICC_ALLNOTESOFF;
	midi_tx[3] = 0;
	stm32synth_component_midi_writebuff(midi_tx);

	return;
}

void midi_play_noteon(uint8_t _ch, uint8_t _notenum, uint8_t _velocity)
{
	uint8_t midi_tx[4];

	midi_tx[0] = (0x1 << 4) | (STM32SYNTH_MIDIEVT0_NOTEON & 0x0f);
	midi_tx[1] = ((STM32SYNTH_MIDIEVT0_NOTEON & 0x0f) << 4) | (_ch & 0x0f);
	midi_tx[2] = _notenum & 0x7f;
	midi_tx[3] = _velocity & 0x7f;
	stm32synth_component_midi_writebuff(midi_tx);

	return;
}

void midi_play_noteoff(uint8_t _ch, uint8_t _notenum)
{
	uint8_t midi_tx[4];

	midi_tx[0] = (0x1 << 4) | (STM32SYNTH_MIDIEVT0_NOTEOFF & 0x0f);
	midi_tx[1] = ((STM32SYNTH_MIDIEVT0_NOTEOFF & 0x0f) << 4) | (_ch & 0x0f);
	midi_tx[2] = _notenum & 0x7f;
	midi_tx[3] = 0 & 0x7f;
	stm32synth_component_midi_writebuff(midi_tx);

	return;
}

// Private
stm32synth_res_t stm32synth_component_inputmidi(uint8_t *_midi, uint32_t _tick, uint32_t _rectick)
{
    char string[10] = {0};

    stm32synth_res_t res = STM32SYNTH_RES_OK;

    uint8_t cIndex_cableNumber = _midi[0];
    uint8_t evnt0 = _midi[1];
    uint8_t evnt1 = _midi[2];
    uint8_t evnt2 = _midi[3];

    stm32synth_midi_evnt0_t message = (stm32synth_midi_evnt0_t)((evnt0 & 0xF0) >> 4);
    uint8_t channel = evnt0 & 0x0F;
    uint8_t internalMessage = (cIndex_cableNumber & 0xF0) >> 4;

    if(internalMessage > 0)
    {
        _midi[0] &= (uint8_t)0x0F;
    }

    if ((message == STM32SYNTH_MIDIEVT0_NOTEON) && (evnt2 == 0))
    {
        message = STM32SYNTH_MIDIEVT0_NOTEOFF;
    }

    switch (message)
    {
    case STM32SYNTH_MIDIEVT0_NOTEOFF:
    case STM32SYNTH_MIDIEVT0_NOTEON:
    {
        note[evnt1].vel = (message == STM32SYNTH_MIDIEVT0_NOTEOFF) ? (0) : (evnt2);

        if (message == STM32SYNTH_MIDIEVT0_NOTEON)
        {
            note[evnt1].col_noteef.green = note[evnt1].vel & 0x03;
            note[evnt1].col_noteef.red = (note[evnt1].vel >> 2) & 0x03;
            note[evnt1].col_noteef.blue = (note[evnt1].vel >> 4) & 0x03;

            note[evnt1].tick = _tick;
            note[evnt1].cnt = 0;
            note[evnt1].ch = channel;
        }

        // replacement
        if((internalMessage != 1) && ((state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PARA) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) || (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_PLAY)))
        {
            _midi[1] = (_midi[1] & 0xf0) | (sequencer.sel_ch & 0x0f);
        }

        if((internalMessage != 1) && (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) && (sequencer.sel_bar == sequencer.cnt_bar) && (sequencer_play.play))
        {
            if(sequencer.recInThisBar == 0)
            {
                sequencer.layer_cnt[sequencer.sel_bar]++;
                sequencer.recInThisBar = 1;
            }

            if (message == STM32SYNTH_MIDIEVT0_NOTEON)
            {
                //note on
                for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
                {
                    if(sequencer_rec[sequencer.sel_bar][i].nn == 0)
                    {
                        sequencer_rec[sequencer.sel_bar][i].nn = evnt1;
                        sequencer_rec[sequencer.sel_bar][i].vel = evnt2;
                        sequencer_rec[sequencer.sel_bar][i].ch = sequencer.sel_ch;
                        sequencer_rec[sequencer.sel_bar][i].layer = sequencer.layer_cnt[sequencer.sel_bar];
                        sequencer_rec[sequencer.sel_bar][i].start = SEQUENCER_RECTIMERESOLUTION * (sequencer.bpmtick * sequencer.cnt_beat + _rectick) / (sequencer.bpmtick * sequencer.beat);
                        sequencer_rec[sequencer.sel_bar][i].release = 0;
                        break;
                    }
                }
            }
        }

        if((internalMessage != 1) && (state.base_code == STM32SYNTH_DISPSTATE_SEQUENCER_INPUT) && (sequencer_play.play))
        {
            if (message == STM32SYNTH_MIDIEVT0_NOTEOFF)
            {
                //note off
                uint8_t found = 0;
                uint8_t beat = sequencer.cnt_bar;

                for(uint8_t b = 0; b < sequencer.bar; b++)
                {
                    for(uint16_t i = 0; i < SEQUENCER_MAXCHORDPARBAR; i++)
                    {
                        if((sequencer_rec[beat][i].ch == sequencer.sel_ch) && (sequencer_rec[beat][i].nn == evnt1) && (sequencer_rec[beat][i].release == 0))
                        {
                            sequencer_rec[beat][i].release = (SEQUENCER_RECTIMERESOLUTION * b) + (SEQUENCER_RECTIMERESOLUTION * (sequencer.bpmtick * sequencer.cnt_beat + _rectick) / (sequencer.bpmtick * sequencer.beat));
                            found = 1;
                            break;
                        }
                    }

                    if(found == 1)
                    {
                        break;
                    }

                    beat = (beat > 0) ? (beat - 1) : (sequencer.bar - 1);
                }
            }
        }

        if (state.code != state.base_code)
        {
            res = STM32SYNTH_RES_NG;
        }

        break;
    }

    case STM32SYNTH_MIDIEVT0_PROGRAMCHANGE: // Program Change
    {
        neopixel_fb_clear(&col_reset);
        memcpy(string, stm32synth_prog_text[evnt1], 6);
        neopixel_fb_stringBitmap(4, 1, string, 0, &col_string, 1);

        if (internalMessage > 0)
        {
            // neopixel_fb_xbar(0, 6, 32, &col_string);
        }
        else
        {
            neopixel_fb_xbar(2 * channel, 7, 2, &col_string);
        }

        state.code = STM32SYNTH_DISPSTATE_MIDIIN;
        state.cnt = 1;
        state.tick = _tick;

        break;
    }

    case STM32SYNTH_MIDIEVT0_CONTROLCHANGE: // Control Change
    {

        stm32synth_res_t res = stm32synth_midi_inputmidiCC(internalMessage, channel, evnt1, evnt2);
        
        if(res == STM32SYNTH_RES_OK)
        {
            state.code = STM32SYNTH_DISPSTATE_MIDIIN;
            state.cnt = 1;
            state.tick = _tick;
        }

        break;
    }
    default:
    {
        fb_color24_t col_valuebar = {0, 0, 0};

        neopixel_fb_clear(&col_reset);
        memcpy(string, stm32synth_evt_text[message - STM32SYNTH_MIDIEVT0_NOTEOFF], 3);
        sprintf(string + 3, ".%3d", evnt2);
        neopixel_fb_stringBitmap(2, 1, string, 0, &col_string, 1);

        col_valuebar.green = 10 * evnt2 / 127;
        col_valuebar.blue = 10 - col_valuebar.green;
        neopixel_fb_xbar(0, 7, 32 * evnt2 / 127, &col_valuebar);

        neopixel_fb_xbar(2 * channel, 6, 2, &col_string);

        state.code = STM32SYNTH_DISPSTATE_MIDIIN;
        state.cnt = 1;
        state.tick = _tick;

        break;
    }
    }

    return res;
}

stm32synth_res_t stm32synth_component_midi_writebuff(uint8_t *_midi)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    memcpy(midibuff.buff[midibuff.wpos], _midi, 4);
    midibuff.wpos++;
    if (midibuff.wpos >= STM32SYNTH_MIDIBUFF_SIZE)
    {
        midibuff.wpos = 0;
    }

    return res;
}

stm32synth_res_t stm32synth_component_midi_readbuff(uint8_t *_midi)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if (midibuff.rpos == midibuff.wpos)
    {
        return STM32SYNTH_RES_NG;
    }

    memcpy(_midi, midibuff.buff[midibuff.rpos], 4);
    midibuff.rpos++;
    if (midibuff.rpos >= STM32SYNTH_MIDIBUFF_SIZE)
    {
        midibuff.rpos = 0;
    }

    return res;
}

stm32synth_res_t stm32synth_midi_inputmidiCC(uint8_t _im, uint8_t _ch, uint8_t _cc, uint8_t _val)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if((_cc == STM32SYNTH_MIDICC_DAMPERPEDAL) || (_cc == STM32SYNTH_MIDICC_USER_MODECONTROL))
    {
        return STM32SYNTH_RES_NG;
    }
    
    char string[10] = {0};
    fb_color24_t col_valuebar = {0, 0, 0};

    neopixel_fb_clear(&col_reset);
    memcpy(string, stm32synth_cc_text[_cc], 3);
    sprintf(string + 3, ".%3d", _val);
    neopixel_fb_stringBitmap(2, 1, string, 0, &col_string, 1);

    col_valuebar.green = 10 * _val / 127;
    col_valuebar.blue = 10 - col_valuebar.green;
    neopixel_fb_xbar(0, 7, 32 * _val / 127, &col_valuebar);

    if (_im > 0)
    {
        // neopixel_fb_xbar(0, 6, 32, &col_string);
    }
    else
    {
        neopixel_fb_xbar(2 * _ch, 6, 2, &col_string);
    }

    return res;
}

const char stm32synth_evt_text[][3] = {
    "NTF", // STM32SYNTH_MIDIEVT0_NOTEOFF
    "NTO", // STM32SYNTH_MIDIEVT0_NOTEON
    "AFT", // STM32SYNTH_MIDIEVT0_AFTERTOUCH
    "CCH", // STM32SYNTH_MIDIEVT0_CONTROLCHANGE
    "PCH", // STM32SYNTH_MIDIEVT0_PROGRAMCHANGE
    "CAF", // STM32SYNTH_MIDIEVT0_CHANNELAFTERTOUCH
    "PBE", // STM32SYNTH_MIDIEVT0_PITCHBEND
};

const char stm32synth_cc_text[][3] = {
    "BNK", // STM32SYNTH_MIDICC_BANKSLECT
    "MWL", // STM32SYNTH_MIDICC_MODWHELL
    "BCT", // STM32SYNTH_MIDICC_BREATHCONT
    "G03", // STM32SYNTH_MIDICC_GENERAL3
    "FCT", // STM32SYNTH_MIDICC_FOOTCONT
    "PTM", // STM32SYNTH_MIDICC_PORTAMENTOTIME
    "DEN", // STM32SYNTH_MIDICC_DATAENTRY
    "VOL", // STM32SYNTH_MIDICC_VOLUME
    "BAL", // STM32SYNTH_MIDICC_BALANCE
    "G09", // STM32SYNTH_MIDICC_GENERAL9
    "PAN", // STM32SYNTH_MIDICC_PAN
    "EXP", // STM32SYNTH_MIDICC_EXPRESSION
    "E1C", // STM32SYNTH_MIDICC_EFFECTCONT1
    "E2C", // STM32SYNTH_MIDICC_EFFECTCONT2
    "G14", // STM32SYNTH_MIDICC_GENERAL14
    "G15", // STM32SYNTH_MIDICC_GENERAL15
    "G16", // STM32SYNTH_MIDICC_GENERAL16
    "G17", // STM32SYNTH_MIDICC_GENERAL17
    "G18", // STM32SYNTH_MIDICC_GENERAL18
    "G19", // STM32SYNTH_MIDICC_GENERAL19
    "AKL", // STM32SYNTH_MIDICC_USER_ATTACKLEVEL
    "1NA", // STM32SYNTH_MIDICC_USER_WAVE1SINEAMP
    "1QA", // STM32SYNTH_MIDICC_USER_WAVE1SQUAMP
    "1QD", // STM32SYNTH_MIDICC_USER_WAVE1SQUDUTY
    "1TA", // STM32SYNTH_MIDICC_USER_WAVE1TRIAMP
    "1TP", // STM32SYNTH_MIDICC_USER_WAVE1TRIPEAK
    "1PT", // STM32SYNTH_MIDICC_USER_WAVE1PITCH
    "1OC", // STM32SYNTH_MIDICC_USER_WAVE1OCTAVE
    "2NA", // STM32SYNTH_MIDICC_USER_WAVE2SINEAMP
    "2QA", // STM32SYNTH_MIDICC_USER_WAVE2SQUAMP
    "2QD", // STM32SYNTH_MIDICC_USER_WAVE2SQUDUTY
    "2TA", // STM32SYNTH_MIDICC_USER_WAVE2TRIAMP
    "2TP", // STM32SYNTH_MIDICC_USER_WAVE2TRIPEAK
    "2PT", // STM32SYNTH_MIDICC_USER_WAVE2PITCH
    "2OC", // STM32SYNTH_MIDICC_USER_WAVE2OCTAVE
    "DLV", // STM32SYNTH_MIDICC_USER_DISTORTION_LEVEL
    "PBN", // STM32SYNTH_MIDICC_USER_PITCHBYNOTE
    "MCH", // STM32SYNTH_MIDICC_USER_CH0MIRRORCH
    "TTY", // STM32SYNTH_MIDICC_USER_TRE_TYPE
    "TAM", // STM32SYNTH_MIDICC_USER_TRE_AMP
    "TPE", // STM32SYNTH_MIDICC_USER_TRE_PEAK
    "TFR", // STM32SYNTH_MIDICC_USER_TRE_FREQ
    "VTY", // STM32SYNTH_MIDICC_USER_VIB_TYPE
    "VPE", // STM32SYNTH_MIDICC_USER_VIB_PEAK
    "VAM", // STM32SYNTH_MIDICC_USER_VIB_AMP
    "VFR", // STM32SYNTH_MIDICC_USER_VIB_FREQ
    "WOW", // STM32SYNTH_MIDICC_USER_WOW_TYPE
    "WPK", // STM32SYNTH_MIDICC_USER_WOW_PEAK
    "WAM", // STM32SYNTH_MIDICC_USER_WOW_AMP
    "WFR", // STM32SYNTH_MIDICC_USER_WOW_FREQ
    "EVD", // STM32SYNTH_MIDICC_USER_ENV_VOL_DIFF
    "EVT", // STM32SYNTH_MIDICC_USER_ENV_VOL_TIME
    "EFD", // STM32SYNTH_MIDICC_USER_ENV_FREQ_DIFF
    "EFT", // STM32SYNTH_MIDICC_USER_ENV_FREQ_TIME
    "EWD", // STM32SYNTH_MIDICC_USER_ENV_WOW_DIFF
    "EWT", // STM32SYNTH_MIDICC_USER_ENV_WOW_TIME
    "DRT", // STM32SYNTH_MIDICC_USER_DRUM_TYPE
    "DLF", // STM32SYNTH_MIDICC_USER_DRUM_LPF_FCNN
    "DHF", // STM32SYNTH_MIDICC_USER_DRUM_HPF_FCNN
    "DLQ", // STM32SYNTH_MIDICC_USER_DRUM_LPF_Q
    "DHQ", // STM32SYNTH_MIDICC_USER_DRUM_HPF_Q
    "G61", // STM32SYNTH_MIDICC_GENERAL61
    "G62", // STM32SYNTH_MIDICC_GENERAL62
    "G63", // STM32SYNTH_MIDICC_GENERAL63
    "DMP", // STM32SYNTH_MIDICC_DAMPERPEDAL
    "SST", // STM32SYNTH_MIDICC_SOSTENTOPEDAL
    "SFT", // STM32SYNTH_MIDICC_SOFTPEDAL
    "LFS", // STM32SYNTH_MIDICC_LEGATOFOOTSWITCH
    "HLD", // STM32SYNTH_MIDICC_HOLD2
    "SC1", // STM32SYNTH_MIDICC_SOUNDCONT1
    "SC2", // STM32SYNTH_MIDICC_SOUNDCONT2
    "LPQ", // STM32SYNTH_MIDICC_USER_LPFQFACTOR
    "RLT", // STM32SYNTH_MIDICC_USER_RELEASETIME
    "AKT", // STM32SYNTH_MIDICC_USER_ATTACKTIME
    "LPC", // STM32SYNTH_MIDICC_USER_LPFCUTOFF
    "G75", // STM32SYNTH_MIDICC_GENERAL75
    "G76", // STM32SYNTH_MIDICC_GENERAL76
    "G77", // STM32SYNTH_MIDICC_GENERAL77
    "G78", // STM32SYNTH_MIDICC_GENERAL78
    "G79", // STM32SYNTH_MIDICC_GENERAL79
    "DCT", // STM32SYNTH_MIDICC_USER_DECAYTIME
    "G81", // STM32SYNTH_MIDICC_GENERAL81
    "G82", // STM32SYNTH_MIDICC_GENERAL82
    "G83", // STM32SYNTH_MIDICC_GENERAL83
    "PTC", // STM32SYNTH_MIDICC_PORTAMENTOCC
    "G85", // STM32SYNTH_MIDICC_GENERAL85
    "G86", // STM32SYNTH_MIDICC_GENERAL86
    "G87", // STM32SYNTH_MIDICC_GENERAL87
    "HRV", // STM32SYNTH_MIDICC_HIGHRESVELOCITYPREFIX
    "G89", // STM32SYNTH_MIDICC_GENERAL89
    "G90", // STM32SYNTH_MIDICC_GENERAL90
    "RVB", // STM32SYNTH_MIDICC_USER_REVERB_LEVEL
    "ED2", // STM32SYNTH_MIDICC_EFFECTDEPTH2
    "ED3", // STM32SYNTH_MIDICC_EFFECTDEPTH3
    "ED4", // STM32SYNTH_MIDICC_EFFECTDEPTH4
    "ED5", // STM32SYNTH_MIDICC_EFFECTDEPTH5
    "DIN", // STM32SYNTH_MIDICC_DATAINCREMENT
    "DDE", // STM32SYNTH_MIDICC_DATADECREMENT
    "NLS", // STM32SYNTH_MIDICC_NRPNLSB
    "NMS", // STM32SYNTH_MIDICC_NRPNMSB
    "RLS", // STM32SYNTH_MIDICC_RPNLSB
    "RMS", // STM32SYNTH_MIDICC_RPNMSB
    "MTT", // STM32SYNTH_MIDICC_USER_MTRE_TYPE
    "MTP", // STM32SYNTH_MIDICC_USER_MTRE_PEAK
    "MTA", // STM32SYNTH_MIDICC_USER_MTRE_AMP
    "MTF", // STM32SYNTH_MIDICC_USER_MTRE_FREQ
    "MVT", // STM32SYNTH_MIDICC_USER_MVIB_TYPE
    "MVP", // STM32SYNTH_MIDICC_USER_MVIB_PEAK
    "MVA", // STM32SYNTH_MIDICC_USER_MVIB_AMP
    "MVF", // STM32SYNTH_MIDICC_USER_MVIB_FREQ
    "MWT", // STM32SYNTH_MIDICC_USER_MWOW_TYPE
    "MWP", // STM32SYNTH_MIDICC_USER_MWOW_PEAK
    "MWA", // STM32SYNTH_MIDICC_USER_MWOW_AMP
    "MWF", // STM32SYNTH_MIDICC_USER_MWOW_FREQ
    "MLQ", // STM32SYNTH_MIDICC_USER_MLPFQFACTOR
    "MLC", // STM32SYNTH_MIDICC_USER_MLPFCUTOFF
    "116", // STM32SYNTH_MIDICC_GENERAL116
    "117", // STM32SYNTH_MIDICC_GENERAL117
    "PRE", // STM32SYNTH_MIDICC_USER_PRESET
    "SAV", // STM32SYNTH_MIDICC_USER_SAVECONFIG
    "AOF", // STM32SYNTH_MIDICC_ALLSOUNDOFF
    "RAL", // STM32SYNTH_MIDICC_RESETALL
    "LOC", // STM32SYNTH_MIDICC_LOCALSWITCH
    "ANO", // STM32SYNTH_MIDICC_ALLNOTESOFF
    "OMO", // STM32SYNTH_MIDICC_OMNIMODEOFF
    "OMN", // STM32SYNTH_MIDICC_OMNIMODEON
    "MON", // STM32SYNTH_MIDICC_MONOMODE
    "POL", // STM32SYNTH_MIDICC_POLYMODE
};

const char stm32synth_prog_text[][6] = {
    // Piano Timbres:
    "ACGRPN", // Acoustic Grand Piano
    "BRACPN", // Bright Acoustic Piano
    "ELGRPN", // Electric Grand Piano
    "HONKPN", // Honky-tonk Piano
    "RHODPN", // Rhodes Piano
    "CHORPN", // Chorused Piano
    "HAPSCD", // Harpsichord
    "CLAVNT", // Clavinet

    // Chromatic Percussion:
    "CELEST", // Celesta
    "GLKSPL", // Glockenspiel
    "MUSBOX", // Music Box
    "VIBRPH", // Vibraphone
    "MARIMB", // Marimba
    "XYLOPH", // Xylophone
    "TUBBEL", // Tubular Bells
    "DULCIM", // Dulcimer

    // Organ Timbres:
    "HAMORG", // Hammond Organ
    "PERORG", // Percussive Organ
    "RCKORG", // Rock Organ
    "CHRORG", // Church Organ
    "REEORG", // Reed Organ
    "ACCORD", // Accordion
    "HARMON", // Harmonica
    "TNGACD", // Tango Accordion

    // Guitar Timbres:
    "NYLGTR", // Acoustic Nylon Guitar
    "STLGTR", // Acoustic Steel Guitar
    "JAZGTR", // Electric Jazz Guitar
    "CLNGTR", // Electric Clean Guitar
    "MUTGTR", // Electric Muted Guitar
    "OVRGTR", // Overdriven Guitar
    "DISGTR", // Distortion Guitar
    "GTRHMN", // Guitar Harmonics

    // Bass Timbres:
    "ACSTBS", // Acoustic Bass
    "FINGBS", // Fingered Electric Bass
    "PLELBS", // Plucked Electric Bass
    "FRETBS", // Fretless Bass
    "SLAPB1", // Slap Bass 1
    "SLAPB2", // Slap Bass 2
    "SYNTB1", // Synth Bass 1
    "SYNTB2", // Synth Bass 2

    // String Timbres:
    "VIOLIN", // Violin
    "VIOLA ", // Viola
    "CELLO ", // Cello
    "CONTRB", // Contrabass
    "TRESTR", // Tremolo Strings
    "PIZSTR", // Pizzicato Strings
    "ORCHHP", // Orchestral Harp
    "TIMPNI", // Timpani

    // Ensemble Timbres:
    "STRENS", // String Ensemble 1
    "STREN2", // String Ensemble 2
    "SYNSTR", // Synth Strings 1
    "SYNST2", // Synth Strings 2
    "CHOAAH", // Choir "Aah"
    "CHOOOH", // Choir "Ooh"
    "SYNVOC", // Synth Voice
    "ORCHIT", // Orchestral Hit

    // Brass Timbres:
    "TRUMPT", // Trumpet
    "TROMBN", // Trombone
    "TUBA  ", // Tuba
    "MUTTRP", // Muted Trumpet
    "FRHORN", // French Horn
    "BRASSC", // Brass Section
    "SYBRAS", // Synth Brass 1
    "SYBRA2", // Synth Brass 2

    // Reed Timbres:
    "SOPRSX", // Soprano Sax
    "ALTOSX", // Alto Sax
    "TENOSX", // Tenor Sax
    "BRTNSX", // Baritone Sax
    "OBOE  ", // Oboe
    "ENHORN", // English Horn
    "BASSON", // Bassoon
    "CLARNT", // Clarinet

    // Pipe Timbres:
    "PICCOL", // Piccolo
    "FLUTE ", // Flute
    "RECORD", // Recorder
    "PANFLT", // Pan Flute
    "BOTBLE", // Bottle Blow
    "SHAKUH", // Shakuhachi
    "WHISTL", // Whistle
    "OCARIN", // Ocarina

    // Synth Lead:
    "SQLEAD", // Square Wave Lead
    "SWLEAD", // Sawtooth Wave Lead
    "CALLIO", // Calliope Lead
    "CHIFFL", // Chiff Lead
    "CHARAN", // Charang Lead
    "VOLEAD", // Voice Lead
    "FIFTHS", // Fifths Lead
    "BASSLD", // Bass Lead

    // Synth Pad:
    "NEAGPD", // New Age Pad
    "WARMPD", // Warm Pad
    "POLYPD", // Polysynth Pad
    "CHORPD", // Choir Pad
    "BOWDPD", // Bowed Pad
    "MTLCPD", // Metallic Pad
    "HALOPD", // Halo Pad
    "SWEPPD", // Sweep Pad

    // Synth Effects:
    "RAINEF", // Rain Effect
    "SOUNDE", // Soundtrack Effect
    "CRYSTL", // Crystal Effect
    "ATMOSP", // Atmosphere Effect
    "BRIGHT", // Brightness Effect
    "GOBLIN", // Goblins Effect
    "ECHOES", // Echoes Effect
    "SCIFIE", // Sci-Fi Effect

    // Ethnic Timbres:
    "SITAR ", // Sitar
    "BANJO ", // Banjo
    "SHAMIS", // Shamisen
    "KOTO  ", // Koto
    "KALIMB", // Kalimba
    "BAGPIP", // Bagpipe
    "FIDDLE", // Fiddle
    "SHANAI", // Shanai

    // Sound Effects:
    "TINKLE", // Tinkle Bell
    "AGOGO ", // Agogo
    "STEELD", // Steel Drums
    "WOODBL", // Woodblock
    "TAIKOD", // Taiko Drum
    "MELOTM", // Melodic Tom
    "SYNDRM", // Synth Drum
    "REVCSM", // Reverse Cymbal

    // Sound Effects:
    "GFRETN", // Guitar Fret Noise
    "BREATH", // Breath Noise
    "SEASHR", // Seashore
    "BIRDTW", // Bird Tweet
    "TELEPH", // Telephone Ring
    "HELICP", // Helicopter
    "APPLSE", // Applause
    "GUNSHT", // Gun Shot
};

const char mode_text[][7] = {
    "FREEPLY", // MODE_STATE_FREEPLAY
    "METRNOM", // MODE_STATE_METRONOME
    "SQ-PARA", // MODE_STATE_SEQUENCER_PARA
    "SQ-PROG", // MODE_STATE_SEQUENCER_INPUT
    "SQ-PLAY", // MODE_STATE_SEQUENCER_PLAY
    "SETTING", // MODE_STATE_SETTING
};
