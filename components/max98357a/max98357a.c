/**
 * @file max98357a.c
 *
 * @brief Driver for MAX98357A I2S audio amplifier with SPKM.28.8.A speaker.
 *
 * Uses the ESP-IDF 5.x new I2S driver API (driver/i2s_std.h).
 * The MAX98357A is configured in standard I2S (Philips) mode.
 * Audio output is stereo on the wire; for the mono SPKM.28.8.A speaker the
 * amplifier sums both channels internally when GAIN/SD is left floating or
 * tied HIGH, so feeding the same data on L and R works correctly.
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "max98357a.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//---------------------------------- MACROS -----------------------------------
#define TAG "max98357a"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/** Maximum single write chunk sent to I2S DMA (bytes). */
#define WRITE_CHUNK_BYTES (MAX98357A_DMA_BUF_LEN * 4U)

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static i2s_chan_handle_t s_tx_chan  = NULL;
static int               s_sd_gpio = -1;
static uint32_t          s_sample_rate   = MAX98357A_SAMPLE_RATE;
static uint8_t           s_bits_per_sample = MAX98357A_BITS_PER_SAMPLE;
static bool              s_initialised   = false;
static volatile bool     s_stop_requested = false;
static volatile bool     s_channel_running = false;

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
static void _sd_mode_gpio_init(int gpio_num);

//------------------------------ PUBLIC FUNCTIONS -----------------------------

esp_err_t max98357a_init(const max98357a_cfg_t *cfg)
{
    if(s_initialised)
    {
        ESP_LOGW(TAG, "already initialised");
        return ESP_OK;
    }

    /* Resolve configuration (use defaults when cfg == NULL). */
    int      i2s_num         = cfg ? cfg->i2s_num          : MAX98357A_I2S_NUM;
    int      gpio_bclk       = cfg ? cfg->gpio_bclk         : MAX98357A_GPIO_BCLK;
    int      gpio_lrc        = cfg ? cfg->gpio_lrc          : MAX98357A_GPIO_LRC;
    int      gpio_dout       = cfg ? cfg->gpio_dout         : MAX98357A_GPIO_DOUT;
    int      gpio_sd         = cfg ? cfg->gpio_sd_mode      : MAX98357A_SD_MODE_GPIO;
    uint32_t sample_rate     = cfg ? cfg->sample_rate       : MAX98357A_SAMPLE_RATE;
    uint8_t  bits_per_sample = cfg ? cfg->bits_per_sample   : MAX98357A_BITS_PER_SAMPLE;

    /* Map bits_per_sample to i2s_data_bit_width_t. */
    i2s_data_bit_width_t bit_width;
    switch(bits_per_sample)
    {
        case 16: bit_width = I2S_DATA_BIT_WIDTH_16BIT; break;
        case 24: bit_width = I2S_DATA_BIT_WIDTH_24BIT; break;
        case 32: bit_width = I2S_DATA_BIT_WIDTH_32BIT; break;
        default:
            ESP_LOGE(TAG, "unsupported bits_per_sample: %u", bits_per_sample);
            return ESP_ERR_INVALID_ARG;
    }

    /* Create I2S TX channel. */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        (i2s_port_t)i2s_num, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = MAX98357A_DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = MAX98357A_DMA_BUF_LEN;

    esp_err_t ret = i2s_new_channel(&chan_cfg, &s_tx_chan, NULL);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure standard (Philips) I2S mode. */
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bit_width,
                                                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)gpio_bclk,
            .ws   = (gpio_num_t)gpio_lrc,
            .dout = (gpio_num_t)gpio_dout,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(s_tx_chan, &std_cfg);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(ret));
        i2s_del_channel(s_tx_chan);
        s_tx_chan = NULL;
        return ret;
    }

    ret = i2s_channel_enable(s_tx_chan);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(ret));
        i2s_del_channel(s_tx_chan);
        s_tx_chan = NULL;
        return ret;
    }

    /* Optional SD_MODE / shutdown pin. */
    _sd_mode_gpio_init(gpio_sd);
    s_sd_gpio        = gpio_sd;
    s_sample_rate    = sample_rate;
    s_bits_per_sample = bits_per_sample;
    s_initialised    = true;
    s_channel_running = true;

    /* Enable amplifier by default. */
    max98357a_set_enable(true);

    ESP_LOGI(TAG, "initialised – I2S%d, %"PRIu32" Hz, %u-bit, BCLK=%d LRC=%d DOUT=%d",
             i2s_num, sample_rate, bits_per_sample,
             gpio_bclk, gpio_lrc, gpio_dout);
    return ESP_OK;
}

