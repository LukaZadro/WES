idf_component_register(
    SRCS "accelerometer.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer log
)
