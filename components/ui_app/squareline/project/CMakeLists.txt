SET(SOURCES screens/ui_HomePage.c
    screens/ui_Poruke1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c)

add_library(ui ${SOURCES})
