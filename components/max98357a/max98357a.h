/**
 * @file max98357a.h
 *
 * @brief Driver for MAX98357A I2S audio amplifier with SPKM.28.8.A speaker.
 *
 * The MAX98357A receives digital audio via I2S and drives the speaker directly.
 * Use max98357a_init() once, then max98357a_play_raw() to output PCM samples,
 * or max98357a_play_tone() for a simple test beep.
 *
 * COPYRIGHT NOTICE: (c) 2025 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef MAX98357A_H
#define MAX98357A_H

//--------------------------------- INCLUDES ----------------------------------
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//---------------------------------- MACROS -----------------------------------

/** I2S peripheral to use (0 or 1). */
#ifndef MAX98357A_I2S_NUM
#define MAX98357A_I2S_NUM       (0)
#endif

/**
 * GPIO pins – change these to match your wiring on the BL Dev Kit.
 * Connect MAX98357A: BCLK→GPIO_BCLK, LRC→GPIO_LRC, DIN→GPIO_DOUT.
 * SD_MODE is optional: tie HIGH to always-on, or connect to GPIO_SD_MODE.
 * Set MAX98357A_SD_MODE_GPIO to -1 if not connected.
 */
#ifndef MAX98357A_GPIO_BCLK
#define MAX98357A_GPIO_BCLK     (26)
#endif

#ifndef MAX98357A_GPIO_LRC
#define MAX98357A_GPIO_LRC      (25)
#endif

#ifndef MAX98357A_GPIO_DOUT
#define MAX98357A_GPIO_DOUT     (22)
#endif

#ifndef MAX98357A_SD_MODE_GPIO
#define MAX98357A_SD_MODE_GPIO  (-1)  /* -1 = not connected, tie SHDN HIGH */
#endif

/** Default audio parameters. */
#ifndef MAX98357A_SAMPLE_RATE
#define MAX98357A_SAMPLE_RATE   (44100U)
#endif

#ifndef MAX98357A_BITS_PER_SAMPLE
#define MAX98357A_BITS_PER_SAMPLE (16U)
#endif

/** DMA buffer configuration.
 *  BUF_LEN matches TONE_CHUNK_FRAMES (256) so each play_tone write fills
 *  exactly one descriptor — keeps latency at BUF_COUNT × 5.8 ms ≈ 17 ms. */
#define MAX98357A_DMA_BUF_COUNT (3U)
#define MAX98357A_DMA_BUF_LEN   (256U)

//-------------------------------- DATA TYPES ---------------------------------

/** Driver configuration – pass to max98357a_init(). */
typedef struct
{
    int     i2s_num;          /**< I2S port number (0 or 1).               */
    int     gpio_bclk;        /**< Bit clock GPIO.                         */
    int     gpio_lrc;         /**< Word select (LR clock) GPIO.            */
    int     gpio_dout;        /**< Serial data out GPIO (ESP32 → amp).     */
    int     gpio_sd_mode;     /**< Shutdown/mode GPIO, -1 if not used.     */
    uint32_t sample_rate;     /**< Sample rate in Hz (e.g. 44100).         */
    uint8_t  bits_per_sample; /**< Bits per sample: 16 or 32.             */
} max98357a_cfg_t;

//------------------------- PUBLIC FUNCTION PROTOTYPES ------------------------

/**
 * @brief Initialise the MAX98357A driver and the I2S peripheral.
 *
 * @param cfg  Pointer to driver configuration. Pass NULL to use the
 *             compile-time defaults defined by the MAX98357A_* macros.
 * @return ESP_OK on success, otherwise an ESP error code.
 */
esp_err_t max98357a_init(const max98357a_cfg_t *cfg);

/**
 * @brief Deinitialise the driver and release the I2S peripheral.
 *
 * @return ESP_OK on success.
 */
esp_err_t max98357a_deinit(void);

/**
 * @brief Write raw PCM samples to the speaker.
 *
 * Samples must match the bit depth and channel count (stereo, both channels
 * identical for a mono speaker) configured in max98357a_init().
 *
 * @param data          Pointer to PCM sample buffer.
 * @param len_bytes     Number of bytes in the buffer.
 * @param timeout_ms    Maximum time to wait for DMA space (ms).
 * @return ESP_OK on success, ESP_ERR_TIMEOUT if DMA full.
 */
esp_err_t max98357a_play_raw(const void *data, size_t len_bytes,
                             uint32_t timeout_ms);

/**
 * @brief Generate and play a sine-wave tone.
 *
 * Blocks until the tone has been written to the DMA buffers.
 *
 * @param freq_hz       Tone frequency in Hz (e.g. 440 for A4).
 * @param duration_ms   Duration in milliseconds.
 * @param volume        Volume scalar 0–100 (100 = full scale).
 * @return ESP_OK on success.
 */
esp_err_t max98357a_play_tone(uint32_t freq_hz, uint32_t duration_ms,
                              uint8_t volume);

/**
 * @brief Enable or disable the amplifier via the SD_MODE pin.
 *
 * Has no effect if MAX98357A_SD_MODE_GPIO is -1.
 *
 * @param enable  true  = amplifier on, false = amplifier off (low-power).
 */
void max98357a_set_enable(bool enable);

/**
 * @brief Signal all ongoing playback to stop at the next chunk boundary (~6 ms).
 *        Call max98357a_resume_playback() before starting new audio.
 */
void max98357a_stop_playback(void);

/**
 * @brief Clear the stop flag so new playback calls work normally.
 */
void max98357a_resume_playback(void);

/**
 * @brief Disable then re-enable the I2S channel to immediately silence DMA output.
 *        MUST be called from the audio task context (NOT the LVGL task) to avoid
 *        blocking the LVGL task while the audio task holds the I2S write lock.
 */
void max98357a_cut_audio(void);

void play_sos(void);
void play_tetris(void);

#endif /* MAX98357A_H */
