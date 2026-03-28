#include "nav.h"
#include "ui.h"

/* ------------------------------------------------------------------ */
/*  Shared key state (written from any task, read by LVGL gui task)   */
/* ------------------------------------------------------------------ */
static volatile uint32_t         _cur_key   = 0;
static volatile lv_indev_state_t _cur_state = LV_INDEV_STATE_REL;

static lv_indev_t *_keypad_indev = NULL;

/* ------------------------------------------------------------------ */
/*  Per-screen focus groups                                            */
/* ------------------------------------------------------------------ */
static lv_group_t *_group_home   = NULL;
static lv_group_t *_group_poruke = NULL;
static lv_group_t *_group_tetris = NULL;
static lv_group_t *_group_memory = NULL;
static lv_group_t *_group_sos    = NULL;
static lv_group_t *_group_klav   = NULL;

/* ------------------------------------------------------------------ */
/*  Keypad read callback (called by LVGL every tick)                   */
/* ------------------------------------------------------------------ */
static void _keypad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    data->key   = _cur_key;
    data->state = _cur_state;
}

/* ------------------------------------------------------------------ */
/*  Screen-loaded callbacks — switch the active group                  */
/* ------------------------------------------------------------------ */
static void _on_home_loaded(lv_event_t *e)
{
    (void)e;
    lv_indev_set_group(_keypad_indev, _group_home);
}

static void _on_poruke_loaded(lv_event_t *e)
{
    (void)e;
    lv_indev_set_group(_keypad_indev, _group_poruke);
}

static void _on_tetris_loaded(lv_event_t *e)
{
    (void)e;
    lv_indev_set_group(_keypad_indev, _group_tetris);
}

static void _on_memory_loaded(lv_event_t *e)
{
    (void)e;
    lv_indev_set_group(_keypad_indev, _group_memory);
}

static void _on_sos_loaded(lv_event_t *e)
{
    (void)e;
    lv_indev_set_group(_keypad_indev, _group_sos);
}

static void _on_klav_loaded(lv_event_t *e)
{
    (void)e;
    lv_indev_set_group(_keypad_indev, _group_klav);
}

/* ------------------------------------------------------------------ */
/*  Back navigation (runs in LVGL task via lv_async_call)              */
/* ------------------------------------------------------------------ */
static void _do_go_back(void *arg)
{
    (void)arg;
    lv_obj_t *scr = lv_scr_act();

    if (scr == ui_TetrisScreen) {
        /* back button fires exit_tetris + screen change */
        lv_event_send(ui_ImgButton15, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_MemoryScreen) {
        lv_event_send(ui_ImgButton17, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_SOSScreen) {
        /* GumbSOS click stops SOS music and goes home */
        lv_event_send(ui_GumbSOS, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_PorukeScreen) {
        lv_event_send(ui_ImgButton16, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_KlavijaturaScreen) {
        lv_event_send(ui_ImgButton10, LV_EVENT_CLICKED, NULL);
    }
    /* On HomePage there is nowhere to go back to */
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */
void nav_init(void)
{
    /* Register keypad input device */
    static lv_indev_drv_t kp_drv;
    lv_indev_drv_init(&kp_drv);
    kp_drv.type    = LV_INDEV_TYPE_KEYPAD;
    kp_drv.read_cb = _keypad_read_cb;
    _keypad_indev  = lv_indev_drv_register(&kp_drv);

    /* Create per-screen groups */
    _group_home   = lv_group_create();
    _group_poruke = lv_group_create();
    _group_tetris = lv_group_create();
    _group_memory = lv_group_create();
    _group_sos    = lv_group_create();
    _group_klav   = lv_group_create();

    /* ---- HomePage ---- */
    lv_group_add_obj(_group_home, ui_Memory);
    lv_group_add_obj(_group_home, ui_Poruke);
    lv_group_add_obj(_group_home, ui_Tetris);
    lv_group_add_obj(_group_home, ui_Klavijatura);
    lv_group_add_obj(_group_home, ui_SOS);
    lv_group_add_obj(_group_home, ui_ColorSwitch);

    /* ---- PorukeScreen ---- */
    lv_group_add_obj(_group_poruke, ui_ImgButton16);  /* back button */
    lv_group_add_obj(_group_poruke, ui_Keyboard1);

    /* ---- TetrisScreen ---- */
    lv_group_add_obj(_group_tetris, ui_ImgButton15);  /* back button */

    /* ---- MemoryScreen ---- */
    lv_group_add_obj(_group_memory, ui_ImgButton17);  /* back button */

    /* ---- SOSScreen ---- */
    lv_group_add_obj(_group_sos, ui_GumbSOS);

    /* ---- KlavijaturaScreen ---- */
    lv_group_add_obj(_group_klav, ui_ImgButton10);    /* back button */
    lv_group_add_obj(_group_klav, ui_CNote);
    lv_group_add_obj(_group_klav, ui_DNote);
    lv_group_add_obj(_group_klav, ui_ENote);
    lv_group_add_obj(_group_klav, ui_FNote);
    lv_group_add_obj(_group_klav, ui_GNote);
    lv_group_add_obj(_group_klav, ui_ANote);
    lv_group_add_obj(_group_klav, ui_BNote);
    lv_group_add_obj(_group_klav, ui_C2Note);
    lv_group_add_obj(_group_klav, ui_CisNote);
    lv_group_add_obj(_group_klav, ui_DisNote);
    lv_group_add_obj(_group_klav, ui_FisNote);
    lv_group_add_obj(_group_klav, ui_GisNote);
    lv_group_add_obj(_group_klav, ui_AisNote);

    /* Register screen-loaded event callbacks */
    lv_obj_add_event_cb(ui_HomePage,         _on_home_loaded,   LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_PorukeScreen,     _on_poruke_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_TetrisScreen,     _on_tetris_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_MemoryScreen,     _on_memory_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_SOSScreen,        _on_sos_loaded,    LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_KlavijaturaScreen,_on_klav_loaded,   LV_EVENT_SCREEN_LOADED, NULL);

    /* Set initial group — HomePage is already displayed */
    lv_indev_set_group(_keypad_indev, _group_home);
}

void nav_send_key(uint32_t key, lv_indev_state_t state)
{
    _cur_key   = key;
    _cur_state = state;
}

void nav_go_back(void)
{
    lv_async_call(_do_go_back, NULL);
}
