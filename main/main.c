#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "display.h"
#include "touch.h"
#include "audio_dsp.h"
#include "system_mon.h"

static const char *TAG = "main";

typedef enum {
    UI_MODE_SPECTRUM = 0,
    UI_MODE_3D_GRAPHICS,
    UI_MODE_SYSTEM_DASHBOARD,
    UI_MODE_TOUCH_OSCILLOSCOPE,
    UI_MODE_MAX
} ui_mode_t;

static ui_mode_t current_ui_mode = UI_MODE_SPECTRUM;
static SemaphoreHandle_t data_mutex = NULL;

static spectrum_data_t shared_spectrum = {0};
static system_stats_t shared_stats = {0};

// 3D Cube & Particle State Definitions
typedef struct {
    float x, y, z;
} point3d_t;

typedef struct {
    float x, y, vx, vy;
    uint16_t color;
    uint8_t life;
} particle_t;

#define NUM_PARTICLES 30
static particle_t particles[NUM_PARTICLES];

static point3d_t cube_vertices[8] = {
    {-25, -25, -25}, { 25, -25, -25}, { 25,  25, -25}, {-25,  25, -25},
    {-25, -25,  25}, { 25, -25,  25}, { 25,  25,  25}, {-25,  25,  25}
};

static int cube_edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

static float angle_x = 0.0f;
static float angle_y = 0.0f;

static void reset_particles(void) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = 120.0f;
        particles[i].y = 120.0f;
        float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
        float speed = 1.0f + ((float)rand() / RAND_MAX) * 3.0f;
        particles[i].vx = cosf(angle) * speed;
        particles[i].vy = sinf(angle) * speed;
        particles[i].life = 20 + rand() % 40;
        particles[i].color = (rand() % 2 == 0) ? COLOR_RETRO_SYNTHPINK : COLOR_RETRO_CYAN;
    }
}

