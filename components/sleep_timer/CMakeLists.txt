idf_component_register(
    SRCS "sleep_timer.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer esp_hw_support log lvgl
)
