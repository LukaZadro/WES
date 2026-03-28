set(COMPONENT_SRCS "max98357a.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES esp_driver_i2s esp_driver_gpio freertos log)

register_component()
