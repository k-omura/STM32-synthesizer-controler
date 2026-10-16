/**
 * @file io_display_renderer.h
 * @author k-omura
 * @brief Display renderer definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef IO_DISPLAY_RENDERER_H
#define IO_DISPLAY_RENDERER_H

#include <stdint.h>
#include "main.h"
#include "domain_display_model.h"
#include "io_display_driver.h"

/** @brief Selectable items shown in the SETTING display. */
typedef enum
{
    IO_DISPLAY_SETTING_ITEM_ANIMATION = 0,
    IO_DISPLAY_SETTING_ITEM_GOTODFU,
    IO_DISPLAY_SETTING_ITEM_COUNT,
} io_display_setting_item_t;

/** @brief MIDI NOTE animation selected in SETTING. */
typedef enum
{
    IO_DISPLAY_ANIMATION_MODE_1 = 0,
    IO_DISPLAY_ANIMATION_MODE_2,
    IO_DISPLAY_ANIMATION_MODE_COUNT,
} io_display_animation_mode_t;

void io_display_renderer_init(SPI_HandleTypeDef *hspi_neopixel);
void io_display_renderer_set_seq_channel(uint8_t channel);
void io_display_renderer_set_seq_playmode(uint8_t playmode);
/**
 * @brief Updates the selected SETTING item and MIDI NOTE animation mode.
 * @param setting_item Currently selected SETTING item.
 * @param animation_mode Selected MIDI NOTE animation mode.
 */
void io_display_renderer_set_setting_view(io_display_setting_item_t setting_item, io_display_animation_mode_t animation_mode);
/** @brief Clears active note animations and pending display note events. */
void io_display_renderer_reset_note_state(void);
void io_display_renderer_render(const domain_display_model_t *model);
void io_display_renderer_flush(void);

#endif
