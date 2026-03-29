set(SQUARELINE_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/squareline)
set(SQUARELINE_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR}/squareline/project)

if(EXISTS "${SQUARELINE_ROOT_DIR}/ui.c")
    set(SQUARELINE_DIR ${SQUARELINE_ROOT_DIR})
elseif(EXISTS "${SQUARELINE_PROJECT_DIR}/ui.c")
    set(SQUARELINE_DIR ${SQUARELINE_PROJECT_DIR})
else()
    message(FATAL_ERROR "ui_app: Could not find SquareLine sources (ui.c) in squareline or squareline/project")
endif()

message(STATUS "ui_app: Using SquareLine sources from ${SQUARELINE_DIR}")

file(GLOB_RECURSE SQUARELINE_SRCS "${SQUARELINE_DIR}/*.c")
list(FILTER SQUARELINE_SRCS EXCLUDE REGEX ".*/(backup|cache)/.*\\.c$")

file(GLOB_RECURSE SQUARELINE_HDRS "${SQUARELINE_DIR}/*.h")
list(FILTER SQUARELINE_HDRS EXCLUDE REGEX ".*/(backup|cache)/.*\\.h$")

set(INCLUDE_DIRS "")
foreach(_hdr ${SQUARELINE_HDRS})
    get_filename_component(_dir ${_hdr} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

idf_component_register(
    SRCS "ui_app.c" "tetris_ui_draw.c" "memory_ui_draw.c" ${SQUARELINE_SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES lvgl lvgl_esp32_drivers max98357a freertos tetris mqtt_client_bl memory
)
