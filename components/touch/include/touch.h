#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_PAD_NUM   TOUCH_PAD_NUM9 // GPIO 32 (T9)

typedef enum {
    TOUCH_EVENT_NONE = 0,
    TOUCH_EVENT_TAP,
    TOUCH_EVENT_DOUBLE_TAP,
    TOUCH_EVENT_LONG_PRESS
} touch_event_t;

/**
 * @brief Initialize T9 capacitive touch pad and hardware filter.
 */
esp_err_t touch_init(void);

/**
 * @brief Read current filtered touch raw value.
 */
uint16_t touch_read_raw(void);

/**
 * @brief Poll touch state machine and return detected gesture event.
 */
touch_event_t touch_poll_event(void);

#ifdef __cplusplus
}
#endif

#endif // TOUCH_H
