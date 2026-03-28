SET(SOURCES screens/ui_HomePage.c
    screens/ui_TetrisScreen.c
    screens/ui_MemoryScreen.c
    screens/ui_SOSScreen.c
    screens/ui_PorukeScreen.c
    screens/ui_KlavijaturaScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_527192083.c
    images/ui_img_klavijatura_icon_80x80_png.c
    images/ui_img_memory_game_80x80_png.c
    images/ui_img_messages_icon_80x80_png.c
    images/ui_img_tetris_icon_80x80_png.c
    images/ui_img_sos_30x30_png.c
    images/ui_img_back_button_30x30_png.c
    images/ui_img_sos_120x120_png.c)

add_library(ui ${SOURCES})
