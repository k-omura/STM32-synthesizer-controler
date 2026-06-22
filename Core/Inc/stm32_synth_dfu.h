/*
 * stm32_synth_dfu.h
 *
 *  Created on: July 1, 2025
 *      Author: k-omura
 */

#ifndef STM32_SYNTH_COMPONENTS_STM32_SYNTH_DFU_H_
#define STM32_SYNTH_COMPONENTS_STM32_SYNTH_DFU_H_

#include "stm32_synth_midi.h"

#define SYSTEM_MEMORY_RESET_VECTOR (0x1FFF0000) // System memory reset vector address

stm32synth_res_t stm32_synth_goto_dfu(void);

#endif /* STM32_SYNTH_COMPONENTS_STM32_SYNTH_DFU_H_ */
