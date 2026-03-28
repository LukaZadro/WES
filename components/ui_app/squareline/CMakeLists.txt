file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

# Ignore temporary/generated backups if present.
list(FILTER SOURCES EXCLUDE REGEX ".*/(backup|cache)/.*\\.c$")

add_library(ui ${SOURCES})
