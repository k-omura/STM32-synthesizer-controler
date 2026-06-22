/*
 * mode.c
 *
 *  Created on: May 7, 2025
 *      Author: k-omura
 */

#include "mode.h"
#include "neopixel.h"
#include "stm32_synth_dfu.h"

#define DEFAULT_BPM_STEP (5)

static mode_t mode = {MODE_STATE_FREEPLAY, 0};
static re_btn_t re_btn[2];
static re_rot_t re_rot[2];

static uint16_t cnt_freemode = 0;
static uint16_t cnt_metronome_bpm = DEFAULT_BPM;
static uint16_t cnt_channel = 16;
static uint16_t cnt_reverb = 64;
static uint16_t cnt_midi[INSIDE_PARA_COUNT][16] = {0};

static seqpara_t cnt_seqpara_select = CNTSEQ_BPM;
static uint16_t cnt_seqpara_bpm = DEFAULT_BPM;
static uint16_t cnt_seqpara_beat = 4;
static uint16_t cnt_seqpara_bar = 2;
static uint16_t cnt_seqpara_sel_bar = 0;
static uint16_t cnt_seqpara_sel_channel = 0;
static uint16_t cnt_seqpara_metronome = 1;
static uint16_t cnt_seqpara_playmode = 0;
static uint16_t cnt_setting = 0;

// Private function prototypes
void cnt_midi_val(uint8_t _re, uint8_t _cont, int16_t _count);

void cnt_midi_cont_change(uint8_t *_cnt, uint8_t _re);
void cnt_midi_program();
void cnt_midi_vol();
void cnt_midi_channel();
void cnt_midi_reverb();
void cnt_midi_mode();
void cnt_midi_modepara();

void cnt_metronme_tap2bpm(uint32_t _tick);

void cnt_seq_bpm();
void cnt_seq_beat();
void cnt_seq_bar();
void cnt_seq_selbar();
void cnt_seq_channel();
void cnt_seq_metronome();
void cnt_seq_playmutech();
void cnt_seq_playmode();

void cnt_setting_execute();

//
typedef enum
{
  RE1_CNT_MODE,
  RE1_CNT_VOLUME,
  RE1_CNT_PROGRAM,

  RE1_CNT_COUNT,
} re1_cnt_t;

static void (*submode_re1_p[])() = {
    cnt_midi_mode,
    cnt_midi_vol,
    cnt_midi_program,
};

typedef enum
{
  RE2_CNT_CHANNEL,
  RE2_CNT_REVERB,

  RE2_CNT_MODEPARA,
  RE2_CNT_COUNT,
} re2_cnt_t;

static void (*submode_re2_p[])() = {
    cnt_midi_channel,
    cnt_midi_reverb,

    cnt_midi_modepara,
};

// Public function
void re_exti_callback(uint16_t _GPIO_Pin)
{
  switch (_GPIO_Pin)
  {

  case RE1_BTN_Pin:
  {
    if (HAL_GPIO_ReadPin(re_btn[0].port, re_btn[0].pin) == GPIO_PIN_RESET)
    {
      if (re_btn[0].state == RE_BTN_STATE_RELEASE)
      {
        re_btn[0].state = RE_BTN_STATE_PUSHED;
      }
    }
    else
    {
      if (re_btn[0].state != RE_BTN_STATE_RELEASE)
      {
        re_btn[0].state = RE_BTN_STATE_RELEASE;
      }
    }
    re_btn[0].prev_state = re_btn[0].state;
    break;
  }

  case RE1_P1_Pin:
  case RE1_P2_Pin:
  {
    if (re_rot[0].firstpin == 0)
    {
      re_rot[0].firstpin = _GPIO_Pin;
    }
    else
    {
      re_rot[0].secondpin = _GPIO_Pin;
    }

    break;
  }

  case RE2_BTN_Pin:
  {
    if (HAL_GPIO_ReadPin(re_btn[1].port, re_btn[1].pin) == GPIO_PIN_RESET)
    {
      if (re_btn[1].state == RE_BTN_STATE_RELEASE)
      {
        re_btn[1].state = RE_BTN_STATE_PUSHED;
      }
    }
    else
    {
      if (re_btn[1].state != RE_BTN_STATE_RELEASE)
      {
        re_btn[1].state = RE_BTN_STATE_RELEASE;
      }
    }
    re_btn[1].prev_state = re_btn[1].state;
    break;
  }

  case RE2_P1_Pin:
  case RE2_P2_Pin:
  {
    if (re_rot[1].firstpin == 0)
    {
      re_rot[1].firstpin = _GPIO_Pin;
    }
    else
    {
      re_rot[1].secondpin = _GPIO_Pin;
    }

    break;
  }
  }

  return;
}

