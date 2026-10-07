#ifndef SYSTEM_MON_H
#define SYSTEM_MON_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_WIFI_RESULTS 5

typedef struct {
    char ssid[33];
    int8_t rssi;
    uint8_t authmode;
} wifi_ap_info_t;

typedef struct {
    uint32_t free_heap;
    uint32_t min_free_heap;
    float cpu_core0_load;
    float cpu_core1_load;
    float chip_temp_c;
    bool nvs_ok;
    uint16_t wifi_ap_count;
    wifi_ap_info_t wifi_aps[MAX_WIFI_RESULTS];
} system_stats_t;

/**
 * @brief Initialize NVS storage and Wi-Fi scanner subsystem.
 */
esp_err_t system_mon_init(void);

/**
 * @brief Update system statistics (Heap, CPU loads, Wi-Fi AP scan).
 */
void system_mon_update(system_stats_t *out_stats);

/**
 * @brief Trigger an asynchronous Wi-Fi AP scan.
 */
void system_mon_trigger_wifi_scan(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_MON_H
