<<<<<<< HEAD
set(COMPONENT_SRCS "app_main.c  gui.c")
set(COMPONENT_ADD_INCLUDEDIRS "")
set(COMPONENT_REQUIRES max98357a esp_timer lvgl lvgl_esp32_drivers ui_app log)
=======
#set(COMPONENT_SRCS "app_main.c  gui.c buttons.c esp_log.c")
#set(COMPONENT_ADD_INCLUDEDIRS "")
>>>>>>> 9b63525 (Dodani joystick i gumbovi)

#register_component()

idf_component_register(
    SRCS "app_main.c" "gui.c"
    INCLUDE_DIRS "."
    REQUIRES buttons lvgl lvgl_esp32_drivers joystick
)