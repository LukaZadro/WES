set(COMPONENT_SRCS "app_main.c  gui.c")
set(COMPONENT_ADD_INCLUDEDIRS "")
set(COMPONENT_REQUIRES max98357a wifi_station mqtt_client_bl esp_timer lvgl lvgl_esp32_drivers ui_app log)

register_component()
