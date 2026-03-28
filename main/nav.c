#include "nav.h"
#include "ui.h"
#include "max98357a.h"

/* ------------------------------------------------------------------ */
/*  Shared key state for ENTER only (written from any task)            */
/* ------------------------------------------------------------------ */
static volatile uint32_t         _cur_key   = 0;
static volatile lv_indev_state_t _cur_state = LV_INDEV_STATE_REL;

static lv_indev_t  *_keypad_indev = NULL;
static lv_group_t  *_active_group = NULL;

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
/* ------------------------------------------------------------------ */
static lv_obj_t *_home_objs[6];
static lv_obj_t *_klav_objs[14];

/* ------------------------------------------------------------------ */
/*  Focus style — thick bright outline so the user can see clearly     */
/*  which icon is selected                                             */
/* ------------------------------------------------------------------ */
static lv_style_t _style_focused;

static void _apply_focus_style(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &_style_focused, LV_STATE_FOCUSED);
}

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
/*  Spatial directional navigation + keyboard routing                  */
/* ------------------------------------------------------------------ */
static void _do_spatial_nav(void *arg)
{
    uint32_t dir = (uint32_t)(uintptr_t)arg;

    lv_group_t *group = _active_group;
    if (!group) return;

    lv_obj_t *focused = lv_group_get_focused(group);
    if (!focused) { lv_group_focus_next(group); return; }

    /* ---- Special case: keyboard widget ----
     * When the lv_keyboard is focused, send the directional key directly
     * into it.  The keyboard (built on lv_btnmatrix) navigates its own
     * internal buttons when it receives LV_EVENT_KEY with arrow keys,
     * and presses the highlighted button on LV_KEY_ENTER.            */
    if (focused == ui_Keyboard1)
    {
        uint32_t kdir = dir;   /* must be an addressable lvalue for event param */
        lv_event_send(focused, LV_EVENT_KEY, &kdir);
        return;
    }

    /* ---- Spatial navigation for all other screens ---- */
    lv_obj_t  **objs  = NULL;
    uint32_t    count = 0;
    lv_obj_t   *scr   = lv_scr_act();

    if      (scr == ui_HomePage)          { objs = _home_objs; count = 6;  }
    else if (scr == ui_KlavijaturaScreen) { objs = _klav_objs; count = 14; }
    else {
        /* Poruke (2 objects), Tetris, Memory, SOS: simple next/prev */
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

        bool    ok    = false;
        int32_t score = 0;
        switch (dir)
        {
            case LV_KEY_RIGHT: ok = (dx >  5); score = dx*dx + 4*dy*dy; break;
            case LV_KEY_LEFT:  ok = (dx < -5); score = dx*dx + 4*dy*dy; break;
            case LV_KEY_DOWN:  ok = (dy >  5); score = 4*dx*dx + dy*dy; break;
            case LV_KEY_UP:    ok = (dy < -5); score = 4*dx*dx + dy*dy; break;
            default: break;
        }

        if (ok && score < best_score) { best_score = score; best = obj; }
    }

    if (best)
        lv_group_focus_obj(best);
    else
    {
        /* Nothing in that direction — wrap */
        if (dir == LV_KEY_RIGHT || dir == LV_KEY_DOWN)
            lv_group_focus_next(group);
        else
            lv_group_focus_prev(group);
    }
}

/* ------------------------------------------------------------------ */
/*  Back navigation                                                     */
/* ------------------------------------------------------------------ */
static void _do_go_back(void *arg)
{
    (void)arg;

    /* Kill all audio before any screen transition */
    max98357a_stop_playback();

    lv_obj_t *scr = lv_scr_act();

    if      (scr == ui_TetrisScreen)      lv_event_send(ui_ImgButton15,    LV_EVENT_CLICKED, NULL);
    else if (scr == ui_MemoryScreen)      lv_event_send(ui_ImgButton17,    LV_EVENT_CLICKED, NULL);
    else if (scr == ui_SOSScreen)         lv_event_send(ui_GumbSOS,        LV_EVENT_CLICKED, NULL);
    else if (scr == ui_PorukeScreen)      lv_event_send(ui_BackButtonMsg,  LV_EVENT_CLICKED, NULL);
    else if (scr == ui_KlavijaturaScreen) lv_event_send(ui_ImgButton10,    LV_EVENT_CLICKED, NULL);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */
void nav_init(void)
{
    /* --- Focus style: thick white outline, high contrast --- */
    lv_style_init(&_style_focused);
    lv_style_set_outline_width(&_style_focused, 4);
    lv_style_set_outline_color(&_style_focused, lv_color_white());
    lv_style_set_outline_pad(&_style_focused,   4);
    lv_style_set_border_width(&_style_focused,  2);
    lv_style_set_border_color(&_style_focused,  lv_color_white());

    /* --- Register keypad input device (ENTER only) --- */
    static lv_indev_drv_t kp_drv;
    lv_indev_drv_init(&kp_drv);
    kp_drv.type    = LV_INDEV_TYPE_KEYPAD;
    kp_drv.read_cb = _keypad_read_cb;
    _keypad_indev  = lv_indev_drv_register(&kp_drv);

    /* --- Create per-screen groups --- */
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
    for (int i = 0; i < 6; i++) {
        lv_group_add_obj(_group_home, _home_objs[i]);
        _apply_focus_style(_home_objs[i]);
    }

    /* ---- PorukeScreen ---- */
    lv_group_add_obj(_group_poruke, ui_BackButtonMsg);
    lv_group_add_obj(_group_poruke, ui_Keyboard1);
    _apply_focus_style(ui_BackButtonMsg);
    _apply_focus_style(ui_Keyboard1);

    /* ---- TetrisScreen ---- */
    lv_group_add_obj(_group_tetris, ui_ImgButton15);
    _apply_focus_style(ui_ImgButton15);

    /* ---- MemoryScreen ---- */
    lv_group_add_obj(_group_memory, ui_ImgButton17);
    _apply_focus_style(ui_ImgButton17);

    /* ---- SOSScreen ---- */
    lv_group_add_obj(_group_sos, ui_GumbSOS);
    _apply_focus_style(ui_GumbSOS);

    /* ---- KlavijaturaScreen ---- */
    _klav_objs[0]  = ui_ImgButton10;
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
    for (int i = 0; i < 14; i++) {
        lv_group_add_obj(_group_klav, _klav_objs[i]);
        _apply_focus_style(_klav_objs[i]);
    }

    /* --- Screen-loaded event callbacks --- */
    lv_obj_add_event_cb(ui_HomePage,          _on_home_loaded,   LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_PorukeScreen,      _on_poruke_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_TetrisScreen,      _on_tetris_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_MemoryScreen,      _on_memory_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_SOSScreen,         _on_sos_loaded,    LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_KlavijaturaScreen, _on_klav_loaded,   LV_EVENT_SCREEN_LOADED, NULL);

    /* Initial group — HomePage is already displayed */
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
    lv_async_call(_do_spatial_nav, (void *)(uintptr_t)dir);
}
