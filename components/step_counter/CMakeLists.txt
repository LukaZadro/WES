idf_component_register(
    SRCS "step_counter.c"
    INCLUDE_DIRS "."
    REQUIRES accelerometer log freertos
)

target_link_libraries(${COMPONENT_LIB} PRIVATE m)