esp_err_t max98357a_deinit(void)
{
    if(!s_initialised)
    {
        return ESP_OK;
    }

    max98357a_set_enable(false);
    i2s_channel_disable(s_tx_chan);
    i2s_del_channel(s_tx_chan);
    s_tx_chan     = NULL;
    s_initialised = false;

    ESP_LOGI(TAG, "deinitialised");
    return ESP_OK;
}

esp_err_t max98357a_play_raw(const void *data, size_t len_bytes,
                             uint32_t timeout_ms)
{
    if(!s_initialised || !data || len_bytes == 0U)
    {
        return ESP_ERR_INVALID_STATE;
    }

    size_t        written  = 0;
    const uint8_t *p       = (const uint8_t *)data;
    size_t         remaining = len_bytes;

    while(remaining > 0U)
    {
        if(s_stop_requested) { return ESP_ERR_INVALID_STATE; }

        size_t chunk = (remaining > WRITE_CHUNK_BYTES) ? WRITE_CHUNK_BYTES
                                                       : remaining;
        esp_err_t ret = i2s_channel_write(s_tx_chan, p, chunk, &written,
                                          pdMS_TO_TICKS(timeout_ms));
        if(ret == ESP_ERR_TIMEOUT)
        {
            ESP_LOGW(TAG, "write timeout");
            return ESP_ERR_TIMEOUT;
        }
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "i2s_channel_write error: %s", esp_err_to_name(ret));
            return ret;
        }
        p         += written;
        remaining -= written;
    }
    return ESP_OK;
}

