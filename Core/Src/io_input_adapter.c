/**
 * @file io_input_adapter.c
 * @author k-omura
 * @brief Input adapter implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "io_input_adapter.h"
#include "io_input_driver.h"

#include <string.h>

#define IO_INPUT_QUEUE_CAPACITY (16)  //!< Capacity of the input event queue
#define IO_INPUT_LONG_PRESS_MS (600U) //!< Long press threshold in milliseconds

typedef enum
{
    IO_INPUT_INDEX_RE1 = 0,
    IO_INPUT_INDEX_RE2,
    IO_INPUT_INDEX_COUNT,
} io_input_index_t;

typedef struct
{
    app_event_t queue[IO_INPUT_QUEUE_CAPACITY];
    uint8_t write_pos;
    uint8_t read_pos;
} io_input_adapter_state_t;

static io_input_adapter_state_t g_state;
static uint8_t g_btn_pressed_state[IO_INPUT_INDEX_COUNT];

/**
 * @brief Pushes an event into the input adapter's event queue.
 * @param ev Pointer to the event to push.
 */
static void io_input_adapter_push_event(const app_event_t *ev)
{
    uint8_t next = (uint8_t)((g_state.write_pos + 1U) % IO_INPUT_QUEUE_CAPACITY);
    if (next == g_state.read_pos)
    {
        return;
    }

    g_state.queue[g_state.write_pos] = *ev;
    g_state.write_pos = next;
}

static void io_input_adapter_emit_long_press(uint8_t idx, uint32_t now_tick)
{
    app_event_t ev;
    io_input_driver_btn_t *btn = io_input_driver_get_btn(idx);

    if ((btn == 0) || (btn->state != IO_INPUT_DRIVER_BTN_PUSHING))
    {
        return;
    }

    if ((btn->tick != 0U) && ((now_tick - btn->tick) >= IO_INPUT_LONG_PRESS_MS))
    {
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_LONG_PRESS;
        ev.source = (idx == IO_INPUT_INDEX_RE1) ? APP_INPUT_SOURCE_RE1 : APP_INPUT_SOURCE_RE2;
        ev.code = (uint16_t)idx;
        ev.tick = now_tick;
        io_input_adapter_push_event(&ev);
        btn->tick = 0;
    }
}

/**
 * @brief Initializes the input adapter by clearing the event queue and resetting the read and write positions.
 */
void io_input_adapter_init(void)
{
    uint8_t i;

    memset(&g_state, 0, sizeof(g_state));

    for (i = 0; i < IO_INPUT_INDEX_COUNT; i++)
    {
        g_btn_pressed_state[i] = io_input_driver_btn_check(i);
    }
}

/**
 * @brief Handles a timer event for the input adapter. This function processes rotary encoder and button events.
 * @param now_tick The current tick count.
 */
void io_input_adapter_on_timer(uint32_t now_tick)
{
    app_event_t ev;

    /* Process rotary encoder events */
    for (uint8_t i = 0; i < IO_INPUT_INDEX_COUNT; i++)
    {
        uint8_t pressed = io_input_driver_btn_check(i);

        if (pressed != g_btn_pressed_state[i])
        {
            memset(&ev, 0, sizeof(ev));
            ev.type = (pressed != 0U) ? EV_BUTTON_DOWN : EV_BUTTON_UP;
            ev.source = (i == IO_INPUT_INDEX_RE1) ? APP_INPUT_SOURCE_RE1 : APP_INPUT_SOURCE_RE2;
            ev.code = i;
            ev.tick = now_tick;
            io_input_adapter_push_event(&ev);
            g_btn_pressed_state[i] = pressed;
        }

        io_input_adapter_emit_long_press(i, now_tick);

        io_input_driver_rot_t *rot = io_input_driver_get_rot(i);
        if (rot != NULL && rot->count != 0)
        {
            memset(&ev, 0, sizeof(ev));

            /* Use EV_ROTATE for encoder rotation */
            ev.type = EV_ROTATE;
            ev.source = (i == 0) ? APP_INPUT_SOURCE_RE1 : APP_INPUT_SOURCE_RE2;
            ev.code = i; /* Encoder ID */
            ev.delta = rot->count;
            ev.tick = now_tick;

            /* Push one consolidated event to avoid starving button events. */
            io_input_adapter_push_event(&ev);

            /* Reset count in driver */
            rot->count = 0;
        }
    }

    /* Button events are emitted here after debounce-confirmed state transitions. */
}

/**
 * @brief Polls the input adapter for the next available event.
 * @param out_event Pointer to the event structure to fill with the next event.
 * @return 1 if an event was available and filled, 0 otherwise.
 */
uint8_t io_input_adapter_poll(app_event_t *out_event)
{
    if ((out_event == 0) || (g_state.read_pos == g_state.write_pos))
    {
        return 0;
    }

    *out_event = g_state.queue[g_state.read_pos];
    g_state.read_pos = (uint8_t)((g_state.read_pos + 1U) % IO_INPUT_QUEUE_CAPACITY);
    return 1;
}
