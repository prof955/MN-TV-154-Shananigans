idf_component_register(
    SRCS "system_mon.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi nvs_flash esp_timer
)
