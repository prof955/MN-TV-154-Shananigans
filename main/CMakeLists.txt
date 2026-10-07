idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ""
    REQUIRES display touch audio_dsp system_mon freertos esp_timer esp_lcd driver esp_driver_ledc esp_driver_i2s esp_driver_gpio esp_driver_spi esp_driver_touch_sens esp_wifi nvs_flash
)
