/**
 * @file io_input_driver.c
 * @author k-omura
 * @brief Input driver implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "io_input_driver.h"
#include "io_display_driver.h"
#include "io_input_adapter.h"

static io_input_driver_btn_t g_btn[2];
static io_input_driver_rot_t g_rot[2];

/**
 * @brief Initializes the input driver, including buttons and rotary encoders.
 * @param led_type The type of LED to initialize in the display driver.
 */
void io_input_driver_init(uint8_t led_type)
{
    /* Initialize button structures */
    g_btn[0].port = GPIOC;
    g_btn[0].pin = RE1_BTN_Pin;
    g_btn[0].state = IO_INPUT_DRIVER_BTN_RELEASE;
    g_btn[0].prev_state = IO_INPUT_DRIVER_BTN_RELEASE;
    g_btn[0].tick = 0;

    g_btn[1].port = GPIOB;
    g_btn[1].pin = RE2_BTN_Pin;
    g_btn[1].state = IO_INPUT_DRIVER_BTN_RELEASE;
    g_btn[1].prev_state = IO_INPUT_DRIVER_BTN_RELEASE;
    g_btn[1].tick = 0;

    /* Initialize rotary encoder structures */
    g_rot[0].port[0] = GPIOC;
    g_rot[0].port[1] = GPIOA;
    g_rot[0].pin[0] = RE1_P1_Pin;
    g_rot[0].pin[1] = RE1_P2_Pin;
    g_rot[0].firstpin = 0;
    g_rot[0].secondpin = 0;
    g_rot[0].tick = 0;
    g_rot[0].count = 0;
    g_rot[0].control = 0;

    g_rot[1].port[0] = GPIOB;
    g_rot[1].port[1] = GPIOB;
    g_rot[1].pin[0] = RE2_P1_Pin;
    g_rot[1].pin[1] = RE2_P2_Pin;
    g_rot[1].firstpin = 0;
    g_rot[1].secondpin = 0;
    g_rot[1].tick = 0;
    g_rot[1].count = 0;
    g_rot[1].control = 1;

    /* Set LED type for neopixel driver */
    io_display_driver_settype(led_type);
}

/**
 * @brief Handles an external interrupt (EXTI) callback for a GPIO pin.
 * @param gpio_pin The GPIO pin that triggered the interrupt.
 */
void io_input_driver_exti_callback(uint16_t gpio_pin)
{
    switch (gpio_pin)
    {
    case RE1_BTN_Pin:
    {
        if (HAL_GPIO_ReadPin(g_btn[0].port, g_btn[0].pin) == GPIO_PIN_RESET)
        {
            if (g_btn[0].state == IO_INPUT_DRIVER_BTN_RELEASE)
            {
                g_btn[0].state = IO_INPUT_DRIVER_BTN_PUSHED;
            }
        }
        else
        {
            if (g_btn[0].state != IO_INPUT_DRIVER_BTN_RELEASE)
            {
                g_btn[0].state = IO_INPUT_DRIVER_BTN_RELEASE;
            }
        }
        g_btn[0].prev_state = g_btn[0].state;
        break;
    }

    case RE1_P1_Pin:
    case RE1_P2_Pin:
    {
        if (g_rot[0].firstpin == 0)
        {
            g_rot[0].firstpin = gpio_pin;
        }
        else
        {
            g_rot[0].secondpin = gpio_pin;
        }
        break;
    }

    case RE2_BTN_Pin:
    {
        if (HAL_GPIO_ReadPin(g_btn[1].port, g_btn[1].pin) == GPIO_PIN_RESET)
        {
            if (g_btn[1].state == IO_INPUT_DRIVER_BTN_RELEASE)
            {
                g_btn[1].state = IO_INPUT_DRIVER_BTN_PUSHED;
            }
        }
        else
        {
            if (g_btn[1].state != IO_INPUT_DRIVER_BTN_RELEASE)
            {
                g_btn[1].state = IO_INPUT_DRIVER_BTN_RELEASE;
            }
        }
        g_btn[1].prev_state = g_btn[1].state;
        break;
    }

    case RE2_P1_Pin:
    case RE2_P2_Pin:
    {
        if (g_rot[1].firstpin == 0)
        {
            g_rot[1].firstpin = gpio_pin;
        }
        else
        {
            g_rot[1].secondpin = gpio_pin;
        }
        break;
    }

    default:
        break;
    }
}

/**
 * @brief Timer callback for the input driver. This function handles button debouncing and rotary encoder state updates.
 */
void io_input_driver_timer_callback(void)
{
    uint8_t i;

    for (i = 0; i < 2; i++)
    {
        /* Button state machine */
        if (HAL_GPIO_ReadPin(g_btn[i].port, g_btn[i].pin) == GPIO_PIN_RESET)
        {
            if (g_btn[i].state == IO_INPUT_DRIVER_BTN_PUSHED)
            {
                g_btn[i].prev_state = g_btn[i].state;
                g_btn[i].state = IO_INPUT_DRIVER_BTN_PUSHING;
                g_btn[i].tick = HAL_GetTick();
            }
        }

        /* Rotary encoder phase detection */
        if ((g_rot[i].firstpin != 0) && (g_rot[i].secondpin != 0))
        {
            GPIO_PinState pin1_state = HAL_GPIO_ReadPin(g_rot[i].port[0], g_rot[i].pin[0]);
            GPIO_PinState pin2_state = HAL_GPIO_ReadPin(g_rot[i].port[1], g_rot[i].pin[1]);

            if ((pin1_state == GPIO_PIN_SET) && (pin1_state == pin2_state))
            {
                if (g_rot[i].firstpin == g_rot[i].pin[0])
                {
                    /* Pin B changed first: CCW rotation */
                    g_rot[i].count--;
                }
                else
                {
                    /* Pin A changed first: CW rotation */
                    g_rot[i].count++;
                }
                g_rot[i].firstpin = 0;
                g_rot[i].secondpin = 0;
            }
        }
    }
}

/**
 * @brief Handles input driver events. This function processes input events and pushes them to the adapter.
 * @param now_tick The current tick count.
 */
void io_input_driver_handle(uint32_t now_tick)
{
    /* Call adapter to process input events and push them to app_runtime */
    /* Button and encoder events are handled by the adapter, not here */
    io_input_adapter_on_timer(now_tick);
}

/**
 * @brief Checks the state of a button for a given encoder.
 * @param encoder_id The ID of the encoder.
 * @return 1 if the button is pressed, 0 otherwise.
 */
uint8_t io_input_driver_btn_check(uint8_t encoder_id)
{
    if (encoder_id >= 2)
    {
        return 0;
    }

    return (HAL_GPIO_ReadPin(g_btn[encoder_id].port, g_btn[encoder_id].pin) == GPIO_PIN_RESET) ? 1 : 0;
}

/**
 * @brief Gets a pointer to the button structure for a given encoder ID.
 * @param id The ID of the encoder.
 * @return Pointer to the button structure, or NULL if the ID is invalid.
 */
io_input_driver_btn_t *io_input_driver_get_btn(uint8_t id)
{
    if (id >= 2)
    {
        return 0;
    }

    return &g_btn[id];
}

/**
 * @brief Gets a pointer to the rotary encoder structure for a given encoder ID.
 * @param id The ID of the encoder.
 * @return Pointer to the rotary encoder structure, or NULL if the ID is invalid.
 */
io_input_driver_rot_t *io_input_driver_get_rot(uint8_t id)
{
    if (id >= 2)
    {
        return 0;
    }

    return &g_rot[id];
}
