#include "nav.h"
#include "ui.h"
#include "max98357a.h"

/* ------------------------------------------------------------------ */
/*  Shared key state for ENTER only (written from any task)            */
/* ------------------------------------------------------------------ */
static volatile uint32_t         _cur_key   = 0;
static volatile lv_indev_state_t _cur_state = LV_INDEV_STATE_REL;

static lv_indev_t  *_keypad_indev   = NULL;
static lv_group_t  *_active_group   = NULL;  /* tracks whichever group is currently live */

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
/*  Object arrays for spatial navigation                               */
/*  (maintained manually — avoids LVGL internal linked-list access)   */
/* ------------------------------------------------------------------ */
static lv_obj_t *_home_objs[6];   /* Memory, Poruke, Tetris, Klav, SOS, ColorSwitch */
static lv_obj_t *_klav_objs[14];  /* back-btn, C D E F G A B C2, Cis Dis Fis Gis Ais */

/* ------------------------------------------------------------------ */
/*  Keypad read callback — used only for LV_KEY_ENTER                  */
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
static void _set_active_group(lv_group_t *g)
{
    _active_group = g;
    lv_indev_set_group(_keypad_indev, g);
}

static void _on_home_loaded(lv_event_t *e)   { (void)e; _set_active_group(_group_home);   }
static void _on_poruke_loaded(lv_event_t *e) { (void)e; _set_active_group(_group_poruke); }
static void _on_tetris_loaded(lv_event_t *e) { (void)e; _set_active_group(_group_tetris); }
static void _on_memory_loaded(lv_event_t *e) { (void)e; _set_active_group(_group_memory); }
static void _on_sos_loaded(lv_event_t *e)    { (void)e; _set_active_group(_group_sos);    }
static void _on_klav_loaded(lv_event_t *e)   { (void)e; _set_active_group(_group_klav);   }

/* ------------------------------------------------------------------ */
/*  Spatial directional navigation                                     */
/*                                                                     */
/*  Finds the nearest object in the joystick direction using absolute  */
/*  screen coordinates.  Heavily penalises off-axis deviation so the  */
/*  "most directly right/left/up/down" object always wins.            */
/* ------------------------------------------------------------------ */
static void _do_spatial_nav(void *arg)
{
    uint32_t dir = (uint32_t)(uintptr_t)arg;

    lv_group_t *group = _active_group;
    if (!group) return;

    lv_obj_t *focused = lv_group_get_focused(group);
    if (!focused) { lv_group_focus_next(group); return; }

    /* Choose the object list for the current screen */
    lv_obj_t  **objs  = NULL;
    uint32_t    count = 0;
    lv_obj_t   *scr   = lv_scr_act();

    if      (scr == ui_HomePage)         { objs = _home_objs; count = 6;  }
    else if (scr == ui_KlavijaturaScreen){ objs = _klav_objs; count = 14; }
    else {
        /* Only 1-2 items on other screens — simple NEXT/PREV is enough */
        if (dir == LV_KEY_RIGHT || dir == LV_KEY_DOWN)
            lv_group_focus_next(group);
        else
            lv_group_focus_prev(group);
        return;
    }

    /* Absolute centre of the focused object */
    lv_area_t fa;
    lv_obj_get_coords(focused, &fa);
    int32_t fx = (fa.x1 + fa.x2) / 2;
    int32_t fy = (fa.y1 + fa.y2) / 2;

    lv_obj_t *best       = NULL;
    int32_t   best_score = INT32_MAX;

    for (uint32_t i = 0; i < count; i++)
    {
        lv_obj_t *obj = objs[i];
        if (!obj || obj == focused) continue;

        lv_area_t oa;
        lv_obj_get_coords(obj, &oa);
        int32_t ox = (oa.x1 + oa.x2) / 2;
        int32_t oy = (oa.y1 + oa.y2) / 2;

        int32_t dx = ox - fx;
        int32_t dy = oy - fy;

        /* Check the object is genuinely in the requested direction */
        bool ok = false;
        int32_t score = 0;
        switch (dir)
        {
            case LV_KEY_RIGHT:
                ok    = (dx > 5);
                score = dx * dx + 4 * dy * dy; /* penalise vertical drift */
                break;
            case LV_KEY_LEFT:
                ok    = (dx < -5);
                score = dx * dx + 4 * dy * dy;
                break;
            case LV_KEY_DOWN:
                ok    = (dy > 5);
                score = 4 * dx * dx + dy * dy; /* penalise horizontal drift */
                break;
            case LV_KEY_UP:
                ok    = (dy < -5);
                score = 4 * dx * dx + dy * dy;
                break;
            default:
                break;
        }

        if (ok && score < best_score)
        {
            best_score = score;
            best       = obj;
        }
    }

    if (best)
        lv_group_focus_obj(best);
    else
    {
        /* Nothing in that direction — wrap around */
        if (dir == LV_KEY_RIGHT || dir == LV_KEY_DOWN)
            lv_group_focus_next(group);
        else
            lv_group_focus_prev(group);
    }
}

