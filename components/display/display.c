#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

static const char *TAG = "display";

static esp_lcd_panel_handle_t panel_handle = NULL;
static uint16_t *frame_buffer = NULL;
static uint8_t current_brightness = 100;

// Built-in 8x8 ASCII Font Bitmap (ASCII 32 to 126)
static const uint8_t font8x8_basic[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // 35 '#'
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // 36 '$'
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // 37 '%'
    {0x1C,0x36,0x1C,0x3A,0x6E,0x33,0x6F,0x00}, // 38 '&'
    {0x06,0x06,0x04,0x00,0x00,0x00,0x00,0x00}, // 39 '\''
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 '('
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 '*'
    {0x00,0x0C,0x0C,0x3E,0x0C,0x0C,0x00,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x18}, // 44 ','
    {0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 '.'
    {0x00,0x03,0x06,0x0C,0x18,0x30,0x60,0x00}, // 47 '/'
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // 48 '0'
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // 49 '1'
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, // 50 '2'
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, // 51 '3'
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, // 52 '4'
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, // 53 '5'
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, // 54 '6'
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // 55 '7'
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // 56 '8'
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, // 57 '9'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // 58 ':'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // 59 ';'
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 '<'
    {0x00,0x00,0x3E,0x00,0x3E,0x00,0x00,0x00}, // 61 '='
    {0x30,0x18,0x0C,0x03,0x0C,0x18,0x30,0x00}, // 62 '>'
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00}, // 63 '?'
    {0x3E,0x63,0x6F,0x6B,0x6F,0x03,0x3E,0x00}, // 64 '@'
    {0x1C,0x36,0x63,0x63,0x7F,0x63,0x63,0x00}, // 65 'A'
    {0x3E,0x66,0x66,0x3E,0x66,0x66,0x3E,0x00}, // 66 'B'
    {0x1E,0x33,0x03,0x03,0x03,0x33,0x1E,0x00}, // 67 'C'
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, // 68 'D'
    {0x3F,0x03,0x03,0x1F,0x03,0x03,0x3F,0x00}, // 69 'E'
    {0x3F,0x03,0x03,0x1F,0x03,0x03,0x03,0x00}, // 70 'F'
    {0x1E,0x33,0x03,0x3B,0x63,0x33,0x3E,0x00}, // 71 'G'
    {0x63,0x63,0x63,0x7F,0x63,0x63,0x63,0x00}, // 72 'H'
    {0x1C,0x08,0x08,0x08,0x08,0x08,0x1C,0x00}, // 73 'I'
    {0x38,0x10,0x10,0x10,0x10,0x11,0x0E,0x00}, // 74 'J'
    {0x63,0x33,0x1B,0x0F,0x1B,0x33,0x63,0x00}, // 75 'K'
    {0x03,0x03,0x03,0x03,0x03,0x03,0x3F,0x00}, // 76 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 77 'M'
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, // 78 'N'
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, // 79 'O'
    {0x3E,0x63,0x63,0x3E,0x03,0x03,0x03,0x00}, // 80 'P'
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, // 81 'Q'
    {0x3E,0x63,0x63,0x3E,0x1B,0x33,0x63,0x00}, // 82 'R'
    {0x1E,0x33,0x07,0x0E,0x30,0x33,0x1E,0x00}, // 83 'S'
    {0x3F,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00}, // 84 'T'
    {0x63,0x63,0x63,0x63,0x63,0x63,0x3E,0x00}, // 85 'U'
    {0x63,0x63,0x63,0x36,0x1C,0x1C,0x08,0x00}, // 86 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 'W'
    {0x63,0x63,0x36,0x1C,0x36,0x63,0x63,0x00}, // 88 'X'
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x0C,0x00}, // 89 'Y'
    {0x3F,0x30,0x18,0x0C,0x06,0x03,0x3F,0x00}, // 90 'Z'
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 91 '['
    {0x00,0x60,0x30,0x18,0x0C,0x06,0x03,0x00}, // 92 '\'
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 93 ']'
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // 95 '_'
};

static inline uint16_t swap_bytes(uint16_t val) {
    return (val >> 8) | (val << 8);
}

esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Initializing ST7789 LCD Display...");

    // 1. LCD Power Control Pin (GPIO 21, Active LOW)
    gpio_config_t pwr_cfg = {
        .pin_bit_mask = (1ULL << PIN_NUM_LCD_PWR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pwr_cfg);
    gpio_set_level(PIN_NUM_LCD_PWR, 0); // Active LOW to power on LCD

    // 2. LEDC PWM Backlight Setup (GPIO 19) - Full 100% Brightness Initial
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_NUM_LCD_BL,
        .duty           = 255, // Max initial 100% duty cycle
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Force LEDC update for 100% initial brightness
    display_set_brightness(100);

    // 3. SPI Bus Setup
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 4. Panel IO Configuration
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 40 * 1000 * 1000, // 40MHz SPI clock
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    // 5. ST7789 Panel Driver Setup
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 6. Allocate Frame Buffer in DMA memory
    frame_buffer = (uint16_t *)heap_caps_malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!frame_buffer) {
        ESP_LOGE(TAG, "Failed to allocate DMA frame buffer!");
        return ESP_ERR_NO_MEM;
    }

    display_clear(COLOR_RETRO_DARKBG);
    display_flush();

    ESP_LOGI(TAG, "Display initialization complete at 100%% brightness.");
    return ESP_OK;
}

void display_set_brightness(uint8_t percentage)
{
    if (percentage > 100) percentage = 100;
    current_brightness = percentage;
    uint32_t duty = (percentage * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void display_toggle_brightness(void)
{
    if (current_brightness > 50) {
        display_set_brightness(30);
    } else {
        display_set_brightness(100);
    }
}

uint8_t display_get_brightness(void)
{
    return current_brightness;
}

uint16_t *display_get_frame_buffer(void)
{
    return frame_buffer;
}

esp_err_t display_flush(void)
{
    if (!panel_handle || !frame_buffer) return ESP_ERR_INVALID_STATE;
    return esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, frame_buffer);
}

void display_clear(uint16_t color)
{
    if (!frame_buffer) return;
    uint16_t swapped = swap_bytes(color);
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        frame_buffer[i] = swapped;
    }
}

void display_draw_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT || !frame_buffer) return;
    frame_buffer[y * DISPLAY_WIDTH + x] = swap_bytes(color);
}

void display_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        display_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void display_draw_rect(int x, int y, int w, int h, uint16_t color)
{
    display_draw_line(x, y, x + w - 1, y, color);
    display_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    display_draw_line(x, y, x, y + h - 1, color);
    display_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void display_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            display_draw_pixel(j, i, color);
        }
    }
}

void display_draw_circle(int x0, int y0, int r, uint16_t color)
{
    int x = r;
    int y = 0;
    int err = 0;

    while (x >= y) {
        display_draw_pixel(x0 + x, y0 + y, color);
        display_draw_pixel(x0 + y, y0 + x, color);
        display_draw_pixel(x0 - y, y0 + x, color);
        display_draw_pixel(x0 - x, y0 + y, color);
        display_draw_pixel(x0 - x, y0 - y, color);
        display_draw_pixel(x0 - y, y0 - x, color);
        display_draw_pixel(x0 + y, y0 - x, color);
        display_draw_pixel(x0 + x, y0 - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void display_draw_char(int x, int y, char c, uint16_t fg_color, uint16_t bg_color, bool transparent)
{
    if (c < 32 || c > 126) c = '?';
    const uint8_t *bitmap = font8x8_basic[c - 32];

    for (int row = 0; row < 8; row++) {
        uint8_t line = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (line & (1 << col)) {
                display_draw_pixel(x + col, y + row, fg_color);
            } else if (!transparent) {
                display_draw_pixel(x + col, y + row, bg_color);
            }
        }
    }
}

void display_draw_string(int x, int y, const char *str, uint16_t fg_color, uint16_t bg_color, bool transparent)
{
    int curr_x = x;
    while (*str) {
        if (*str == '\n') {
            curr_x = x;
            y += 8;
        } else {
            display_draw_char(curr_x, y, *str, fg_color, bg_color, transparent);
            curr_x += 8;
        }
        str++;
    }
}
