SET(SOURCES screens/ui_HomePage.c
    screens/ui_TetrisScreen.c
    screens/ui_MemoryScreen.c
    screens/ui_SOSScreen.c
    screens/ui_PorukeScreen.c
    screens/ui_KlavijaturaScreen.c
    screens/ui_HomePage2.c
    screens/ui_KoraciScreen.c
    screens/ui_TemperaturaScreen.c
    screens/ui_VoiceChangerScreen.c
    screens/ui_KameraScreen.c
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
    images/ui_img_sos_icon_120x120_png.c
    images/ui_img_289807621.c
    images/ui_img_1995866322.c
    images/ui_img_1108033562.c
    images/ui_img_702957783.c
    images/ui_img_1392414027.c)

add_library(ui ${SOURCES})