void re_tim_callback()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    // button
    if (HAL_GPIO_ReadPin(re_btn[i].port, re_btn[i].pin) == GPIO_PIN_RESET)
    {
      if (re_btn[i].state == RE_BTN_STATE_PUSHED)
      {
        re_btn[i].prev_state = re_btn[i].state;
        re_btn[i].state = RE_BTN_STATE_PUSHING;
        // re_btn[i].tick = HAL_GetTick();
      }
    }

    // encoder
    if ((re_rot[i].firstpin != 0) && (re_rot[i].secondpin != 0))
    {
      GPIO_PinState RE_P1 = HAL_GPIO_ReadPin(re_rot[i].port[0], re_rot[i].pin[0]);
      GPIO_PinState RE_P2 = HAL_GPIO_ReadPin(re_rot[i].port[1], re_rot[i].pin[1]);

      if ((RE_P1 == GPIO_PIN_SET) && (RE_P1 == RE_P2))
      {
        if (re_rot[i].firstpin == re_rot[i].pin[0])
        { // Otherwise B changed first and we're moving CCW
          re_rot[i].count--;
        }
        else
        { // Means pin A Changed first - We're Rotating Clockwise
          re_rot[i].count++;
        }
        re_rot[i].firstpin = 0;
        re_rot[i].secondpin = 0;
      }
    }
  }

  return;
}

void re_init()
{
  // HW config
  re_rot[0].port[0] = GPIOC;
  re_rot[0].port[1] = GPIOA;
  re_rot[0].pin[0] = RE1_P1_Pin;
  re_rot[0].pin[1] = RE1_P2_Pin;
  re_rot[1].port[0] = GPIOB;
  re_rot[1].port[1] = GPIOB;
  re_rot[1].pin[0] = RE2_P1_Pin;
  re_rot[1].pin[1] = RE2_P2_Pin;

  re_btn[0].port = GPIOC;
  re_btn[0].pin = RE1_BTN_Pin;
  re_btn[1].port = GPIOB;
  re_btn[1].pin = RE2_BTN_Pin;

  // Control config
  re_rot[0].control = RE1_CNT_VOLUME;
  re_rot[1].control = RE2_CNT_REVERB;

  for (uint8_t i = 0; i < 16; i++)
  {
    cnt_midi[INSIDE_PARA_VOLUME][i] = 100;
    cnt_midi[INSIDE_PARA_PROGRAM][i] = 0;
  }

  // read board type & set
  uint8_t led_type = (HAL_GPIO_ReadPin(LED_TYPE_GPIO_Port, LED_TYPE_Pin) == GPIO_PIN_RESET) ? (1) : (0);
  neopixel_settype(led_type);

  return;
}

