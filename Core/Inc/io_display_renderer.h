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

void io_display_renderer_init(SPI_HandleTypeDef *hspi_neopixel);
void io_display_renderer_set_seq_channel(uint8_t channel);
void io_display_renderer_set_seq_playmode(uint8_t playmode);
void io_display_renderer_render(const domain_display_model_t *model);
void io_display_renderer_flush(void);

#endif
