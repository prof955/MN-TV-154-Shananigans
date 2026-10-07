idf_component_register(
    SRCS "touch.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_touch_sens freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-error=cpp")