void re_hundle(uint32_t _tick)
{
  uint16_t excutemode = 0;

  for (uint8_t i = 0; i < 2; i++)
  {
    // button
    if ((re_btn[i].state == RE_BTN_STATE_PUSHING) && (re_btn[i].prev_state == RE_BTN_STATE_PUSHED))
    {
      re_btn[i].prev_state = re_btn[i].state;

      if ((i == 1) && (re_rot[0].control == RE1_CNT_MODE))
      {
        excutemode = RE2_CNT_MODEPARA;

        if (mode.mode == MODE_STATE_SEQUENCER_PARA)
        {
          cnt_midi_cont_change(&cnt_seqpara_select, 2);
        }

        if (mode.mode == MODE_STATE_METRONOME)
        {
          cnt_metronme_tap2bpm(_tick);
        }

        if(mode.mode == MODE_STATE_SEQUENCER_INPUT)
        {
          stm32synth_seq_progpush((cnt_seqpara_sel_bar < cnt_seqpara_bar) ? (0) : (1));
        }

        if(mode.mode == MODE_STATE_SEQUENCER_PLAY)
        {
          cnt_seq_playmode();
        }

        if(mode.mode == MODE_STATE_SETTING)
        {
          cnt_setting_execute();
        }
      }
      else
      {
        cnt_midi_cont_change(&re_rot[i].control, i);
        excutemode = re_rot[i].control;
      }

      if (i == 0)
      {
        submode_re1_p[excutemode]();
      }
      else
      {
        submode_re2_p[excutemode]();
      }
    }

    // encoder
    if (re_rot[i].count)
    {
      if ((i == 1) && (re_rot[0].control == RE1_CNT_MODE))
      {
        excutemode = RE2_CNT_MODEPARA;
      }
      else
      {
        excutemode = re_rot[i].control;
      }

      cnt_midi_val(i, excutemode, re_rot[i].count);
      re_rot[i].count = 0;

      if (i == 0)
      {
        submode_re1_p[excutemode]();
      }
      else
      {
        submode_re2_p[excutemode]();
      }
    }
  }

  return;
}

uint8_t re_static_check(uint8_t _re)
{
  return (HAL_GPIO_ReadPin(re_btn[_re].port, re_btn[_re].pin) == GPIO_PIN_RESET) ? (1) : (0);
}

// Private
void cnt_midi_modepara()
{
  switch (mode.mode)
  {
  case MODE_STATE_FREEPLAY:
  {
    stm32synth_disp_freeplay(cnt_freemode);
    break;
  }
  case MODE_STATE_METRONOME:
  {
    cnt_seqpara_bpm = cnt_metronome_bpm;
    stm32synth_disp_metronome(cnt_metronome_bpm);
    break;
  }
  case MODE_STATE_SEQUENCER_PARA:
  {
    // stm32synth_disp_metronome(cnt_metronome_bpm);
    switch (cnt_seqpara_select)
    {
    case CNTSEQ_BPM:
    {
      cnt_seq_bpm();
      break;
    }
    case CNTSEQ_BEAT:
    {
      cnt_seq_beat();
      break;
    }
    case CNTSEQ_BAR:
    {
      cnt_seq_bar();
      break;
    }
    case CNTSEQ_CHANNEL:
    {
      cnt_seq_channel();
      break;
    }
    case CNTSEQ_METRONOME:
    {
      cnt_seq_metronome();
      break;
    }
    default:
      break;
    }

    break;
  }
  case MODE_STATE_SEQUENCER_INPUT:
  {
    cnt_seq_selbar();
    break;
  }
  case MODE_STATE_SEQUENCER_PLAY:
  {
    cnt_seq_playmutech();
    break;
  }
  case MODE_STATE_SETTING:
  {
    stm32synth_disp_setting(cnt_setting);
    break;
  }
  default:
    break;
  }
  return;
}

