/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BOARD_LED_Pin GPIO_PIN_13
#define BOARD_LED_GPIO_Port GPIOC
#define RE1_BTN_Pin GPIO_PIN_14
#define RE1_BTN_GPIO_Port GPIOC
#define RE1_BTN_EXTI_IRQn EXTI15_10_IRQn
#define RE1_P1_Pin GPIO_PIN_15
#define RE1_P1_GPIO_Port GPIOC
#define RE1_P1_EXTI_IRQn EXTI15_10_IRQn
#define RE1_P2_Pin GPIO_PIN_0
#define RE1_P2_GPIO_Port GPIOA
#define RE1_P2_EXTI_IRQn EXTI0_IRQn
#define bleConnect_Pin GPIO_PIN_3
#define bleConnect_GPIO_Port GPIOA
#define bleInitMode_Pin GPIO_PIN_2
#define bleInitMode_GPIO_Port GPIOB
#define LED_TYPE_Pin GPIO_PIN_10
#define LED_TYPE_GPIO_Port GPIOB
#define RE2_BTN_Pin GPIO_PIN_7
#define RE2_BTN_GPIO_Port GPIOB
#define RE2_BTN_EXTI_IRQn EXTI9_5_IRQn
#define RE2_P1_Pin GPIO_PIN_8
#define RE2_P1_GPIO_Port GPIOB
#define RE2_P1_EXTI_IRQn EXTI9_5_IRQn
#define RE2_P2_Pin GPIO_PIN_9
#define RE2_P2_GPIO_Port GPIOB
#define RE2_P2_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
