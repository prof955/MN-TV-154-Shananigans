#include "touch.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/touch_pad.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "touch";

static uint16_t touch_baseline = 0;
#define TOUCH_THRESHOLD_DELTA 150

static int64_t press_start_time = 0;
static int64_t last_tap_time = 0;
static bool is_pressed = false;
static bool long_press_handled = false;

esp_err_t touch_init(void)
{
    ESP_LOGI(TAG, "Initializing T9 Capacitive Touch Pad (GPIO 32)...");

    ESP_ERROR_CHECK(touch_pad_init());
    ESP_ERROR_CHECK(touch_pad_set_voltage(TOUCH_HVOLT_2_7V, TOUCH_LVOLT_0_5V, TOUCH_HVOLT_ATTEN_1V));
    ESP_ERROR_CHECK(touch_pad_config(TOUCH_PAD_NUM, 0));
    ESP_ERROR_CHECK(touch_pad_filter_start(10));

    // Allow filter to stabilize and read baseline
    vTaskDelay(pdMS_TO_TICKS(100));
    touch_pad_read_filtered(TOUCH_PAD_NUM, &touch_baseline);
    ESP_LOGI(TAG, "Touch baseline value: %d", touch_baseline);

    return ESP_OK;
}

uint16_t touch_read_raw(void)
{
    uint16_t val = 0;
    touch_pad_read_filtered(TOUCH_PAD_NUM, &val);
    return val;
}

touch_event_t touch_poll_event(void)
{
    uint16_t current_val = 0;
    touch_pad_read_filtered(TOUCH_PAD_NUM, &current_val);

    int64_t now = esp_timer_get_time() / 1000; // time in ms
    bool currently_touched = (touch_baseline > current_val) && ((touch_baseline - current_val) > TOUCH_THRESHOLD_DELTA);

    touch_event_t event = TOUCH_EVENT_NONE;

    if (currently_touched && !is_pressed) {
        // Touch down
        is_pressed = true;
        press_start_time = now;
        long_press_handled = false;
    } else if (currently_touched && is_pressed) {
        // Touch held
        if (!long_press_handled && (now - press_start_time) >= 800) {
            long_press_handled = true;
            event = TOUCH_EVENT_LONG_PRESS;
        }
    } else if (!currently_touched && is_pressed) {
        // Touch released
        is_pressed = false;
        int64_t duration = now - press_start_time;

        if (!long_press_handled && duration < 800) {
            if ((now - last_tap_time) < 350) {
                event = TOUCH_EVENT_DOUBLE_TAP;
                last_tap_time = 0;
            } else {
                last_tap_time = now;
                event = TOUCH_EVENT_TAP;
            }
        }
    }

    return event;
}