/* ------------------------------------------------------------------ */
/*  Back navigation (runs in LVGL task via lv_async_call)              */
/* ------------------------------------------------------------------ */
static void _do_go_back(void *arg)
{
    (void)arg;

    /* Kill all audio immediately before any screen transition.
     * Each sound task's resume_playback() call will re-arm when needed. */
    max98357a_stop_playback();

    lv_obj_t *scr = lv_scr_act();

    if (scr == ui_TetrisScreen) {
        lv_event_send(ui_ImgButton15, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_MemoryScreen) {
        lv_event_send(ui_ImgButton17, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_SOSScreen) {
        lv_event_send(ui_GumbSOS, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_PorukeScreen) {
        lv_event_send(ui_BackButtonMsg, LV_EVENT_CLICKED, NULL);
    } else if (scr == ui_KlavijaturaScreen) {
        lv_event_send(ui_ImgButton10, LV_EVENT_CLICKED, NULL);
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */
void nav_init(void)
{
    /* Register keypad input device (used for LV_KEY_ENTER only) */
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
    _home_objs[0] = ui_Memory;
    _home_objs[1] = ui_Poruke;
    _home_objs[2] = ui_Tetris;
    _home_objs[3] = ui_Klavijatura;
    _home_objs[4] = ui_SOS;
    _home_objs[5] = ui_ColorSwitch;
    for (int i = 0; i < 6; i++)
        lv_group_add_obj(_group_home, _home_objs[i]);

    /* ---- PorukeScreen ---- */
    lv_group_add_obj(_group_poruke, ui_BackButtonMsg);
    lv_group_add_obj(_group_poruke, ui_Keyboard1);

    /* ---- TetrisScreen ---- */
    lv_group_add_obj(_group_tetris, ui_ImgButton15);

    /* ---- MemoryScreen ---- */
    lv_group_add_obj(_group_memory, ui_ImgButton17);

    /* ---- SOSScreen ---- */
    lv_group_add_obj(_group_sos, ui_GumbSOS);

    /* ---- KlavijaturaScreen ---- */
    _klav_objs[0]  = ui_ImgButton10;  /* back button (top-left) */
    _klav_objs[1]  = ui_CNote;
    _klav_objs[2]  = ui_DNote;
    _klav_objs[3]  = ui_ENote;
    _klav_objs[4]  = ui_FNote;
    _klav_objs[5]  = ui_GNote;
    _klav_objs[6]  = ui_ANote;
    _klav_objs[7]  = ui_BNote;
    _klav_objs[8]  = ui_C2Note;
    _klav_objs[9]  = ui_CisNote;
    _klav_objs[10] = ui_DisNote;
    _klav_objs[11] = ui_FisNote;
    _klav_objs[12] = ui_GisNote;
    _klav_objs[13] = ui_AisNote;
    for (int i = 0; i < 14; i++)
        lv_group_add_obj(_group_klav, _klav_objs[i]);

    /* Register screen-loaded event callbacks */
    lv_obj_add_event_cb(ui_HomePage,          _on_home_loaded,   LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_PorukeScreen,      _on_poruke_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_TetrisScreen,      _on_tetris_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_MemoryScreen,      _on_memory_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_SOSScreen,         _on_sos_loaded,    LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_KlavijaturaScreen, _on_klav_loaded,   LV_EVENT_SCREEN_LOADED, NULL);

    /* Set initial group — HomePage is already displayed */
    _set_active_group(_group_home);
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

void nav_move_dir(uint32_t dir)
{
    /* Schedule spatial navigation to run in the LVGL task */
    lv_async_call(_do_spatial_nav, (void *)(uintptr_t)dir);
}
