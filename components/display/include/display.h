#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screen dimensions
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240

// ST7789 Pin Mapping
#define PIN_NUM_MOSI      13
#define PIN_NUM_SCLK      14
#define PIN_NUM_CS        15
#define PIN_NUM_DC        2
#define PIN_NUM_RST       (-1)
#define PIN_NUM_LCD_PWR   21
#define PIN_NUM_LCD_BL    19

// Retro RGB565 Palette Colors
#define COLOR_RETRO_DARKBG    0x10A2 // Deep Charcoal Dark Gray
#define COLOR_RETRO_AMBER     0xFD20 // Warm Amber CRT Phosphor
#define COLOR_RETRO_GOLD      0xFEA0 // Vintage Hi-Fi Gold
#define COLOR_RETRO_BRICKRED  0xD986 // Vintage Equalizer Peak Red
#define COLOR_RETRO_SYNTHPINK 0xF971 // Synthwave Neon Pink
#define COLOR_RETRO_CYAN      0x2F7B // Vaporwave Neon Cyan
#define COLOR_RETRO_CRT_GREEN 0x2704 // Phosphor CRT Green
#define COLOR_RETRO_MINT      0x7FE1 // Mint Green Accent
#define COLOR_RETRO_PURPLE    0x5013 // Deep Retro Violet
#define COLOR_RETRO_BLACK     0x0000
#define COLOR_RETRO_WHITE     0xFFFF

/**
 * @brief Initialize ST7789 display, SPI bus, LCD power, and PWM backlight.
 */
esp_err_t display_init(void);

/**
 * @brief Set screen backlight brightness (0 to 100 %).
 */
void display_set_brightness(uint8_t percentage);

/**
 * @brief Toggle backlight between 100% and 30%.
 */
void display_toggle_brightness(void);

/**
 * @brief Get current brightness level (100 or 30).
 */
uint8_t display_get_brightness(void);

/**
 * @brief Get reference to frame buffer allocated in DMA-capable memory.
 */
uint16_t *display_get_frame_buffer(void);

/**
 * @brief Flush frame buffer to LCD display via SPI DMA.
 */
esp_err_t display_flush(void);

/**
 * @brief Fill entire frame buffer with a single color.
 */
void display_clear(uint16_t color);

/**
 * @brief Draw a single pixel in frame buffer.
 */
void display_draw_pixel(int x, int y, uint16_t color);

/**
 * @brief Draw a straight line using Bresenham algorithm.
 */
void display_draw_line(int x0, int y0, int x1, int y1, uint16_t color);

/**
 * @brief Draw a rectangle outline.
 */
void display_draw_rect(int x, int y, int w, int h, uint16_t color);

/**
 * @brief Draw a filled rectangle.
 */
void display_fill_rect(int x, int y, int w, int h, uint16_t color);

/**
 * @brief Draw a circle outline.
 */
void display_draw_circle(int x0, int y0, int r, uint16_t color);

/**
 * @brief Draw a single character using built-in 8x8 font.
 */
void display_draw_char(int x, int y, char c, uint16_t fg_color, uint16_t bg_color, bool transparent);

/**
 * @brief Draw a string using built-in 8x8 font.
 */
void display_draw_string(int x, int y, const char *str, uint16_t fg_color, uint16_t bg_color, bool transparent);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
