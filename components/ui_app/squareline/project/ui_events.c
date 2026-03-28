// ui_events.c
#include "ui.h"
#include "max98357a.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_app/tetris_ui_draw.h"
#include "freertos/queue.h"

/* ------------------------------------------------------------------ */
/*  Shared task handles                                                 */
/* ------------------------------------------------------------------ */
static TaskHandle_t  _tetris_task_hdl = NULL;
static TaskHandle_t  _sos_task_hdl    = NULL;

static volatile bool _tetris_running  = false;
static volatile bool _sos_running     = false;

/* ------------------------------------------------------------------ */
/*  Color switch – Ben10 (blue/boy) vs. pink (girl) mode              */
/* ------------------------------------------------------------------ */

void is_blue_mode(lv_event_t * e)
{
    (void)e;
    /* Only act when the switch is NOT checked (blue/Ben10 mode) */
    if (ui_ColorSwitch && lv_obj_has_state(ui_ColorSwitch, LV_STATE_CHECKED))
        return;

    lv_obj_set_style_bg_img_src(ui_HomePage, &ui_img_527192083,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_HomePage, lv_color_hex(0x88BADC),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HomePage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void roza_boja(lv_event_t * e)
{
    (void)e;
    /* Only act when the switch IS checked (pink/Barbie mode) */
    if (ui_ColorSwitch && !lv_obj_has_state(ui_ColorSwitch, LV_STATE_CHECKED))
        return;

    /* Apply original image asset from images for pink mode */
    lv_obj_set_style_bg_img_src(ui_HomePage, &ui_img_wp2844947_png,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_opa(ui_HomePage, LV_OPA_COVER,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_recolor_opa(ui_HomePage, LV_OPA_TRANSP,
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_HomePage, lv_color_hex(0xFF69B4),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HomePage, LV_OPA_TRANSP,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* ------------------------------------------------------------------ */
/*  Tetris music                                                        */
/* ------------------------------------------------------------------ */

static void _tetris_task(void *arg)
{
    static const int16_t silence[256] = {0};

    while (_tetris_running)
    {
        //play_tetris();          /* internally aborts quickly when stop flag set */

        if (!_tetris_running) break;

        /* ~500 ms silence gap between loops */
        for (int i = 0; i < 86 && _tetris_running; i++)
            max98357a_play_raw(silence, sizeof(silence), 200);
    }

    _tetris_task_hdl = NULL;
    vTaskDelete(NULL);
}

void tetris_music_on(lv_event_t * e)
{
    setup_tetris_ui();
    
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED && _tetris_task_hdl == NULL)
    {
        max98357a_resume_playback();
        _tetris_running = true;
        xTaskCreate(_tetris_task, "tetris", 4096, NULL, 5, &_tetris_task_hdl);
    }
}

void exit_tetris(lv_event_t * e)
{
	destroy_tetris_ui();
    (void)e;
    _tetris_running = false;
    max98357a_stop_playback();
    /* Task checks _tetris_running and self-deletes; handle is cleared there */
}

/* ------------------------------------------------------------------ */
/*  SOS music                                                           */
/* ------------------------------------------------------------------ */

/* Interruptible delay: checks _sos_running every 10 ms */
static void _sos_delay(uint32_t ms)
{
    TickType_t end = xTaskGetTickCount() + pdMS_TO_TICKS(ms);
    while (_sos_running && xTaskGetTickCount() < end)
        vTaskDelay(pdMS_TO_TICKS(10));
}

static void _sos_task(void *arg)
{
#define SOS_DOT   150
#define SOS_DASH  450
#define SOS_GAP   150
#define SOS_LGAP  450

    while (_sos_running)
    {
        /* S = ... */
        if (!_sos_running) break;
        max98357a_play_tone(880, SOS_DOT,  80); _sos_delay(SOS_GAP);
        if (!_sos_running) break;
        max98357a_play_tone(880, SOS_DOT,  80); _sos_delay(SOS_GAP);
        if (!_sos_running) break;
        max98357a_play_tone(880, SOS_DOT,  80); _sos_delay(SOS_LGAP);
        /* O = --- */
        if (!_sos_running) break;
        max98357a_play_tone(660, SOS_DASH, 80); _sos_delay(SOS_GAP);
        if (!_sos_running) break;
        max98357a_play_tone(660, SOS_DASH, 80); _sos_delay(SOS_GAP);
        if (!_sos_running) break;
        max98357a_play_tone(660, SOS_DASH, 80); _sos_delay(SOS_LGAP);
        /* S = ... */
        if (!_sos_running) break;
        max98357a_play_tone(880, SOS_DOT,  80); _sos_delay(SOS_GAP);
        if (!_sos_running) break;
        max98357a_play_tone(880, SOS_DOT,  80); _sos_delay(SOS_GAP);
        if (!_sos_running) break;
        max98357a_play_tone(880, SOS_DOT,  80);

        /* 1-second pause between repetitions */
        _sos_delay(1000);
    }

    _sos_task_hdl = NULL;
    vTaskDelete(NULL);

#undef SOS_DOT
#undef SOS_DASH
#undef SOS_GAP
#undef SOS_LGAP
}

void sos_music_on(lv_event_t * e)
{
    (void)e;
    if (_sos_task_hdl == NULL)
    {
        max98357a_resume_playback();
        _sos_running = true;
        xTaskCreate(_sos_task, "sos", 4096, NULL, 5, &_sos_task_hdl);
    }
}

void sos_music_off(lv_event_t * e)
{
    (void)e;
    _sos_running = false;
    max98357a_stop_playback();
}

/* ------------------------------------------------------------------ */
/*  Piano keyboard                                                      */
/*                                                                     */
/*  A single worker task reads from a queue.  If multiple keys are     */
/*  pressed quickly the queue is drained and only the latest key       */
/*  plays, preventing pile-up.  Note duration is 350 ms.              */
/* ------------------------------------------------------------------ */

static QueueHandle_t _piano_queue    = NULL;
static TaskHandle_t  _piano_task_hdl = NULL;

static void _piano_worker(void *arg)
{
    uint32_t freq;
    while (1)
    {
        if (xQueueReceive(_piano_queue, &freq, portMAX_DELAY) == pdTRUE)
        {
            /* Drain any extra queued keys – play only the latest */
            uint32_t latest = freq;
            while (xQueueReceive(_piano_queue, &latest, 0) == pdTRUE) {}

            max98357a_resume_playback();
            max98357a_play_tone(latest, 350, 80);
        }
    }
}

static void _piano_init(void)
{
    if (_piano_queue == NULL)
    {
        _piano_queue = xQueueCreate(4, sizeof(uint32_t));
        xTaskCreate(_piano_worker, "piano", 2048, NULL, 5, &_piano_task_hdl);
    }
}

static void _play_note(uint32_t freq)
{
    _piano_init();
    xQueueSend(_piano_queue, &freq, 0);   /* non-blocking: drop if full */
}

/* ---- White keys ---- */
void c_note_event(lv_event_t * e)  { (void)e; _play_note(262); }   /* C4  262 Hz */
void d_note_event(lv_event_t * e)  { (void)e; _play_note(294); }   /* D4  294 Hz */
void e_note_event(lv_event_t * e)  { (void)e; _play_note(330); }   /* E4  330 Hz */
void f_note_event(lv_event_t * e)  { (void)e; _play_note(349); }   /* F4  349 Hz */
void g_note_event(lv_event_t * e)  { (void)e; _play_note(392); }   /* G4  392 Hz */
void a_note_event(lv_event_t * e)  { (void)e; _play_note(440); }   /* A4  440 Hz */
void b_note_event(lv_event_t * e)  { (void)e; _play_note(494); }   /* B4  494 Hz */
void c2_note_event(lv_event_t * e) { (void)e; _play_note(523); }   /* C5  523 Hz */

/* ---- Black keys ---- */
void cis_note_event(lv_event_t * e) { (void)e; _play_note(277); }  /* C#4 277 Hz */
void dis_note_event(lv_event_t * e) { (void)e; _play_note(311); }  /* D#4 311 Hz */
void fis_note_event(lv_event_t * e) { (void)e; _play_note(370); }  /* F#4 370 Hz */
void gis_note_event(lv_event_t * e) { (void)e; _play_note(415); }  /* G#4 415 Hz */
void ais_note_event(lv_event_t * e) { (void)e; _play_note(466); }  /* A#4 466 Hz */
