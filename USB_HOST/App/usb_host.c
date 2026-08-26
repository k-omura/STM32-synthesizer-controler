/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file            : usb_host.c
 * @version         : v1.0_Cube
 * @brief           : This file implements the USB Host
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_audio.h"

/* USER CODE BEGIN Includes */

// #include "stdio.h"
// #include "string.h"
#include "usbh_MIDI.h"
#include "io_midi_types.h"
#include "io_midi_transport.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define RX_BUFF_SIZE 256                     /* USB MIDI buffer : max received data 256 bytes */
static uint8_t MIDI_RX_Buffer[RX_BUFF_SIZE]; // MIDI reception buffer

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
static void ProcessReceivedMidiDatas(void);
/* USER CODE END PFP */

/* USB Host core handle declaration */
USBH_HandleTypeDef hUsbHostFS;
static ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

void MIDI_Application(void)
{
  switch (Appli_state)
  {
  case APPLICATION_DISCONNECT:
    USBH_MIDI_Stop(&hUsbHostFS);
    HAL_GPIO_WritePin(GPIOC, BOARD_LED_Pin, GPIO_PIN_SET);
    Appli_state = APPLICATION_IDLE;
    break;

  case APPLICATION_READY:
    USBH_MIDI_Receive(&hUsbHostFS, MIDI_RX_Buffer, RX_BUFF_SIZE);
    HAL_GPIO_WritePin(GPIOC, BOARD_LED_Pin, GPIO_PIN_RESET);
    Appli_state = APPLICATION_IDLE;
    break;

  case APPLICATION_START:
    HAL_GPIO_WritePin(GPIOC, BOARD_LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOC, BOARD_LED_Pin, GPIO_PIN_SET);
    Appli_state = APPLICATION_IDLE;
    break;

  default:
    break;
  }
}

void USBH_MIDI_ReceiveCallback(USBH_HandleTypeDef *phost)
{
  ProcessReceivedMidiDatas();
  USBH_MIDI_Receive(&hUsbHostFS, MIDI_RX_Buffer, RX_BUFF_SIZE); // start a new reception
}

static void ProcessReceivedMidiDatas(void)
{
  uint32_t numberOfPackets;
  uint8_t *ptr = MIDI_RX_Buffer;

  numberOfPackets = USBH_MIDI_GetLastReceivedDataSize(&hUsbHostFS) / 4; // each USB midi package is 4 bytes long

  while (numberOfPackets--)
  {
    uint8_t evnt0 = *(ptr + 1);
    evnt0 &= 0xF0;
    switch (evnt0)
    {
    case IO_MIDI_EVT0_NOTEOFF:
    case IO_MIDI_EVT0_NOTEON:
    case IO_MIDI_EVT0_AFTERTOUCH:
    case IO_MIDI_EVT0_CONTROLCHANGE:
    case IO_MIDI_EVT0_PROGRAMCHANGE:
    case IO_MIDI_EVT0_CHANNELAFTERTOUCH:
    case IO_MIDI_EVT0_PITCHBEND:
    {
      /*
       *(ptr + 1) &= 0xF0; // clear the channel bits
       *(ptr + 1) |= 0x01; // set the channel to 1 (MIDI channel 2)
       */
      io_midi_transport_push_rx_packet(ptr);
      break;
    }
    default:
      break;
    }

    ptr += 4;
  }
}

/* USER CODE END 0 */

/*
 * user callback declaration
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
 * Init USB host library, add supported class and start the library
 * @retval None
 */
void MX_USB_HOST_Init(void)
{
  /* USER CODE BEGIN USB_HOST_Init_PreTreatment */

  /* USER CODE END USB_HOST_Init_PreTreatment */

  /* Init host Library, add supported class and start the library. */
  if (USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_RegisterClass(&hUsbHostFS, USBH_AUDIO_CLASS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_Start(&hUsbHostFS) != USBH_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_HOST_Init_PostTreatment */

  //*
  if (USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_RegisterClass(&hUsbHostFS, USBH_MIDI_CLASS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_Start(&hUsbHostFS) != USBH_OK)
  {
    Error_Handler();
  }
  //*/

  /* USER CODE END USB_HOST_Init_PostTreatment */
}

/*
 * Background task
 */
void MX_USB_HOST_Process(void)
{
  /* USB Host Background task */
  USBH_Process(&hUsbHostFS);
}
/*
 * user callback definition
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  /* USER CODE BEGIN CALL_BACK_1 */
  switch (id)
  {
  case HOST_USER_SELECT_CONFIGURATION:
    break;

  case HOST_USER_DISCONNECTION:
    // HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);
    Appli_state = APPLICATION_DISCONNECT;
    break;

  case HOST_USER_CLASS_ACTIVE:
    Appli_state = APPLICATION_READY;
    break;

  case HOST_USER_CONNECTION:
    // HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_RESET);
    Appli_state = APPLICATION_START;
    break;

  default:
    break;
  }
  /* USER CODE END CALL_BACK_1 */
}

/**
 * @}
 */

/**
 * @}
 */
