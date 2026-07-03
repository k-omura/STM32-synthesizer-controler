/**
 * @file io_display_driver.h
 * @author k-omura
 * @brief Display driver definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef IO_DISPLAY_DRIVER_H
#define IO_DISPLAY_DRIVER_H

#include <stdint.h>
#include "main.h"

#define IO_DISPLAY_DRIVER_WIDTH 32                                                    //!< Width of the display in pixels
#define IO_DISPLAY_DRIVER_HEIGHT 8                                                    //!< Height of the display in pixels
#define IO_DISPLAY_DRIVER_PIXELS (IO_DISPLAY_DRIVER_WIDTH * IO_DISPLAY_DRIVER_HEIGHT) //!< Total number of pixels in the display

typedef struct
{
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} io_display_driver_color_t;

typedef enum
{
    IO_DISPLAY_DRIVER_ICON_PLAY,
    IO_DISPLAY_DRIVER_ICON_STOP,
    IO_DISPLAY_DRIVER_ICON_RECORD,
} io_display_driver_icon_t;

void io_display_driver_init(SPI_HandleTypeDef *hspi, uint8_t led_type);
void io_display_driver_flush(void);
void io_display_driver_settype(uint8_t type);

void io_display_driver_clear(io_display_driver_color_t *color);
void io_display_driver_pixel(int16_t x, int16_t y, io_display_driver_color_t *color);
void io_display_driver_pixel_random(int16_t x, int16_t y);
void io_display_driver_xbar(int16_t x, int16_t y, uint8_t length, io_display_driver_color_t *color);
void io_display_driver_ybar(int16_t x, int16_t y, uint8_t length, io_display_driver_color_t *color);
void io_display_driver_mosaic(void);
void io_display_driver_logo(uint8_t count, io_display_driver_color_t *color);
void io_display_driver_icon(io_display_driver_icon_t icon, io_display_driver_color_t *color);
void io_display_driver_string(int16_t x, int16_t y, const char text[], uint8_t len, io_display_driver_color_t *color, uint8_t scale);

#endif
