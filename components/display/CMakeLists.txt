idf_component_register(
    SRCS "display.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_driver_ledc esp_driver_gpio esp_driver_spi
)