// -----------------------------------------------------------------------------
// CORE 0 TASK: Audio DSP processing, system statistics, Wi-Fi scan trigger
// -----------------------------------------------------------------------------
static void audio_sys_task(void *pvParameters)
{
    ESP_LOGI(TAG, "audio_sys_task running on Core %d", xPortGetCoreID());

    int scan_counter = 0;

    while (1) {
        spectrum_data_t spec_local;
        audio_dsp_process(&spec_local);

        scan_counter++;
        if (scan_counter >= 150) { // Trigger Wi-Fi AP scan every ~3 seconds
            scan_counter = 0;
            system_mon_trigger_wifi_scan();
        }

        system_stats_t stats_local;
        system_mon_update(&stats_local);

        if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            shared_spectrum = spec_local;
            shared_stats = stats_local;
            xSemaphoreGive(data_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

// -----------------------------------------------------------------------------
// UI RENDERERS
// -----------------------------------------------------------------------------

// Mode 0: 16-Band Vintage Equalizer
static void render_spectrum_mode(const spectrum_data_t *spec)
{
    display_clear(COLOR_RETRO_DARKBG);

    // Title Bar
    display_draw_string(10, 8, "16-BAND VINTAGE HI-FI EQ", COLOR_RETRO_GOLD, COLOR_RETRO_DARKBG, true);
    display_draw_line(10, 18, 230, 18, COLOR_RETRO_GOLD);

    // Equalizer Grid & Bars
    int bar_width = 11;
    int gap = 3;
    int start_x = 10;
    int base_y = 210;
    int max_bar_h = 160;

    for (int i = 0; i < SPECTRUM_BANDS; i++) {
        int x = start_x + i * (bar_width + gap);
        int bar_h = (int)(spec->band_values[i] * max_bar_h);
        int peak_y = base_y - (int)(spec->band_peaks[i] * max_bar_h);

        // Gradient coloring: Mint -> Gold -> Brick Red
        for (int h = 0; h < bar_h; h += 3) {
            int current_y = base_y - h;
            uint16_t color = COLOR_RETRO_MINT;
            if (h > max_bar_h * 0.7f) {
                color = COLOR_RETRO_BRICKRED;
            } else if (h > max_bar_h * 0.4f) {
                color = COLOR_RETRO_GOLD;
            }
            display_fill_rect(x, current_y - 2, bar_width, 2, color);
        }

        // Draw Peak Hold Indicator Line
        if (peak_y < base_y) {
            display_draw_line(x, peak_y, x + bar_width - 1, peak_y, COLOR_RETRO_BRICKRED);
        }
    }

    // Status Footer
    char buf[32];
    snprintf(buf, sizeof(buf), "RMS:%2d%%  CLAP:%s", (int)spec->rms_volume, spec->clap_detected ? "DETECT!" : "NONE   ");
    display_draw_string(10, 222, buf, spec->clap_detected ? COLOR_RETRO_BRICKRED : COLOR_RETRO_AMBER, COLOR_RETRO_DARKBG, true);
}

// Mode 1: Synthwave 3D Wireframe Cube & Particles
static void render_3d_graphics_mode(void)
{
    display_clear(COLOR_RETRO_DARKBG);

    display_draw_string(24, 8, "SYNTHWAVE 3D SIMULATION", COLOR_RETRO_SYNTHPINK, COLOR_RETRO_DARKBG, true);
    display_draw_line(24, 18, 216, 18, COLOR_RETRO_CYAN);

    // 1. Particle Simulation
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].life--;

        if (particles[i].life <= 0 || particles[i].x < 0 || particles[i].x > 240 || particles[i].y < 0 || particles[i].y > 240) {
            particles[i].x = 120.0f;
            particles[i].y = 120.0f;
            float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
            float speed = 1.0f + ((float)rand() / RAND_MAX) * 3.0f;
            particles[i].vx = cosf(angle) * speed;
            particles[i].vy = sinf(angle) * speed;
            particles[i].life = 20 + rand() % 40;
        }

        display_draw_pixel((int)particles[i].x, (int)particles[i].y, particles[i].color);
    }

    // 2. 3D Cube Matrix Projections
    angle_x += 0.04f;
    angle_y += 0.05f;

    point3d_t proj[8];
    for (int i = 0; i < 8; i++) {
        // Rotate around X
        float y1 = cube_vertices[i].y * cosf(angle_x) - cube_vertices[i].z * sinf(angle_x);
        float z1 = cube_vertices[i].y * sinf(angle_x) + cube_vertices[i].z * cosf(angle_x);
        // Rotate around Y
        float x2 = cube_vertices[i].x * cosf(angle_y) + z1 * sinf(angle_y);
        float z2 = -cube_vertices[i].x * sinf(angle_y) + z1 * cosf(angle_y);

        // Perspective projection
        float distance = 120.0f;
        float fov = 150.0f / (distance + z2);

        proj[i].x = 120.0f + x2 * fov;
        proj[i].y = 120.0f + y1 * fov;
    }

    for (int i = 0; i < 12; i++) {
        int p1 = cube_edges[i][0];
        int p2 = cube_edges[i][1];
        display_draw_line((int)proj[p1].x, (int)proj[p1].y, (int)proj[p2].x, (int)proj[p2].y, COLOR_RETRO_CYAN);
    }
}

// Mode 2: Retro CRT Terminal System Dashboard
static void render_system_dashboard_mode(const system_stats_t *stats)
{
    display_clear(COLOR_RETRO_DARKBG);

    // CRT Screen Frame
    display_draw_rect(4, 4, 232, 232, COLOR_RETRO_CRT_GREEN);
    display_draw_string(16, 12, "CRT TERMINAL DASHBOARD", COLOR_RETRO_CRT_GREEN, COLOR_RETRO_DARKBG, true);
    display_draw_line(12, 22, 228, 22, COLOR_RETRO_CRT_GREEN);

    char buf[40];
    snprintf(buf, sizeof(buf), "CORE 0 CPU: %5.1f%%", stats->cpu_core0_load);
    display_draw_string(12, 32, buf, COLOR_RETRO_AMBER, COLOR_RETRO_DARKBG, true);

    snprintf(buf, sizeof(buf), "CORE 1 CPU: %5.1f%%", stats->cpu_core1_load);
    display_draw_string(12, 44, buf, COLOR_RETRO_AMBER, COLOR_RETRO_DARKBG, true);

    snprintf(buf, sizeof(buf), "FREE SRAM : %ld B", stats->free_heap);
    display_draw_string(12, 56, buf, COLOR_RETRO_MINT, COLOR_RETRO_DARKBG, true);

    snprintf(buf, sizeof(buf), "MIN HEAP  : %ld B", stats->min_free_heap);
    display_draw_string(12, 68, buf, COLOR_RETRO_MINT, COLOR_RETRO_DARKBG, true);

    snprintf(buf, sizeof(buf), "CHIP TEMP : %.1f C", stats->chip_temp_c);
    display_draw_string(12, 80, buf, COLOR_RETRO_GOLD, COLOR_RETRO_DARKBG, true);

    snprintf(buf, sizeof(buf), "NVS STATUS: %s", stats->nvs_ok ? "OK" : "ERROR");
    display_draw_string(12, 92, buf, stats->nvs_ok ? COLOR_RETRO_MINT : COLOR_RETRO_BRICKRED, COLOR_RETRO_DARKBG, true);

    display_draw_line(12, 104, 228, 104, COLOR_RETRO_CRT_GREEN);
    display_draw_string(12, 110, "LIVE WI-FI ACCESS POINTS:", COLOR_RETRO_CYAN, COLOR_RETRO_DARKBG, true);

    for (int i = 0; i < stats->wifi_ap_count && i < 5; i++) {
        snprintf(buf, sizeof(buf), "%-14.14s %d dBm", stats->wifi_aps[i].ssid, stats->wifi_aps[i].rssi);
        display_draw_string(12, 124 + i * 12, buf, COLOR_RETRO_WHITE, COLOR_RETRO_DARKBG, true);
    }
    if (stats->wifi_ap_count == 0) {
        display_draw_string(12, 124, "Scanning networks...", COLOR_RETRO_GOLD, COLOR_RETRO_DARKBG, true);
    }
}

// Mode 3: T9 Amber CRT Touch Oscilloscope
static void render_touch_oscilloscope_mode(void)
{
    static float scope_buffer[240] = {0};
    uint16_t raw_touch = touch_read_raw();

    // Shift wave buffer left
    for (int i = 0; i < 239; i++) {
        scope_buffer[i] = scope_buffer[i + 1];
    }
    scope_buffer[239] = (float)raw_touch;

    display_clear(COLOR_RETRO_DARKBG);

    // Scope Header
    display_draw_string(16, 8, "T9 AMBER TOUCH SCOPE", COLOR_RETRO_AMBER, COLOR_RETRO_DARKBG, true);
    display_draw_line(10, 18, 230, 18, COLOR_RETRO_AMBER);

    // Oscilloscope Grid Lines
    for (int x = 20; x < 240; x += 40) {
        for (int y = 30; y < 210; y += 8) {
            display_draw_pixel(x, y, COLOR_RETRO_PURPLE);
        }
    }
    for (int y = 30; y <= 210; y += 30) {
        for (int x = 10; x < 230; x += 8) {
            display_draw_pixel(x, y, COLOR_RETRO_PURPLE);
        }
    }

    // Plot Waveform
    for (int x = 10; x < 229; x++) {
        int y1 = 180 - (int)((scope_buffer[x] / 3000.0f) * 120.0f);
        int y2 = 180 - (int)((scope_buffer[x + 1] / 3000.0f) * 120.0f);

        if (y1 < 25) {
            y1 = 25;
        }
        if (y1 > 210) {
            y1 = 210;
        }
        if (y2 < 25) {
            y2 = 25;
        }
        if (y2 > 210) {
            y2 = 210;
        }

        display_draw_line(x, y1, x + 1, y2, COLOR_RETRO_AMBER);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "RAW T9: %4d  BL: %d%%", raw_touch, display_get_brightness());
    display_draw_string(10, 222, buf, COLOR_RETRO_GOLD, COLOR_RETRO_DARKBG, true);
}

// -----------------------------------------------------------------------------
// CORE 1 TASK: Touch event handler & UI Render Loop
// -----------------------------------------------------------------------------
static void ui_render_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ui_render_task running on Core %d", xPortGetCoreID());

    int64_t last_time = esp_timer_get_time();
    int frame_count = 0;
    int current_fps = 60;

    reset_particles();

    while (1) {
        // 1. Touch Gesture Polling
        touch_event_t event = touch_poll_event();
        if (event == TOUCH_EVENT_TAP) {
            current_ui_mode = (current_ui_mode + 1) % UI_MODE_MAX;
            ESP_LOGI(TAG, "UI Mode switched to %d", current_ui_mode);
        } else if (event == TOUCH_EVENT_DOUBLE_TAP) {
            if (current_ui_mode == UI_MODE_SPECTRUM) {
                audio_dsp_reset_peaks();
                ESP_LOGI(TAG, "Spectrum peaks reset!");
            } else if (current_ui_mode == UI_MODE_3D_GRAPHICS) {
                reset_particles();
                ESP_LOGI(TAG, "Particles reset!");
            }
        } else if (event == TOUCH_EVENT_LONG_PRESS) {
            display_toggle_brightness();
            ESP_LOGI(TAG, "Backlight toggled to %d%%", display_get_brightness());
        }

        // 2. Fetch Thread-safe Shared Data
        spectrum_data_t spec_copy;
        system_stats_t stats_copy;

        if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            spec_copy = shared_spectrum;
            stats_copy = shared_stats;
            xSemaphoreGive(data_mutex);
        }

        // 3. Render Active Mode
        switch (current_ui_mode) {
            case UI_MODE_SPECTRUM:
                render_spectrum_mode(&spec_copy);
                break;
            case UI_MODE_3D_GRAPHICS:
                render_3d_graphics_mode();
                break;
            case UI_MODE_SYSTEM_DASHBOARD:
                render_system_dashboard_mode(&stats_copy);
                break;
            case UI_MODE_TOUCH_OSCILLOSCOPE:
                render_touch_oscilloscope_mode();
                break;
            default:
                break;
        }

        // Render FPS Badge at top right
        char fps_buf[16];
        snprintf(fps_buf, sizeof(fps_buf), "%d FPS", current_fps);
        display_draw_string(180, 8, fps_buf, COLOR_RETRO_GOLD, COLOR_RETRO_DARKBG, true);

        // 4. Flush to ST7789 LCD via SPI DMA
        display_flush();

        // 5. FPS Counter Calculation
        frame_count++;
        int64_t now = esp_timer_get_time();
        if (now - last_time >= 1000000) {
            current_fps = frame_count;
            frame_count = 0;
            last_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS target
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 Hardware Showcase Application...");

    data_mutex = xSemaphoreCreateMutex();
    assert(data_mutex != NULL);

    // Hardware Initializations
    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(touch_init());
    ESP_ERROR_CHECK(audio_dsp_init());
    ESP_ERROR_CHECK(system_mon_init());

    // Dual-core FreeRTOS task pinning
    xTaskCreatePinnedToCore(audio_sys_task, "audio_sys_task", 8192, NULL, 5, NULL, 0); // Core 0
    xTaskCreatePinnedToCore(ui_render_task, "ui_render_task", 8192, NULL, 5, NULL, 1); // Core 1
}
