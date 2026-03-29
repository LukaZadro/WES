idf_component_register(
    SRCS "app_main.c" "gui.c" "nav.c"
    INCLUDE_DIRS "."
    REQUIRES buttons joystick max98357a tetris wifi_station mqtt_client_bl
             esp_timer lvgl lvgl_esp32_drivers ui_app log memory
             accelerometer sleep_timer sht31
)