void cnt_midi_val(uint8_t _re, uint8_t _cont, int16_t _count)
{
  int32_t min = 0;
  int32_t max = 127;
  int32_t step = 1;
  int32_t val;
  uint16_t *in_val;
  uint16_t *mrr_val;

  if (_re == 0)
  {
    switch (_cont)
    {
    case RE1_CNT_MODE:
    {
      min = 0;
      max = (MODE_STATE_COUNT - 1);
      in_val = (uint16_t *)&mode.mode;
      break;
    }
    case RE1_CNT_VOLUME:
    {
      uint16_t cnt = INSIDE_PARA_VOLUME;
      step = 4;
      if (cnt_channel > 15)
      {
        in_val = &cnt_midi[cnt][0];
        mrr_val = in_val;
      }
      else
      {
        in_val = &cnt_midi[cnt][cnt_channel];
      }
      break;
    }
    case RE1_CNT_PROGRAM:
    {
      uint16_t cnt = INSIDE_PARA_PROGRAM;
      if (cnt_channel > 15)
      {
        in_val = &cnt_midi[cnt][0];
        mrr_val = in_val;
      }
      else
      {
        in_val = &cnt_midi[cnt][cnt_channel];
      }
      break;
    }
    default:
    {
      return;
      break;
    }
    }
  }
  else
  {
    switch (_cont)
    {
    case RE2_CNT_CHANNEL:
    {
      min = 0;
      max = 16;
      in_val = &cnt_channel;
      break;
    }
    case RE2_CNT_REVERB:
    {
      step = 4;
      in_val = &cnt_reverb;
      break;
    }
    case RE2_CNT_MODEPARA:
    {
      switch (mode.mode)
      {
      case MODE_STATE_FREEPLAY:
      {
        min = 0;
        max = 1;
        in_val = &cnt_freemode;
        break;
      }
      case MODE_STATE_METRONOME:
      {
        min = DEFAULT_BPM_MIN;
        max = DEFAULT_BPM_MAX;
        step = DEFAULT_BPM_STEP;
        in_val = &cnt_metronome_bpm;
        break;
      }
      case MODE_STATE_SEQUENCER_PARA:
      {
        switch (cnt_seqpara_select)
        {
        case CNTSEQ_BPM:
        {
          min = DEFAULT_BPM_MIN;
          max = DEFAULT_BPM_MAX;
          step = DEFAULT_BPM_STEP;
          in_val = &cnt_seqpara_bpm;
          break;
        }
        case CNTSEQ_BEAT:
        {
          min = 2;
          max = 32;
          in_val = &cnt_seqpara_beat;
          break;
        }
        case CNTSEQ_BAR:
        {
          min = 1;
          max = SEQUENCER_MAXBAR;
          in_val = &cnt_seqpara_bar;
          break;
        }
        case CNTSEQ_CHANNEL:
        {
          min = 0;
          max = 15;
          in_val = &cnt_seqpara_sel_channel;
          break;
        }
        case CNTSEQ_METRONOME:
        {
          min = 0;
          max = 1;
          in_val = &cnt_seqpara_metronome;
          break;
        }
        default:
        {
          return;
          break;
        }
        }
        break;
      }
      case MODE_STATE_SEQUENCER_INPUT:
      {
        min = 0;
        max = (cnt_seqpara_bar * 2) - 1;
        in_val = &cnt_seqpara_sel_bar;
        break;
      }
      case MODE_STATE_SEQUENCER_PLAY:
      {
        min = 0;
        max = 16;
        in_val = &cnt_seqpara_playmode;
        break;
      }
      case MODE_STATE_SETTING:
      {
        min = 0;
        max = 0;
        in_val = &cnt_setting;
        break;
      }
      default:
      {
        return;
        break;
      }
      }
      break;
    }
    default:
    {
      return;
      break;
    }
    }
  }

  val = *in_val + (step * _count);

  if (val < min)
  {
    *in_val = min;
  }
  else if (val > max)
  {
    *in_val = max;
  }
  else
  {
    *in_val = val;
  }

  if (((_re == 0) && (_cont == RE1_CNT_VOLUME)) || ((_re == 0) && (_cont == RE1_CNT_PROGRAM)))
  {
    if (cnt_channel > 15)
    {
      for (uint8_t i = 1; i < 16; i++)
      {
        *mrr_val++ = *in_val;
      }
    }
  }

  return;
}

