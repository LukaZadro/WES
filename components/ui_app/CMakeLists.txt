set(SQUARELINE_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/squareline)
set(SQUARELINE_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR}/squareline/project)

# Prefer the main squareline export folder. Fall back to squareline/project
# for older project layouts.
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

set(COMPONENT_SRCS 
    "ui_app.c"
    ${SQUARELINE_SRCS}
)

file(GLOB_RECURSE SQUARELINE_INCLUDES "${SQUARELINE_DIR}/*.h")
list(FILTER SQUARELINE_INCLUDES EXCLUDE REGEX ".*/(backup|cache)/.*\\.h$")

foreach(_hdr ${SQUARELINE_INCLUDES})
    get_filename_component(_dir ${_hdr} DIRECTORY)
    list(APPEND COMPONENT_ADD_INCLUDEDIRS ${_dir})
endforeach()

list(REMOVE_DUPLICATES COMPONENT_ADD_INCLUDEDIRS)

set(COMPONENT_PRIV_REQUIRES lvgl lvgl_esp32_drivers)

register_component()
