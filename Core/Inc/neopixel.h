/*
 * neopixel.h
 *
 *  Created on: 2021/03/13
 *      Author: k-omura
 */

#include "main.h"
#include "stdio.h"
#include "stdint.h"

#define NEOPIXEL_HEIGHT 8
#define NEOPIXEL_WIDTH 32
#define NEOPIXEL_PIXELS (NEOPIXEL_HEIGHT * NEOPIXEL_WIDTH)
#define NEOPIXEL_PIXELS_RAW (12 * NEOPIXEL_PIXELS + 1)

typedef struct{
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} fb_color24_t;

typedef enum{
    SEQ_ICON_PLAY,
    SEQ_ICON_STOP,
    SEQ_ICON_RECORD,
    SEQ_ICON_COUNT
} seq_icon_t;

void neopixel_flush(SPI_HandleTypeDef *hspi);
void neopixel_settype(uint8_t);

void neopixel_fb_clear(fb_color24_t*);
void neopixel_fb_pixel(int16_t, int16_t, fb_color24_t*);
void neopixel_fb_pixel_rand(int16_t, int16_t);
void neopixel_fb_xbar(int16_t, int16_t, uint8_t, fb_color24_t*);
void neopixel_fb_ybar(int16_t, int16_t, uint8_t, fb_color24_t*);
void neopixel_fb_mosaic();
void neopixel_fb_logo(uint8_t, fb_color24_t*);
void neopixel_fb_icon(seq_icon_t, fb_color24_t*);
void neopixel_fb_stringBitmap(int16_t, int16_t, const char _character[], uint8_t, fb_color24_t*, uint8_t);
void neopixel_fb_characterBitmap8(int16_t*, int16_t, char, fb_color24_t*, uint8_t);
void neopixel_fb_characterBitmap5(int16_t*, int16_t, char, fb_color24_t*);

//---------------------------------
extern const unsigned char FONT8x8[][97][8];
extern const unsigned char FONT5x3[][2];
extern const unsigned char OTAKURO_LOGO[][32];
extern const unsigned char seq_icon[][8];