void cnt_midi_cont_change(uint8_t *_cnt, uint8_t _re)
{
  uint16_t countmax;

  switch (_re)
  {
  case 0: // RE1
  {
    countmax = RE1_CNT_COUNT - 1;
    break;
  }
  case 1: // RE2
  {
    countmax = RE2_CNT_COUNT - 2;
    break;
  }
  case 2: // RE2_SEQ
  {
    countmax = CNTSEQ_METRONOME;
    break;
  }
  default:
  {
    return;
    break;
  }
  }

  if (*_cnt < countmax)
  {
    (*_cnt)++;
  }
  else
  {
    (*_cnt) = 0;
  }

  return;
}

void cnt_midi_program()
{
  if (cnt_channel > 15)
  {
    stm32synth_midi_programAll((uint8_t)cnt_midi[INSIDE_PARA_PROGRAM][0]);
  }
  else if (cnt_channel >= 0)
  {
    stm32synth_midi_program((uint8_t)cnt_channel, (uint8_t)cnt_midi[INSIDE_PARA_PROGRAM][cnt_channel]);
  }
  return;
}

void cnt_midi_vol()
{
  if (cnt_channel > 15)
  {
    stm32synth_midi_volumeAll((uint8_t)cnt_midi[INSIDE_PARA_VOLUME][0]);
  }
  else if (cnt_channel >= 0)
  {
    stm32synth_midi_volume((uint8_t)cnt_channel, (uint8_t)cnt_midi[INSIDE_PARA_VOLUME][cnt_channel]);
  }
  return;
}

void cnt_midi_mode()
{
  stm32synth_disp_mode(mode.mode);
  return;
}

void cnt_midi_channel()
{
  stm32synth_disp_channel((uint8_t)cnt_channel);
  return;
}

void cnt_midi_reverb()
{
  stm32synth_midi_reverb((uint8_t)cnt_reverb);
  return;
}

void cnt_metronme_tap2bpm(uint32_t _tick)
{
  static uint32_t last_tick = 0;
  
  uint16_t bpm = (60 * 1000) / (_tick - last_tick);
  bpm = bpm - (bpm % DEFAULT_BPM_STEP); // Round to nearest DEFAULT_BPM_STEP
  last_tick = _tick;

  if ((DEFAULT_BPM_MIN <= bpm) && (bpm <= DEFAULT_BPM_MAX))
  {
    cnt_metronome_bpm = bpm;
    cnt_seqpara_bpm = cnt_metronome_bpm;
    stm32synth_disp_metronome(cnt_metronome_bpm);
  }

  return;
}

void cnt_seq_bpm()
{
  cnt_metronome_bpm = cnt_seqpara_bpm;
  stm32synth_seq_bpm(cnt_seqpara_bpm);
  return;
}

void cnt_seq_beat()
{
  stm32synth_seq_beat(cnt_seqpara_beat);
  return;
}

void cnt_seq_bar()
{
  stm32synth_seq_bar(cnt_seqpara_bar);
  return;
}

void cnt_seq_selbar()
{
  stm32synth_seq_selbar(cnt_seqpara_sel_bar % cnt_seqpara_bar, (cnt_seqpara_sel_bar < cnt_seqpara_bar) ? (0) : (1));
  //stm32synth_seq_selbar(cnt_seqpara_sel_bar, 0);
  return;
}

void cnt_seq_metronome()
{
  stm32synth_seq_metronome(cnt_seqpara_metronome);
  return;
}

void cnt_seq_channel()
{
  stm32synth_seq_channel(cnt_seqpara_sel_channel);
  return;
}

void cnt_seq_playmutech()
{
  stm32synth_seq_mutech(cnt_seqpara_playmode);
}

void cnt_seq_playmode()
{
  if(cnt_seqpara_playmode == 0)
  {
    stm32synth_seq_playtoggle();
  }
  else
  {
    stm32synth_seq_mutechtoggle(cnt_seqpara_playmode - 1);
  }
}

void cnt_setting_execute()
{
  switch(cnt_setting)
  {
    case SET_DFU:
      stm32_synth_goto_dfu();
      break;
    default:
      break;
  }

  return;
}
