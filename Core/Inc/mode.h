/*
 * mode.h
 *
 *  Created on: May 7, 2025
 *      Author: omurakosuke
 */

#ifndef INC_MODE_H_
#define INC_MODE_H_

#include "main.h"
#include "stm32_synth_midi.h"

typedef struct
{
    mode_state_t mode;
    uint32_t tick;
} mode_t;

typedef enum
{
    INSIDE_PARA_VOLUME = 0,
    INSIDE_PARA_PROGRAM,
    INSIDE_PARA_COUNT,
} inside_para_t;

typedef enum
{
    RE_BTN_STATE_RELEASE = 0,
    RE_BTN_STATE_PUSHED,
    RE_BTN_STATE_PUSHING,
} re_btn_state_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;

    re_btn_state_t state;
    re_btn_state_t prev_state;
    uint32_t tick;

} re_btn_t;

typedef struct
{
    GPIO_TypeDef *port[2];
    uint16_t pin[2];

    uint16_t firstpin;
    uint16_t secondpin;

    uint32_t tick;
    int16_t count;
    uint8_t control;
} re_rot_t;

void re_init();
void re_hundle(uint32_t _tick);
void re_exti_callback(uint16_t _GPIO_Pin);
void re_tim_callback();
uint8_t re_static_check(uint8_t _re);


#endif /* INC_GUI_H_ */
