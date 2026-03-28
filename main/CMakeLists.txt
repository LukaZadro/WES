idf_component_register(
    SRCS "app_main.c" "gui.c"
    INCLUDE_DIRS 
        "."
        "../components/ui_app/squareline/project"
    REQUIRES buttons lvgl lvgl_esp32_drivers joystick ui_app
)