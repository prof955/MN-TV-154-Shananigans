#ifndef AUDIO_DSP_H
#define AUDIO_DSP_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_SAMPLE_RATE     22050
#define FFT_N                 512
#define SPECTRUM_BANDS        16

// INMP441 I2S GPIO Pins
#define PIN_I2S_BCLK          26
#define PIN_I2S_WS            25
#define PIN_I2S_SD            22

typedef struct {
    float band_values[SPECTRUM_BANDS];   // Current band magnitudes normalized 0.0 to 1.0
    float band_peaks[SPECTRUM_BANDS];    // Peak hold positions normalized 0.0 to 1.0
    uint8_t peak_decay_counters[SPECTRUM_BANDS]; // Counters for peak decay delay
    float rms_volume;                    // Total audio RMS volume percentage (0-100%)
    bool clap_detected;                  // Transient clap detection flag
    float raw_audio_wave[FFT_N];         // Live raw audio waveform for oscilloscope
} spectrum_data_t;

/**
 * @brief Initialize INMP441 I2S standard driver and precalculate Hann window & FFT tables.
 */
esp_err_t audio_dsp_init(void);

/**
 * @brief Read audio samples from I2S DMA, execute 512-point Real FFT, update 16 spectrum bands.
 */
void audio_dsp_process(spectrum_data_t *out_spectrum);

/**
 * @brief Reset spectrum peak hold values immediately.
 */
void audio_dsp_reset_peaks(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_DSP_H