esp_err_t max98357a_play_tone(uint32_t freq_hz, uint32_t duration_ms,
                              uint8_t volume)
{
    if(!s_initialised)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if(freq_hz == 0U || duration_ms == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if(volume > 100U)
    {
        volume = 100U;
    }

    /* Generate and stream in small chunks to avoid large heap allocation. */
    #define TONE_CHUNK_FRAMES (256U)  /* stereo frames per chunk */
    uint32_t bytes_per_sample = s_bits_per_sample / 8U;
    /* chunk holds TONE_CHUNK_FRAMES stereo frames */
    size_t   chunk_bytes = TONE_CHUNK_FRAMES * 2U * bytes_per_sample;

    uint8_t *buf = (uint8_t *)malloc(chunk_bytes);
    if(!buf)
    {
        ESP_LOGE(TAG, "malloc failed (%u bytes)", (unsigned)chunk_bytes);
        return ESP_ERR_NO_MEM;
    }

    uint32_t total_samples  = (uint32_t)((uint64_t)s_sample_rate * duration_ms / 1000U);
    float    scale          = (32767.0f * volume) / 100.0f;
    uint32_t sample_idx     = 0;
    esp_err_t ret           = ESP_OK;

    while(sample_idx < total_samples)
    {
        uint32_t frames = total_samples - sample_idx;
        if(frames > TONE_CHUNK_FRAMES) frames = TONE_CHUNK_FRAMES;

        for(uint32_t f = 0; f < frames; f++)
        {
            float   theta  = 2.0f * (float)M_PI * freq_hz *
                             (sample_idx + f) / (float)s_sample_rate;
            int16_t sample = (int16_t)(scale * sinf(theta));

            if(s_bits_per_sample == 16)
            {
                int16_t *p = (int16_t *)buf;
                p[f * 2]     = sample;
                p[f * 2 + 1] = sample;
            }
            else
            {
                int32_t *p = (int32_t *)buf;
                p[f * 2]     = (int32_t)sample << 16;
                p[f * 2 + 1] = (int32_t)sample << 16;
            }
        }

        if(s_stop_requested) break;
        ret = max98357a_play_raw(buf, frames * 2U * bytes_per_sample, 1000U);
        if(ret != ESP_OK) break;
        sample_idx += frames;
    }

    free(buf);
    return ret;
}

void max98357a_set_enable(bool enable)
{
    if(s_sd_gpio < 0)
    {
        return; /* pin not connected */
    }
    gpio_set_level((gpio_num_t)s_sd_gpio, enable ? 1 : 0);
    ESP_LOGD(TAG, "amplifier %s", enable ? "enabled" : "disabled");
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _sd_mode_gpio_init(int gpio_num)
{
    if(gpio_num < 0)
    {
        return;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)gpio_num, 0); /* start disabled */
}

void max98357a_stop_playback(void)
{
    if(!s_initialised || !s_tx_chan) return;

<<<<<<< HEAD
    /* Signal play_raw/play_tone to exit at the next chunk boundary. */
=======
    /* Signal any ongoing play_raw to abort at the next chunk boundary (~6 ms).
     * Do NOT call i2s_channel_disable here — that call blocks if another task
     * is currently inside i2s_channel_write, which would freeze the LVGL task.
     * Instead, call max98357a_cut_audio() from within the audio task itself
     * after it exits its playback loop. */
>>>>>>> 9006a62 (svi gumbovi i joystick rade)
    s_stop_requested = true;
}

<<<<<<< HEAD
    /* Disable the hardware immediately — cuts audio output instantly.
     * Do NOT re-enable here: re-enabling causes the DMA to replay the
     * last buffer in a loop, which is exactly the "note stuck forever" bug.
     * resume_playback() will re-enable when new audio is about to start. */
    if(s_channel_running)
    {
        i2s_channel_disable(s_tx_chan);
        s_channel_running = false;
    }
=======
void max98357a_cut_audio(void)
{
    if(!s_initialised || !s_tx_chan) return;

    /* Immediately silence the I2S DMA output.
     * MUST be called from the audio task that owns the channel (NOT from the
     * LVGL task), otherwise i2s_channel_disable may block waiting for the
     * task that holds the write lock. */
    i2s_channel_disable(s_tx_chan);
    i2s_channel_enable(s_tx_chan);
>>>>>>> 9006a62 (svi gumbovi i joystick rade)
}

void max98357a_resume_playback(void)
{
    s_stop_requested = false;
    if(!s_channel_running)
    {
        i2s_channel_enable(s_tx_chan);
        s_channel_running = true;
    }
}

void play_sos(void)
{
    /* SOS in Morse code: ... --- ...
       Short = 150ms, Long = 450ms, gap between signals = 150ms, gap between letters = 450ms */
    #define DOT  150
    #define DASH 450
    #define GAP  150
    #define LGAP 450

    /* S = ... */
    max98357a_play_tone(880, DOT,  80); vTaskDelay(pdMS_TO_TICKS(GAP));
    max98357a_play_tone(880, DOT,  80); vTaskDelay(pdMS_TO_TICKS(GAP));
    max98357a_play_tone(880, DOT,  80); vTaskDelay(pdMS_TO_TICKS(LGAP));
    /* O = --- */
    max98357a_play_tone(660, DASH, 80); vTaskDelay(pdMS_TO_TICKS(GAP));
    max98357a_play_tone(660, DASH, 80); vTaskDelay(pdMS_TO_TICKS(GAP));
    max98357a_play_tone(660, DASH, 80); vTaskDelay(pdMS_TO_TICKS(LGAP));
    /* S = ... */
    max98357a_play_tone(880, DOT,  80); vTaskDelay(pdMS_TO_TICKS(GAP));
    max98357a_play_tone(880, DOT,  80); vTaskDelay(pdMS_TO_TICKS(GAP));
    max98357a_play_tone(880, DOT,  80);

    #undef DOT
    #undef DASH
    #undef GAP
    #undef LGAP
}

void play_tetris(void)
{
    /* Tetris A (Korobeiniki) – correct frequencies, 160 BPM
       E=187ms  Q=375ms  DQ=562ms  H=750ms */
    static const uint16_t notes[] = {
        /* Part 1 */
        659, 494, 523, 587, 523, 494,
        440, 440, 523, 659, 587, 523,
        494, 523, 587, 659, 523, 440, 440,
        /* Part 2 */
        587, 698, 880, 784, 698,
        659, 523, 659, 587, 523,
        494, 494, 523, 587, 659, 523, 440, 440
    };
    static const uint16_t durations[] = {
        /* Part 1 */
        375, 187, 187, 375, 187, 187,
        375, 187, 187, 375, 187, 187,
        562, 187, 375, 375, 375, 375, 750,
        /* Part 2 */
        375, 187, 375, 187, 187,
        562, 187, 375, 187, 187,
        375, 187, 187, 375, 375, 375, 375, 750
    };

    int len = sizeof(notes) / sizeof(notes[0]);

    for(int i = 0; i < len; i++)
    {
        max98357a_play_tone(notes[i], durations[i], 80);
        /* 20ms zero-amplitude gap keeps DMA fed with silence */
        max98357a_play_tone(notes[i], 20, 0);
    }
}
